/*
 *  MassifG - massifg_parser.h
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

#ifndef __MASSIFG_PARSER_H__
#define __MASSIFG_PARSER_H__

#include <glib.h>

/* Data structures */

/**
 * MassifgHeapTreeNode:
 * @num_children: The number of children this node has.
 * @total_mem_B: Memory usage under this node.
 * @label: String label identifying which function this is.
 * @parsing_remaining_children: Used internally by the parser. Should be 0 after correct parsing.
 * @parsing_depth: Used internally by the parser. Should be equal to the depth of the tree.
 *
 * Represents one node in the heap tree.
 */
typedef struct _MassifgHeapTreeNode {
	gint num_children;
	glong total_mem_B;
	GString *label;

	/* Only used while parsing */
	gint parsing_remaining_children;
	gint parsing_depth; 

} MassifgHeapTreeNode;

/**
 * MassifgSnapshot:
 * @snapshot_no: The number of this snapshot.
 * @time: Time this snapshot was taken. This can have different units, see #MassifgOutputData
 * @mem_heap_B: Heap memory usage in bytes.
 * @mem_heap_extra_B: Heap overhead in bytes.
 * @mem_stacks_B: Stack memory usage in bytes.
 * @heap_tree_desc: String describing what kind of heap tree we have.
 * @heap_tree: The heap tree as a tree of #MassifgHeapTreeNode objects.
 *
 *
 * Represents a single massif snapshot.
 */
struct _MassifgSnapshot {
	gint snapshot_no;

	gint64 time;
	gint64 mem_heap_B;
	gint64 mem_heap_extra_B;
	gint64 mem_stacks_B;

	GString *heap_tree_desc;
	GNode *heap_tree;
};
typedef struct _MassifgSnapshot MassifgSnapshot;

/**
 * MassifgOutputData:
 * @snapshots: List of #MassifgSnapshot objects representing the snapshots.
 * @desc: Description string. Can be set by the user when running massif.
 * @cmd: The command massif executed.
 * @time_unit: The time unit massif used. Possible values are "i"|"ms"|"b".
 * @max_time: The maximum value of the time.
 * @max_mem_allocation: The maximum value of total memory allocation.
 *
 *
 * Represents all the data massif outputs.
 *
 * Note: @max_time and @max_mem_allocation is not provided by the massif output
 * format directly but is provided by the parser. These attributes might go
 * away in a future version.
 */
struct _MassifgOutputData {
	GList *snapshots;

	GString *desc;
	GString *cmd;
	GString *time_unit;

	gint64 max_time;
	gint64 max_mem_allocation;
};
typedef struct _MassifgOutputData MassifgOutputData;

/* Public functions */
MassifgOutputData *massifg_parse_file(gchar *filename, GError **error);
MassifgOutputData *massifg_parse_iochannel(GIOChannel *io_channel, GError **error);
void massifg_output_data_free(MassifgOutputData *data);

#endif /* __MASSIFG_PARSER_H__ */

