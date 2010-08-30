/*
 *  MassifG - massifg_graph.c
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

#include <glib.h>

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

#include "massifg_parser.h"
#include "massifg_graph.h"

/* This file implements the graphing functionality of MassifG
 * Currently two modes are supported by the graph, one "simple" and one "detailed"
 * The "simple" mode shows an area plot over stack, heap and "heap extra" memory usage
 * for each snapshot
 * The "detailed" mode shows an area plot, which is broken down
 * to show how different functions contribute to the heap memory usage
 * for each snapshot. Currently only the first children of the heap tree is displayed */

/* Data structures */
/* Enum that represents the different possible simple data series */
typedef enum {
	MASSIFG_DATA_SERIES_TIME,
	MASSIFG_DATA_SERIES_HEAP,
	MASSIFG_DATA_SERIES_HEAP_EXTRA,
	MASSIFG_DATA_SERIES_STACKS,
	MASSIFG_DATA_SERIES_LAST /* NOTE: only used to calculate the number of elements in the enum */
} MassifgDataSeries;

/* Indexed by MassifgDataSeries */
gchar *MASSIFG_DATA_SERIES_DESC[] = {"Time", "Heap", "Heap Extra", "Stacks"};

typedef struct {
	MassifgDataSeries series;
	double *data_array;
	gint index;
} FillDataArrayFuncArg;

/* Private functions */
/* Fills arg->data_array with values from the specified series */
void
fill_data_array_func(gpointer data, gpointer user_data) {
	MassifgSnapshot *snapshot = (MassifgSnapshot *)data;
	FillDataArrayFuncArg *arg = (FillDataArrayFuncArg *)user_data;
	double y_val = 0;

	switch (arg->series) {
	case MASSIFG_DATA_SERIES_TIME:
		y_val = (double)snapshot->time;
		break;
	case MASSIFG_DATA_SERIES_HEAP:
		y_val = (double)snapshot->mem_heap_B;
		break;
	case MASSIFG_DATA_SERIES_HEAP_EXTRA:
		y_val = (double)snapshot->mem_heap_extra_B;
		break;
	case MASSIFG_DATA_SERIES_STACKS:
		y_val = (double)snapshot->mem_stacks_B;
		break;
	case MASSIFG_DATA_SERIES_LAST:
		g_assert_not_reached();
		break;
	}
	arg->data_array[arg->index++] = y_val;
}

/* Returns a data vector for a single MassifgDataSeries */
GOData *
data_from_snapshots(GList *snapshots, MassifgDataSeries series) {
	FillDataArrayFuncArg foreach_arg;
	gint length = g_list_length(snapshots);
	double *array = g_malloc_n(length, sizeof(double));

	foreach_arg.series = series;
	foreach_arg.data_array = array;
	foreach_arg.index = 0;

	g_list_foreach((gpointer)snapshots, fill_data_array_func, (gpointer)&foreach_arg);
	g_assert_cmpint(foreach_arg.index, ==, length);
	return go_data_vector_val_new(array, length, NULL);
}

/* Utility function to add a serie to the plot */
void
massifg_graph_add_series(MassifgGraph *graph, GOData *name, GOData *x, GOData *y) {
	GogSeries *gog_series = gog_plot_new_series(graph->plot);

	gog_series_set_name(gog_series, (GODataScalar *)name, NULL);
	gog_series_set_dim(gog_series, 0, x, NULL);
	gog_series_set_dim(gog_series, 1, y, NULL);
}

/* Adds all the simple data series to graph
 * The graph should be cleared for data series before this is called */
void
massifg_graph_update_simple(MassifgGraph *graph) {
	MassifgDataSeries ds;
	for (ds=MASSIFG_DATA_SERIES_HEAP; ds<=MASSIFG_DATA_SERIES_STACKS; ds++) {
		GOData *series_data = data_from_snapshots(graph->data->snapshots, ds);
		GOData *series_name = go_data_scalar_str_new(MASSIFG_DATA_SERIES_DESC[ds], FALSE);

		/* TODO: don't get this for each series, as it is identical for all of them 
		 * The same object cannot be added to all of them because that screws up clearing the series (same object is freed several times) */
		GOData *time_data = data_from_snapshots(graph->data->snapshots,	MASSIFG_DATA_SERIES_TIME);

		massifg_graph_add_series(graph, series_name, time_data, series_data);
	}
}

