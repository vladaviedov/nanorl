/**
 * @file format.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.2
 * @date 2026
 * @license LGPLv3.0
 * @brief Terminfo parameterized string formatting.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Format parameterized string.
 *
 * @param[in] fmt - Format string.
 * @param[in] arg_count - Argument count.
 * @param[in] ... - Argument list.
 * @return Formatted output.
 * @note Caller responsible for freeing the return value.
 */
char *nrl_terminfo_string_format(const char *fmt, uint32_t arg_count, ...);

/**
 * @brief Parse formatted string with 2 arguments.
 *
 * @param[in] fmt - Format specifier.
 * @param[in] data - Formatted string.
 * @param[out] p1 - Argument 1 buffer.
 * @param[out] p2 - Argument 2 buffer.
 * @return true - Parsed successfully.\n
 *         false - Parsing failed.
 */
bool nrl_terminfo_string_parse2(const char *fmt,
								const char *data,
								int32_t *p1,
								int32_t *p2);
