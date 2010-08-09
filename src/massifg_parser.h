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
typedef struct _MassifgSnapshotLeaf {
	glong total_mem_B;
	GString *label;
	GList *children;

} MassifgSnapshotLeaf;

struct _MassifgSnapshot {
	gint snapshot_no;

	glong time;
	glong mem_heap_B;
	glong mem_heap_extra_B;
	glong mem_stacks_B;

	GString *heap_tree_desc;
};
typedef struct _MassifgSnapshot MassifgSnapshot;

struct _MassifgOutputData {
	GList *snapshots;

	GString *desc;
	GString *cmd;
	GString *time_unit;

	/* Not provided by the output format directly, but set by the parser 
	 * as it builds up the data structure. */
	glong max_time;
	glong max_mem_allocation;
};
typedef struct _MassifgOutputData MassifgOutputData;

/* Public functions */
MassifgOutputData *massifg_parse_file(gchar *filename, GError **error);
MassifgOutputData *massifg_parse_iochannel(GIOChannel *io_channel, GError **error);
void massifg_output_data_free(MassifgOutputData *data);

#endif /* __MASSIFG_PARSER_H__ */

