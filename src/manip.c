/**
 * @file manip.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.2
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

#include <c-utils/uchar.h>
#include <c-utils/ustring.h>
#include <c-utils/vector-ext.h>
#include <c-utils/vector.h>

#include "escape.h"
#include "render.h"
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

	// We lose the original string data here, so we need to reset the cursor
	// position
	uint32_t saved_cursor = line->cursor;
	line->cursor = 0;
	nrl_render_sync_cursors(line);

	// Add null-char to end of the string
	uchar null_char = 0;
	vec_push(&line->buffer, &null_char);

	// Create export data
	nrl_state export_data;
	uchar *uc_line = vec_collect(&line->buffer);
	if (handler->format == NRL_DF_UTF32) {
		export_data.line.utf32_line = uc_line;
		export_data.cursor = saved_cursor;
	} else {
		export_data.line.utf8_line = utf8_encode(uc_line);

		// To calculate byte cursor, we can replace the selected uchar with a
		// null, encode the string in UTF-8 and check its length. I don't see
		// a more efficient way to do this.
		uc_line[saved_cursor] = 0;
		char *utf8_before_cursor = utf8_encode(uc_line);
		export_data.cursor = strlen(utf8_before_cursor);

		free(uc_line);
		free(utf8_before_cursor);
	}

	// Execute handler
	nrl_state import_data = handler->func(&export_data, handler->id);

	// Parse import data
	if (handler->format == NRL_DF_UTF32) {
		uchar *uc_line = import_data.line.utf32_line;
		vec_bulk_insert(&line->buffer, 0, uc_line, ustrlen(uc_line));
		line->cursor = import_data.cursor;

		// Cleanup strings
		free(export_data.line.utf32_line);
		free(import_data.line.utf32_line);
	} else {
		char *utf8_line = import_data.line.utf8_line;
		uchar *uc_line = utf8_decode(utf8_line, NULL);
		vec_bulk_insert(&line->buffer, 0, uc_line, ustrlen(uc_line));

		// Need to do the same trick when we import data as well
		utf8_line[import_data.cursor] = '\0';
		bool error_flag;
		uchar *uc_before_cursor = utf8_decode(utf8_line, &error_flag);
		line->cursor = ustrlen(uc_before_cursor);
		if (error_flag) {
			// We may have stepped in the middle of a unicode point, and then we
			// need to adjust the cursor by backtracking to the last common
			// point and adding 1
			uint32_t adjust = line->cursor;
			while (uc_line[adjust] != uc_before_cursor[adjust]) {
				adjust--;
			}
			line->cursor = adjust + 1;
		}

		free(uc_line);
		free(uc_before_cursor);
		free(export_data.line.utf8_line);
		free(import_data.line.utf8_line);
	}

	line->dirty = true;
	return true;
}
#endif // CUSTOM_ESCAPES

static void escape_backspace(line_data *line) {
	if (line->cursor > 0) {
		escape_left(line);
		escape_delete(line);
	}
}

static void escape_left(line_data *line) {
	if (line->cursor > 0) {
		line->cursor--;
		nrl_render_sync_cursors(line);
	}
}

static void escape_right(line_data *line) {
	if (line->cursor < line->buffer.count) {
		line->cursor++;
		nrl_render_sync_cursors(line);
	}
}

static void escape_delete(line_data *line) {
	// If cursor is at count, there is no character under the cursor
	if (line->cursor < line->buffer.count) {
		vector_status res = vec_erase(&line->buffer, line->cursor, NULL);
		assert(res == VECTOR_STATUS_OK);

		line->dirty = true;
	}
}

static void escape_home(line_data *line) {
	line->cursor = 0;
	nrl_render_sync_cursors(line);
}

static void escape_end(line_data *line) {
	line->cursor = line->buffer.count;
	nrl_render_sync_cursors(line);
}
