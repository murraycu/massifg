/*
 *  MassifG - massifg.c
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

#include <gtk/gtk.h>

#include "massifg.h"
#include "massifg_parser.h"
#include "massifg_graph.h"
#include "massifg_utils.h"
#include "massifg_gtkui.h"


int
main (int argc, char **argv) {
	/* Initialize application data structure */
	MassifgApplication app;
	app.output_data = NULL;
	app.filename = NULL;
	app.gtk_builder = NULL;
	app.argc_ptr = &argc;
	app.argv_ptr = &argv;

	massifg_utils_configure_debug_output();

	/* Create the UI */
	if (massifg_gtkui_init(&app) != 0) {
		g_message("Failed to create GTK+ UI");
		return 1;
	}

	if (argc == 2) { 
		app.filename = argv[1];
	}

	/* Present the UI and hand over control to the gtk mainloop */
	massifg_gtkui_start(&app);

	if (app.output_data != NULL) {
		massifg_output_data_free(app.output_data);
	}
	g_object_unref(G_OBJECT(app.gtk_builder));
	return 0;
}
