/**
 * @file dfa.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.2
 * @date 2024-2025
 * @license LGPLv3.0
 * @brief Simplified DFA for escape sequences.
 */
#pragma once

#include <stdbool.h>

#include "config.h"
#include "terminfo.h"

/**
 * @union dfa_acceptor
 * Stored DFA acceptor value.
 *
 * @var dfa_acceptor::input
 * Standard escape sequence.
 *
 * @var dfa_acceptor::custom
 * User-configurable escape sequence.
 */
typedef union {
	terminfo_input input;
	terminfo_custom custom;
} dfa_acceptor;

/**
 * @enum dfa_result
 * DFA parse result.
 *
 * @var dfa_result::DFA_RES_EMPTY
 * Nothing was matched.
 *
 * @var dfa_result::DFA_RES_INPUT
 * Standard sequence matched.
 *
 * @var dfa_result::DFA_RES_CUSTOM
 * User-configurable sequence matched.
 */
typedef enum {
	DFA_RES_EMPTY,
	DFA_RES_INPUT,
	DFA_RES_CUSTOM,
} dfa_result;

/**
 * @brief Build an escape sequence DFA from terminfo data.
 */
void nrl_dfa_build(void);

/**
 * @brief Run the escape sequence parser.
 *
 * @param[in] next_char - Next character acquisition function.
 * @param[out] accept_buf - Buffer for parsed escape sequence.
 * @return Parse result (see @ref dfa_result).
 */
dfa_result nrl_dfa_parse(char (*next_char)(), dfa_acceptor *accept_buf);

#if DEBUG == 1
/**
 * @brief Print DFA tree to standard out.
 */
void nrl_dfa_print(void);
#endif // DEBUG
