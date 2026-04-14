#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <c-utils/uchar.h>
#include <c-utils/ustring.h>
#include <nanorl/nanorl.h>
#include <nanorl/escape.h>

#ifdef __GNUC__
#define unused __attribute__((unused))
#else
#define unused
#endif

static void ex_simple(void);
static void ex_normal(void);
static void ex_obscured(void);
static void ex_hidden(void);
static void ex_preload(void);
static void ex_escape_utf8(void);
static void ex_escape_utf32(void);

static const char *err_to_string(nrl_error err);
static nrl_state tab_func(const nrl_state *state, unused nrl_escape code);
static nrl_state ja_convert_func(const nrl_state *state, unused nrl_escape code);

int main(void) {
	printf("nanorl version: %s\n\n", nrl_version);

	ex_simple();
	ex_normal();
	ex_obscured();
	ex_hidden();
	ex_preload();
	ex_escape_utf8();
	ex_escape_utf32();

	return 0;
}

/**
 * @brief Simplified usage example.
 */
static void ex_simple(void) {
	// Simplified API access
	char *input = nrl_readline("my prompt: ");

	printf("You typed: %s\n\n", input);
	free(input);
}

/**
 * @brief Normal usage example.
 */
static void ex_normal(void) {
	// Get default config and adjust prompt
	nrl_config config = nrl_default_config();
	config.prompt = "enter something: ";

	// Call nanorl
	nrl_error error;
	char *input = nanorl(&config, &error);

	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);
}

/**
 * @brief Obscured echo mode.
 */
static void ex_obscured(void) {
	// Get default config and adjust settings
	nrl_config config = nrl_default_config();
	config.prompt = "input secret: ";
	config.echo_mode = NRL_ECHO_OBSCURED;

	// Call nanorl with obscured echo
	nrl_error error;
	char *input = nanorl(&config, &error);

	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);
}

/**
 * @brief Hidden echo mode.
 */
static void ex_hidden(void) {
	// Get default config and adjust settings
	nrl_config config = nrl_default_config();
	config.prompt = "input secret: ";
	config.echo_mode = NRL_ECHO_OFF;

	// Call nanorl with no echo
	nrl_error error;
	char *input = nanorl(&config, &error);

	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);
}

/**
 * @brief Preload text edit example.
 */
static void ex_preload(void) {
	// Get default config and adjust settings
	nrl_config config = nrl_default_config();
	config.prompt = "edit this text: ";
	config.preload = "hello world";

	// Call nanorl
	nrl_error error;
	char *input = nanorl(&config, &error);

	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);

}

/**
 * @brief UTF-8 custom escapes.
 */
static void ex_escape_utf8(void) {
	// Define escape handler
	// This one will insert 4 spaces when tab is pressed
	nrl_escape_handler tab_handler = {
		.id = NRL_ESC_TAB,
		.format = NRL_DF_UTF8,
		.func = &tab_func,
	};

	// Collect all needed handlers
	nrl_escape_handler *handler_list[] = {
		&tab_handler,
		NULL,
	};

	// Modify config
	nrl_config config = nrl_default_config();
	config.prompt = "try hitting tab: ";
	config.custom_handlers = handler_list;

	// Call nanorl
	nrl_error error;
	char *input = nanorl(&config, &error);

	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);
}

/**
 * @brief UTF-32 custom escapes.
 */
static void ex_escape_utf32(void) {
	// Define escape handler
	// This one will convert between Japanese hiragana & katakana characters
	// You may need to disable your IME before trying
	nrl_escape_handler ja_convert = {
		.id = NRL_ESC_KEY_F7,
		.format = NRL_DF_UTF32,
		.func = &ja_convert_func,
	};

	// Collect all needed handlers
	nrl_escape_handler *handler_list[] = {
		&ja_convert,
		NULL,
	};

	// Modify config
	nrl_config config = nrl_default_config();
	config.prompt = "try hitting f7: ";
	config.preload = "こんにちは！";
	config.custom_handlers = handler_list;

	// Call nanorl
	nrl_error error;
	char *input = nanorl(&config, &error);

	printf("%s\n", err_to_string(error));
	printf("You typed: %s\n\n", input);
	free(input);
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
	uint32_t new_len = strlen(state->line.utf8_line) + 4 + 1;
	char *modified = malloc(sizeof(char) * new_len);

	// Add four spaces at cursor
	strncpy(modified, state->line.utf8_line, state->cursor);
	modified[state->cursor] = '\0';
	strcat(modified, "    ");
	strcat(modified, state->line.utf8_line + state->cursor);

	// Create state import object
	nrl_state new_state = {
		.line.utf8_line = modified,
		.cursor = state->cursor + 4,
	};

	return new_state;
}

static nrl_state ja_convert_func(const nrl_state *state, unused nrl_escape code) {
	uchar *data = state->line.utf32_line;
	size_t len = ustrlen(data);

	// Create a copy of the string
	uchar *modified = ustrdup(data);

	for (size_t i = 0; i < len; i++) {
		uchar uc = data[i];

		// Is hiragana
		if (uc >= 0x3041 && uc <= 0x3097) {
			// Convert to katakana
			modified[i] += 0x60;
		}

		// Is katakana
		if (uc >= 0x30a1 && uc <= 0x30f7) {
			// Convert to hiragana
			modified[i] -= 0x60;
		}
	}

	// Create state import object
	nrl_state new_state = {
		.line.utf32_line = modified,
		.cursor = state->cursor,
	};

	return new_state;
}
