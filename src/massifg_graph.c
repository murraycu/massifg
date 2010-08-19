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

/* Data structures */
/* Enum that represents the different possible data series */
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
void
fill_data_array_func(gpointer data, gpointer user_data) {
	MassifgSnapshot *snapshot = (MassifgSnapshot *)data;
	FillDataArrayFuncArg *arg = (FillDataArrayFuncArg *)user_data;
	double y_val;

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

/* */
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

/* */
void
massifg_graph_update_simple(MassifgGraph *graph, GOData *time_data) {
	MassifgDataSeries ds;
	for (ds=MASSIFG_DATA_SERIES_HEAP; ds<=MASSIFG_DATA_SERIES_STACKS; ds++) {
		GogSeries *gog_series = gog_plot_new_series(graph->plot);
		GOData *series_data = data_from_snapshots(graph->data->snapshots, ds);
		GOData *series_name = go_data_scalar_str_new(MASSIFG_DATA_SERIES_DESC[ds], FALSE);
		gog_series_set_name(gog_series, series_name, &graph->error);
		gog_series_set_dim(gog_series, 0, time_data, &graph->error);
		gog_series_set_dim(gog_series, 1, series_data, &graph->error);
	}
}

typedef struct {
	MassifgGraph *graph;
	GOData *time_data;
	GList *snapshot_details;
} AddDetailsSerieArg;

void
add_details_serie_foreach(gpointer key, gpointer value, gpointer user_data) {
	gchar *label = (gchar *)key;
	AddDetailsSerieArg *arg = (AddDetailsSerieArg *)user_data;
	MassifgGraph *graph = arg->graph;

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
	GOData *series_data = go_data_vector_val_new(array, length, NULL);

	/* Add it to the graph */
	GogSeries *gog_series = gog_plot_new_series(graph->plot);
	GOData *series_name = go_data_scalar_str_new(label, FALSE);
	gog_series_set_name(gog_series, series_name, &graph->error);
	gog_series_set_dim(gog_series, 0, arg->time_data, &graph->error);
	gog_series_set_dim(gog_series, 1, series_data, &graph->error);
}


typedef struct {
	GHashTable *functions;
	GHashTable *function_labels;
} AddDetailsArg;

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

/* */
void
massifg_graph_update_detailed(MassifgGraph *graph, GOData *time_data) {
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

		g_node_children_foreach(s->heap_tree, G_TRAVERSE_ALL,
			add_details_foreach, (gpointer)&foreach_arg);

		l = l->next;
	}

	/* For each function, create and add a data serie to graph */
	AddDetailsSerieArg arg;
	arg.graph = graph;
	arg.time_data = time_data;
	arg.snapshot_details = snapshot_details;

	g_hash_table_foreach(function_labels, add_details_serie_foreach, (gpointer)&arg);

	/* FIXME: free snapshot_details */
	g_hash_table_destroy(function_labels);
}

/* Public functions */

/* Initialize.
 * Must be called before the first call to massifg_graph_new() */
void
massifg_graph_init(void) {
	libgoffice_init();
	go_plugins_init(NULL, NULL, NULL, NULL, TRUE, GO_TYPE_PLUGIN_LOADER_MODULE);
}

MassifgGraph *
massifg_graph_new(void) {
	GogGraph *go_graph = NULL;
	GogChart *chart = NULL;

	MassifgGraph *graph = (MassifgGraph *)g_malloc(sizeof(MassifgGraph));
	graph->data = NULL;

	/* Defaul settings */
	graph->detailed = FALSE;
	graph->has_legend = FALSE;

	/* Create a graph widget, and get the embedded graph and chart */
	graph->widget = go_graph_widget_new(NULL);
	go_graph = go_graph_widget_get_graph(GO_GRAPH_WIDGET(graph->widget));
	chart = go_graph_widget_get_chart(GO_GRAPH_WIDGET(graph->widget));

	/* Create a plot and add it to the chart */
	graph->plot = (GogPlot *)gog_plot_new_by_name("GogAreaPlot");

	if (!graph->plot) {
		g_critical("Cannot create plot. Possibly because plugins are not found");
	}

	g_object_set (G_OBJECT (graph->plot), "type", "stacked", NULL);

	gog_object_add_by_name(GOG_OBJECT (chart), "Plot", GOG_OBJECT(graph->plot));

	return graph;
}

void massifg_graph_free(MassifgGraph *graph) {

	/* FIXME: actually free the stuff used by graph */
	g_free(graph);
}

/* Utility function that updates the graph data members, and then redraws it */
void 
massifg_graph_update(MassifgGraph *graph, MassifgOutputData *data) {
	/* Update graph members */
	if (data) {
		graph->data = data;
	}

	/* Ensure we have data to plot */
	if (!graph->data) {
		return;
	}

	/* Update the data series */
	gog_plot_clear_series(graph->plot); /* TODO: verify that we are not responsible for freeing */
	GOData *time_data = data_from_snapshots(graph->data->snapshots,
						MASSIFG_DATA_SERIES_TIME);

	if (graph->detailed) {
		massifg_graph_update_detailed(graph, time_data);
	}
	else {
		massifg_graph_update_simple(graph, time_data);
	}
}

/* If the graph should be detailed or not */
void
massifg_graph_set_show_details(MassifgGraph *graph, gboolean is_detailed) {

	graph->detailed = is_detailed;
	massifg_graph_update(graph, NULL);
}

/* Enable/disable display of legend */
void
massifg_graph_set_show_legend(MassifgGraph *graph, gboolean has_legend) {
	GogObject *gog_object = NULL;
	GogChart *chart = go_graph_widget_get_chart(GO_GRAPH_WIDGET(graph->widget));

	graph->has_legend = has_legend;

	if (has_legend) {
		gog_object_add_by_name(GOG_OBJECT(chart), "Legend", NULL);
	}
	else {
		/* Remove existing legend, if any */
		gog_object = gog_object_get_child_by_name(GOG_OBJECT(chart), "Legend");
		if (gog_object) {
			gog_object_clear_parent(gog_object);
			g_object_unref(G_OBJECT(gog_object));
		}

	}
}

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