typedef struct {
	MassifgGraph *graph;
	GList *snapshot_details;
} AddDetailsSerieArg;

/* Add a single detailed data series, as specified by key */
void
add_details_serie_foreach(gpointer key, gpointer value, gpointer user_data) {
	gchar *label = (gchar *)key;
	AddDetailsSerieArg *arg = (AddDetailsSerieArg *)user_data;
	MassifgGraph *graph = arg->graph;
	GOData *series_data, *time_data, *series_name;

	/* Get the actual data series */
	GList *l = arg->snapshot_details;
	GHashTable *functions = NULL;
	int i = 0;
	int length = g_list_length(l);

	double *array = g_new(double, length);

	while (l) {
		functions = (GHashTable *)l->data;
		array[i] = GPOINTER_TO_INT(g_hash_table_lookup(functions, label));
		l = l->next;
		i++;
	}
	series_data = go_data_vector_val_new(array, length, NULL);
	time_data = data_from_snapshots(graph->data->snapshots,	MASSIFG_DATA_SERIES_TIME);
	series_name = go_data_scalar_str_new(label, FALSE);

	/* Add it to the graph */
	massifg_graph_add_series(graph, series_name, time_data, series_data);
}


typedef struct {
	GHashTable *functions;
	GHashTable *function_labels;
} AddDetailsArg;

/* Adds function values to two hash tables:
 * ->functions is a table over the functions and their memory usage in a single snapshot 
 * ->function_labels is a table over all the functions in a MassifgOutputData */
void
add_details_foreach(GNode *node, gpointer user_data) {
	MassifgHeapTreeNode *n = (MassifgHeapTreeNode *)node->data;
	AddDetailsArg *arg = (AddDetailsArg *)user_data;

	/* Note: key is not copied and value is trucated to 32bit */
	g_hash_table_insert(arg->functions, n->label->str, GINT_TO_POINTER(n->total_mem_B));

	if (!g_hash_table_lookup(arg->function_labels, n->label->str)) {
		g_hash_table_insert(arg->function_labels, n->label->str, NULL);
	}
}

/* Adds all the detailed data series to graph
 * The graph should have been cleared for data series before this is called */
void
massifg_graph_update_detailed(MassifgGraph *graph) {
	AddDetailsSerieArg serie_arg;
	GHashTable *function_labels = g_hash_table_new(g_str_hash, g_str_equal);
	/* List of GHashTables with "function_label": mem_B, needs to be freed */
	GList *snapshot_details = NULL;

	/* Build datastructures for the functions we want to show */
	AddDetailsArg foreach_arg;
	MassifgSnapshot *s = NULL;
	GHashTable *ht = NULL;
	GList *l = graph->data->snapshots;
	foreach_arg.function_labels = function_labels;
	while (l) {
		s = (MassifgSnapshot *)l->data;
		ht = g_hash_table_new(g_str_hash, g_str_equal);
		snapshot_details = g_list_append(snapshot_details, ht);
		foreach_arg.functions = ht;

		/* Note: we only look at the direct children of the root, because that
		 * is the behaviour that ms_print and massif_grapher has */
		if (s->heap_tree) {
			g_node_children_foreach(s->heap_tree, G_TRAVERSE_ALL,
				add_details_foreach, (gpointer)&foreach_arg);
		}
		l = l->next;
	}

	/* For each function, create and add a data serie to graph */
	serie_arg.graph = graph;
	serie_arg.snapshot_details = snapshot_details;

	g_hash_table_foreach(function_labels, add_details_serie_foreach, (gpointer)&serie_arg);

	/* FIXME: free snapshot_details */
	g_hash_table_destroy(function_labels);
}

void
massifg_graph_update(MassifgGraph *graph) {
	/* Update the data series */
	gog_plot_clear_series(graph->plot); /* TODO: verify that we are not responsible for freeing */

	if (graph->detailed) {
		massifg_graph_update_detailed(graph);
	}
	else {
		massifg_graph_update_simple(graph);
	}
}

