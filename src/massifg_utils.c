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

