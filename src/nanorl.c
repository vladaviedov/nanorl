/**
 * @file nanorl.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.1
 * @date 2024-2026
 * @license LGPLv3.0
 * @brief Small line editing library.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700 // Use SIGWINCH if available
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

#include <c-utils/uchar.h>
#include <c-utils/vector.h>

#include "dfa.h"
#include "io.h"
#include "manip.h"
#include "render.h"
#include "terminfo.h"

/**
 * @def NRL_VERSION
 * nanorl version, normally set by the build system.
 *
 * @note Fallback value is shown.
 */
#ifndef NRL_VERSION
#define NRL_VERSION "2.0.1"
#endif // NRL_VERSION

// extern
const char *nrl_version = NRL_VERSION;

/**
 * Stored original termios settings for restoration.
 */
static struct termios old_attrs;

// Stored signal handlers
static struct sigaction old_sighup_sa;
static struct sigaction old_sigint_sa;
static struct sigaction old_sigterm_sa;
static struct sigaction old_sigquit_sa;
static struct sigaction old_sigwinch_sa;

/**
 * Storage for signal numbers receieved.
 */
static int intr_code;

/**
 * Default nanorl configuration.
 */
static const nrl_config default_conf = {
	.read_file = STDIN_FILENO,
	.echo_file = STDOUT_FILENO,
	.prompt = NULL,
	.preload = NULL,
	.assume_smkx = false,
	.echo_mode = NRL_ECHO_ON,
	.custom_ignore_default = true,
	.custom_handlers = NULL,
};

#define safe_assign(var_ptr, val)                                              \
	if (var_ptr != NULL) {                                                     \
		*var_ptr = val;                                                        \
	}

static void sigwinch_handler();
static void generic_handler(int code);
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
		.buffer = vec_init(sizeof(uchar)),
		.cursor = 0,
		.render_cursor = 0,
		.dirty = false,
	};

	input_type read_res;
	input_buf read_buf;
	while ((read_res = nrl_io_read(&read_buf)) != INPUT_STOP) {
		if (read_res == INPUT_TEXT) {
			nrl_manip_insert_text(&line, read_buf.text, read_buf.length);
		} else if (read_res == INPUT_ESCAPE) {
			nrl_manip_eval_escape(&line, read_buf.escape.input);
		} else if (read_res == INPUT_CUSTOM_ESCAPE) {
#if CUSTOM_ESCAPES == 1
			if (!nrl_manip_eval_custom(&line, read_buf.escape.custom)) {
#endif
				if (!config->custom_ignore_default) {
					// If custom escapes are disabled or not set, just convert
					// to the ASCII representation
					const char *as_text
						= nrl_lookup_custom(read_buf.escape.custom);
					for (uint32_t i = 0; i < strlen(as_text); i++) {
						// ASCII is compatible with uchar
						if (nrl_io_parse_control((uchar)as_text[i],
												 &read_buf)) {
							nrl_manip_insert_text(&line, read_buf.text,
												  read_buf.length);
						} else {
							uchar as_unicode = (uchar)(*(as_text + i));
							nrl_manip_insert_text(&line, &as_unicode, 1);
						}
					}
				}
#if CUSTOM_ESCAPES == 1
			}
#endif
		}

		// Perform a full re-render
		if (!read_buf.more && line.dirty) {
			nrl_render_redraw(&line);
		}
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
	uchar null_char = 0;
	vec_push(&line.buffer, &null_char);

	// Interrupt condition
	if (errno == EINTR) {
		safe_assign(error, NRL_ERROR_INTERRUPT);
	} else {
		safe_assign(error, NRL_ERROR_OK);
	}

	// Extract data as an regular string
	uchar *uc_data = vec_collect(&line.buffer);
	char *data = utf8_encode(uc_data);
	free(uc_data);

	return data;
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
 * @brief Signal handler for SIGWINCH.
 *
 * @param[in] code - Signal code.
 */
static void sigwinch_handler() {
	nrl_render_notify_resize();
}

/**
 * @brief Signal handler for all signals (except SIGWINCH).
 *
 * @param[in] code - Signal code.
 */
static void generic_handler(int code) {
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
		fprintf(stderr,
				"[nanorl] warning: unable to parse terminal information\n");
	} else {
		nrl_dfa_build();
	}

#if DEBUG == 1
	nrl_dfa_print();
#endif // DEBUG

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

	// Setup sigwinch signal
#if defined(SIGWINCH)
	struct sigaction nrl_sigwinch_sa;
	sigemptyset(&nrl_sigwinch_sa.sa_mask);
	nrl_sigwinch_sa.sa_flags = SA_RESTART;
	nrl_sigwinch_sa.sa_handler = &sigwinch_handler;

	if (sigaction(SIGWINCH, &nrl_sigwinch_sa, &old_sigwinch_sa) < 0) {
		return false;
	}
#endif // defined(SIGWINCH)

	// Setup other signals
	struct sigaction nrl_sa;
	sigemptyset(&nrl_sa.sa_mask);
	nrl_sa.sa_flags = 0;
	nrl_sa.sa_handler = &generic_handler;

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

#if CUSTOM_ESCAPES == 1
	// Custom handlers are disabled for secure data
	if (config->echo_mode == NRL_ECHO_ON) {
		nrl_manip_make_custom_table(config);
	}
#endif // CUSTOM_ESCAPES

	nrl_io_echo_state(config->echo_mode != NRL_ECHO_OFF);
	if (!nrl_io_flush()) {
		return false;
	}

	// Needs to run after IO init is complete & prompt is printed. This hands
	// over the control of the terminal fully to the renderer. Other parts of
	// the code MUST NOT directly write to to echo file from this point onwards
	// until deinit.
	return nrl_render_init(config->echo_mode, config->echo_file);
}

/**
 * @brief Perform library teardown.
 *
 * @param[in] config - Configuration.
 * @return true - Successful teardown. \n
 *         false - Teardown failed.
 */
static bool deinit(const nrl_config *config) {
#if CUSTOM_ESCAPES == 1
	nrl_manip_clear_custom_table();
#endif // CUSTOM_ESCAPES

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

#if defined(SIGWINCH)
	if (sigaction(SIGWINCH, &old_sigwinch_sa, NULL) < 0) {
		return false;
	}
#endif // defined(SIGWINCH)

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
