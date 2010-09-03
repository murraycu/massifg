/*
 *  MassifG - massifg_utils.n
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

#ifndef MASSIFG_UTILS_H__
#define MASSIFG_UTILS_H__

gchar *massifg_utils_get_resource_file(const gchar *filename);
void massifg_utils_log_ignore(const gchar *log_domain, GLogLevelFlags log_level,
			const gchar *message,
			gpointer user_data);
void massifg_utils_free_foreach(gpointer data, gpointer user_data);
void massifg_utils_configure_debug_output(void);

gchar *massifg_str_cut_region(gchar *src, int cut_start, int cut_end);
int massifg_str_count_char(gchar *str, gchar c);
gchar *massifg_str_copy_region(gchar *src, gint start_idx, gint stop_idx);

#endif /* MASSIFG_UTILS_H__ */
