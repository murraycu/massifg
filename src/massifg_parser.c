/*
 *  MassifG - massifg_parser.c
 *
 *  Copyright (C) 2010 Openismus GmbH
 *
 *  Author: Jon Nordby <jonn@openismus.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:massifg_parser
 * @short_description: Functions for parsing massif output files.
 * @title: MassifG Parser
 * @stability: Unstable
 *
 * These functions builds up a datastructure that represents
 * the output from massif.
 */

#include <stdlib.h> /* for atoi() / strtol() */
#include <string.h>

#include <glib.h>

#include "massifg_parser.h"
#include "massifg_parser_private.h"
#include "massifg_utils.h"

/* Private datastructures */
#define MASSIFG_PARSE_ERROR g_quark_from_string("MASSIFG_PARSE_ERROR")
static const gint MASSIFG_PARSE_ERROR_NOSNAPSHOTS = 1;

/* Enum for the different possible states the parser state-machine can be in */
typedef enum {
	STATE_DESC,
	STATE_CMD,
	STATE_TIME_UNIT,

	STATE_SNAPSHOT,
	STATE_SNAPSHOT_TIME,
	STATE_SNAPSHOT_MEM_HEAP,
	STATE_SNAPSHOT_MEM_HEAP_EXTRA,
	STATE_SNAPSHOT_MEM_STACKS,
	STATE_SNAPSHOT_HEAP_TREE,

	STATE_SNAPSHOT_HEAP_TREE_NODE
} MassifgParserState;

/* Data strucure for the parser, which is passed around to
 * the central functions that implement the parser */
struct _MassifgParser {
	MassifgParserState current_state;
	MassifgSnapshot *current_snapshot;
	GNode *ht_current_parent;
	gint current_line_number;
	MassifgOutputData *output_data;
};
typedef struct _MassifgParser MassifgParser;

/* Private functions */

MassifgParser *
massifg_parser_new(void) {
	MassifgParser *parser = g_new(MassifgParser, 1);

	parser->current_state = STATE_DESC;
	parser->current_line_number = 0;
	parser->current_snapshot = NULL;
	parser->output_data = NULL;
	parser->ht_current_parent = NULL;

	return parser;
}

void
massifg_parser_free(MassifgParser *parser) {
	/* Currently all pointers point to data which is owned by someone else
	 * so we only free the MassifgParser structure */
	g_free(parser);

}

/* Turn the line into tokens, splitting on delim
 * Caller is responsible for freeing return value with g_strfreev () */
static gchar **
massifg_tokenify_line(const gchar *line, const gchar *delim) {
	gchar **tokens;

	tokens = g_strsplit(line, delim, 2);
	g_debug("Tokenified entry: key=\"%s\", value=\"%s\"",
		tokens[0], tokens[1]);

	return tokens;
}

/* Parse a header element, 
 * Format: "key: value", where value is a string
 * Sets element to the value parsed
 * If line does not start with prefix, this function does nothing */
static void
massifg_parse_header_element(MassifgParser *parser, const gchar *line,
				const gchar *prefix, GString *element,
				MassifgParserState next_state) {
	gchar **kv_tokens;
	if (g_str_has_prefix(line, prefix)) {
		kv_tokens = massifg_tokenify_line(line, ": ");
		g_string_printf(element, "%s", kv_tokens[1]);
		g_strfreev(kv_tokens);
		parser->current_state = next_state;
	}

}

/* Parse a snapshot element.
 * Format: "key=value", where value is a number
 * Sets element to the value parsed
 * If line does not start with prefix, this function does nothing */
static void
massifg_parse_snapshot_element(MassifgParser *parser, const gchar *line,
				const gchar *prefix, gint64 *element,
				MassifgParserState next_state) {
	gchar **kv_tokens;
	if (g_str_has_prefix(line, prefix)) {
		kv_tokens = massifg_tokenify_line(line, "=");
		*element = (gint64)strtoll(kv_tokens[1], NULL, 10);
		g_strfreev(kv_tokens);
		parser->current_state = next_state;
	}

}


MassifgSnapshot *
massifg_snapshot_new() {
	MassifgSnapshot *snapshot = g_new(MassifgSnapshot, 1);

	snapshot->heap_tree_desc = g_string_new("");
	snapshot->heap_tree = NULL;

	/* Initialize */
	snapshot->snapshot_no = -1;
	snapshot->time = -2;
	snapshot->mem_heap_B = -3;
	snapshot->mem_heap_extra_B = -4;
	snapshot->mem_stacks_B = -5;
	return snapshot;
}