/* Public functions */

/* Initialize.
 * Must be called before the first call to massifg_graph_new() */
void
massifg_graph_init(void) {
	libgoffice_init();
	go_plugins_init(NULL, NULL, NULL, NULL, TRUE, GO_TYPE_PLUGIN_LOADER_MODULE);
}

/* Create a new graph.
 * Free with massifg_graph_free */
MassifgGraph *
massifg_graph_new(void) {
	GogChart *chart = NULL;

	MassifgGraph *graph = (MassifgGraph *)g_malloc(sizeof(MassifgGraph));
	graph->data = NULL;

	/* Defaul settings */
	graph->detailed = FALSE;
	graph->has_legend = FALSE;

	/* Create a graph widget, and get the embedded graph and chart */
	graph->widget = go_graph_widget_new(NULL);
	chart = go_graph_widget_get_chart(GO_GRAPH_WIDGET(graph->widget));

	/* Create a plot and add it to the chart */
	graph->plot = (GogPlot *)gog_plot_new_by_name("GogAreaPlot");
	g_object_set (G_OBJECT (graph->plot), "type", "stacked", NULL);

	gog_object_add_by_name(GOG_OBJECT (chart), "Plot", GOG_OBJECT(graph->plot));

	return graph;
}

/* Free a MassifgGraph */
void massifg_graph_free(MassifgGraph *graph) {

	/* FIXME: actually free the stuff used by graph */
	g_free(graph);
}

/* Set the data to visualize */
void 
massifg_graph_set_data(MassifgGraph *graph, MassifgOutputData *data) {
	if (graph->data) {
		massifg_output_data_free(graph->data);
	}
	graph->data = data;
	massifg_graph_update(graph);
}


/* If the graph should be detailed or not */
void
massifg_graph_set_show_details(MassifgGraph *graph, gboolean is_detailed) {

	graph->detailed = is_detailed;
	if (graph->data) {
		massifg_graph_update(graph);
	}
}

/* Enable/disable display of legend */
void
massifg_graph_set_show_legend(MassifgGraph *graph, gboolean show_legend) {
	GogObject *gog_object = NULL;
	GogChart *chart = go_graph_widget_get_chart(GO_GRAPH_WIDGET(graph->widget));

	if (show_legend && !graph->has_legend) {
		gog_object_add_by_name(GOG_OBJECT(chart), "Legend", NULL);
	}

	if (!show_legend) {
		/* Remove existing legend, if any */
		gog_object = gog_object_get_child_by_name(GOG_OBJECT(chart), "Legend");
		if (gog_object) {
			gog_object_clear_parent(gog_object);
			g_object_unref(G_OBJECT(gog_object));
		}

	}
	graph->has_legend = show_legend;

}

/* Get the widget which displays the graph */
GtkWidget *massifg_graph_get_widget(MassifgGraph *graph) {
	return graph->widget;
}

/* Get the data the graph represents */
MassifgOutputData *massifg_graph_get_data(MassifgGraph *graph) {
	return graph->data;
}


/* Render graph to the cairo context cr,
 * with the specified width and height */
gboolean
massifg_graph_render_to_cairo(MassifgGraph *graph, cairo_t *cr,
				gint width, gint height) {
	gboolean retval;

	GogGraph *go_graph = go_graph_widget_get_graph(GO_GRAPH_WIDGET(graph->widget));
	GogRenderer *renderer = gog_renderer_new(go_graph);

	retval = gog_renderer_render_to_cairo(renderer, cr, width, height);
	g_object_unref(G_OBJECT(renderer));
	return retval;
}


/* Render the graph to a png file */
gboolean
massifg_graph_render_to_png(MassifgGraph *graph, gchar *filename, int w, int h) {
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	cairo_t *cr = cairo_create(surface);

	if (massifg_graph_render_to_cairo(graph, cr, w, h)) {
		return FALSE;
	}
	cairo_surface_write_to_png(surface, filename);
	return TRUE;
}
