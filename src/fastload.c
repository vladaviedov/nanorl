/**
 * @file fastload.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024-2025
 * @license LGPLv3.0
 * @brief Terminfo optimization for common terminals.
 */
#define _POSIX_C_SOURCE 200809L
#include "fastload.h"

#include "config.h"

#if FASTLOAD == 1
#include <string.h>

#include "terminfo.h"

static const char *xterm_inputs_stub[TII_COUNT] = {
	"\033OD", "\033OC", "\177", "\033OH", "\033OF", "\033[3~",
};
static const char *xterm_customs_stub[TIC_COUNT] = {
	"\033OA",
	"\033OB",
	"\t",
};
static const char *xterm_outputs_stub[TIO_COUNT] = {
	"\b",
	"\033[C",
	"\033[?1l\033>",
	"\033[?1h\033=",
};
static const char *xterm_specials_stub[TIS_COUNT] = {
	"\033[%i%d;%dR",
	"\033[6n",
};

void nrl_fl_xterm(char **inputs,
				  char **customs,
				  char **outputs,
				  char **specials) {
	memcpy(inputs, &xterm_inputs_stub, TII_COUNT * sizeof(char *));
	memcpy(customs, &xterm_customs_stub, TIC_COUNT * sizeof(char *));
	memcpy(outputs, &xterm_outputs_stub, TIO_COUNT * sizeof(char *));
	memcpy(specials, &xterm_specials_stub, TIS_COUNT * sizeof(char *));
}
#endif // FASTLOAD
