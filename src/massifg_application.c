/*
 *  MassifG - massifg_application.c
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

#include "massifg_application.h"
#include "massifg_parser.h"
#include "massifg_graph.h"
#include "massifg_utils.h"
#include "massifg_gtkui.h"


G_DEFINE_TYPE (MassifgApplication, massifg_application, G_TYPE_OBJECT);

/* Unref object once and only once, to avoid trying to unref an invalid object */
void
gobject_safe_unref(GObject *object) {
	if (object) {
		g_object_unref(object);
		object = NULL;
	}
}

/* Get a new MassifgApplication instance */
MassifgApplication *
massifg_application_new(int *argc_ptr, char ***argv_ptr) {
	g_type_init();
	MassifgApplication *app = g_object_new(MASSIFG_TYPE_APPLICATION, NULL);

	app->argc_ptr = argc_ptr;
	app->argv_ptr = argv_ptr;
	return app;
}

/* Initialize application data structure */
static void
massifg_application_init(MassifgApplication *self) {
	self->argc_ptr = NULL;
	self->argv_ptr = NULL;

	self->output_data = NULL;
	self->graph = NULL;
	self->filename = NULL;
	self->gtk_builder = NULL;
}


/* Free simple types */
static void
massifg_application_finalize(GObject *gobject) {
	MassifgApplication *app = MASSIFG_APPLICATION(gobject);
	g_free(app->filename);
}

/* Free all references to objects */
static void
massifg_application_dispose(GObject *gobject) {
	MassifgApplication *app = MASSIFG_APPLICATION(gobject);

	if (app->output_data) {
		massifg_output_data_free(app->output_data);
	}
	massifg_graph_free(app->graph);

	gobject_safe_unref(G_OBJECT(app->gtk_builder));
}

static void
massifg_application_class_init(MassifgApplicationClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = massifg_application_dispose;
  gobject_class->finalize = massifg_application_finalize;

}

/* Free a MassifgApplication */
void
massifg_application_free(MassifgApplication *app) {
	g_object_unref(G_OBJECT(app));
}


/* Set the currently open file 
 * This will copy the filename, so it is safe to use with temporary strings */
void
massifg_application_set_file(MassifgApplication *app, gchar *filename) {
	GError *error = NULL;
	MassifgOutputData *new_data = NULL;
	gchar *filename_copy = g_strdup(filename);

	/* Try to parse the file */
	new_data = massifg_parse_file(filename_copy, &error);

	if (new_data) {
		/* Parsing succeeded */
		massifg_graph_set_data(app->graph, new_data);
		g_free(app->filename);
		app->filename = filename_copy;
	}
	else {
		/* Parsing failed */
		/* FIXME: this should not be tied directly to the gtk ui */
		massifg_gtkui_errormsg(app, "Unable to parse file %s: %s",
				filename_copy, error->message);
		g_error_free(error);
		return;
	}

}

/* Separate from main for testing purposes */
int
massifg_application_run(MassifgApplication *app) {
	/* Setup */
	massifg_utils_configure_debug_output();
	massifg_graph_init();

	/* Create the UI */
	if (massifg_gtkui_init(app) != 0) {
		g_critical("Failed to create GTK+ UI");
		return 1;
	}

	if (*app->argc_ptr == 2) {
		massifg_application_set_file(app, (*app->argv_ptr)[1]);
	}

	/* Present the UI and hand over control to the gtk mainloop */
	massifg_gtkui_start(app);
	return 0;
}

