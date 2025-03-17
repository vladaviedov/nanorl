/**
 * @file manip.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024-2025
 * @license LGPLv3.0
 * @brief Line manipations.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <c-utils/vector.h>

#include "config.h"
#include "nanorl.h"
#include "terminfo.h"

/**
 * @struct line_data
 * Represents the line being edited in memory.
 *
 * @var line_data::buffer
 * Resizable character buffer.
 *
 * @var line_data::cursor
 * Current virtual cursor placement.
 *
 * @var line_data::render_cursor
 * Current real cursor placement.
 *
 * @var line_data::dirty
 * Set when line is modified: memory and screen are out of sync.
 */
typedef struct {
	vector buffer;
	uint32_t cursor;
	uint32_t render_cursor;
	bool dirty;
} line_data;

/**
 * @brief Insert ASCII characters into the line.
 *
 * @param[in,out] line - Line data object.
 * @param[in] data - Characters to insert.
 * @param[in] length - Character count.
 */
void nrl_manip_insert_ascii(line_data *line, const char *data, uint32_t length);

/**
 * @brief Evaluate an escape sequence.
 *
 * @param[in,out] line - Line data object.
 * @param[in] escape - Escape sequence identifier.
 */
void nrl_manip_eval_escape(line_data *line, terminfo_input escape);

#if CUSTOM_ESCAPES == 1
/**
 * @brief Populate the custom escape handler table.
 *
 * @param[in] config - Configuration.
 */
void nrl_manip_make_custom_table(const nrl_config *config);

/**
 * @brief Clear the custom escape handler table.
 */
void nrl_manip_clear_custom_table(void);

/**
 * @brief Evaluate a custom escape sequence.
 *
 * @param[in,out] line - Line data object.
 * @param[in] escape - Custom escape sequence identifier.
 * @return true - A handler was invoked. \n
 *         false - A handler was not set.
 */
bool nrl_manip_eval_custom(line_data *line, terminfo_custom escape);
#endif
