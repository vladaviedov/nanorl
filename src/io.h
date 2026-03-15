/**
 * @file io.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.1
 * @date 2024-2026
 * @license LGPLv3.0
 * @brief Input and output processing.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <c-utils/uchar.h>

#include "dfa.h"
#include "terminfo.h"

#define SINGLE_BUF_SIZE 16

/**
 * @enum input_type
 * Type of input received.
 *
 * @var input_type::INPUT_ASCII
 * ASCII character or a string of ASCII characters.
 *
 * @var input_type::INPUT_UTF8
 * Multibyte UTF-8 character.
 *
 * @var input_type::INPUT_ESCAPE
 * Valid escape code received.
 *
 * @var input_type::INPUT_CUSTOM_ESCAPE
 * Valid configurable escape code received.
 *
 * @var input_type::INPUT_STOP
 * End condition received.
 */
typedef enum {
	INPUT_TEXT,
	INPUT_ESCAPE,
	INPUT_CUSTOM_ESCAPE,
	INPUT_STOP,
} input_type;

/**
 * @struct input_buf
 * Buffer for a single input from read.
 *
 * @var input_buf::escape
 * Buffer for an input sequence identifier.
 * Used with @ref input_type::INPUT_ESCAPE and @ref
 * input_type::INPUT_CUSTOM_ESCAPE.
 *
 * @var input_buf::eof
 * Buffer for EOF flag.
 * Used with @ref input_type::INPUT_STOP.
 *
 * @var input_buf::text.
 * Buffer for a text sequence.
 * Used with other input types.
 *
 * @var input_buf::length
 * Length of data in the text array.
 *
 * @var input_buf::more
 * Flag for whether there is more input in the buffer currently.
 */
typedef struct {
	dfa_acceptor escape;
	bool eof;
	uchar text[SINGLE_BUF_SIZE];
	uint32_t length;
	bool more;
} input_buf;

/**
 * @brief Initialize buffers and set files.
 *
 * @param[in] read_fd - Read file descriptor.
 * @param[in] echo_fd - Echo file descriptor.
 * @param[in] preload - Preload value.
 */
void nrl_io_init(int read_fd, int echo_fd, const char *preload);

/**
 * @brief Read data from input.
 *
 * @param[in,out] buffer - Buffer for input.
 * @return Type of input saved into buffer.
 */
input_type nrl_io_read(input_buf *buffer);

/**
 * @brief Perform a read bypassing buffering and DFA.
 *
 * @param[out] buffer - Data buffer.
 * @param[in] buf_size - Data buffer size.
 * @return Bytes read.
 */
ssize_t nrl_io_raw_read(char *buffer, uint32_t buf_size);

/**
 * @brief Check if the input is a C0 code and if so, populate buffer with a
 * printable representation.
 *
 * @param[in] c - Character.
 * @param[out] buffer - Buffer for input.
 * @return true - Character is a C0 code; buffer populated. \n
 *         false - Character is printable.
 */
bool nrl_io_parse_control(uchar c, input_buf *buffer);

/**
 * @brief Write data to output (with buffering).
 *
 * @param[in] data - Data buffer.
 * @param[in] length - Data length.
 * @return Whether write succeeded.
 * @note May write to file, but most likely to buffer.
 */
bool nrl_io_write(const char *data, uint32_t length);

/**
 * @brief Send escape sequence to the output.
 *
 * @param[in] escape - Escape sequence identifier.
 */
bool nrl_io_write_escape(terminfo_output escape);

/**
 * @brief Send buffered data to echo file.
 *
 * @return Whether write succeeded.
 */
bool nrl_io_flush(void);

/**
 * @brief Zero all buffer data (for secure applications).
 */
void nrl_io_wipe_buffers(void);

/**
 * @brief Enables or disabled echo.
 *
 * @param[in] enabled - If echo should be enabled.
 */
void nrl_io_echo_state(bool enabled);
