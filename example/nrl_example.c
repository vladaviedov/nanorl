#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nanorl/nanorl.h>

int main(void) {
	// Basic usage
	char *input = nrl_readline("enter something: ");
	printf("You typed: %s\n\n", input);
	free(input);

	return 0;
}
