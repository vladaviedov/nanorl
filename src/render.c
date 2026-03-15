/**
 * @file render.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.1
 * @date 2025-2026
 * @license LGPLv3.0
 * @brief Line rendering.
 */
#include "render.h"

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <c-utils/uchar.h>
#include <c-utils/ucwidth.h>

#include "format.h"
#include "io.h"
#include "manip.h"
#include "nanorl.h"
#include "terminfo.h"

#define CPR_RES_BUF_SIZE 512

typedef struct {
	uint32_t row;
	uint32_t col;
} pos_2d;

// Current echo mode
static nrl_echo_mode echo_mode;
// Width of line rendered last redraw
static uint32_t last_rendered_width = 0;
// Cursor capabilities
static bool term_smart = false;
// Echo file descriptor
static int echo_fd = -1;
// Resize signal received
volatile sig_atomic_t resize_flag = false;

// Terminal dimensions
static pos_2d term_size = { .row = 0, .col = 0 };
// In-memory cursor position
static pos_2d cursor_pos = { .row = 0, .col = 0 };

static void redraw_normal(line_data *line);
static void redraw_obscured(line_data *line);
static void move_to_pos_normal(line_data *line, uint32_t pos);
static void move_to_pos_obscured(line_data *line, uint32_t pos);
static pos_2d linear_offset_to_2d(pos_2d origin, int32_t pos);
static bool query_size(pos_2d *buf);
static bool query_cursor(pos_2d *buf);
static bool move_cursor_2d(pos_2d location);
static void handle_scroll(void);

bool nrl_render_init(nrl_echo_mode mode, int echo_file) {
	echo_mode = mode;
	last_rendered_width = 0;
	term_smart = nrl_term_is_smart();
	echo_fd = echo_file;

	// Figure out start cursor position & terminal size
	if (term_smart && echo_mode != NRL_ECHO_OFF) {
		if (!query_cursor(&cursor_pos) || !query_size(&term_size)) {
			return false;
		}
	}

	return true;
}

void nrl_render_notify_resize(void) {
	resize_flag = true;
}

void nrl_render_redraw(line_data *line) {
	// Terminate string: this is a hack to get utf8_encode to work properly.
	// Refactor maybe?
	uchar null_char = 0;
	vec_push(&line->buffer, &null_char);

	// On sigwinch, need to reinit the positioning
	if (resize_flag && term_smart && echo_mode != NRL_ECHO_OFF) {
		query_cursor(&cursor_pos);
		query_size(&term_size);
	}

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

	// Unterminate string
	vec_erase(&line->buffer, line->buffer.count - 1, NULL);

	line->dirty = false;
	resize_flag = false;
	nrl_io_flush();
}

void nrl_render_sync_cursors(line_data *line) {
	assert(term_smart);

	switch (echo_mode) {
	case NRL_ECHO_ON:
		move_to_pos_normal(line, line->cursor);
		break;
	case NRL_ECHO_OBSCURED:
		move_to_pos_obscured(line, line->cursor);
		break;
	case NRL_ECHO_OFF:
		break;
	}

	nrl_io_flush();
}

/**
 * @brief Redraw line (normal echo mode).
 *
 * @param[in] line - Line data.
 */
static void redraw_normal(line_data *line) {
	if (!term_smart) {
		// On dumb terminals, only echo the unprinted chars
		char *data = utf8_encode(line->buffer.data + line->render_cursor);
		nrl_io_write(data, strlen(data));
		return;
	}

	// Convert to printable string
	char *data = utf8_encode(line->buffer.data);
	uint32_t len = strlen(data);

	// Move cursor to the beginning
	move_to_pos_normal(line, 0);

	// Print line data
	nrl_io_write(data, len);

	// Recompute position
	line->render_cursor = line->buffer.count - 1;
	int wr_width = ucswidth(line->buffer.data, line->buffer.count - 1);
	assert(wr_width != -1);
	cursor_pos = linear_offset_to_2d(cursor_pos, wr_width);
	handle_scroll();

	// Account for erased characters
	int32_t to_erase = (int32_t)last_rendered_width - (line->buffer.count - 1);
	for (int32_t i = 0; i < to_erase; i++) {
		nrl_io_write(" ", 1);
	}

	// Update for next cycle & move to line cursor
	last_rendered_width = wr_width;
	move_to_pos_normal(line, line->cursor);
	free(data);
}

