/**
 * @file manip.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024-2025
 * @license LGPLv3.0
 * @brief Line manipations.
 */
#define _POSIX_C_SOURCE 200809L
#include "manip.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <c-utils/vector-ext.h>
#include <c-utils/vector.h>

#include "escape.h"
#include "io.h"
#include "terminfo.h"

typedef struct {
	terminfo_input value;
	void (*func)(line_data *);
} escape_manip;

static void escape_backspace(line_data *line);
static void escape_left(line_data *line);
static void escape_right(line_data *line);
static void escape_delete(line_data *line);
static void escape_home(line_data *line);
static void escape_end(line_data *line);

static const escape_manip esc_manips[] = {
	{ TII_KEY_BACKSPACE, &escape_backspace },
	{ TII_KEY_LEFT, &escape_left },
	{ TII_KEY_RIGHT, &escape_right },
	{ TII_KEY_DELETE, &escape_delete },
	{ TII_KEY_HOME, &escape_home },
	{ TII_KEY_END, &escape_end },
	{ 0, NULL },
};

#if CUSTOM_ESCAPES == 1
static const nrl_escape_handler *escape_table[TIC_COUNT] = { NULL };
#endif // CUSTOM_ESCAPES

void nrl_manip_insert_text(line_data *line,
							const uchar *data,
							uint32_t length) {
	vector_status res
		= vec_bulk_insert(&line->buffer, line->cursor, data, length);
	assert(res == VECTOR_STATUS_OK);

	line->cursor += length;
	line->dirty = true;
}

void nrl_manip_eval_escape(line_data *line, terminfo_input escape) {
	const escape_manip *manip = esc_manips;
	while (manip->func != NULL) {
		if (manip->value == escape) {
			manip->func(line);
			break;
		}

		manip++;
	}
}

#if CUSTOM_ESCAPES == 1
void nrl_manip_make_custom_table(const nrl_config *config) {
	nrl_escape_handler **trav = config->custom_handlers;
	if (trav == NULL) {
		return;
	}

	const nrl_escape_handler *item;
	while ((item = *trav++) != NULL) {
		// Verify escape identifier
		if (item->id >= 0 && item->id < TIC_COUNT) {
			escape_table[item->id] = item;
		}
	}
}

void nrl_manip_clear_custom_table(void) {
	memset(escape_table, 0, sizeof(escape_table));
}

bool nrl_manip_eval_custom(line_data *line, terminfo_custom escape) {
	const nrl_escape_handler *handler = escape_table[escape];
	if (handler == NULL) {
		return false;
	}

	// Add null-char to end of the string
	char null_char = '\0';
	vec_push(&line->buffer, &null_char);

	nrl_state export_state = {
		.line = vec_collect(&line->buffer),
		// TODO: UTF8 related stuff
		.cursor
		= (handler->cursor_type == NRL_CT_BYTE) ? line->cursor : line->cursor,
	};

	nrl_state import_state = handler->func(&export_state, handler->id);
	uint32_t import_len = strlen(import_state.line);
	vec_bulk_insert(&line->buffer, 0, import_state.line, import_len);

	// Cleanup strings
	free(export_state.line);
	free(import_state.line);

	// TODO: handle utf8 stuff
	line->cursor
		= (import_state.cursor > import_len) ? import_len : import_state.cursor;
	line->dirty = true;

	return true;
}
#endif // CUSTOM_ESCAPES

static void escape_backspace(line_data *line) {
	if (line->cursor > 0) {
		line->cursor--;
		escape_delete(line);
	}
}

static void escape_left(line_data *line) {
	if (line->cursor > 0) {
		line->cursor--;
		line->render_cursor--;
		nrl_io_write_escape(TIO_CURSOR_LEFT);
	}
}

static void escape_right(line_data *line) {
	if (line->cursor < line->buffer.count) {
		line->cursor++;
		line->render_cursor++;
		nrl_io_write_escape(TIO_CURSOR_RIGHT);
	}
}

static void escape_delete(line_data *line) {
	// TODO: utf8 handling
	// If cursor is at count, there is no character under the cursor
	if (line->cursor < line->buffer.count) {
		vector_status res = vec_erase(&line->buffer, line->cursor, NULL);
		assert(res == VECTOR_STATUS_OK);

		line->dirty = true;
	}
}

static void escape_home(line_data *line) {
	for (uint32_t i = 0; i < line->cursor; i++) {
		nrl_io_write_escape(TIO_CURSOR_LEFT);
	}

	line->cursor = 0;
	line->render_cursor = 0;
}

static void escape_end(line_data *line) {
	for (uint32_t i = line->cursor; i < line->buffer.count; i++) {
		nrl_io_write_escape(TIO_CURSOR_RIGHT);
	}

	line->cursor = line->buffer.count;
	line->render_cursor = line->buffer.count;
}
