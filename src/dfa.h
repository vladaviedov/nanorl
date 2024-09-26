/**
 * @cond internal
 * @file dfa.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Simplified DFA for escape sequences.
 */
#pragma once

#include <stdbool.h>

#include "terminfo.h"
#include "config.h"

/**
 * @brief Build an escape sequence DFA from terminfo data.
 */
void nrl_dfa_build(void);

/**
 * @brief Run the escape sequence parser.
 *
 * @param[in] next_char - Next character acquisition function.
 * @param[out] accept_buf - Buffer for parsed escape sequence.
 * @return true - Escape sequence parsed. \n
 *         false - Nothing was matched.
 */
bool nrl_dfa_parse(char (*next_char)(), terminfo_input *accept_buf);

#if DFA_DEBUG == 1
/**
 * @brief Print DFA tree to standard out.
 */
void nrl_dfa_print(void);
#endif // DEBUG

// @endcond
