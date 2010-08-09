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

#include <stdlib.h> /* for atoi() / strtol() */

#include <glib.h>

#include "massifg_parser.h"
#include "massifg_utils.h"

/* Private datastructures */
#define MASSIFG_PARSE_ERROR g_quark_from_string("MASSIFG_PARSE_ERROR")
#define MASSIFG_PARSE_ERROR_NOSNAPSHOTS 1

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

	STATE_SNAPSHOT_HEAP_TREE_LEAF
} MassifgParserState;

/* Data strucure for the parser, which is passed around to
 * the central functions that implement the parser */
struct _MassifgParser {
	MassifgParserState current_state;
	MassifgSnapshot *current_snapshot;
	MassifgOutputData *output_data;
};
typedef struct _MassifgParser MassifgParser;


/* Private functions */

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

/* Parse a header element, setting element to the value parsed
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

/* Parse a snapshot element, setting element to the value parsed
 * If line does not start with prefix, this function does nothing */
static void
massifg_parse_snapshot_element(MassifgParser *parser, const gchar *line,
				const gchar *prefix, glong *element,
				MassifgParserState next_state) {
	gchar **kv_tokens;
	if (g_str_has_prefix(line, prefix)) {
		kv_tokens = massifg_tokenify_line(line, "=");
		*element = strtol(kv_tokens[1], NULL, 10);
		g_strfreev(kv_tokens);
		parser->current_state = next_state;
	}

}

/* Parse a single line, based on the current state of the parser
 * NOTE: function assumes that the line does not contain any trailing newline character */
