

#include <glib.h>

#include <massifg_application.h>
#include <massifg_gtkui.h>

#include "common.h"




int open_files_cb_repeat_count = 5;

/* Forcefully destroy the main window 
 * Workaround because the window of test does not seem to be removed
 * before the window of the last test is */
void
massifg_application_destroy(MassifgApplication *app) {
	GtkWidget *main_window = NULL;
	main_window = GTK_WIDGET (gtk_builder_get_object (app->gtk_builder,
		 MASSIFG_GTKUI_MAIN_WINDOW));

	gtk_widget_destroy(main_window);

}

/* Open the same file over and over again. */
gboolean
open_files_cb(gpointer data) {
	gchar *filename = NULL;
	MassifgApplication *app = (MassifgApplication *)data;
	static int file_no = 0;

	if (file_no % 2) {
		filename = get_test_file(TEST_INPUT_LONG);
	}
	else {
		filename = get_test_file(TEST_INPUT_800);
	}

	massifg_application_set_file(app, filename, NULL);
	file_no++;
	g_free(filename);

	if (file_no == open_files_cb_repeat_count) {
		return FALSE;
	}
	return TRUE;
}

gboolean
quit_gtkmain_cb(gpointer data) {
	gtk_main_quit();
	return FALSE;
}

gboolean
enable_legend_cb(gpointer data) {
	MassifgApplication *app = (MassifgApplication *)data;
	massifg_graph_set_show_legend(app->graph, TRUE);
	return FALSE;
}

gboolean
disable_legend_cb(gpointer data) {
	MassifgApplication *app = (MassifgApplication *)data;
	massifg_graph_set_show_legend(app->graph, FALSE);
	return FALSE;
}

gboolean
enable_details_cb(gpointer data) {
	MassifgApplication *app = (MassifgApplication *)data;
	massifg_graph_set_show_details(app->graph, TRUE);
	return FALSE;
}

gboolean
disable_details_cb(gpointer data) {
	MassifgApplication *app = (MassifgApplication *)data;
	massifg_graph_set_show_details(app->graph, FALSE);
	return FALSE;
}

/* Tests */
void
application_start_quit(void) {
	int argc = 1;
	char **argv = g_new(char *, argc);
	argv[0] = "bin/massifg";

	MassifgApplication *app = massifg_application_new(&argc, &argv);
	g_timeout_add_seconds(1, quit_gtkmain_cb, NULL);

	massifg_application_run(app);

	massifg_application_destroy(app);
	massifg_application_free(app);
	g_usleep(G_USEC_PER_SEC*1);

}

void
application_start_open_file(void) {
	int argc = 2;
	char **argv = g_new(char *, argc);
	argv[0] = "bin/massifg";
	argv[1] = get_test_file(TEST_INPUT_LONG);

	MassifgApplication *app = massifg_application_new(&argc, &argv);
	g_timeout_add_seconds(1, quit_gtkmain_cb, NULL);

	massifg_application_run(app);

	massifg_application_destroy(app);
	massifg_application_free(app);
	g_free(argv[1]);
	g_usleep(G_USEC_PER_SEC*1);

}

void
application_open_many_files(void) {
	int argc = 1;
	char **argv = g_new(char *, argc);
	argv[0] = "bin/massifg";

	MassifgApplication *app = massifg_application_new(&argc, &argv);
	g_timeout_add_seconds(1, open_files_cb, app);
	g_timeout_add_seconds(open_files_cb_repeat_count+2, quit_gtkmain_cb, NULL);

	massifg_application_run(app);

	massifg_application_destroy(app);
	massifg_application_free(app);
	g_usleep(G_USEC_PER_SEC*1);
}

void
application_toogle_detailed_view(void) {
	int argc = 2;
	char **argv = g_new(char *, argc);
	argv[0] = "bin/massifg";
	argv[1] = get_test_file(TEST_INPUT_LONG);

	MassifgApplication *app = massifg_application_new(&argc, &argv);
	g_timeout_add_seconds(1, enable_details_cb, app);
	g_timeout_add_seconds(2, disable_details_cb, app);
	g_timeout_add_seconds(3, quit_gtkmain_cb, NULL);

	massifg_application_run(app);

	massifg_application_destroy(app);
	massifg_application_free(app);
	g_free(argv[1]);
	g_usleep(G_USEC_PER_SEC*1);

}

void
application_toogle_legend(void) {
	int argc = 2;
	char **argv = g_new(char *, argc);
	argv[0] = "bin/massifg";
	argv[1] = get_test_file(TEST_INPUT_LONG);

	MassifgApplication *app = massifg_application_new(&argc, &argv);
	g_timeout_add_seconds(1, disable_legend_cb, app);
	g_timeout_add_seconds(2, enable_legend_cb, app);
	g_timeout_add_seconds(3, quit_gtkmain_cb, NULL);

	massifg_application_run(app);

	massifg_application_destroy(app);
	massifg_application_free(app);
	g_free(argv[1]);
	g_usleep(G_USEC_PER_SEC*1);


}

int
main (int argc, char **argv) {
/*	g_mem_set_vtable(glib_mem_profiler_table);
 * Disabled due to https://bugzilla.gnome.org/show_bug.cgi?id=627700
 * This effectively makes it impossible to use glibs memory profiler to test
 * for memory leakage when goffice is involved. Instead we have to use an
 * external program like valgrind */

	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/application/start-quit", application_start_quit);
	g_test_add_func("/application/start-open-file", application_start_open_file);

	g_test_add_func("/application/open-many-files", application_open_many_files);

	g_test_add_func("/application/toggle-details", application_toogle_detailed_view);
	g_test_add_func("/application/toggle-legend", application_toogle_legend);


	massifg_utils_configure_debug_output();
	return g_test_run();
}
