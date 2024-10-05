/**
 * @cond internal
 * @file fastload.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Terminfo optimization for common terminals.
 */
#pragma once

#include "config.h"

#if FASTLOAD == 1
/**
 * @brief Load 'xterm' configuration.
 *
 * @param[out] inputs - Input storage to fill.
 * @param[out] outputs - Output storage to fill.
 */
void nrl_fl_xterm(char **inputs, char **outputs);
#endif // FASTLOAD

// @endcond
