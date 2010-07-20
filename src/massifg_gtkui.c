/*
 *  MassifG - massifg_gtkui.c
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

/* */

#include <gtk/gtk.h>
#include <stdarg.h>

#include "massifg.h"
#include "massifg_gtkui.h"
#include "massifg_utils.h"

#define MAIN_WINDOW "mainwindow"
#define MAIN_WINDOW_VBOX "mainvbox"
#define OPEN_DIALOG "openfiledialog"

#define MAIN_WINDOW_MENU "/MainMenu"


/* Private functions */
/* Present a dialog to the user with the message given by msg_format and the following arguments
 * These arguments follow the same conventions as printf() and friends.
 * Note that the message can be formatted with Pango markup language, if wanted */
void
massifg_gtkui_errormsg(MassifgApplication *app, const gchar *msg_format, ...) {
	GtkMessageDialog *error_dialog = NULL;
	GString *markup_string = NULL;
	GtkWindow *main_window = NULL;
	va_list argp;

	/* Initialize */
	main_window = GTK_WINDOW(gtk_builder_get_object(app->gtk_builder, MAIN_WINDOW));
	error_dialog = gtk_message_dialog_new(main_window,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "");
	markup_string = g_string_new("");

	/* Prepare markup string, */
	va_start(argp, msg_format);
	g_string_vprintf(markup_string, msg_format, argp);
	va_end(argp);

	/* Present the dialog to the user */
	gtk_message_dialog_set_markup(error_dialog, markup_string->str);
	gtk_dialog_run(GTK_DIALOG(error_dialog));

	gtk_widget_destroy(error_dialog);
}

void
massifg_gtkui_file_changed(MassifgApplication *app) {
	GtkImage *graph_image = NULL;

	graph_image = GTK_IMAGE(gtk_builder_get_object (app->gtk_builder, "image"));

	/* Parse the file */
	/* FIXME: currently does not return NULL on invalid input, but segfaults later */
	app->output_data = massifg_parse_file(app->filename);
	if (app->output_data == NULL) {
		massifg_gtkui_errormsg(app, "Unable to parse file %s", app->filename); /* Parsing failed */
		return;
	}

	/* Draw the graph */
	massifg_draw_graph(app->output_data);

	/* Update the UI */
	gtk_image_set_from_file(graph_image, "massifg-graph-test.png");

}


/* Signal handlers */
/* Destroy event handler for the main window, hooked up though glade/gtkbuilder */
void mainwindow_destroy(GtkObject *object, gpointer   user_data) {
	gtk_main_quit();
}


/* Actions */
void
quit_action(GtkAction *action, gpointer data) {
	mainwindow_destroy(NULL, NULL);
}

void
open_file_action(GtkAction *action, gpointer data) {
	static gboolean buttons_added = FALSE;
	GtkWidget *open_dialog = NULL;
	MassifgApplication *app = (MassifgApplication *)data;

	open_dialog = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, OPEN_DIALOG));

	/* Add the reponse buttons, but only once */
	/* NOTE: It does not seem that there is a way to add this using the Glade UI,
	 * and have gtkbuilder set it up, which would have been nicer */
	if (!buttons_added) {
		gtk_dialog_add_buttons(GTK_DIALOG(open_dialog),
				GTK_STOCK_OPEN, GTK_RESPONSE_OK,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
		buttons_added = TRUE;
	}

	/* Run the dialog, get the chosen filename */
	if (gtk_dialog_run(GTK_DIALOG(open_dialog)) == GTK_RESPONSE_OK) {
		app->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (open_dialog));
	}
	gtk_widget_hide(open_dialog);
	massifg_gtkui_file_changed(app);
}


/* Public functions */
gint
massifg_gtkui_init(MassifgApplication *app) {
	/* TODO: look up these at runtime, to support not running from the build dir */
	gchar *gladefile_path = "ui/massifg.glade";
	gchar *uifile_path = "ui/menu.ui";

	GtkUIManager *uimanager = NULL;
	GtkWidget *window = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *menubar = NULL;

	GError *error = NULL;
	GtkActionGroup *action_group = NULL;
	GtkActionEntry actions[] =
	{ /* action name, stock id, label, accelerator, tooltip, action (callback) */
	  { "FileMenuAction", NULL, "_File", NULL, NULL, NULL},
	  { "QuitAction", GTK_STOCK_QUIT, "_Quit", NULL, NULL, G_CALLBACK(quit_action)},
	  { "OpenFileAction", GTK_STOCK_OPEN, "_Open", NULL, NULL, G_CALLBACK(open_file_action)},
	};
	const int num_actions = G_N_ELEMENTS(actions);

	gtk_init (app->argc_ptr, app->argv_ptr);
	g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, massifg_utils_log_ignore, NULL);

	/* GTK builder */
	app->gtk_builder = gtk_builder_new();

	if (!gtk_builder_add_from_file (app->gtk_builder, gladefile_path, &error))
	{
		g_message ("%s", error->message);
		g_error_free (error);
		return 1;
	}

	window = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MAIN_WINDOW));
	vbox = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MAIN_WINDOW_VBOX));

	gtk_builder_connect_signals (app->gtk_builder,NULL);

	/* UI manager */
	action_group = gtk_action_group_new ("action group");
	gtk_action_group_add_actions (action_group, actions, num_actions, app);

	uimanager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (uimanager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_file (uimanager, uifile_path, &error))
	{
		g_message ("Building menus failed: %s", error->message);
		g_error_free (error);
		return 1;
	}

	/* Menubar */
	menubar = gtk_ui_manager_get_widget (uimanager, MAIN_WINDOW_MENU);
	gtk_window_add_accel_group (GTK_WINDOW (window),
		               gtk_ui_manager_get_accel_group (uimanager));

	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 1);
	gtk_box_reorder_child (GTK_BOX (vbox), menubar, 0);

	return 0;
}

/* Present window, and start the gtk mainloop */
void
massifg_gtkui_start(MassifgApplication *app) {
	GtkWidget *main_window = NULL;
	main_window = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MAIN_WINDOW));

	/* There might already be a file chosen */
	/* TODO: Preferably we should to this in an idle callback of the gtk mainloop */
	if (app->filename != NULL) {
		massifg_gtkui_file_changed(app);
	}

	gtk_widget_show_all(main_window);
	gtk_main();
}