/* Parse snapshot identifier, and initialize the snapshot datastructure. Format:
 * #-----------
 * snapshot=N
 * #-----------
 */
void
massifg_parse_snapshot(MassifgParser *parser, const gchar *line) {
	gchar **kv_tokens;
	if (g_str_has_prefix(line, "snapshot=")) {
		parser->current_snapshot = massifg_snapshot_new();

		/* Actually parse and set correct snapshot number */
		kv_tokens = massifg_tokenify_line(line, "=");
		parser->current_snapshot->snapshot_no = atoi(kv_tokens[1]);
		g_strfreev(kv_tokens);

		/* Add to output data structure */
		parser->output_data->snapshots =
			g_list_append(parser->output_data->snapshots, parser->current_snapshot);

		parser->current_state = STATE_SNAPSHOT_TIME;

	}
}

/* Parses the snapshot element "time", and maintains a maximum value */
void
massifg_parse_snapshot_time(MassifgParser *parser, const gchar *line) {
	massifg_parse_snapshot_element(parser, line, "time=",
			&parser->current_snapshot->time, STATE_SNAPSHOT_MEM_HEAP);

	/* Check if this snapshots values for time is larger than the ones before it 
	 * NOTE: this should pretty much always be true */
	if (parser->current_snapshot->time > parser->output_data->max_time) {
		parser->output_data->max_time = parser->current_snapshot->time;
	}
}

/* Parses the snapshot element "mem_stacks_B", and maintains a maximum value of the sum of memory */
void
massifg_parse_snapshot_mem_stacks(MassifgParser *parser, const gchar *line) {
	gint64 total_mem_allocation = 0;

	massifg_parse_snapshot_element(parser, line, "mem_stacks_B=",
			&parser->current_snapshot->mem_stacks_B, STATE_SNAPSHOT_HEAP_TREE);

	/* Check if this snapshots values for memory allocation is larger than the ones before it */
	total_mem_allocation = parser->current_snapshot->mem_heap_B + 
			parser->current_snapshot->mem_heap_extra_B +
			parser->current_snapshot->mem_stacks_B;
	if (total_mem_allocation > parser->output_data->max_mem_allocation) {
		parser->output_data->max_mem_allocation = total_mem_allocation;
	}
}

/* Parse heap tree identifier 
 * Format: "heap_tree=value", where value can be "details", "empty" or "peak"*/
void
massifg_parse_heap_tree_desc(MassifgParser *parser, const gchar *line) {
	gchar **kv_tokens;
	if (g_str_has_prefix(line, "heap_tree=")) {
		kv_tokens = massifg_tokenify_line(line, "=");
		g_string_printf(parser->current_snapshot->heap_tree_desc, "%s", kv_tokens[1]);
		g_strfreev(kv_tokens);

		if (g_strcmp0(parser->current_snapshot->heap_tree_desc->str, "empty") == 0)
			parser->current_state = STATE_SNAPSHOT;
		else if (g_strcmp0(parser->current_snapshot->heap_tree_desc->str, "detailed") == 0)
			parser->current_state = STATE_SNAPSHOT_HEAP_TREE_NODE;
	}
}

gint
massifg_str_count_leading_spaces(const gchar *str) {
	guint i = 0;
	gint num_spaces = 0;

	for (i=0; i<strlen(str); i++) {
		if (str[i] != ' ')
			break;
		num_spaces++;
	}
	return num_spaces;
}

/* Set simple node attributes.
 * Simple attributes are those that can be found just by parsing the line,
 * without considering context */
void
massifg_heap_tree_node_init_simple_attributes(MassifgHeapTreeNode *node, const gchar *line) {
	gchar *tmp_str = NULL;
	gchar **tokens = NULL;

	/* Copy and strip whitespace */
	tmp_str = g_strdup(line);
	g_strstrip(tmp_str);

	tokens = g_strsplit(tmp_str, " ", 3);
	g_free(tmp_str);

	/* Copy, dropping the leading n and the trailing : character */
	tmp_str = massifg_str_copy_region(tokens[0], 1, strlen(tokens[0])-1);

	node->num_children = (int)strtol(tmp_str, NULL, 10);
	g_free(tmp_str);

	node->total_mem_B = strtol(tokens[1], NULL, 10);
	g_string_printf(node->label, "%s", tokens[2]);

	node->parsing_depth = massifg_str_count_leading_spaces(line);
	node->parsing_remaining_children = node->num_children;

	g_strfreev(tokens);
}

/* Return a pointer to a newly allocated and initialized MassifgHeapTreeNode */
MassifgHeapTreeNode *
massifg_heap_tree_node_new(const gchar *line) {
	MassifgHeapTreeNode *node = g_new(MassifgHeapTreeNode, 1);

	node->label = g_string_new("");
	massifg_heap_tree_node_init_simple_attributes(node, line);

	return node;
}

