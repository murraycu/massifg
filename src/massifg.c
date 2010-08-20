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


/* Initialize application data structure */
MassifgApplication *
massifg_application_new(int *argc_ptr, char ***argv_ptr) {
	MassifgApplication *app = g_new(MassifgApplication, 1);

	app->argc_ptr = argc_ptr;
	app->argv_ptr = argv_ptr;

	app->output_data = NULL;
	app->graph = NULL;
	app->filename = NULL;
	app->gtk_builder = NULL;

	return app;
}

/* Free a MassifgApplication */
void
massifg_application_free(MassifgApplication *app) {
	if (app->output_data != NULL) {
		massifg_output_data_free(app->output_data);
	}
	g_object_unref(G_OBJECT(app->gtk_builder));

	/* FIXME: free graph, and possibly filename */
}


/* Set the currently open file */
void
massifg_application_set_file(MassifgApplication *app, gchar *filename) {
	/* TODO: when parsing fails, keep the previous graph */
	GError *error = NULL;
	app->filename = filename;

	/* Parse the file */
	if (app->output_data != NULL) {
		massifg_output_data_free(app->output_data);
	}
	app->output_data = massifg_parse_file(app->filename, &error);


	if (app->output_data == NULL && app->filename != NULL) {
		/* FIXME: this should not be tied directly to the gtk ui */
		massifg_gtkui_errormsg(app, "Unable to parse file %s: %s",
				app->filename, error->message); /* Parsing failed */
		g_error_free(error);
		return;
	}

	if (app->output_data != NULL) {
		massifg_graph_update(app->graph, app->output_data);
	}

}

int
main (int argc, char **argv) {
	/* Setup */
	MassifgApplication *app = massifg_application_new(&argc, &argv);
	massifg_utils_configure_debug_output();
	massifg_graph_init();

	/* Create the UI */
	if (massifg_gtkui_init(app) != 0) {
		g_critical("Failed to create GTK+ UI");
		return 1;
	}

	if (argc == 2) { 
		massifg_application_set_file(app, argv[1]);
	}

	/* Present the UI and hand over control to the gtk mainloop */
	massifg_gtkui_start(app);

	/* Cleanup */
	massifg_application_free(app);
	return 0;
}
