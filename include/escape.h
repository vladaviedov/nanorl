/**
 * @file escape.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2025
 * @license LGPLv3.0
 * @brief User escape code configuration.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @enum nrl_escape
 * Identifiers for configuratble sequences.
 */
typedef enum {
	NRL_ESC_KEY_UP = 0,
	NRL_ESC_KEY_DOWN,
	NRL_ESC_TAB,
} nrl_escape;

/**
 * @enum nrl_cursor_type
 * Changes the meaning of @ref nrl_state::cursor.
 * @note nanorl (will) handle input in the UTF8 encoding. Using @ref
 * nrl_cursor_type::NRL_CT_BYTE allows the line buffer to still be edited by the
 * called without having to implement UTF8 handling.
 *
 * @var nrl_cursor_type::NRL_CT_BYTE
 * Cursor position corresponds to array index position.
 *
 * @var nrl_cursor_type::NRL_CT_UTF8
 * Cursor position accounts for multibyte UTF8 characters.
 */
typedef enum {
	NRL_CT_BYTE,
	NRL_CT_UTF8,
} nrl_cursor_type;

/**
 * @struct nrl_state
 * Exported line state representation.
 *
 * @var nrl_state::line
 * Null-terminated string containing line data.
 *
 * @var nrl_state::cursor
 * Current cursor position.
 * @see nrl_cursor_type
 */
typedef struct {
	char *line;
	uint32_t cursor;
} nrl_state;

/**
 * @struct nrl_escape_handler
 * User handler definition.
 *
 * @var nrl_escape_handler::id
 * Escape code identifier.
 *
 * @var nrl_escape_handler::cursor_type
 * Cursor position caclulation method (applied to both input and output).
 *
 * @var nrl_escape_handler::func
 * User-defined handler function.
 * @param[in] state - Current line state.
 * @param[in] code - Escape code which triggered the handler (same as @ref
 * nrl_escape_handler::id).
 * @return Modified line state.
 * @warning The implementation of the handler must ensure to not place
 * unprintable characters into the modified line.
 */
typedef struct {
	nrl_escape id;
	nrl_cursor_type cursor_type;
	nrl_state (*func)(const nrl_state *state, nrl_escape code);
} nrl_escape_handler;
