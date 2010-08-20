/*
 *  MassifG - massifg.h
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

#ifndef __MASSIFG_H__
#define __MASSIFG_H__

#include <gtk/gtk.h>
#include "massifg_parser.h"
#include "massifg_graph.h"

/* Datastructure containing shared application state */
typedef struct {
	int *argc_ptr;
	char ***argv_ptr;
	gchar *filename;
	MassifgOutputData *output_data;
	MassifgGraph *graph;
	GtkBuilder *gtk_builder;
} MassifgApplication;

MassifgApplication *massifg_application_new(int *argc_ptr, char ***argv_ptr);
void massifg_application_free(MassifgApplication *app);

#endif /* __MASSIFG_H__ */
