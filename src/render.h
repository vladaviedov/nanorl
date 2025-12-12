/**
 * @file render.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2025
 * @license LGPLv3.0
 * @brief Line rendering.
 */
#pragma once

#include "manip.h"
#include "nanorl.h"

/**
 * @brief Initialize render module.
 *
 * @param[in] mode - Echo mode.
 * @param[in] echo_file - Echo file descriptor.
 */
void nrl_render_init(nrl_echo_mode mode, int echo_file);

/**
 * @brief Update stored terminal dimensions.
 *
 * @return true - Successfully update dimensions.\n
 *         false - All methods failed.
 */
bool nrl_render_query_size(void);

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
