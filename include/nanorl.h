/**
 * @file nanorl.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Small line editing library.
 */
#pragma once

#include <stdbool.h>

// Fallback library version
// Normally this is set by the build system
#ifndef NRL_VERSION
#define NRL_VERSION "pre0.1"
#endif // NRL_VERSION

/**
 * @enum nrl_echo_mode
 * Echo mode.
 *
 * @var nrl_echo_mode::NRL_ECHO_ON
 * Typed characters are not printed to the screen.
 *
 * @var nrl_echo_mode::NRL_ECHO_OFF
 * Typed characters are printed normally to the screen.
 *
 * @var nrl_echo_mode::NRL_ECHO_OBSCURED
 * Replacement characters are printed for each typed character.
 */
typedef enum {
	NRL_ECHO_OFF = 0,
	NRL_ECHO_ON = 1,
	NRL_ECHO_OBSCURED,
} nrl_echo_mode;

/**
 * @enum nrl_error
 * Error codes for nanorl library functions.
 *
 * @var nrl_error::NRL_ERROR_OK
 * No errors detected; data returned.
 *
 * @var nrl_error::NRL_ERROR_INTERRUPT
 * Interrupted by signal; data returned.
 *
 * @var nrl_error::NRL_ERROR_SYSTEM
 * System error occured, check errno; data not returned.
 *
 * @var nrl_error::NRL_ERROR_EOF
 * End-of-file reached with no input; data not returned.
 *
 * @var nrl_error::NRL_ERROR_ARG
 * Invalid configuration; data not returned.
 */
typedef enum {
	NRL_ERROR_OK = 0,
	NRL_ERROR_INTERRUPT = -1,
	NRL_ERROR_SYSTEM = -2,
	NRL_ERROR_EOF = -3,

	NRL_ERROR_ARG = -4,
} nrl_error;

/**
 * @struct nrl_config
 * Configuration options.
 *
 * @var nrl_config::read_file
 * Character input file descriptor.
 *
 * @var nrl_config::echo_file
 * Character echo file descriptor.
 *
 * @var nrl_config::prompt
 * @info Can be NULL.
 * Prompt message printed to the user.
 *
 * @var nrl_config::preload
 * @info Can be NULL.
 * Initial line buffer text.
 *
 * @var nrl_config::assume_smkx
 * Should be set if the caller application utilized xterm application mode.
 *
 * @var nrl_config::echo_mode
 * Echo behavior mode.
 */
typedef struct {
	int read_file;
	int echo_file;

	const char *prompt;
	const char *preload;

	bool assume_smkx;
	nrl_echo_mode echo_mode;
} nrl_config;

/**
 * @brief Start nanorl.
 *
 * @param[in] config - nanorl configuration.
 * @param[out] error - Error code buffer (can be NULL).
 * @return Inputted line; NULL on some errors.
 */
char *nanorl(const nrl_config *config, nrl_error *error);

/**
 * @brief Start nanorl with default settings and provided prompt.
 *
 * @param[in] prompt - Prompt message printed to the user.
 * @return Inputted line; NULL on some errors.
 */
char *nrl_readline(const char *prompt);

/**
 * @brief Retrieve the default configuration.
 *
 * @return Default config.
 */
nrl_config nrl_default_config(void);
