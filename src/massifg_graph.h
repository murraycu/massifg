/*
 *  MassifG - massifg_graph.h
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

#include <goffice/goffice.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/go-plugin-loader-module.h>
#include <goffice/data/go-data-simple.h>
#include <goffice/graph/gog-data-set.h>
#include <goffice/graph/gog-label.h>
#include <goffice/graph/gog-object.h>
#include <goffice/graph/gog-plot.h>
#include <goffice/graph/gog-series.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/gtk/go-graph-widget.h>

#include <cairo.h>
#include <glib.h>

#include "massifg_parser.h"

#ifndef MASSIFG_GRAPH_H__
#define MASSIFG_GRAPH_H__

/* Data structures */

/**
 * MassifgGraph:
 *
 * Holds the shared state used in the graph.
 */
typedef struct {
	/*< private >*/
	MassifgOutputData *data;
	GtkWidget *widget;
	GError *error;

	gboolean detailed;
	gboolean has_legend;

	GogPlot *plot;
} MassifgGraph;

/* Public functions */
void massifg_graph_init(void);

MassifgGraph *massifg_graph_new(void);
void massifg_graph_free(MassifgGraph *graph);

void massifg_graph_set_data(MassifgGraph *graph, MassifgOutputData *data);
void massifg_graph_set_show_details(MassifgGraph *graph, gboolean show_details);
void massifg_graph_set_show_legend(MassifgGraph *graph, gboolean show_legend);

GtkWidget *massifg_graph_get_widget(MassifgGraph *graph);
MassifgOutputData *massifg_graph_get_data(MassifgGraph *graph);

gboolean massifg_graph_render_to_cairo(MassifgGraph *graph, cairo_t *cr, const guint width, const guint height);
gboolean massifg_graph_render_to_png(MassifgGraph *graph, const gchar *filename, const guint width, const guint height);

#endif /* MASSIFG_GRAPH_H__ */
