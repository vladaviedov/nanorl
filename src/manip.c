/**
 * @cond internal
 * @file manip.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Line manipations.
 */
#define _POSIX_C_SOURCE 200809L
#include "manip.h"

#include <assert.h>
#include <stdint.h>

#include <c-utils/vector-ext.h>
#include <c-utils/vector.h>

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

void nrl_manip_insert_ascii(line_data *line,
							const char *data,
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

// @endcond
