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
 * Type definition for UTF-32 character.
 */
typedef uint32_t uchar;

/**
 * @enum nrl_escape
 * Identifiers for configuratble sequences.
 */
typedef enum {
	NRL_ESC_KEY_CATAB = 0,
	NRL_ESC_KEY_CLEAR,
	NRL_ESC_KEY_CTAB,
	NRL_ESC_KEY_DL,
	NRL_ESC_KEY_DOWN,
	NRL_ESC_KEY_EIC,
	NRL_ESC_KEY_EOL,
	NRL_ESC_KEY_EOS,
	NRL_ESC_KEY_F0,
	NRL_ESC_KEY_F1,
	NRL_ESC_KEY_F2,
	NRL_ESC_KEY_F3,
	NRL_ESC_KEY_F4,
	NRL_ESC_KEY_F5,
	NRL_ESC_KEY_F6,
	NRL_ESC_KEY_F7,
	NRL_ESC_KEY_F8,
	NRL_ESC_KEY_F9,
	NRL_ESC_KEY_F10,
	NRL_ESC_KEY_IC,
	NRL_ESC_KEY_IL,
	NRL_ESC_KEY_LL,
	NRL_ESC_KEY_NPAGE,
	NRL_ESC_KEY_PPAGE,
	NRL_ESC_KEY_SF,
	NRL_ESC_KEY_SR,
	NRL_ESC_KEY_STAB,
	NRL_ESC_KEY_UP,
	NRL_ESC_TAB,
} nrl_escape;

/**
 * @enum nrl_data_format
 * Chooses the format in which data is exported to and imported from custom
 * escape handlers. This affects how cursor position is calculated as well.
 * @note Interally, nanorl uses unicode points (UTF-32). The UTF-8 mode allows
 * applications which do not deal with unicode to create handlers.
 *
 * @var nrl_cursor_type::NRL_DF_UTF8
 * Data is in UTF-8 format.
 *
 * @var nrl_cursor_type::NRL_DF_UTF32
 * Data is in UTF-32 format.
 */
typedef enum {
	NRL_DF_UTF8,
	NRL_DF_UTF32,
} nrl_data_format;

/**
 * @union nrl_line
 * Line data in different formats.
 *
 * @var nrl_line::utf8_line
 * UTF-8 representation.
 *
 * @var nrl_line::utf32_line
 * UTF-32 representation.
 */
typedef union {
	char *utf8_line;
	uchar *utf32_line;
} nrl_line;

/**
 * @struct nrl_state
 * Exported line state representation.
 *
 * @var nrl_state::line
 * Line data.
 * @see nrl_data_format
 *
 * @var nrl_state::cursor
 * Current cursor position.
 */
typedef struct {
	nrl_line line;
	uint32_t cursor;
} nrl_state;

/**
 * @struct nrl_escape_handler
 * User handler definition.
 *
 * @var nrl_escape_handler::id
 * Escape code identifier.
 *
 * @var nrl_escape_handler::format
 * Format for data export/import.
 * @see nrl_data_format
 *
 * @var nrl_escape_handler::func
 * User-defined handler function.
 * @param[in] state - Current line state.
 * @param[in] code - Escape code which triggered the handler (same as @ref
 * nrl_escape_handler::id).
 * @return Modified line state.
 * @note The handler function should allocate memory for the line returned.
 * @note If the handler is in UTF-8 mode, and places the cursor in the middle
 * of a single unicode point, the cursor will jump to the end of the unicode
 * point.
 * @warning The handler function must ensure to not place control characters
 * into the modified line.
 */
typedef struct {
	nrl_escape id;
	nrl_data_format format;
	nrl_state (*func)(const nrl_state *state, nrl_escape code);
} nrl_escape_handler;
