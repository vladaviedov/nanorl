/**
 * @file render.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2025
 * @license LGPLv3.0
 * @brief Line rendering.
 */
#include "render.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <c-utils/uchar.h>
#include <c-utils/ucwidth.h>

#include "io.h"
#include "manip.h"
#include "nanorl.h"
#include "terminfo.h"

// Current echo mode
static nrl_echo_mode echo_mode;
// Width of line rendered last redraw
static uint32_t last_rendered_width = 0;
// Cursor capabilities
static bool cursor_cap = false;

static void redraw_normal(line_data *line);
static void redraw_obscured(line_data *line);
static void move_to_pos(line_data *line, uint32_t pos);

void nrl_render_init(nrl_echo_mode mode) {
	echo_mode = mode;
	last_rendered_width = 0;
	cursor_cap = nrl_cursor_capability();
}

void nrl_render_redraw(line_data *line) {
	switch (echo_mode) {
	case NRL_ECHO_ON:
		redraw_normal(line);
		break;
	case NRL_ECHO_OBSCURED:
		redraw_obscured(line);
		break;
	case NRL_ECHO_OFF:
		break;
	}

	line->dirty = false;
	line->render_cursor = line->cursor;

	nrl_io_flush();
}

void nrl_render_sync_cursors(line_data *line) {
	move_to_pos(line, line->cursor);
	nrl_io_flush();
}

/**
 * @brief Redraw line in normal echo mode.
 *
 * @param[in] line - Line data.
 */
static void redraw_normal(line_data *line) {
	if (!cursor_cap) {
		// On dumb terminals, only echo the unprinted chars
		char *data = utf8_encode(line->buffer.data + line->render_cursor);
		nrl_io_write(data, strlen(data));
		return;
	}

	// Convert to printable string
	char *data = utf8_encode(line->buffer.data);
	uint32_t len = strlen(data);

	// Move cursor to the beginning
	move_to_pos(line, 0);

	// Print line data
	nrl_io_write(data, len);

	// Account for erased characters
	for (uint32_t i = line->buffer.count; i < last_rendered_width; i++) {
		nrl_io_write(" ", 1);
		line->render_cursor++;
	}

	// Update for next cycle
	last_rendered_width = ucswidth(line->buffer.data, line->buffer.count);

	// Move cursor to correct location
	move_to_pos(line, line->cursor);
}

/**
 * @brief Redraw line in obscured echo mode.
 *
 * @param[in] line - Line data.
 */
static void redraw_obscured(line_data *line) {
	if (!cursor_cap) {
		// On dumb terminals, just add the new characters
		for (uint32_t i = 0; i < line->buffer.count - line->render_cursor;
			 i++) {
			nrl_io_write("*", 1);
		}
		return;
	}

	// Move cursor to the beginning
	move_to_pos(line, 0);

	// Print line data
	for (uint32_t i = 0; i < line->buffer.count; i++) {
		nrl_io_write("*", 1);
	}

	// Account for erased characters
	for (uint32_t i = line->buffer.count; i < last_rendered_width; i++) {
		nrl_io_write(" ", 1);
		line->render_cursor++;
	}

	// Update for next cycle
	last_rendered_width = line->render_cursor;

	// Move cursor to correct location
	move_to_pos(line, line->cursor);
}

/**
 * @brief Move render cursor to an arbitrary position.
 *
 * @param[in] line - Line data.
 * @param[in] pos - Desired position.
 */
static void move_to_pos(line_data *line, uint32_t pos) {
	int32_t offset = pos - line->render_cursor;
	if (offset == 0) {
		return;
	}

	if (offset > 0) {
		// Need to move right
		while (offset-- > 0) {
			// Get width of character we are stepping over
			const uchar *step = vec_at(&line->buffer, line->render_cursor);
			int width = ucwidth(*step);
			assert(width != -1);

			// Move to the right
			for (int i = 0; i < width; i++) {
				nrl_io_write_escape(TIO_CURSOR_RIGHT);
			}
		}
	} else {
		// Need to move left
		while (offset++ < 0) {
			// Get width of character we are stepping over
			const uchar *step = vec_at(&line->buffer, line->render_cursor - 1);
			int width = ucwidth(*step);
			assert(width != -1);

			// Move to the right
			for (int i = 0; i < width; i++) {
				nrl_io_write_escape(TIO_CURSOR_LEFT);
			}
		}
	}

	line->render_cursor = pos;
}