/* Free a MassifgHeapTreeNode */
void
massifg_heap_tree_node_free(MassifgHeapTreeNode *node) {
	g_string_free(node->label, TRUE);
	g_free(node);
}


/* Find the parent the next node should go to after the end of a subtree has been reached
 * Returns NULL if no suitable parent can be found */
GNode *
massifg_heap_tree_get_next_parent(GNode *current_parent) {
	GNode *next_parent = current_parent->parent;

	while ( next_parent &&
	((MassifgHeapTreeNode *)next_parent->data)->parsing_remaining_children == 0) {
/*		g_message("%d, %s", ((MassifgHeapTreeNode *)next_parent->data)->parsing_remaining_children,*/
/*			((MassifgHeapTreeNode *)next_parent->data)->label->str);*/
		next_parent = next_parent->parent;
	}
	if (next_parent)
		g_debug("Found next parent, label: \"%s\"",
			((MassifgHeapTreeNode *)next_parent->data)->label->str);
	else
		g_debug("Heap tree complete");
	return next_parent;
}

/*
 * n13: 1411172 (heap allocation functions) malloc/new/new[], --alloc-fns, etc.
 *    n4: 209472 0x5792ABD: dictresize (dictobject.c:517)
 *      n0: 576 in 1 place, below massif's threshold (01.00%)
 * n$num_children: $memory_cost $label
 * Number of leading spaces indicate the depth of the element in the tree
 */
void
massifg_parse_heap_tree_node(MassifgParser *parser, const gchar *line) {
	MassifgSnapshot *snapshot = parser->current_snapshot;
	GNode *next_parent = parser->ht_current_parent;
	MassifgHeapTreeNode *tmp_node = NULL;

	/* Create a new node */
	MassifgHeapTreeNode *new_node = massifg_heap_tree_node_new(line);

	/* Add the node to the tree */
	if (!snapshot->heap_tree) {
		/* This is the first node, create the root */
		g_debug("Creating heap tree root node");
		snapshot->heap_tree = g_node_new((gpointer)new_node);
		next_parent = snapshot->heap_tree;
	}

	else {
		/* This is an ordinary node, add it to its parent */
		g_node_append_data(parser->ht_current_parent, new_node);
		tmp_node = (MassifgHeapTreeNode *)parser->ht_current_parent->data;
		tmp_node->parsing_remaining_children--;
		g_assert(tmp_node->parsing_remaining_children >= 0);
		next_parent = g_node_last_child(parser->ht_current_parent);
	}

	/* Check if we are at the end of a tree */
	if (new_node->num_children == 0) {
		/* End of a subtree, the next node belongs to a parent further up
		 * This parent can be found by traversing back up the tree and locating the
		 * first node with non-zero parsing_expected_children */
		next_parent = massifg_heap_tree_get_next_parent(next_parent);
		if (!next_parent) {
			/* No node has missing children, so this was the
			 * last node in the heap tree,
			 * and we expect a new snapshot to come next */
			parser->current_state = STATE_SNAPSHOT;
		}
		else {
			tmp_node = (MassifgHeapTreeNode *)parser->ht_current_parent->data;
			g_assert(new_node->parsing_depth ==  tmp_node->parsing_depth+1);
		}
	}

	/* Set the parent for the next node */
	parser->ht_current_parent = next_parent;
}

/* Parse a single line, based on the current state of the parser
 * NOTE: function assumes that the line does not contain any trailing newline character */
