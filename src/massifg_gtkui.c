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

/**
 * SECTION:massifg_gtkui
 * @short_description: Functions for creating and starting the GTK UI
 * @title: MassifG GTK UI
 *
 * Note: These functions are meant to be used internally in MassifG
 */

#include <gtk/gtk.h>
#include <stdarg.h>

#include "massifg_application.h"
#include "massifg_gtkui.h"
#include "massifg_graph.h"
#include "massifg_utils.h"

static const gchar MAIN_WINDOW_VBOX[] = "mainvbox";
static const gchar OPEN_DIALOG[] = "openfiledialog";
static const gchar SAVE_DIALOG[] = "savefiledialog";
static const gchar MAIN_WINDOW_MENU[] = "/MainMenu";

/* Private functions */
static void
print_op_begin_print(GtkPrintOperation *operation,
		GtkPrintContext *context, gpointer user_data) {

	gtk_print_operation_set_n_pages(operation, 1);
}

static void
print_op_draw_page(GtkPrintOperation *operation,
		GtkPrintContext *context, gint page_nr, gpointer data) {

	gboolean render_success = FALSE;
	MassifgApplication *app = (MassifgApplication *)data;
	cairo_t *cr;
	gdouble width, height;

	cr = gtk_print_context_get_cairo_context (context);
	width = gtk_print_context_get_width (context);
	height = gtk_print_context_get_width (context);

	render_success = massifg_graph_render_to_cairo(app->graph, cr, width, height);
	if (!render_success) {
		g_critical("Rendering graph failed");
	}

}

/* Signal handlers */
/* Destroy event handler for the main window, hooked up though glade/gtkbuilder */
void mainwindow_destroy(GtkObject *object, gpointer   user_data) {
	gtk_main_quit();
}

void mainwindow_update_title(MassifgApplication *app, gpointer user_data) {
	GtkWindow *main_window = NULL;
	main_window = GTK_WINDOW(gtk_builder_get_object (app->gtk_builder, MASSIFG_GTKUI_MAIN_WINDOW));

	/* TODO: include application name */
	if (app->filename) {
		gtk_window_set_title(main_window, app->filename);
	}
}

/* Actions */
void
quit_action(GtkAction *action, gpointer data) {
	mainwindow_destroy(NULL, NULL);
}

void
open_file_action(GtkAction *action, gpointer data) {
	static gboolean buttons_added = FALSE;
	GError *error = NULL;
	GtkWidget *open_dialog = NULL;
	MassifgApplication *app = (MassifgApplication *)data;
	gchar *filename = NULL;

	open_dialog = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, OPEN_DIALOG));

	/* Add the reponse buttons, but only once */
	/* NOTE: It does not seem that there is a way to add this using the Glade UI,
	 * and have gtkbuilder set it up, which would have been nicer */
	if (!buttons_added) {
		gtk_dialog_add_buttons(GTK_DIALOG(open_dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_OK,
				NULL);
		buttons_added = TRUE;
	}

	/* Run the dialog, get the chosen filename */
	if (gtk_dialog_run(GTK_DIALOG(open_dialog)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (open_dialog));
	}
	gtk_widget_hide(open_dialog);

	if (!filename) {
		/* User did not select a file */
		return;
	}

	if (!massifg_application_set_file(app, filename, &error)) {
		massifg_gtkui_errormsg(app, "Unable to parse file %s: %s",
				filename, error->message);
		g_error_free(error);
	}
}

void
save_file_action(GtkAction *action, gpointer data) {
	/* TODO: support a way to set the size */
	int width = 2000;
	int height = 1000;
	static gboolean buttons_added = FALSE;
	GtkWidget *save_dialog = NULL;
	MassifgApplication *app = (MassifgApplication *)data;
	gchar *filename = NULL;

	save_dialog = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, SAVE_DIALOG));

	if (!buttons_added) {
		gtk_dialog_add_buttons(GTK_DIALOG(save_dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_OK,
				NULL);
		buttons_added = TRUE;
	}


	/* Run the dialog, get the chosen filename */
	if (gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (save_dialog));
	}
	gtk_widget_hide(save_dialog);

	/* TODO: add .png to filename if not existing */
	if (filename) {
		if (!massifg_graph_render_to_png(app->graph, filename, width, height)) {
			massifg_gtkui_errormsg(app, "%s", "Error: Could not save PNG file");
		}
	}
}

