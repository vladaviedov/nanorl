/**
 * @cond internal
 * @file terminfo.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief terminfo parser.
 */
#pragma once

#include <stdbool.h>

/**
 * @enum terminfo_input
 * Internal identifiers for terminfo input sequences.
 */
typedef enum {
	TII_KEY_LEFT,
	TII_KEY_RIGHT,
	TII_KEY_BACKSPACE,
	TII_KEY_HOME,
	TII_KEY_END,
	TII_KEY_DELETE,
} terminfo_input;

/**
 * @def TII_COUNT
 * Total entries in @ref terminfo_input
 */
#define TII_COUNT 6

/**
 * @enum terminfo_output
 * Internal identifiers for terminfo output sequences.
 */
typedef enum {
	TIO_CURSOR_LEFT,
	TIO_CURSOR_RIGHT,
	TIO_KEYPAD_LOCAL,
	TIO_KEYPAD_XMIT,
} terminfo_output;

/**
 * @def TIO_COUNT
 * Total entries in @ref terminfo_output
 */
#define TIO_COUNT 4

/**
 * @brief Find and load terminfo data for the user's terminal.
 *
 * @return true - Success.\n
 *         false - Failed to load.
 */
bool nrl_load_terminfo(void);

/**
 * @brief Get ASCII string for input escape sequence.
 *
 * @param[in] id - Interal identifier.
 * @return ASCII representation, null-terminated string.
 * @note Should only be called after nrl_load_terminfo.
 */
const char *nrl_lookup_input(terminfo_input id);

/**
 * @brief Get ASCII string for output escape sequence.
 *
 * @param[in] id - Interal identifier.
 * @return ASCII representation, null-terminated string.
 * @note Should only be called after nrl_load_terminfo.
 */
const char *nrl_lookup_output(terminfo_output id);

// @endcond
