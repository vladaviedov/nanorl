#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nanorl/nanorl.h>

int main(void) {
	nrl_config config = nrl_default_config();
	config.prompt = "enter something: ";

	// Basic usage
	char *input = nanorl(&config, NULL);
	printf("You typed: %s\n\n", input);
	free(input);

	// Obscured
	config.echo_mode = NRL_ECHO_OBSCURED;
	input = nanorl(&config, NULL);
	printf("You typed: %s\n\n", input);
	free(input);

	// No echo
	config.echo_mode = NRL_ECHO_OFF;
	input = nanorl(&config, NULL);
	printf("You typed: %s\n\n", input);
	free(input);

	// Preload
	config.echo_mode = NRL_ECHO_ON;
	config.preload = "value";
	input = nanorl(&config, NULL);
	printf("You typed: %s\n\n", input);
	free(input);

	return 0;
}
