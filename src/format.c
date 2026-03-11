/**
 * @file format.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.0
 * @date 2026
 * @license LGPLv3.0
 * @brief Terminfo parameterized string formatting.
 */
#define _POSIX_C_SOURCE 200809L
#include "format.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <c-utils/stack.h>
#include <c-utils/vector-ext.h>
#include <c-utils/vector.h>

char *nrl_terminfo_string_format(const char *fmt, uint32_t arg_count, ...) {
	// Output string vector
	vector out = vec_init(sizeof(char));
	// Value stack
	stack st = stack_init(sizeof(int32_t));
	// Input parameter list
	int32_t plist[9] = { 0 };
	// Scratchpad buffers
	int32_t scratch_x = 0;
	char scratch_buf[128];

	// Parse parameter list
	va_list args;
	va_start(args, arg_count);
	for (uint32_t i = 0; i < arg_count; i++) {
		plist[i] = va_arg(args, int32_t);
	}
	va_end(args);

	const char *last_added = fmt;
	char c;
	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			continue;
		}

		// Push normal characters into output
		vec_bulk_push(&out, last_added, fmt - last_added - 1);
		last_added = fmt;

		// Format replacement
		// TODO: implemeent other cases
		c = *fmt++;
		switch (c) {
		case 'p':
			// Push to stack
			c = *fmt++;
			if (c < '1' || c > '9') {
				// err
				return NULL;
			}

			// Need to correct for 1-indexing
			stack_push(&st, plist + (uint32_t)(c - '0') - 1);
			break;
		case 'i':
			// Increment first two args
			plist[0]++;
			plist[1]++;
			break;
		case 'd':
			// Pop integer & convert to string
			if (stack_pop(&st, &scratch_x) != STACK_STATUS_OK) {
				// err
				return NULL;
			}
			sprintf(scratch_buf, "%d", scratch_x);
			vec_bulk_push(&out, scratch_buf, strlen(scratch_buf));
			break;
		}

		last_added = fmt;
	}

	// Push rest of string + null term
	char null_term = '\0';
	vec_bulk_push(&out, last_added, fmt - last_added - 1);
	vec_push(&out, &null_term);

	stack_deinit(&st);
	return vec_collect(&out);
}

bool nrl_terminfo_string_parse2(const char *fmt,
								const char *data,
								int32_t *p1,
								int32_t *p2) {
	// Parser state
	bool i_flag = false;
	uint32_t current_p = 0;

	char c;
	while ((c = *fmt++) != '\0') {
		// Data string is not the right size
		if (*data == '\0') {
			return false;
		}

		if (c != '%') {
			// Check that format literal matches data literal
			if (c != *data++) {
				return false;
			}
			continue;
		}

		// TODO: implemeent other cases
		c = *fmt++;
		switch (c) {
		case 'i':
			// Increment first two args
			i_flag = true;
			break;
		case 'd': {
			// Try parse integer
			char *endptr;
			int32_t *target;

			switch (current_p) {
			case 0:
				target = p1;
				break;
			case 1:
				target = p2;
				break;
			default:
				return false;
			}

			*target = strtol(data, &endptr, 10);
			if (*target == 0) {
				return false;
			}

			current_p++;
			data = endptr;

			break;
		}
		}
	}

	if (i_flag) {
		(*p1)--;
		(*p2)--;
	}

	return true;
}
