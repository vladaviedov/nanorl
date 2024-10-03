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

static bool check_args(const nrl_config *config);
static bool init(const nrl_config *config);
static bool deinit(const nrl_config *config);

char *nanorl(const nrl_config *config, nrl_error *error) {
	if (!check_args(config)) {
		safe_assign(error, NRL_ERROR_ARG);
		return NULL;
	}
	if (!init(config)) {
		safe_assign(error, NRL_ERROR_SYSTEM);
		return NULL;
	}

	// Write prompt, if there is one
	if (config->prompt != NULL) {
		if (!nrl_io_write(config->prompt, strlen(config->prompt))) {
			safe_assign(error, NRL_ERROR_SYSTEM);
			return NULL;
		}
	}
	if (!nrl_io_flush()) {
		safe_assign(error, NRL_ERROR_SYSTEM);
		return NULL;
	}

	line_data line = {
		.buffer = vec_init(sizeof(char)),
		.cursor = 0,
		.dirty = false,
	};

	if (!deinit(config)) {
		vec_deinit(&line.buffer);
		safe_assign(error, NRL_ERROR_SYSTEM);
		return NULL;
	}

	if (read_buf.eof && line.buffer.count == 0) {
		vec_deinit(&line.buffer);
		safe_assign(error, NRL_ERROR_EOF);
		return NULL;
	}

	safe_assign(error, NRL_ERROR_OK);
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

	// TODO: sigaction

	nrl_io_init(config->read_file, config->echo_file, config->preload);
	if (!config->assume_smkx) {
		return nrl_io_write_escape(TIO_KEYPAD_XMIT);
	}

	return true;
}

static bool deinit(const nrl_config *config) {
	if (isatty(config->read_file)) {
		if (tcsetattr(config->read_file, TCSAFLUSH, &old_attrs) < 0) {
			return false;
		}
	}

	// TODO: sigaction

	if (!nrl_io_write("\n", 1)) {
		return false;
	}
	if (!nrl_io_write_escape(TIO_KEYPAD_LOCAL)) {
		return false;
	}
	return nrl_io_flush();
}

// @endcond
