/**
 * @cond internal
 * @file terminfo.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief terminfo parser.
 */
#define _POSIX_C_SOURCE 200809L
#include "terminfo.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fastload.h"

/**
 * @def MAGIC_INT16
 * Legacy format magic number. Entry uses 16-bit numbers.
 * @note Reference: 'man term'
 */
#define MAGIC_INT16 0432

/**
 * @def MAGIC_INT32
 * Extended format magic number. Entry uses 32-bit numbers.
 * @note Reference: 'man term'
 */
#define MAGIC_INT32 01036

/**
 * @var sysdb_path
 * Locations of the system terminfo databases. At least one macro should be
 * enabled during compilation.
 * @note Reference: 'man terminfo'
 */
static const char *sysdb_path[] = {

#if TERMINFO_DEBIAN == 1
	"/etc/terminfo",
	"/lib/terminfo",
#endif // TERMINFO_DEBIAN

#if TERMINFO_FREEBSD == 1
	"/usr/share/etc/terminfo",
#endif // TERMINFO_FREEBSD

#if TERMINFO_NETBSD == 1
	"/usr/share/misc/terminfo",
#endif // TERMINFO_NETBSD

#if TERMINFO_COMMON == 1
	"/usr/share/terminfo/",
#endif // TERMINFO_COMMON

	NULL,
};

/**
 * @var input_seq_indices
 * Indices into the strings terminfo array for input escape sequences.
 * @note Reference: ncurses source 'include/Caps'
 */
static const uint8_t input_seq_indices[] = {
	79u,  // key_left
	83u,  // key_right
	55u,  // key_backspace
	76u,  // key_home
	164u, // key_end
	59u,  // key_dc
};

/**
 * @var output_seq_indices
 * Indices into the strings terminfo array for output escape sequences.
 * @note Reference: ncurses source 'include/Caps'
 */
static const uint8_t output_seq_indices[] = {
	14u, // cursor_left
	17u, // cursor_right,
	88u, // keypad_local
	89u  // keypad_xmit
};

static bool attempted_load = false;
static bool load_result = false;

static char *inputs[TII_COUNT] = { NULL };
static char *outputs[TIO_COUNT] = { NULL };

static FILE *find_entry(const char *term);
static FILE *try_open(const char *db_path, const char *term);
static bool parse(FILE *terminfo);

bool nrl_load_terminfo(void) {
	if (attempted_load) {
		return load_result;
	}

	attempted_load = true;

	const char *env_term = getenv("TERM");
	if (env_term == NULL) {
		// TODO: warning
		return false;
	}

#if FASTLOAD == 1
	if (strstr(env_term, "xterm")) {
		nrl_fl_xterm((char **)&inputs, (char **)&outputs);
	}
#endif // FASTLOAD

	FILE *terminfo = find_entry(env_term);
	if (terminfo == NULL) {
		return false;
	}

	load_result = parse(terminfo);
	return load_result;
}

const char *nrl_lookup_input(terminfo_input id) {
	return inputs[id];
}

const char *nrl_lookup_output(terminfo_output id) {
	return outputs[id];
}

/** Static */

/**
 * @brief Find the terminfo entry for the given terminal.
 *
 * @param[in] term - Terminal name.
 * @return Open file or NULL (if does not exist).
 */
static FILE *find_entry(const char *term) {
	// $TERMINFO
	const char *env_terminfo = getenv("TERMINFO");
	if (env_terminfo != NULL) {
		FILE *entry = try_open(env_terminfo, term);
		if (entry != NULL) {
			return entry;
		}
	}

	// $HOME/.terminfo
	const char *env_home = getenv("HOME");
	if (env_home != NULL) {
		uint32_t db_path_len = strlen(env_home) + strlen("/.terminfo") + 1;
		char db_path[db_path_len];
		sprintf(db_path, "%s/.terminfo", env_home);

		FILE *entry = try_open(db_path, term);
		if (entry != NULL) {
			return entry;
		}
	}

	// $TERMINFO_DIRS
	const char *env_terminfo_dirs = getenv("TERMINFO_DIRS");
	if (env_terminfo_dirs != NULL) {
		char *copy = strdup(env_terminfo_dirs);

		// Directories are colon-separated
		char *dir = strtok(copy, ":");
		while (dir != NULL) {
			FILE *entry = try_open(dir, term);
			if (entry != NULL) {
				free(copy);
				return entry;
			}

			dir = strtok(NULL, ":");
		}

		free(copy);
	}

	// System databases
	const char **sysdb_trav = sysdb_path;
	const char *sysdb;
	while ((sysdb = *sysdb_trav++) != NULL) {
		FILE *entry = try_open(sysdb, term);
		if (entry != NULL) {
			return entry;
		}
	}

	return NULL;
}

/**
 * @brief Try opening the entry in a certain terminfo database directory.
 *
 * @param[in] db_path - Path to database.
 * @param[in] term - Terminal name.
 * @return Open file or NULL (if does not exist).
 */
static FILE *try_open(const char *db_path, const char *term) {
	// Path format: TERMINFO/FIRST_LETTER/TERMINAL \0
	uint32_t length = strlen(db_path) + 3 + strlen(term) + 1;
	char full_path[length];
	snprintf(full_path, length, "%s/%c/%s", db_path, term[0], term);

	return fopen(full_path, "r");
}

/**
 * @brief Parse terminfo entry.
 *
 * @param[in] terminfo - Terminfo file.
 * @return true - Success.\n
 *         false - Failed to parse.
 */
static bool parse(FILE *terminfo) {
	// Parse header
	// Reference: 'man term'
	uint16_t header[6];
	fread(header, sizeof(uint16_t), 6, terminfo);

	uint32_t number_size;
	switch (header[0]) {
	case MAGIC_INT16:
		number_size = sizeof(int16_t);
		break;
	case MAGIC_INT32:
		number_size = sizeof(int32_t);
		break;
	default:
		return false;
	}

	uint32_t string_start = header[1] + header[2];
	// Extra padding byte
	if (string_start & 1) {
		string_start++;
	}
	string_start += number_size * header[3];

	// Load strings section
	if (fseek(terminfo, string_start, SEEK_CUR) < 0) {
		return false;
	}
	int16_t strings[header[4]];
	if (fread(strings, sizeof(int16_t), header[4], terminfo) != header[4]) {
		return false;
	}

	// Load strings table
	char strings_table[header[5]];
	if (fread(strings_table, sizeof(char), header[5], terminfo) != header[5]) {
		return false;
	}

	// Lookup all relevant capability
	for (uint32_t i = 0; i < TII_COUNT; i++) {
		int16_t offset = strings[input_seq_indices[i]];
		char *sequence = (offset < 0) ? NULL : (strings_table + offset);

		if (sequence == NULL || strlen(sequence) == 0) {
			inputs[i] = NULL;
		} else {
			inputs[i] = strdup(sequence);
		}
	}
	for (uint32_t i = 0; i < TIO_COUNT; i++) {
		int16_t offset = strings[output_seq_indices[i]];
		char *sequence = (offset < 0) ? NULL : (strings_table + offset);

		if (sequence == NULL || strlen(sequence) == 0) {
			outputs[i] = NULL;
		} else {
			outputs[i] = strdup(sequence);
		}
	}

	return true;
}