/**
 * @brief Redraw line (obscured echo mode).
 *
 * @param[in] line - Line data.
 */
static void redraw_obscured(line_data *line) {
	if (!term_smart) {
		// On dumb terminals, just add the new characters
		for (uint32_t i = 0; i < line->buffer.count - 1 - line->render_cursor;
			 i++) {
			nrl_io_write("*", 1);
		}
		return;
	}

	// Move cursor to the beginning
	move_to_pos_obscured(line, 0);

	// Print line data
	uint32_t wr_width = line->buffer.count - 1;
	for (uint32_t i = 0; i < wr_width; i++) {
		nrl_io_write("*", 1);
	}

	// Recompute position
	line->render_cursor = wr_width;
	cursor_pos = linear_offset_to_2d(cursor_pos, wr_width);
	handle_scroll();

	// Account for erased characters
	int32_t to_erase = (int32_t)last_rendered_width - (line->buffer.count - 1);
	for (int32_t i = 0; i < to_erase; i++) {
		nrl_io_write(" ", 1);
	}

	// Update for next cycle & move to line cursor
	last_rendered_width = line->render_cursor;
	move_to_pos_obscured(line, line->cursor);
}

/**
 * @brief Move render cursor to an arbitrary position (normal rendering mode).
 *
 * @param[in] line - Line data.
 * @param[in] pos - Desired position.
 */
static void move_to_pos_normal(line_data *line, uint32_t pos) {
	int32_t move_width = 0;
	bool dir_fwd = ((int32_t)pos - (int32_t)line->render_cursor) > 0;

	while (line->render_cursor != pos) {
		// Get width of character we are stepping over
		uint32_t step_idx
			= dir_fwd ? line->render_cursor : line->render_cursor - 1;
		const uchar *step = vec_at(&line->buffer, step_idx);

		int width = ucwidth(*step);
		assert(width != -1);

		// Update cursor
		if (dir_fwd) {
			line->render_cursor++;
			move_width += width;
		} else {
			line->render_cursor--;
			move_width -= width;
		}
	}

	move_cursor_2d(linear_offset_to_2d(cursor_pos, move_width));
}

/**
 * @brief Move render cursor to an arbitrary position (obscured rendering mode).
 *
 * @param[in] line - Line data.
 * @param[in] pos - Desired position.
 */
static void move_to_pos_obscured(line_data *line, uint32_t pos) {
	int32_t offset = (int32_t)pos - (int32_t)line->render_cursor;
	line->render_cursor = (uint32_t)((int32_t)line->render_cursor + offset);

	move_cursor_2d(linear_offset_to_2d(cursor_pos, offset));
}

/**
 * @brief Convert linear offset from origin to a 2D coordinate.
 *
 * @param[in] origin - 2D origin coordinate.
 * @param[in] pos - Linear offset.
 * @return 2D coordinate pointing to desired position.
 */
static pos_2d linear_offset_to_2d(pos_2d origin, int32_t pos) {
	int32_t linear_origin = origin.row * term_size.col + origin.col;
	int32_t linear_p = (int32_t)linear_origin + pos;

	// Fallback condition that happens on row overflow
	// This keeps the rendered string more or less sane looking
	// TODO: implement proper vertical scroll handling
	if (linear_p < 0) {
		pos_2d p = { .row = 0, .col = 0 };
		return p;
	}

	pos_2d p = {
		.row = (uint32_t)linear_p / term_size.col,
		.col = (uint32_t)linear_p % term_size.col,
	};

	return p;
}

/**
 * @brief Query terminal dimensions.
 *
 * @param[out] buf - Position buffer.
 * @return true - Successfully update dimensions.\n
 *         false - All methods failed.
 */
