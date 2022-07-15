/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   hex_parser.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: tbruinem <tbruinem@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/07/15 21:54:22 by tbruinem      #+#    #+#                 */
/*   Updated: 2022/07/16 00:24:19 by tbruinem      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

//! We can't properly calculate beforehand how many digits the decimal representation will need

typedef enum {
	BUFFER_FINAL,
	BUFFER_INTERMEDIATE
} BufferType;

typedef struct {
	char* buffer;
	size_t len; //How far we are from the *back* of the allocated memory
	size_t capacity;
	char* intermediate;
} String;

#define HEX_TABLE "0123456789ABCDEF"
// #define HEX_STRING "AA18ABA43B50DEEF38598FAF87D2AB634E4571C130A9BCA7B878267414FAAB8B471BD8965F5C9FC3818485EAF529C26246F3055064A8DE19C8C338BE5496CBAEB059DC0B358143B44A35449EB264113121A455BD7FDE3FAC919E94B56FB9BB4F651CDB23EAD439D6CD523EB08191E75B35FD13A7419B3090F24787BD4F4E1967"
#define HEX_STRING "CF2"

void	print_string(String* string) {
	size_t start_index = string->capacity - string->len;
	write(STDOUT_FILENO, string->buffer + start_index, string->len);
	write(STDOUT_FILENO, "\n", 1);
}

//! If the length increases to capacity, we have to reallocate both buffers
void	increase_string(String* string) {
	char* old_buffer = string->buffer;
	assert(string->capacity == string->len);
	string->capacity *= 2;
	string->buffer = malloc((sizeof(char) * string->capacity) + 1);

	free(string->intermediate);
	string->intermediate = malloc((sizeof(char) * string->capacity) + 1);
	
	memcpy(string->buffer + string->len, old_buffer, string->len);
	free(old_buffer);
}

//! Initialize the string with buffers of a given capacity
void	init_string(String* string, size_t capacity) {
	string->buffer = malloc((sizeof(char) * capacity) + 1);
	string->intermediate = malloc((sizeof(char) * capacity) + 1);
	string->len = 0;
	string->capacity = capacity;
	memset(string->buffer, '0', string->capacity);
}

//! Replaces the result with the contents of intermediate
void	apply_intermediate(String* result) {
	memcpy(result->buffer, result->intermediate, result->capacity);
}

static char* get_string_buffer(String* string, BufferType buffer_type) {
	switch (buffer_type) {
		case BUFFER_FINAL: return string->buffer;
		case BUFFER_INTERMEDIATE: return string->intermediate;
	}
}

//! Apply an increase to a decimal digit
//! This addition might result in an additional digit being needed to represent the number
void	apply_increase(String* string, size_t offset, uint8_t increase, BufferType buffer_type) {
	dprintf(2, "OFFSET: %ld | INCREASE: %u\n", offset, increase);
	//! Increase the length
	if (offset >= string->len) {
		string->len++;
		if (string->len >= string->capacity) {
			increase_string(string);
		}
	}
	print_string(string);
	sleep(1);

	const size_t index = (string->capacity - offset) - 1;
	dprintf(2, "index: %ld\n", index);

	char* const buffer = get_string_buffer(string, buffer_type);

	uint8_t current_digit_value = buffer[index] - '0';
	uint8_t increased_value = current_digit_value + increase;
	dprintf(2, "DIGIT: %c | OLD: %u | NEW: %u\n", buffer[index], current_digit_value, increased_value);
	if (increased_value >= 10) {
		apply_increase(string, offset+1, increased_value / 10, buffer_type);
	}
	buffer[index] = (increased_value % 10) + '0';
}

//! Create an intermediate value by multiplying the current decimal representation in 'buffer'
void	set_intermediate(String* string) {
	dprintf(2, "SET INTERMEDIATE\n");
	//! Loop backwards over the current digits
	//! Minor numbers first
	memset(string->intermediate, '0', string->capacity);
	const size_t original_len = string->len;
	for (size_t i = 0; i < original_len; i++) {
		const size_t index = (string->capacity - i) - 1;

		const uint8_t decimal_value = string->buffer[index] - '0';
		const uint8_t increased_value = decimal_value * 16;
		apply_increase(string, i, increased_value, BUFFER_INTERMEDIATE);
	}
}

void	apply_multiplication(String* string) {
	set_intermediate(string);
	apply_intermediate(string);
}

//! We will have created the string at the very back of our allocated memory
//! So we need to move this up to the start and null-terminate it
char* finalize(String* string) {
	free(string->intermediate);
	size_t final_length = (string->capacity - string->len);
	memmove(string->buffer, string->buffer + final_length, string->len);
	string->buffer[final_length] = '\0';
	return string->buffer;
}

//! Every digit we add has the potential to introduce a new digit
char*	hex_to_dec(char* hex_string) {
	String result;

	size_t len = strlen(hex_string);
	init_string(&result, len);

	//for every digit, calculate the new result after multiplying by base (16)
	//then convert the digit to decimal, apply this to the string
	for (size_t i = 0; hex_string[i]; i++) {
		char digit = hex_string[i];
		const int decimal = strchr(HEX_TABLE, (int)digit) - HEX_TABLE;
		if (i) {
			//Apply the multiplication
			apply_multiplication(&result);
		}
		print_string(&result);
		//Apply the addition of the new digit
		apply_increase(&result, 0, decimal, BUFFER_FINAL);
		print_string(&result);
	}
	return finalize(&result);
}

int main() {
	char* decimal_result = hex_to_dec(HEX_STRING);
	printf("%s\n", decimal_result);
}
