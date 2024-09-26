/**
 * @cond internal
 * @file io.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Input and output processing.
 */
#define _POSIX_C_SOURCE 200809L
#include "io.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "dfa.h"
#include "terminfo.h"

#define IO_BUF_SIZE 4096
#define CHAR_EOT 4

static int read_file = -1;
static int echo_file = -1;

static char rd_buf[IO_BUF_SIZE];
static uint32_t rd_count = 0;
static uint32_t rd_used = 0;
static uint32_t rd_pending = 0;

static char wr_buf[IO_BUF_SIZE];
static uint32_t wr_count = 0;

static const char *preload_data = NULL;

static char io_next_char(void);
static ssize_t read_wrapper(int fd, void *buf, size_t count);
static bool parse_ascii_control(char ascii, input_buf *buffer);

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
	if (nrl_dfa_parse(&io_next_char, &buffer->escape)) {
		rd_used += rd_pending;
		rd_pending = 0;

		return INPUT_ESCAPE;
	}

	rd_pending = 0;
	char ascii = rd_buf[rd_used++];

	// Check for stop conditions (newline and EOF)
	if (ascii == '\n' || ascii == CHAR_EOT) {
		buffer->eof = (ascii == CHAR_EOT);
		return INPUT_STOP;
	}

	// TODO: UTF8 handling

	// Check for unprintable control codes
	if (!parse_ascii_control(ascii, buffer)) {
		// Character is printable: place it in buffer ourselves
		buffer->text[0] = ascii;
		buffer->length = 1;
	}

	return INPUT_ASCII;
}

bool nrl_io_write(const char *data, uint32_t length) {
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

/**
 * @brief Get next character from input.
 *
 * @return Next character.
 */
static char io_next_char(void) {
	// No characters in buffer: read in more
	if (rd_used == rd_count) {
		ssize_t bytes = read_wrapper(read_file, rd_buf, IO_BUF_SIZE);

		// Read error or file closed
		if (bytes <= 0) {
			return CHAR_EOT;
		}

		rd_count = bytes;
		rd_used = 0;
		rd_pending = 0;
	}

	// End of buffer reached, but DFA parse is in progress
	// Assume that the sequence size is negligible relative to the buffer size
	if (rd_used + rd_pending == rd_count) {
		memmove(rd_buf, rd_buf + rd_count, rd_pending);

		rd_count = rd_pending;
		rd_used = 0;

		ssize_t bytes = read_wrapper(
			read_file, rd_buf + rd_count, IO_BUF_SIZE - rd_count);
		rd_count += bytes;
	}

	return rd_buf[rd_used + rd_pending++];
}

/**
 * @brief Combines preload and reading logic.
 *
 * @param[in] fd - Read file descriptor.
 * @param[in] buf - Buffer for data.
 * @param[in] count - Max size to read.
 * @return Actual amount of bytes read.
 */
static ssize_t read_wrapper(int fd, void *buf, size_t count) {
	assert(read_file != -1);

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

	return read(fd, buf, count);
}

/**
 * @brief Check if the input is a C0 code and if so, populate buffer with a
 * printable representation.
 *
 * @param[in] ascii - ASCII character.
 * @param[out] buffer - Buffer for input.
 * @return true - Character is a C0 code; buffer populated. \n
 *         false - Character is printable.
 */
static bool parse_ascii_control(char ascii, input_buf *buffer) {
	// C0 codes are below 0x20
	if (ascii >= 0x20) {
		return false;
	}

	// Generally how C0 codes are represented
	buffer->text[0] = '^';
	buffer->text[1] = ascii + 0x40;
	buffer->length = 2;

	return true;
}

// @endcond
