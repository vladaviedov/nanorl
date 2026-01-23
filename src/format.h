/**
 * @file format.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2026
 * @license LGPLv3.0
 * @brief Terminfo parameterized string formatting.
 */
#pragma once

#include <stdint.h>

/**
 * @brief Format parameterized string.
 *
 * @param[in] fmt - Format string.
 * @param[in] arg_count - Argument count.
 * @param[in] ... - Argument list.
 * @return Formatted output.
 */
char *nrl_terminfo_string_format(const char *fmt, uint32_t arg_count, ...);
