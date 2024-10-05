/**
 * @cond internal
 * @file nanorl.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Small line editing library.
 */
#define _POSIX_C_SOURCE 200809L
#include "nanorl.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <c-utils/vector.h>

#include "dfa.h"
#include "io.h"
#include "manip.h"
#include "terminfo.h"

/**
 * @def NRL_VERSION
 * nanorl version, normally set by the build system.
 *
 * @note Fallback value is shown.
 */
#ifndef NRL_VERSION
#define NRL_VERSION "v2-pre0.1"
#endif // NRL_VERSION

// extern
const char *nrl_version = NRL_VERSION;

/**
 * @var old_attrs
 * Stored original termios settings for restoration.
 */
static struct termios old_attrs;

/**
 * @var old_sighup_sa
 * @var old_sigint_sa
 * @var old_sigterm_sa
 * @var old_sigquit_sa
 * Stored original signal handlers.
 */
static struct sigaction old_sighup_sa;
static struct sigaction old_sigint_sa;
static struct sigaction old_sigterm_sa;
static struct sigaction old_sigquit_sa;

/**
 * @var intr_code
 * Storage for signal numbers receieved.
 */
static int intr_code;

/**
 * @var default_conf
 * Default nanorl configuration.
 */
static const nrl_config default_conf = {
	.read_file = STDIN_FILENO,
	.echo_file = STDOUT_FILENO,
	.prompt = NULL,
	.preload = NULL,
	.assume_smkx = false,
	.echo_mode = NRL_ECHO_ON,
};

#define safe_assign(var_ptr, val)                                              \
	if (var_ptr != NULL) {                                                     \
		*var_ptr = val;                                                        \
	}

static void sig_handle(int code);
static bool check_args(const nrl_config *config);
static bool init(const nrl_config *config);
static bool deinit(const nrl_config *config);

char *nanorl(const nrl_config *config, nrl_error *error) {
	errno = 0;

	if (!check_args(config)) {
		safe_assign(error, NRL_ERROR_ARG);
		return NULL;
	}
	if (!init(config)) {
		safe_assign(error, NRL_ERROR_SYSTEM);
		return NULL;
	}

	line_data line = {
		.buffer = vec_init(sizeof(char)),
		.cursor = 0,
		.render_cursor = 0,
		.dirty = false,
	};

	input_type read_res;
	input_buf read_buf;
	while ((read_res = nrl_io_read(&read_buf)) != INPUT_STOP) {
		uint32_t rendered_count = line.buffer.count;

		switch (read_res) {
		case INPUT_ASCII:
			nrl_manip_insert_ascii(&line, read_buf.text, read_buf.length);
			break;
		case INPUT_ESCAPE:
			nrl_manip_eval_escape(&line, read_buf.escape);
			break;
		default:
			break;
		}

		// Perform a full re-render
		if (!read_buf.more && line.dirty) {
			// Move cursor to the beginning
			for (uint32_t i = 0; i < line.render_cursor; i++) {
				nrl_io_write_escape(TIO_CURSOR_LEFT);
			}

			// Print line data
			if (config->echo_mode == NRL_ECHO_OBSCURED) {
				for (uint32_t i = 0; i < line.buffer.count; i++) {
					nrl_io_write("*", 1);
				}
			} else {
				nrl_io_write(line.buffer.data, line.buffer.count);
			}
			uint32_t printed_count = line.buffer.count;

			// Account for erased characters
			for (uint32_t i = line.buffer.count; i < rendered_count; i++) {
				nrl_io_write(" ", 1);
				printed_count++;
			}

			// Move cursor to correct location
			for (uint32_t i = printed_count; i > line.cursor; i--) {
				nrl_io_write_escape(TIO_CURSOR_LEFT);
			}

			line.dirty = false;
			line.render_cursor = line.cursor;
		}

		nrl_io_flush();
	}

	if (!deinit(config)) {
		vec_deinit(&line.buffer);
		safe_assign(error, NRL_ERROR_SYSTEM);
		return NULL;
	}

	// EOF condition
	if (read_buf.eof && line.buffer.count == 0) {
		vec_deinit(&line.buffer);
		safe_assign(error, NRL_ERROR_EOF);
		return NULL;
	}

	// Terminate string
	char null_char = '\0';
	vec_push(&line.buffer, &null_char);

	// Interrupt condition
	if (errno == EINTR) {
		safe_assign(error, NRL_ERROR_INTERRUPT);
	} else {
		safe_assign(error, NRL_ERROR_OK);
	}

	return vec_collect(&line.buffer);
}

