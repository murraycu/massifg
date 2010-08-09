/*
 *  MassifG - massifg_utils.c
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

/* This file contains utility functions used in MassifG that does not belong 
 * in a single component */

#include <glib.h>
#include "config.h"

/* Private functions */

/* Search system directories for given filename, as given by the glib function g_get_system_data_dirs()
 * Caller is responsible for freeing the returned array
 * Returns an absolute path, or NULL on */
static gchar *
get_system_file(const gchar *progam_name, const gchar *filename)
{
   gchar *pathname = NULL;
   const gchar* const *system_data_dirs;

   /* Iterate over array of strings to find system data files. */
   for (system_data_dirs = g_get_system_data_dirs (); *system_data_dirs != NULL; system_data_dirs++)
   {
      pathname = g_build_filename(*system_data_dirs, progam_name, filename, NULL);
      if (g_file_test(pathname, G_FILE_TEST_EXISTS))
      {
         break;
      }
      else
      {
         g_free (pathname);
         pathname = NULL;
      }
   }
   return pathname;
}

/* Find a resource file
 * Returns the path to the file, or NULL on failing to find a matching file 
 * Caller is responsible for freeing the results with g_free
 * First checks ./data/ in case the program is running from the build dir
 * Second, INSTALL_PREFIX/share in case of running from a non-system prefix
 * Then, checks the system folders as given by g_get_system_data_dirs()
 */
gchar *
massifg_utils_get_resource_file(const gchar *filename) {
	gchar *pathname = NULL;
	const gchar *run_prefix = NULL;
	const gchar *application_name = "massifg";

	/* Check in ./data/ */
	run_prefix = g_get_current_dir();
	pathname = g_build_filename(run_prefix, "data", filename, NULL);
	if (!g_file_test(pathname, G_FILE_TEST_EXISTS)) {
		g_free(pathname);
		pathname = NULL;
	}
	g_free((gpointer)run_prefix);

	/* If the file was not found, check in INSTALL_PREFIX/share/APPNAME/ */
	if (!pathname) {
		pathname = g_build_filename(INSTALL_PREFIX, "share", application_name, filename, NULL);
		if (!g_file_test(pathname, G_FILE_TEST_EXISTS)) {
			g_free(pathname);
			pathname = NULL;
		}
	}

	/* If the file was not found, check in system directories */
	if (!pathname) {
		pathname = get_system_file(application_name, filename);
	}

	g_debug("Path to resource file \"%s\": %s", filename, pathname);
	return pathname;
}

/* Log function for use with glib logging facilities that just ignores input */
void massifg_utils_log_ignore(const gchar *log_domain, GLogLevelFlags log_level,
			const gchar *message,
			gpointer user_data) {
	;
}

/* Function for freeing each element in a GList.
 * Elements freed using this function should have been allocated using g_alloc and derivatives
 * Meant to be used as a parameter to a g_(s)list_foreach call */
void massifg_utils_free_foreach(gpointer data, gpointer user_data) {
	g_free(data);
}

/* Check for the environment variable MASSIFG_DEBUG,
 * if it is not "enable" or "all", ignore debug output */
void
massifg_utils_configure_debug_output(void) {
	/* TODO: Allow enabling debug output for just the UI, grapher or parser */
	GDebugKey debug_keys[] = { {"enable", 1<<0}, };
	const int num_debug_keys = G_N_ELEMENTS(debug_keys);
	const gchar* debug_string = g_getenv("MASSIFG_DEBUG");

	guint debug = g_parse_debug_string(debug_string, debug_keys, num_debug_keys);
	if (!debug) {
		g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, massifg_utils_log_ignore, NULL); 
	}
}

