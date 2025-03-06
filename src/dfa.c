/**
 * @cond internal
 * @file dfa.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024-2025
 * @license LGPLv3.0
 * @brief Simplified DFA for escape sequences.
 */
#define _POSIX_C_SOURCE 200809L
#include "dfa.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "terminfo.h"

typedef struct dfa_node dfa_node;

/**
 * @union dfa_value
 *
 * @var dfa_value::children
 * For tree nodes - array of child nodes.
 *
 * @var dfa_value::accept
 * For leaf nodes - acceptor value.
 */
typedef union {
	dfa_node *children;
	dfa_acceptor accept;
} dfa_value;

/**
 * @struct dfa_node
 *
 * @var dfa_node::edge
 * Value required to enter this node from a previous node.
 *
 * @var dfa_node::value
 * Stores informating about what to do next.
 *
 * @var dfa_node::children_count
 * Count of children in 'value'. If not null, this is not a leaf node.
 */
struct dfa_node {
	char edge;
	bool custom;
	dfa_value value;
	uint32_t children_count;
};

/**
 * @var root
 * Root DFA element. Its edge value does not matter.
 */
static dfa_node root = {
	.edge = '\0',
	.custom = false,
	.value.children = NULL,
	.children_count = 0,
};

static void
dfa_insert(const char *sequence, dfa_acceptor accept_value, bool custom);

void nrl_dfa_build(void) {
	for (uint32_t i = 0; i < TII_COUNT; i++) {
		const char *sequence = nrl_lookup_input(i);
		if (sequence != NULL) {
			dfa_acceptor acceptor = { .input = i };
			dfa_insert(sequence, acceptor, false);
		}
	}

	for (uint32_t i = 0; i < TIC_COUNT; i++) {
		const char *sequence = nrl_lookup_custom(i);
		if (sequence != NULL) {
			dfa_acceptor acceptor = { .custom = i };
			dfa_insert(sequence, acceptor, true);
		}
	}
}

dfa_result nrl_dfa_parse(char (*next_char)(), dfa_acceptor *accept_buf) {
	// Empty tree
	if (root.children_count == 0) {
		return DFA_RES_EMPTY;
	}

	const dfa_node *trav = &root;

	while (true) {
		char input = next_char();
		for (uint32_t i = 0; i < trav->children_count; i++) {
			const dfa_node *child = &trav->value.children[i];
			if (input == child->edge) {
				// Check if leaf node reached
				if (child->children_count == 0) {
					*accept_buf = child->value.accept;
					return (child->custom) ? DFA_RES_CUSTOM : DFA_RES_INPUT;
				}

				trav = child;
				goto parse_next;
			}
		}

		return DFA_RES_EMPTY;

		// For breaking out of inner for loop
parse_next:
		continue;
	}
}

#if DFA_DEBUG == 1
#include <stdio.h>

void print_helper(const dfa_node *node, uint32_t indent) {
	for (uint32_t i = 0; i < indent; i++) {
		printf("    ");
	}

	if (node->edge == 0) {
		printf("Root");
	} else if (node->edge < 0x20) {
		printf("^%c", node->edge + 0x40);
	} else {
		printf("%c", node->edge);
	}

	printf("\n");

	for (uint32_t i = 0; i < node->children_count; i++) {
		print_helper(&node->value.children[i], indent + 1);
	}
}

void nrl_dfa_print(void) {
	print_helper(&root, 0);
}
#endif

/**
 * @brief Insert a new sequence into the DFA tree.
 *
 * @param[in] sequence - Sequence to add.
 * @param[in] accept_value - Output value on completed match.
 * @param[in] custom - Acceptor value is a configurable sequence.
 */
static void
dfa_insert(const char *sequence, dfa_acceptor accept_value, bool custom) {
	dfa_node *current = &root;

	char edge;
	while ((edge = *sequence++) != '\0') {
		for (uint32_t i = 0; i < current->children_count; i++) {
			dfa_node *child = current->value.children + i;
			if (child->edge == edge) {
				current = child;
				goto insert_next;
			}
		}

		current->value.children
			= realloc(current->value.children,
					  (current->children_count + 1) * sizeof(dfa_node));
		dfa_node *child = current->value.children + current->children_count;
		current->children_count++;

		child->edge = edge;
		child->custom = custom;
		child->value.children = NULL;
		child->children_count = 0;
		current = child;

		// For breaking out of inner for loop
insert_next:
		continue;
	}

	current->value.accept = accept_value;
}

// @endcond