void
print_action(GtkAction *action, gpointer data) {
	GtkWidget *main_window = NULL;
	MassifgApplication *app = (MassifgApplication *)data;
	GError *error = NULL;
	GtkPrintOperationResult result;
	GtkPrintOperation* print_op = NULL;

	main_window = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MASSIFG_GTKUI_MAIN_WINDOW));

	print_op = gtk_print_operation_new();
	g_signal_connect(print_op, "begin-print",
	G_CALLBACK (print_op_begin_print), app);
	g_signal_connect(print_op, "draw-page",
	G_CALLBACK (print_op_draw_page), app);

	result = gtk_print_operation_run(print_op,
		GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
		GTK_WINDOW(main_window), &error);
	if(result == GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		if(error)
		g_warning("Error while printing: %s", error->message);

		g_clear_error(&error);
	}
	/* TODO: Handle saving of page setup. */

	g_object_unref (print_op);
}

void
toggle_details_action(GtkToggleAction *action, gpointer data) {
	MassifgApplication *app = (MassifgApplication *)data;

	massifg_graph_set_show_details(app->graph, gtk_toggle_action_get_active(action));
}

void
toggle_legend_action(GtkToggleAction *action, gpointer data) {
	MassifgApplication *app = (MassifgApplication *)data;

	massifg_graph_set_show_legend(app->graph, gtk_toggle_action_get_active(action));
}

/* Set up actions and menus */
gint
massifg_gtkui_init_menus(MassifgApplication *app) {
	const gchar *uifile_path = NULL;
	GtkActionGroup *action_group = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *menubar = NULL;
	GtkWidget *window = NULL;
	GtkUIManager *uimanager = NULL;
	GError *error = NULL;

	/* Declare actions */
	GtkActionEntry actions[] =
	{ /* action name, stock id, label, accelerator, tooltip, action (callback) */
	  { "FileMenuAction", NULL, "_File", NULL, NULL, NULL},
	  { "QuitAction", GTK_STOCK_QUIT, "_Quit", NULL, NULL, G_CALLBACK(quit_action)},
	  { "OpenFileAction", GTK_STOCK_OPEN, "_Open...", NULL, NULL, G_CALLBACK(open_file_action)},
	  { "SaveFileAction", GTK_STOCK_SAVE, "_Save...", NULL, NULL, G_CALLBACK(save_file_action)},
	  { "PrintAction", GTK_STOCK_PRINT, "_Print...", NULL, NULL, G_CALLBACK(print_action)},

	  { "ViewMenuAction", NULL, "_View", NULL, NULL, NULL},
	};
	const guint num_actions = G_N_ELEMENTS(actions);

	GtkToggleActionEntry view_actions[] = {
	  {"ToggleDetailsAction", NULL, "_Detailed", NULL, NULL, G_CALLBACK(toggle_details_action), FALSE},
	  {"ToggleLegendAction", NULL, "_Legend", NULL, NULL, G_CALLBACK(toggle_legend_action), TRUE}
	};
	const guint num_view_actions = G_N_ELEMENTS(view_actions);

	/* Initialize */
	vbox = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MAIN_WINDOW_VBOX));
	window = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MASSIFG_GTKUI_MAIN_WINDOW));

	uifile_path = massifg_utils_get_resource_file("menu.ui");
	action_group = gtk_action_group_new ("action group");
	uimanager = gtk_ui_manager_new();

	/* Build menus */
	gtk_action_group_add_actions (action_group, actions, num_actions, app);
	gtk_action_group_add_toggle_actions (action_group, view_actions, num_view_actions, app);
	gtk_ui_manager_insert_action_group (uimanager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_file (uimanager, uifile_path, &error))
	{
		g_critical ("Building menus failed: %s", error->message);
		g_error_free (error);
		return 1;
	}

	/* Add the menubar to the window */
	menubar = gtk_ui_manager_get_widget(uimanager, MAIN_WINDOW_MENU);
	gtk_window_add_accel_group (GTK_WINDOW (window),
		               gtk_ui_manager_get_accel_group (uimanager));
	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 1);
	gtk_box_reorder_child (GTK_BOX (vbox), menubar, 0);

	/* Cleanup */
	g_free((gpointer)uifile_path);
	g_object_unref(G_OBJECT(action_group));
	g_object_unref(G_OBJECT(uimanager));

	return 0;
}

