/**
 * @file io.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.0
 * @date 2024-2026
 * @license LGPLv3.0
 * @brief Input and output processing.
 */
#define _POSIX_C_SOURCE 200809L
#include "io.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <c-utils/uchar.h>

#include "dfa.h"
#include "terminfo.h"

#define IO_BUF_SIZE 4096
#define CHAR_EOT 4
#define UNICODE_ERROR_CHAR 0xfffd

static int read_file = -1;
static int echo_file = -1;

static char rd_buf[IO_BUF_SIZE];
static uint32_t rd_count = 0;
static uint32_t rd_used = 0;
static uint32_t rd_pending = 0;

static char wr_buf[IO_BUF_SIZE];
static uint32_t wr_count = 0;

static const char *preload_data = NULL;
static bool echo_enabled = false;

static char next_char(void);
static uchar next_uchar(void);
static ssize_t read_wrapper(int fd, char *buf, size_t count);

void nrl_io_init(int read_fd, int echo_fd, const char *preload) {
	read_file = read_fd;
	echo_file = echo_fd;

	// Reset read buffer counters
	rd_count = 0;
	rd_used = 0;
	rd_pending = 0;

	// Reset echo buffer counters
	wr_count = 0;

	// Handle preload
	preload_data = preload;
}

input_type nrl_io_read(input_buf *buffer) {
	dfa_result parse_result = nrl_dfa_parse(&next_char, &buffer->escape);
	if (parse_result != DFA_RES_EMPTY) {
		rd_used += rd_pending;
		rd_pending = 0;
		buffer->more = (rd_used < rd_count);

		return (parse_result == DFA_RES_CUSTOM) ? INPUT_CUSTOM_ESCAPE
												: INPUT_ESCAPE;
	}

	// This should only happen here if TERM is unset
	if (rd_count == rd_used) {
		rd_count = read_wrapper(read_file, rd_buf, IO_BUF_SIZE);
		rd_used = 0;
	}

	// Get next unicode point
	rd_pending = 0;
	uchar uc = next_uchar();
	buffer->more = (rd_used < rd_count);

	// Check for stop conditions (newline and EOF)
	if (uc == '\n' || uc == CHAR_EOT) {
		buffer->eof = (uc == CHAR_EOT);
		return INPUT_STOP;
	}

	// Check for unprintable control codes
	if (!nrl_io_parse_control(uc, buffer)) {
		// Character is printable: place it in buffer ourselves
		buffer->text[0] = uc;
		buffer->length = 1;
	}

	return INPUT_TEXT;
}

ssize_t nrl_io_raw_read(char *buffer, uint32_t buf_size) {
	assert(read_file != -1);
	assert(buf_size > 0);

	return read(read_file, buffer, buf_size);
}

bool nrl_io_parse_control(uchar c, input_buf *buffer) {
	// Special case: backspace (will not show up normally here)
	if (c == 0x7f) {
		buffer->text[0] = '^';
		buffer->text[1] = '?';
		buffer->length = 2;

		return true;
	}

	// C0 codes are below 0x20
	if (c >= 0x20) {
		return false;
	}

	// Generally how C0 codes are represented
	buffer->text[0] = '^';
	buffer->text[1] = c + 0x40;
	buffer->length = 2;

	return true;
}

bool nrl_io_write(const char *data, uint32_t length) {
	if (!echo_enabled) {
		return true;
	}

	// Will overflow buffer
	if (wr_count + length > IO_BUF_SIZE) {
		if (!nrl_io_flush()) {
			return false;
		}
	}

	// Too big to fix buffer
	if (length > IO_BUF_SIZE) {
		return write(echo_file, data, length) == length;
	}

	memcpy(wr_buf + wr_count, data, length);
	wr_count += length;

	return true;
}

bool nrl_io_write_escape(terminfo_output escape) {
	const char *as_text = nrl_lookup_output(escape);

	// Not supported: skip
	if (as_text == NULL) {
		return true;
	}

	return nrl_io_write(as_text, strlen(as_text));
}

bool nrl_io_flush(void) {
	assert(echo_file != -1);

	if (wr_count == 0) {
		return true;
	}

	if (write(echo_file, wr_buf, wr_count) != wr_count) {
		return false;
	}

	wr_count = 0;
	return true;
}

void nrl_io_wipe_buffers(void) {
	memset(rd_buf, 0, IO_BUF_SIZE);
	memset(wr_buf, 0, IO_BUF_SIZE);
}

void nrl_io_echo_state(bool enabled) {
	echo_enabled = enabled;
}

/**
 * @brief Get next character from input.
 *
 * @return Next character.
 */
static char next_char(void) {
	// No characters in buffer: read in more
	if (rd_used == rd_count) {
		// Reset counters
		rd_used = 0;
		rd_pending = 0;

		// Read in buffer
		rd_count = read_wrapper(read_file, rd_buf, IO_BUF_SIZE);
	}

	// End of buffer reached, but DFA parse is in progress
	// Assume that the sequence size is negligible relative to the buffer size
	if (rd_used + rd_pending == rd_count) {
		memmove(rd_buf, rd_buf + rd_count, rd_pending);

		rd_count = rd_pending;
		rd_used = 0;

		ssize_t bytes = read_wrapper(read_file, rd_buf + rd_count,
									 IO_BUF_SIZE - rd_count);
		rd_count += bytes;
	}

	return rd_buf[rd_used + rd_pending++];
}

/**
 * @brief Get next unicode point from input.
 *
 * @return Next unicode point.
 */
static uchar next_uchar(void) {
	char buffer[4] = { 0 };

	// Load buffer
	buffer[0] = next_char();
	switch (utf8_inspect(buffer[0])) {
	case U8BI_2BYTES:
		buffer[1] = next_char();
		break;
	case U8BI_3BYTES:
		buffer[1] = next_char();
		buffer[2] = next_char();
		break;
	case U8BI_4BYTES:
		buffer[1] = next_char();
		buffer[2] = next_char();
		buffer[3] = next_char();
		break;
	default:
		// Let the parser detect other errors
		break;
	}

	// Parse unicode value
	uchar uc;
	uint32_t consumed = utf8_parse_uchar(buffer, &uc);

	if (consumed == 0) {
		rd_used++;
		rd_pending = 0;

		return UNICODE_ERROR_CHAR;
	}

	// Successful parse
	rd_used += consumed;
	rd_pending = 0;
	return uc;
}

/**
 * @brief Combines preload and reading logic.
 *
 * @param[in] fd - Read file descriptor.
 * @param[in] buf - Buffer for data.
 * @param[in] count - Max size to read.
 * @return Actual amount of bytes read.
 */
static ssize_t read_wrapper(int fd, char *buf, size_t count) {
	assert(fd != -1);
	assert(count > 0);

	if (preload_data != NULL) {
		uint32_t remaining = strlen(preload_data);

		if (remaining > count) {
			memcpy(buf, preload_data, count);
			preload_data += count;
			return count;
		} else {
			memcpy(buf, preload_data, remaining);
			preload_data = NULL;
			return remaining;
		}
	}

	ssize_t bytes = read(fd, buf, count);

	// Read error: place an eof character
	if (bytes <= 0) {
		buf[0] = CHAR_EOT;
		return 1;
	}

	// All good otherwise
	return bytes;
}
