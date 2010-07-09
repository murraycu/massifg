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

#define GLADE_FILE "ui/massifg.glade"
#define UI_FILE "menu.ui"


#define MAIN_WINDOW "mainwindow"
#define MAIN_WINDOW_VBOX "mainvbox"
#define MAIN_WINDOW_MENU "mainmenu"


int
main (int argc, char **argv) {

	GtkBuilder *builder = NULL;
	GtkWidget *window = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *menubar = NULL;
	GtkActionGroup *def_group = NULL;
	GtkUIManager *uimanager = NULL;
	GError *error = NULL;

	gtk_init (&argc, &argv);

	/* GTK builder */
	builder = gtk_builder_new();

	if (!gtk_builder_add_from_file (builder, GLADE_FILE, &error))
	{
		g_message ("%s", error->message);
		g_error_free (error);
		return 1;
	}

	window = GTK_WIDGET (gtk_builder_get_object (builder, MAIN_WINDOW));
	vbox = GTK_WIDGET (gtk_builder_get_object (builder, MAIN_WINDOW_VBOX));

	gtk_builder_connect_signals (builder,NULL);
	g_object_unref (G_OBJECT(builder));

	/* UI manager */
/*	def_group = gtk_action_group_new (ACTION_GROUP);
	gtk_action_group_add_actions (def_group, actions, NUM_ACTIONS, NULL);
	gtk_action_group_add_radio_actions (def_group, radio_actions, NUM_RACTIONS,
		                       -1, G_CALLBACK (get_sentence_action), NULL);

	uimanager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (uimanager, def_group, 0);

	if (!gtk_ui_manager_add_ui_from_file (uimanager, UI_FILE, &error))
	{
		g_message ("Building menus failed: %s", error->message);
		g_error_free (error);
		return 1;
	}

	/* Menubar */
/*	menubar = gtk_ui_manager_get_widget (uimanager, MENU);
	gtk_window_add_accel_group (GTK_WINDOW (window),
		               gtk_ui_manager_get_accel_group (uimanager));

	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 1);
	gtk_box_reorder_child (GTK_BOX (vbox), menubar, 0);


	/* Present window, and start the gtk mainloop */
	gtk_widget_show_all (window);
	gtk_main();

	return 0;
}