/* Public functions */

/**
 * massifg_gtkui_init:
 * @app: A #MassifgApplication
 *
 * Initialize the whole UI.
 * After calling this, the UI can be started by calling massifg_gtkui_start().
 */
gint
massifg_gtkui_init(MassifgApplication *app) {
	const gchar *gladefile_path = NULL;
	GtkWidget *graph_container = NULL;
	GtkWidget *graph_widget = NULL;
	GError *error = NULL;

	/* Initialize */
	gtk_init (app->argc_ptr, app->argv_ptr);

	g_signal_connect(app, "file-changed", G_CALLBACK(mainwindow_update_title), NULL);

	app->gtk_builder = gtk_builder_new();
	gladefile_path = massifg_utils_get_resource_file("massifg.glade");

	/* Build UI */
	if (!gtk_builder_add_from_file (app->gtk_builder, gladefile_path, &error))
	{
		g_critical ("%s", error->message);
		g_error_free (error);
		return 1;

	}
	gtk_builder_connect_signals(app->gtk_builder, NULL);

	/* Add the graph widget */
	graph_container = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, "scrolledwindow1"));
	app->graph = massifg_graph_new();
	graph_widget = massifg_graph_get_widget(app->graph);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(graph_container), graph_widget);

	/* Cleanup */
	g_free((gpointer)gladefile_path);

	return massifg_gtkui_init_menus(app);
}

/** 
 * massifg_gtkui_start:
 * @app: A #MassifgApplication
 *
 * Present the UI, and start the gtk mainloop.
 */
void
massifg_gtkui_start(MassifgApplication *app) {
	GtkWidget *main_window = NULL;
	main_window = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder, MASSIFG_GTKUI_MAIN_WINDOW));

	gtk_widget_show_all(main_window);
	gtk_main();
}

/** 
 * massifg_gtkui_errormsg:
 * @app: A #MassifgApplication
 * @msg_format: printf() like format string
 * @...: printf() like argument list
 *
 * Present a error message to the user.
 * The message can be formatted with Pango markup language.
 */
void
massifg_gtkui_errormsg(MassifgApplication *app, const gchar *msg_format, ...) {
	GtkMessageDialog *error_dialog = NULL;
	GString *markup_string = NULL;
	GtkWindow *main_window = NULL;
	va_list argp;

	/* Initialize */
	main_window = GTK_WINDOW(gtk_builder_get_object(app->gtk_builder, MASSIFG_GTKUI_MAIN_WINDOW));
	error_dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(main_window,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 NULL));
	markup_string = g_string_new("");

	/* Prepare markup string, */
	va_start(argp, msg_format);
	g_string_vprintf(markup_string, msg_format, argp);
	va_end(argp);

	/* Present the dialog to the user */
	gtk_message_dialog_set_markup(error_dialog, markup_string->str);
	gtk_dialog_run(GTK_DIALOG(error_dialog));

	g_string_free(markup_string, TRUE);
	gtk_widget_destroy(GTK_WIDGET(error_dialog));
}
