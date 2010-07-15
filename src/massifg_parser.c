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

#include <glib.h>

#include "massifg_parser.h"

/* Private datastructures */

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

	STATE_SNAPSHOT_HEAP_TREE_LEAF,
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

/* TODO: implement */
static void 
massifg_parse_heap_tree_leaf(MassifgParser *parser, gchar *line) {
	;
}


/* TODO: Make this function smaller, preferably by factoring code used
 * in each case out into one or more reusable functions */
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
		if (g_str_has_prefix(line, "desc: ")) {
			kv_tokens = massifg_tokenify_line(line, ": ");
			g_string_printf(parser->output_data->desc, "%s", kv_tokens[1]);
			g_strfreev(kv_tokens);
			parser->current_state = STATE_CMD;
		}
		break;
	case STATE_CMD: 
		if (g_str_has_prefix(line, "cmd: ")) {
			kv_tokens = massifg_tokenify_line(line, ": ");
			g_string_printf(parser->output_data->cmd, "%s", kv_tokens[1]);
			g_strfreev(kv_tokens);
			parser->current_state = STATE_TIME_UNIT;
		}
		break;
	case STATE_TIME_UNIT: 
		if (g_str_has_prefix(line, "time_unit: ")) {
			kv_tokens = massifg_tokenify_line(line, ": ");
			g_string_printf(parser->output_data->time_unit, "%s", kv_tokens[1]);
			g_strfreev(kv_tokens);
			parser->current_state = STATE_SNAPSHOT;
		}
		break;

	/* Snapshot identifier
	 * #-----------
	 * snapshot=N
	 * #-----------
	 */
	case STATE_SNAPSHOT: 
		if (g_str_has_prefix(line, "snapshot=")) {
			parser->current_snapshot = (MassifgSnapshot*) g_malloc(sizeof(MassifgSnapshot));

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
		if (g_str_has_prefix(line, "time=")) {
			kv_tokens = massifg_tokenify_line(line, "=");
			parser->current_snapshot->time = atoi(kv_tokens[1]);
			g_strfreev(kv_tokens);
			parser->current_state = STATE_SNAPSHOT_MEM_HEAP;
		}
		break;
	case STATE_SNAPSHOT_MEM_HEAP:
		if (g_str_has_prefix(line, "mem_heap_B=")) {
			kv_tokens = massifg_tokenify_line(line, "=");
			parser->current_snapshot->mem_heap_B = atoi(kv_tokens[1]);
			g_strfreev(kv_tokens);
			parser->current_state = STATE_SNAPSHOT_MEM_HEAP_EXTRA;
		}
		break;
	case STATE_SNAPSHOT_MEM_HEAP_EXTRA:
		if (g_str_has_prefix(line, "mem_heap_extra_B=")) {
			kv_tokens = massifg_tokenify_line(line, "=");
			parser->current_snapshot->mem_heap_extra_B = atoi(kv_tokens[1]);
			g_strfreev(kv_tokens);
			parser->current_state = STATE_SNAPSHOT_MEM_STACKS;
		}
		break;
	case STATE_SNAPSHOT_MEM_STACKS:
		if (g_str_has_prefix(line, "mem_stacks_B=")) {
			kv_tokens = massifg_tokenify_line(line, "=");
			parser->current_snapshot->mem_stacks_B = atoi(kv_tokens[1]);
			g_strfreev(kv_tokens);
			/* XXX: look for next snapshot, heap tree parsing not implemented yet */
			parser->current_state = STATE_SNAPSHOT;
		}
		break;
	/* TODO: implement */
	case STATE_SNAPSHOT_HEAP_TREE:

	/* TODO: implement */
	/* Snapshot heap tree entries */
	case STATE_SNAPSHOT_HEAP_TREE_LEAF:
		;
	}

}

/* Function for freeing each element in a GList.
 * Elements freed using this function should have been allocated using g_alloc and derivatives
 * Meant to be used as a parameter to a g_(s)list_foreach call */
static void massifg_free_foreach_func(gpointer data, gpointer user_data) {
	g_free(data);
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

	return data;
}

/* Public functions */

/* Free a MassifgOutputData structure, for instance as returned by
 * massifg_output_data_new() */
massifg_output_data_free(MassifgOutputData *data) {

	g_list_foreach(data->snapshots, massifg_free_foreach_func, NULL);
	g_list_free(data->snapshots);

	g_string_free(data->time_unit, TRUE);
	g_string_free(data->cmd, TRUE);
	g_string_free(data->desc, TRUE);

	g_free(data);
}


/* Parse the data in the given GIOChannel, returning a pointer to
 * the MassifgOutputData structure that represents this data
 * Use massifg_output_data_free() to free the return value */
MassifgOutputData
*massifg_parse_iochannel(GIOChannel *io_channel) {
	MassifgParser *parser = NULL;
	MassifgOutputData *output_data = NULL;

	GError *error = NULL;
	GString *line_string = NULL;
	GIOStatus io_status = G_IO_STATUS_NORMAL;

	/* Initialize */
	output_data = massifg_output_data_new();

	MassifgParser parser_onstack;
	parser = &parser_onstack;
	parser->current_state = STATE_DESC;
	parser->output_data = output_data;

	/* Parse file */
	line_string = g_string_new("initial string");

	while (io_status == G_IO_STATUS_NORMAL) {
		io_status = g_io_channel_read_line_string(io_channel, line_string, NULL, &error);
		line_string->str = g_strchomp(line_string->str); /* Remove newline */
		massifg_parse_line(parser, line_string->str);
	}

	g_string_free(line_string, TRUE);

	return output_data;

}

/* Utility function for parsing a file, see massifg_parse_iochannel() for details
 * Returns NULL on failure */
MassifgOutputData
*massifg_parse_file(gchar *filename) {
	MassifgOutputData *output_data = NULL;
	GIOChannel *io_channel = NULL;
	GError *error = NULL;

	g_debug("Parsing file: %s", filename);

	io_channel = g_io_channel_new_file(filename, "r", &error);
	if (io_channel == NULL) {
		g_message("Cannot open file %s: %s", filename, error->message);
		return NULL;
	}

	output_data = massifg_parse_iochannel(io_channel);
	g_io_channel_unref(io_channel);
	return output_data;
}


