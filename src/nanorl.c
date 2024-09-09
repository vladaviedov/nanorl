/**
 * @file nanorl.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Small line editing library.
 */
#define _POSIX_C_SOURCE 200809L
#include "nanorl.h"

#include <unistd.h>
#include <stdbool.h>

// Sane default nanorl config
static const nrl_config default_conf = {
	.read_file = STDIN_FILENO,
	.echo_file = STDOUT_FILENO,
	.prompt = NULL,
	.preload = NULL,
	.assume_smkx = false,
	.echo_mode = NRL_ECHO_ON,
};

char *nanorl(const nrl_config *config, nrl_error *error) {
	// TODO: implement
	return NULL;
}

char *nrl_readline(const char *prompt) {
	nrl_config config = nrl_default_config();
	config.prompt = prompt;
	return nanorl(&config, NULL);
}

nrl_config nrl_default_config(void) {
	return default_conf;
}
