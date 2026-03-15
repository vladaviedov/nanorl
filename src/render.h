/**
 * @file render.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 2.0.1
 * @date 2025-2026
 * @license LGPLv3.0
 * @brief Line rendering.
 */
#pragma once

#include <stdbool.h>

#include "manip.h"
#include "nanorl.h"

/**
 * @brief Initialize render module.
 *
 * @param[in] mode - Echo mode.
 * @param[in] echo_file - Echo file descriptor.
 * @return true - Init successful.\n
 *         false - Init failed.
 */
bool nrl_render_init(nrl_echo_mode mode, int echo_file);

/**
 * @brief Notify renderer about terminal being resized.
 */
void nrl_render_notify_resize(void);

/**
 * @brief Redraw the line.
 *
 * @param[in] line - Line data.
 */
void nrl_render_redraw(line_data *line);

/**
 * @brief Move the render cursor to the same position as the logic cursor.
 *
 * @param[in] line - Line data.
 */
void nrl_render_sync_cursors(line_data *line);
