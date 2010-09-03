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

/**
 * SECTION:massifg_utils
 * @short_description: Useful utility functions
 * @title: MassifG Utility functions
 *
 * More or less generic functions that do not belong in one single component
 */

#include <string.h>

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

/* Public functions */

/**
 * massifg_utils_get_resource_file:
 * @filename: file to search for
 * @Returns: the path to the file as a newly allocated string (free with g_free()), or NULL on failure
 *
 * Find a resource file
 *
 * First checks ./data/ in case the program is running from the build dir.
 * Secondly, INSTALL_PREFIX/share in case of running from a non-system prefix.
 * Then, checks the system folders as given by g_get_system_data_dirs()
 *
 * Note: Not suitable for use in other applications, hardcodes the application name to "massifg"
 */
gchar *
massifg_utils_get_resource_file(const gchar *filename) {
	gchar *pathname = NULL;
	gchar *run_prefix = NULL;
	const gchar *application_name = PACKAGE_TARNAME;

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

/**
 * massifg_utils_log_ignore:
 * @log_domain: log_domain
 * @log_level: log_level
 * @message: message
 * @user_data: user_data
 *
 * Log function for use with glib logging facilities that just ignores input.
 * Can be used to silence messages of a certain type, see massifg_utils_configure_debug_output()
 */
void massifg_utils_log_ignore(const gchar *log_domain, GLogLevelFlags log_level,
			const gchar *message,
			gpointer user_data) {
	;
}

/**
 * massifg_utils_free_foreach:
 * @data: data
 * @user_data: user_data
 *
 * Utility function for freeing each element in a #GList.
 *
 * Elements freed using this function should have been allocated using g_malloc() and derivatives.
 * Intended to be used as a parameter to a g_(s)list_foreach call.
 */
void massifg_utils_free_foreach(gpointer data, gpointer user_data) {
	g_free(data);
}

/**
 * massifg_utils_configure_debug_output:
 *
 * Checks for the environment variable MASSIFG_DEBUG,
 * if it is not "enable" or "all", debug output will be ignored.
 */
void
massifg_utils_configure_debug_output(void) {
	/* TODO: Allow enabling debug output for just the UI, grapher or parser */
	GDebugKey debug_keys[] = { {"enable", 1<<0}, };
	const guint num_debug_keys = G_N_ELEMENTS(debug_keys);
	const gchar* debug_string = g_getenv("MASSIFG_DEBUG");

	guint debug = g_parse_debug_string(debug_string, debug_keys, num_debug_keys);
	if (!debug) {
		g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, massifg_utils_log_ignore, NULL); 
	}
}


/** 
 * massifg_str_copy_region:
 * @src: String to copy from. Must be %NULL terminated
 * @start_idx: Index in @src to start copying from.
 * @stop_idx: Index in @src to stop copying from. Must be smaller than the length of src
 * @Returns: a newly allocated string. Free with g_free()
 *
 * Copy a region of a string.
 *
 * Indexes start at 0.
 */
gchar *
massifg_str_copy_region(const gchar *src, const guint start_idx, const guint stop_idx) {
	guint i, len;
	gchar *str;

	len = stop_idx-start_idx;
	g_return_val_if_fail(len <= strlen(src), NULL);

	str = g_new0(gchar, len+1); /* 1 for the \0 byte*/
	for (i=0; i<len; i++) {
		str[i] = src[i+start_idx];
	}
	return str;
}

/**
 * massifg_str_count_char:
 * @str: String to search in. Must be %NULL terminated.
 * @c: Character to count
 * @Returns: the number of occurrences of @c in @str
 *
 * Count the number of times a character appears in a string.
 */
guint
massifg_str_count_char(const gchar *str, const gchar c) {
	guint num_occurrences = 0;
	guint i;
	for (i=0; i<strlen(str); i++) {
		if (str[i] == c) {
			num_occurrences++;
		}
	}
	return num_occurrences;
}

/**
 * massifg_str_cut_region:
 * @src: String to copy from
 * @cut_start: Index in @src to start excluding
 * @cut_end: Index in @src to stop excluding
 * @Returns: a newly allocated string. Free with g_free()
 *
 * Copy string, excluding a certain region.
 *
 * Indexes start at 0.
 */
gchar *
massifg_str_cut_region(const gchar *src, const guint cut_start, const guint cut_end) {
	gchar *new_str = NULL;
	gchar *start_str = g_strndup(src, cut_start);
	gchar *end_str = massifg_str_copy_region(src, cut_end+1, strlen(src));

	new_str = g_strconcat(start_str, end_str, NULL);

	g_free(start_str);
	g_free(end_str);

	return new_str;
}