char *nrl_readline(const char *prompt) {
	nrl_config config = nrl_default_config();
	config.prompt = prompt;
	return nanorl(&config, NULL);
}

nrl_config nrl_default_config(void) {
	return default_conf;
}

/**
 * @brief Signal handler for all signals.
 *
 * @param[in] code - Signal code.
 */
static void sig_handle(int code) {
	intr_code = code;
}

/**
 * @brief Perform argument validation
 *
 * @param[in] config - Configuration.
 * @return true - All arguments are valid. \n
 *         false - Invalid arguments detected.
 */
static bool check_args(const nrl_config *config) {
	if (config->read_file < 0 || config->echo_file < 0) {
		return false;
	}

	if (config->echo_mode < NRL_ECHO_OFF
		|| config->echo_mode > NRL_ECHO_OBSCURED) {
		return false;
	}

	return true;
}

/**
 * @brief Perform library initialization.
 *
 * @param[in] config - Configuration.
 * @return true - Successful init. \n
 *         false - Init failed.
 */
static bool init(const nrl_config *config) {
	if (!nrl_load_terminfo()) {
		return false;
	}
	nrl_dfa_build();

#if DFA_DEBUG == 1
	nrl_dfa_print();
#endif // DFA_DEBUG

	if (isatty(config->read_file)) {
		if (tcgetattr(config->read_file, &old_attrs) < 0) {
			return false;
		}

		struct termios new_attrs = old_attrs;
		new_attrs.c_lflag &= ~(ICANON | ECHO);
		if (tcsetattr(config->read_file, TCSAFLUSH, &new_attrs) < 0) {
			return false;
		}
	}

	// Setup signals
	struct sigaction nrl_sa;
	sigemptyset(&nrl_sa.sa_mask);
	nrl_sa.sa_flags = 0;
	nrl_sa.sa_handler = &sig_handle;

	if (sigaction(SIGHUP, &nrl_sa, &old_sighup_sa) < 0
		|| sigaction(SIGINT, &nrl_sa, &old_sigint_sa) < 0
		|| sigaction(SIGTERM, &nrl_sa, &old_sigterm_sa) < 0
		|| sigaction(SIGQUIT, &nrl_sa, &old_sigquit_sa) < 0) {
		return false;
	}

	// IO initialization
	nrl_io_echo_state(true);
	nrl_io_init(config->read_file, config->echo_file, config->preload);
	if (!config->assume_smkx) {
		if (!nrl_io_write_escape(TIO_KEYPAD_XMIT)) {
			return false;
		}
	}

	// Write prompt, if there is one
	if (config->prompt != NULL) {
		if (!nrl_io_write(config->prompt, strlen(config->prompt))) {
			return false;
		}
	}

	nrl_io_echo_state(config->echo_mode != NRL_ECHO_OFF);
	return nrl_io_flush();
}

static bool deinit(const nrl_config *config) {
	if (isatty(config->read_file)) {
		if (tcsetattr(config->read_file, TCSAFLUSH, &old_attrs) < 0) {
			return false;
		}
	}

	// Reset signals
	if (sigaction(SIGHUP, &old_sighup_sa, NULL) < 0
		|| sigaction(SIGINT, &old_sigint_sa, NULL) < 0
		|| sigaction(SIGTERM, &old_sigterm_sa, NULL) < 0
		|| sigaction(SIGQUIT, &old_sigquit_sa, NULL) < 0) {
		return false;
	}

	// Delete secure data remains
	if (config->echo_mode != NRL_ECHO_ON) {
		nrl_io_wipe_buffers();
	}

	nrl_io_echo_state(true);
	if (!nrl_io_write("\n", 1)) {
		return false;
	}
	if (!nrl_io_write_escape(TIO_KEYPAD_LOCAL)) {
		return false;
	}
	return nrl_io_flush();
}

// @endcond