static void 
massifg_parse_line(MassifgParser *parser, gchar *line) {
	g_debug("Parsing line %d: \"%s\". Parser state: %d", parser->current_line_number,
		line, parser->current_state);

	switch (parser->current_state) {

	/* Header entries */
	case STATE_DESC:
		massifg_parse_header_element(parser, line, "desc: ",
			parser->output_data->desc, STATE_CMD);
		break;
	case STATE_CMD:
		massifg_parse_header_element(parser, line, "cmd: ",
			parser->output_data->cmd, STATE_TIME_UNIT);
		break;
	case STATE_TIME_UNIT:
		massifg_parse_header_element(parser, line, "time_unit: ",
				parser->output_data->time_unit, STATE_SNAPSHOT);
		break;

	/* Snapshot identifier */
	case STATE_SNAPSHOT: 
		massifg_parse_snapshot(parser, line);
		break;
	/* Snapshot entries */
	case STATE_SNAPSHOT_TIME:
		massifg_parse_snapshot_time(parser, line);
		break;
	case STATE_SNAPSHOT_MEM_HEAP:
		massifg_parse_snapshot_element(parser, line, "mem_heap_B=",
				&parser->current_snapshot->mem_heap_B,
				STATE_SNAPSHOT_MEM_HEAP_EXTRA);
		break;
	case STATE_SNAPSHOT_MEM_HEAP_EXTRA:
		massifg_parse_snapshot_element(parser, line, "mem_heap_extra_B=",
				&parser->current_snapshot->mem_heap_extra_B,
				STATE_SNAPSHOT_MEM_STACKS);
		break;
	case STATE_SNAPSHOT_MEM_STACKS:
		massifg_parse_snapshot_mem_stacks(parser, line);
		break;

	/* Snapshot heap tree identifier */
	case STATE_SNAPSHOT_HEAP_TREE:
		massifg_parse_heap_tree_desc(parser, line);
		break;
	/* Snapshot heap tree entries */
	case STATE_SNAPSHOT_HEAP_TREE_NODE:
		massifg_parse_heap_tree_node(parser, line);
		break;
	}

}

/* Allocate and initialize a MassifgOutputData structure, returning a pointer to it
 * Use massifg_output_data_free() to free */
MassifgOutputData *massifg_output_data_new() {
	MassifgOutputData *data;
	data = (MassifgOutputData*) g_malloc(sizeof(MassifgOutputData));

	data->snapshots = NULL;

	data->desc = g_string_new("");
	data->cmd = g_string_new("");
	data->time_unit = g_string_new("");

	data->max_time = 0;
	data->max_mem_allocation = 0;

	return data;
}

/* Public functions */

/**
 * massifg_output_data_free:
 * @data: the MassifgOutputData to free
 *
 * Free a #MassifgOutputData.
 */
void massifg_output_data_free(MassifgOutputData *data) {

	g_list_foreach(data->snapshots, massifg_utils_free_foreach, NULL);
	g_list_free(data->snapshots);

	g_string_free(data->time_unit, TRUE);
	g_string_free(data->cmd, TRUE);
	g_string_free(data->desc, TRUE);

	g_free(data);
}

/**
 * massifg_parse_iochannel:
 * @io_channel: #GIOChannel to parse the data from
 * @error: Location to store a #GError or %NULL
 * @Returns: a #MassifgOutputData that represents this data, or %NULL on failure.
 * Use massifg_output_data_free() to free.
 *
 * Parse massif output data from a #GIOChannel.
 */
MassifgOutputData
*massifg_parse_iochannel(GIOChannel *io_channel, GError **error) {
	MassifgOutputData *output_data = NULL;
	MassifgParser *parser = massifg_parser_new();

	GString *line_string = NULL;
	GIOStatus io_status = G_IO_STATUS_NORMAL;

	/* Initialize */
	output_data = massifg_output_data_new();
	parser->output_data = output_data;

	line_string = g_string_new("initial string");

	/* Parse file */
	while (io_status == G_IO_STATUS_NORMAL) {
		io_status = g_io_channel_read_line_string(io_channel, line_string, NULL, error);
		parser->current_line_number++;
		line_string->str = g_strchomp(line_string->str); /* Remove newline */
		massifg_parse_line(parser, line_string->str);
	}
	g_debug("Parsing DONE");

	if (io_status == G_IO_STATUS_ERROR) {
		output_data = NULL;
	}
	if (g_list_length(output_data->snapshots) < 1 ) {
		output_data = NULL;
		g_set_error_literal(error, MASSIFG_PARSE_ERROR, MASSIFG_PARSE_ERROR_NOSNAPSHOTS, "Could not parse any snapshots");
	}

	g_string_free(line_string, TRUE);
	massifg_parser_free(parser);

	return output_data;

}

/**
 * massifg_parse_file:
 * @filename: Path to file to parse. %NULL is invalid
 * @error: Location to store a #GError or %NULL
 * @Returns: a #MassifgOutputData that represents this data, or %NULL on failure.
 *
 * Parse massif output data from file. See also massifg_parse_iochannel().
 */
MassifgOutputData
*massifg_parse_file(gchar *filename, GError **error) {
	MassifgOutputData *output_data = NULL;
	GIOChannel *io_channel = NULL;

	g_return_val_if_fail(filename != NULL, NULL);

	g_debug("Parsing file: %s", filename);

	io_channel = g_io_channel_new_file(filename, "r", error);
	if (io_channel == NULL) {
		return NULL;
	}

	output_data = massifg_parse_iochannel(io_channel, error);
	g_io_channel_unref(io_channel);
	return output_data;
}