static bool query_size(pos_2d *buf) {
// Method 1: ioctl kernel request
// This behavior is not POSIX standard, but is implemented on Linux & BSD
#if defined(TIOCGWINSZ)
	// Modern API
	struct winsize size;

	if (ioctl(echo_fd, TIOCGWINSZ, &size) == 0) {
		buf->row = size.ws_row;
		buf->col = size.ws_col;
		return true;
	}
#elif defined(TIOCGSIZE)
	// Old API
	struct ttysize size;

	if (ioctl(config->echo_file, TIOCGSIZE, &size) == 0) {
		*rows = size.ts_row;
		*cols = size.ts_col;
		return true;
	}
#endif

	// Method 2: cursor position report (CPR) request
	// This is terminal dependent and does not rely on system APIs
	// Fallback way for terminals which support this

	// Save cursor position
	pos_2d saved_pos;
	if (!query_cursor(&saved_pos)) {
		return false;
	}

	// Check the cursor position after moving it all the way to the end
	pos_2d end_pos = { .row = 9999, .col = 9999 };
	if (!move_cursor_2d(end_pos)) {
		return false;
	}
	if (!query_cursor(buf)) {
		return false;
	}

	// Restore old position
	if (!move_cursor_2d(saved_pos)) {
		return false;
	}

	// Cursor position comes back 0-indexed, so need to increment dimenions
	buf->row++;
	buf->col++;

	return true;
}

/**
 * @brief Query cursor location from the terminal.
 *
 * @param[out] buf - Current cursor position.
 * @return true - Request succeeded.
 *         false - Request failed.
 */
static bool query_cursor(pos_2d *buf) {
	// Request CPR
	const char *cpr_req = nrl_lookup_special(TIS_USER7);
	if (!nrl_io_write(cpr_req, strlen(cpr_req)) || !nrl_io_flush()) {
		return false;
	}

	// Lookup CPR response format
	const char *cpr_res = nrl_lookup_special(TIS_USER6);
	char last_char = cpr_res[strlen(cpr_res) - 1];

	// Read in CPR response
	char res_buf[CPR_RES_BUF_SIZE];
	uint32_t res_size = 0;
	do {
		ssize_t read_size
			= nrl_io_raw_read(res_buf + res_size, CPR_RES_BUF_SIZE);
		if (read_size < 0) {
			return false;
		}

		res_size += read_size;
	} while (res_buf[res_size - 1] != last_char);
	assert(res_size != CPR_RES_BUF_SIZE);
	res_buf[res_size] = '\0';

	// Parse CPR response
	int32_t row_buf;
	int32_t col_buf;
	nrl_terminfo_string_parse2(nrl_lookup_special(TIS_USER6), res_buf, &row_buf,
							   &col_buf);

	// Save to buffer
	assert(row_buf >= 0);
	assert(col_buf >= 0);
	buf->row = (uint32_t)row_buf;
	buf->col = (uint32_t)col_buf;

	return true;
}

/**
 * @brief Request terminal to move cursor to a coordinate.
 *
 * @param[in] location - Coordinate to move to.
 * @return true - Request succeeded.\n
 *         false - Request failed.
 */
static bool move_cursor_2d(pos_2d location) {
	const char *format = nrl_lookup_special(TIS_CURSOR_ADDRESS);
	char *move_cmd
		= nrl_terminfo_string_format(format, 2, location.row, location.col);

	if (!nrl_io_write(move_cmd, strlen(move_cmd))) {
		return false;
	}
	free(move_cmd);

	if (!nrl_io_flush()) {
		return false;
	}

	cursor_pos = location;
	return true;
}

/**
 * @brief Handle any weirdness that comes from terminal scrolling.
 */
static void handle_scroll(void) {
	int32_t rows_scrolled = cursor_pos.row - term_size.row + 1;
	if (rows_scrolled <= 0) {
		return;
	}

	// This forces scrolling at the end of the line. It's a hacky way to do it,
	// but it works for now. TODO: refactor?
	if (cursor_pos.col == 0) {
		nrl_io_write("\n", 1);
		nrl_io_flush();
	}

	// All coordinates are shifted by the scrolled amount
	cursor_pos.row -= rows_scrolled;
}
