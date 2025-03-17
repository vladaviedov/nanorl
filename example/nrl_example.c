#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <nanorl/nanorl.h>
#include <nanorl/escape.h>

#ifdef __GNUC__
#define unused __attribute__((unused))
#else
#define unused
#endif

static const char *err_to_string(nrl_error err);
static nrl_state tab_func(const nrl_state *state, unused nrl_escape code);

int main(void) {
	printf("nanorl version: %s\n\n", nrl_version);

	nrl_config config = nrl_default_config();
	nrl_error error;
	config.prompt = "enter something: ";

	// Basic usage
	char *input = nanorl(&config, &error);
	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);

	// Obscured
	config.echo_mode = NRL_ECHO_OBSCURED;
	input = nanorl(&config, &error);
	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);

	// No echo
	config.echo_mode = NRL_ECHO_OFF;
	input = nanorl(&config, &error);
	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);

	// Preload
	config.echo_mode = NRL_ECHO_ON;
	config.prompt = "edit this text: ";
	config.preload = "hello world";
	input = nanorl(&config, &error);
	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);

	// Escape handler
	nrl_escape_handler tab_handler = {
		.id = NRL_ESC_TAB,
		.cursor_type = NRL_CT_BYTE,
		.func = &tab_func,
	};
	nrl_escape_handler *handler_list[] = {
		&tab_handler,
		NULL,
	};
	config.prompt = "try hitting tab: ";
	config.preload = NULL;
	config.custom_handlers = handler_list;
	input = nanorl(&config, &error);
	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);

	return 0;
}

static const char *err_to_string(nrl_error err) {
	switch (err) {
	case NRL_ERROR_OK:
		return "Success!";
	case NRL_ERROR_ARG:
		return "Bad argument";
	case NRL_ERROR_SYSTEM:
		return "System error";
	case NRL_ERROR_EOF:
		return "EOF reached";
	case NRL_ERROR_INTERRUPT:
		return "Interrupted!";
	}

	return NULL;
}

static nrl_state tab_func(const nrl_state *state, unused nrl_escape code) {
	// Create new string with 4 more characters
	uint32_t new_len = strlen(state->line) + 4 + 1;
	char *modified = malloc(sizeof(char) * new_len);

	// Add four spaces at cursor
	strncpy(modified, state->line, state->cursor);
	modified[state->cursor] = '\0';
	strcat(modified, "    ");
	strcat(modified, state->line + state->cursor);

	// Create state import object
	nrl_state new_state = {
		.line = modified,
		.cursor = state->cursor + 4,
	};

	return new_state;
}
