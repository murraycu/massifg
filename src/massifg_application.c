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

/**
 * SECTION:massifg_application
 * @short_description: the main application object
 * @title: MassifG Application
 * @stability: Unstable
 *
 * Note: These functions are meant to be used internally in MassifG.
 */

#include <gtk/gtk.h>

#include "massifg_application.h"
#include "massifg_parser.h"
#include "massifg_graph.h"
#include "massifg_utils.h"
#include "massifg_gtkui.h"


G_DEFINE_TYPE (MassifgApplication, massifg_application, G_TYPE_OBJECT)

enum
{
  SIGNAL_FILE_CHANGED,
  N_SIGNALS
};

static guint massifg_application_signals[N_SIGNALS];


/* Unref object once and only once, to avoid trying to unref an invalid object */
void
gobject_safe_unref(GObject *object) {
	if (object) {
		g_object_unref(object);
		object = NULL;
	}
}

/* Initialize application data structure */
static void
massifg_application_init(MassifgApplication *self) {
	self->argc_ptr = NULL;
	self->argv_ptr = NULL;

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

	massifg_graph_free(app->graph);

	gobject_safe_unref(G_OBJECT(app->gtk_builder));
}

static void
massifg_application_class_init(MassifgApplicationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = massifg_application_dispose;
	gobject_class->finalize = massifg_application_finalize;

	massifg_application_signals[SIGNAL_FILE_CHANGED] =
	g_signal_newv ("file-changed",
		G_TYPE_FROM_CLASS (gobject_class),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		NULL /* closure */,
		NULL /* accumulator */,
		NULL /* accumulator data */,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE /* return_type */,
		0     /* n_params */,
		NULL  /* param_types */);

}

/**
 * massifg_application_new:
 * @argc_ptr: Pointer to argc, the number of elements in the argument array
 * @argv_ptr: Pointer to argv array
 * @Returns: A new #MassifgApplication instance
 *
 * Get a new #MassifgApplication instance.
 * Arguments are typically gotten from main(), but can for testing purposes
 * for instance be provided manually or by g_shell_parse_argv().
 */
MassifgApplication *
massifg_application_new(int *argc_ptr, char ***argv_ptr) {
	MassifgApplication *app;
	g_type_init();
	app = g_object_new(MASSIFG_TYPE_APPLICATION, NULL);

	app->argc_ptr = argc_ptr;
	app->argv_ptr = argv_ptr;
	return app;
}

/**
 * massifg_application_free:
 * @app: The #MassifgApplication to free
 *
 * Free a #MassifgApplication.
 */
void
massifg_application_free(MassifgApplication *app) {
	g_object_unref(G_OBJECT(app));
}


/** 
 * massifg_application_set_file:
 * @app: A #MassifgApplication
 * @filename: Path to filename to set as active. Will be copied internally. %NULL is invalid
 * @error: A place to return a #GError or %NULL
 * @Returns: %TRUE on success or %FALSE on failure
 *
 * Set the currently active file.
 */
gboolean
massifg_application_set_file(MassifgApplication *app, gchar *filename, GError **error) {
	MassifgOutputData *new_data = NULL;
	gchar *filename_copy = g_strdup(filename);

	g_return_val_if_fail(filename != NULL, FALSE);

	/* Try to parse the file */
	new_data = massifg_parse_file(filename_copy, error);

	if (new_data) {
		/* Parsing succeeded */
		massifg_graph_set_data(app->graph, new_data);
		g_free(app->filename);
		app->filename = filename_copy;
		g_signal_emit_by_name(app, "file-changed");
		return TRUE;
	}

	/* Parsing failed */
	return FALSE;


}

/**
 * massifg_application_run:
 * @app: The #MassifgApplication to run
 * @Returns: The applications exit status. Non-zero indicates failure
 *
 * This function will block until the application quits. It is separate from main() so that the application can be tested more easily.
 */
int
massifg_application_run(MassifgApplication *app) {
	GError *error = NULL;
	gchar *filename = NULL;

	/* Setup */
	massifg_utils_configure_debug_output();
	massifg_graph_init();

	/* Create the UI */
	if (massifg_gtkui_init(app) != 0) {
		g_critical("Failed to create GTK+ UI");
		return 1;
	}


	if (*app->argc_ptr == 2) {
		filename = (*app->argv_ptr)[1];

		if (!massifg_application_set_file(app, filename, &error)) {
			massifg_gtkui_errormsg(app, "Unable to parse file %s: %s",
					filename, error->message);
			g_error_free(error);
		}
	}

	/* Present the UI and hand over control to the gtk mainloop */
	massifg_gtkui_start(app);
	return 0;
}