static void 
massifg_parse_line(MassifgParser *parser, gchar *line) {
	gchar **kv_tokens;

	g_debug("Parsing line: \"%s\". Parser state: %d", 
		line, parser->current_state);

	switch (parser->current_state) {

	/* Header entries
	 * Format: "key: value", where value is a string */
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

	/* Snapshot identifier
	 * #-----------
	 * snapshot=N
	 * #-----------
	 */
	case STATE_SNAPSHOT: 
		if (g_str_has_prefix(line, "snapshot=")) {
			parser->current_snapshot = (MassifgSnapshot*) g_malloc(sizeof(MassifgSnapshot));

			parser->current_snapshot->heap_tree_desc = g_string_new("");
			/* Initialize */
			parser->current_snapshot->snapshot_no = -1;
			parser->current_snapshot->time = -2;
			parser->current_snapshot->mem_heap_B = -3;
			parser->current_snapshot->mem_heap_extra_B = -4;
			parser->current_snapshot->mem_stacks_B = -5;

			/* Actually parse and set correct snapshot number */
			kv_tokens = massifg_tokenify_line(line, "=");
			parser->current_snapshot->snapshot_no = atoi(kv_tokens[1]);
			g_strfreev(kv_tokens);

			/* Add to output data structure */
			parser->output_data->snapshots = 
				g_list_append(parser->output_data->snapshots, parser->current_snapshot);

			parser->current_state = STATE_SNAPSHOT_TIME;

		}
		break;

	/* Snapshot entries
	 * Format: "key=value", where value is, most often, a number */
	case STATE_SNAPSHOT_TIME:
		massifg_parse_snapshot_element(parser, line, "time=",
				&parser->current_snapshot->time, STATE_SNAPSHOT_MEM_HEAP);

		/* Check if this snapshots values for time is larger than the ones before it 
		 * NOTE: this should pretty much always be true */
		if (parser->current_snapshot->time > parser->output_data->max_time) {
			parser->output_data->max_time = parser->current_snapshot->time;
		}
		break;
	case STATE_SNAPSHOT_MEM_HEAP:
		massifg_parse_snapshot_element(parser, line, "mem_heap_B=",
				&parser->current_snapshot->mem_heap_B, STATE_SNAPSHOT_MEM_HEAP_EXTRA);
		break;
	case STATE_SNAPSHOT_MEM_HEAP_EXTRA:
		massifg_parse_snapshot_element(parser, line, "mem_heap_extra_B=",
				&parser->current_snapshot->mem_heap_extra_B, STATE_SNAPSHOT_MEM_STACKS);
		break;
	case STATE_SNAPSHOT_MEM_STACKS:
		massifg_parse_snapshot_element(parser, line, "mem_stacks_B=",
				&parser->current_snapshot->mem_stacks_B, STATE_SNAPSHOT_HEAP_TREE);

		/* Check if this snapshots values for memory allocation is larger than the ones before it */
		gint total_mem_allocation = parser->current_snapshot->mem_heap_B + 
				parser->current_snapshot->mem_heap_extra_B +
				parser->current_snapshot->mem_stacks_B;
		if (total_mem_allocation > parser->output_data->max_mem_allocation) {
			parser->output_data->max_mem_allocation = total_mem_allocation;
		}
		break;


	case STATE_SNAPSHOT_HEAP_TREE:
		g_message("STATE_SNAPSHOT_HEAP_TREE");

		if (g_str_has_prefix(line, "heap_tree=")) {
			kv_tokens = massifg_tokenify_line(line, "=");
			g_string_printf(parser->current_snapshot->heap_tree_desc, "%s", kv_tokens[1]);
			g_strfreev(kv_tokens);

			if (g_strcmp0(parser->current_snapshot->heap_tree_desc->str, "empty"))
				parser->current_state = STATE_SNAPSHOT;
			else if (g_strcmp0(parser->current_snapshot->heap_tree_desc->str, "detailed"))
				parser->current_state = STATE_SNAPSHOT_HEAP_TREE_LEAF;
		}
		break;

	/* Snapshot heap tree entries */
	case STATE_SNAPSHOT_HEAP_TREE_LEAF:
		/* TODO: implement */
		parser->current_state = STATE_SNAPSHOT;
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

/* Free a MassifgOutputData structure, for instance as returned by
 * massifg_output_data_new() */
void massifg_output_data_free(MassifgOutputData *data) {

	g_list_foreach(data->snapshots, massifg_utils_free_foreach, NULL);
	g_list_free(data->snapshots);

	g_string_free(data->time_unit, TRUE);
	g_string_free(data->cmd, TRUE);
	g_string_free(data->desc, TRUE);

	g_free(data);
}


/* Parse the data in the given GIOChannel, returning a pointer to
 * the MassifgOutputData structure that represents this data
 * Returns NULL on failure
 * Use massifg_output_data_free() to free the return value */
MassifgOutputData
*massifg_parse_iochannel(GIOChannel *io_channel, GError **error) {
	MassifgParser *parser = NULL;
	MassifgOutputData *output_data = NULL;

	GString *line_string = NULL;
	GIOStatus io_status = G_IO_STATUS_NORMAL;

	/* Initialize */
	output_data = massifg_output_data_new();

	MassifgParser parser_onstack;
	parser = &parser_onstack;
	parser->current_state = STATE_DESC;
	parser->output_data = output_data;

	line_string = g_string_new("initial string");

	/* Parse file */
	while (io_status == G_IO_STATUS_NORMAL) {
		io_status = g_io_channel_read_line_string(io_channel, line_string, NULL, error);
		line_string->str = g_strchomp(line_string->str); /* Remove newline */
		massifg_parse_line(parser, line_string->str);
	}
	if (io_status == G_IO_STATUS_ERROR) {
		output_data = NULL;
	}
	if (g_list_length(output_data->snapshots) < 1 ) {
		output_data = NULL;
		g_set_error_literal(error, MASSIFG_PARSE_ERROR, MASSIFG_PARSE_ERROR_NOSNAPSHOTS, "Could not parse any snapshots");
	}

	g_string_free(line_string, TRUE);

	return output_data;

}

/* Utility function for parsing a file, see massifg_parse_iochannel() for details
 * Returns NULL on failure */
MassifgOutputData
*massifg_parse_file(gchar *filename, GError **error) {
	MassifgOutputData *output_data = NULL;
	GIOChannel *io_channel = NULL;

	g_debug("Parsing file: %s", filename);

	io_channel = g_io_channel_new_file(filename, "r", error);
	if (io_channel == NULL) {
		return NULL;
	}

	output_data = massifg_parse_iochannel(io_channel, error);
	g_io_channel_unref(io_channel);
	return output_data;
}


