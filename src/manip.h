/**
 * @cond internal
 * @file manip.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Line manipations.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <c-utils/vector.h>

#include "terminfo.h"

/**
 * @struct line_data
 * Represents the line being edited in memory.
 *
 * @var line_data::buffer
 * Resizable character buffer.
 *
 * @var line_data::cursor
 * Current cursor placement.
 *
 * @var line_data::dirty
 * Set when line is modified: memory and screen are out of sync.
 */
typedef struct {
	vector buffer;
	uint32_t cursor;
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

// @endcond
