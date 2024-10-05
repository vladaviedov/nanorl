#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nanorl/nanorl.h>

const char *err_to_string(nrl_error err);

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

	return 0;
}

const char *err_to_string(nrl_error err) {
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
