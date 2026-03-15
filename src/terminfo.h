/**
 * @file terminfo.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.0
 * @date 2024-2026
 * @license LGPLv3.0
 * @brief terminfo parser.
 */
#pragma once

#include <stdbool.h>

#include "escape.h"

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
 * Total entries in @ref terminfo_input.
 */
#define TII_COUNT 6

typedef nrl_escape terminfo_custom;

/**
 * @def TIC_COUNT
 * Total entries in @ref nrl_escape.
 */
#define TIC_COUNT 29

/**
 * @enum terminfo_output
 * Internal identifiers for terminfo output sequences.
 */
typedef enum {
	TIO_KEYPAD_LOCAL,
	TIO_KEYPAD_XMIT,
} terminfo_output;

/**
 * @def TIO_COUNT
 * Total entries in @ref terminfo_output.
 */
#define TIO_COUNT 2

/**
 * @enum terminfo_special
 * Internal identifiers for terminfo special sequences.
 */
typedef enum {
	TIS_CURSOR_ADDRESS,
	TIS_USER6,
	TIS_USER7,
} terminfo_special;

/**
 * @def TIS_COUNT
 * Total entries in @ref terminfo_special.
 */
#define TIS_COUNT 3

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
 * @brief Get ASCII string for configuratble input escape sequence.
 *
 * @param[in] id - Interal identifier.
 * @return ASCII representation, null-terminated string.
 * @note Should only be called after nrl_load_terminfo.
 */
const char *nrl_lookup_custom(terminfo_custom id);

/**
 * @brief Get ASCII string for output escape sequence.
 *
 * @param[in] id - Interal identifier.
 * @return ASCII representation, null-terminated string.
 * @note Should only be called after nrl_load_terminfo.
 */
const char *nrl_lookup_output(terminfo_output id);

/**
 * @brief Get ASCII string for special escape sequence.
 *
 * @param[in] id - Interal identifier.
 * @return ASCII representation, null-terminated string.
 * @note Should only be called after nrl_load_terminfo.
 */
const char *nrl_lookup_special(terminfo_special id);

/**
 * @brief Check if the terminal should be treated as "smart" or "dumb"
 *
 * @return true - Enough capabilities enabled for cursor movement.\n
 *         false - Should be treated as "dumb".
 */
bool nrl_term_is_smart(void);
