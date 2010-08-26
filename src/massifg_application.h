/*
 *  MassifG - massifg_application.h
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

#ifndef __MASSIFG_APPLICATION_H__
#define __MASSIFG_APPLICATION_H__

#include <gtk/gtk.h>
#include "massifg_parser.h"
#include "massifg_graph.h"


#include <glib-object.h>

/*
 * Type macros.
 */
#define MASSIFG_TYPE_APPLICATION                  (massifg_application_get_type ())
#define MASSIFG_APPLICATION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MASSIFG_TYPE_APPLICATION, MassifgApplication))
#define MASSIFG_IS_APPLICATION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MASSIFG_TYPE_APPLICATION))
#define MASSIFG_APPLICATION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MASSIFG_TYPE_APPLICATION, MassifgApplicationClass))
#define MASSIFG_IS_APPLICATION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MASSIFG_TYPE_APPLICATION))
#define MASSIFG_APPLICATION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MASSIFG_TYPE_APPLICATION, MassifgApplicationClass))

typedef struct _MassifgApplication        MassifgApplication;
typedef struct _MassifgApplicationClass   MassifgApplicationClass;

struct _MassifgApplication {
	GObject parent_instance;

	int *argc_ptr;
	char ***argv_ptr;
	gchar *filename;
	MassifgOutputData *output_data;
	MassifgGraph *graph;
	GtkBuilder *gtk_builder;
};

struct _MassifgApplicationClass {
	GObjectClass parent_class;

	void (*file_changed) (MassifgApplication *app, gpointer user_data);

};

/* used by MASSIFG_TYPE_APPLICATION */
GType massifg_application_get_type (void);

MassifgApplication *massifg_application_new(int *argc_ptr, char ***argv_ptr);
void massifg_application_free(MassifgApplication *app);

void massifg_application_set_file(MassifgApplication *app, gchar *filename);
int massifg_application_run(MassifgApplication *app);

#endif /* __MASSIFG_APPLICATION_H__ */
