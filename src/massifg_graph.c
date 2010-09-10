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

/**
 * SECTION:massifg_graph
 * @short_description: A graph visualizing massif output over time
 * @title: MassifG Overview Graph
 * @stability: Unstable
 *
 * Implements the graphing functionality of MassifG
 *
 * Currently two modes are supported by the graph, one "simple" and one "detailed".
 * The "simple" mode shows an area plot over stack, heap and heap allocation overhead
 * memory usage for each snapshot.
 *  
 * The "detailed" mode shows an area plot, which is broken down
 * to show how different functions contribute to the heap memory usage
 * for each snapshot. Currently only the first children of the heap tree is displayed
 * in detailed mode
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

#include "massifg_graph.h"
#include "massifg_graph_private.h"

#include "massifg_utils.h"
#include "massifg_parser.h"

/* Data structures */

/**
 * MassifgDataSeries:
 * @MASSIFG_DATA_SERIES_TIME: data series of time
 * @MASSIFG_DATA_SERIES_HEAP: data series of heap memory
 * @MASSIFG_DATA_SERIES_HEAP_EXTRA: data series of extra heap memory
 * @MASSIFG_DATA_SERIES_STACKS: data series of stack memory
 *
 * Enum that represents the different possible simple data series.
 */
typedef enum {
	MASSIFG_DATA_SERIES_TIME,
	MASSIFG_DATA_SERIES_HEAP,
	MASSIFG_DATA_SERIES_HEAP_EXTRA,
	MASSIFG_DATA_SERIES_STACKS,
	/*< private >*/
	MASSIFG_DATA_SERIES_LAST /* NOTE: only used to calculate the number of elements in the enum */
} MassifgDataSeries;

/* Indexed by MassifgDataSeries */
gchar *MASSIFG_DATA_SERIES_DESC[] = {"Time", "Heap", "Heap Extra", "Stacks"};

typedef struct {
	MassifgDataSeries series;
	gdouble *data_array;
	gint index;
} FillDataArrayFuncArg;

/* Private functions */
/* Fills arg->data_array with values from the specified series */
void
fill_data_array_func(gpointer data, gpointer user_data) {
	MassifgSnapshot *snapshot = (MassifgSnapshot *)data;
	FillDataArrayFuncArg *arg = (FillDataArrayFuncArg *)user_data;
	gdouble y_val = 0;

	switch (arg->series) {
	case MASSIFG_DATA_SERIES_TIME:
		y_val = (gdouble)snapshot->time;
		break;
	case MASSIFG_DATA_SERIES_HEAP:
		y_val = (gdouble)snapshot->mem_heap_B;
		break;
	case MASSIFG_DATA_SERIES_HEAP_EXTRA:
		y_val = (gdouble)snapshot->mem_heap_extra_B;
		break;
	case MASSIFG_DATA_SERIES_STACKS:
		y_val = (gdouble)snapshot->mem_stacks_B;
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
	gdouble *array = g_malloc_n(length, sizeof(gdouble));

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

/* 
 * Currently shortens by stripping the function arguments of overloaded C++
 * functions.
 * Example: "std::string::_Rep::_S_create(unsigned int, unsigned int, std::allocator<char> const&) (in /usr/lib/libstdc++.so.6.0.13)" -> "std::string::_Rep::_S_create (in /usr/lib/libstdc++.so.6.0.13)"
 * Result should be freed with g_free */
gchar *
get_short_function_label(const gchar *label) {
	gchar *short_label = NULL;
	guint args_start_idx;
	guint args_end_idx;

	if (massifg_str_count_char(label, '(') == 1) {
		/* No need to shorten */
		short_label = g_strdup(label);
		return short_label;
	}

	args_start_idx = (guint)(g_strstr_len(label, -1, "(")-label);
	args_end_idx = (guint)(g_strstr_len(label, -1, ")")-label);

	short_label = massifg_str_cut_region(label, args_start_idx, args_end_idx);

	return short_label;
}

typedef struct {
	MassifgGraph *graph;
	GList *snapshot_details;
} AddDetailsSerieArg;

/* Add a single detailed data series, as specified by key */
void
add_details_serie_foreach(gpointer data, gpointer user_data) {
	gchar *label = (gchar *)data;
	AddDetailsSerieArg *arg = (AddDetailsSerieArg *)user_data;
	MassifgGraph *graph = arg->graph;
	GOData *series_data, *time_data, *series_name;

	/* Get the actual data series */
	GList *l = arg->snapshot_details;
	GHashTable *functions = NULL;
	gint i = 0;
	const gint length = g_list_length(l);

	gdouble *array = g_new(double, length);

	while (l) {
		functions = (GHashTable *)l->data;
		array[i] = GPOINTER_TO_INT(g_hash_table_lookup(functions, label));
		l = l->next;
		i++;
	}
	series_data = go_data_vector_val_new(array, length, NULL);
	time_data = data_from_snapshots(graph->data->snapshots,	MASSIFG_DATA_SERIES_TIME);
	series_name = go_data_scalar_str_new(get_short_function_label(label), TRUE);

	/* Add it to the graph */
	massifg_graph_add_series(graph, series_name, time_data, series_data);
}

/**
 * AddDetailsArg:
 * @function_labels: A table over all the functions in a #MassifgOutputData, and
 * how many #MassifgSnapshot they appear in
 * @functions_sorted: A sorted list of the keys in @function_labels
 * @functions: A temporary table over the functions and their memory usage in a single #MassifgSnapshot
 *
 *
 */
typedef struct {
	GHashTable *functions;
	GHashTable *function_labels;
	GList *functions_sorted;
} AddDetailsArg;

/* Adds function values */
void
add_details_foreach(GNode *node, gpointer user_data) {
	gboolean exists = FALSE;
	gpointer value = 0;
	MassifgHeapTreeNode *n = (MassifgHeapTreeNode *)node->data;
	AddDetailsArg *arg = (AddDetailsArg *)user_data;
	gchar *label_str = n->label->str;

	/* Note: key is not copied and value is trucated to 32bit */
	g_hash_table_insert(arg->functions, label_str, GINT_TO_POINTER(n->total_mem_B));

	exists = g_hash_table_lookup_extended(arg->function_labels, label_str, NULL, &value);
	if (exists) {
		g_hash_table_insert(arg->function_labels, label_str,
				GINT_TO_POINTER(GPOINTER_TO_INT(value)+1));
	}
	else {
		g_hash_table_insert(arg->function_labels, label_str, GINT_TO_POINTER(0));
	}
}

/**
 * sort_func_label:
 * @a: a value
 * @b: a value to compare with
 * @user_data: user data
 * @Returns: +1 if function @a appears in less snapshots than @b,
 * -1 if function @a appears in more snapshots than @b and 0
 * if @a appears in as many as @b
 *
 * A #GCompareDataFunc to sort snapshots.
 */
gint
sort_func_label(gconstpointer a, gconstpointer b, gpointer user_data) {
	GHashTable *function_labels = (GHashTable *)user_data;

	gchar *label_a = (gchar *)a;
	gchar *label_b = (gchar *)b;
	int value_a = GPOINTER_TO_INT(g_hash_table_lookup(function_labels, label_a));
	int value_b = GPOINTER_TO_INT(g_hash_table_lookup(function_labels, label_b));

	if (value_a == value_b)	return 0;
	if (value_a < value_b) return +1;
	if (value_a > value_b) return -1;
	g_assert_not_reached();
}

/* Sort the functions according to how many snapshots they appear in
 * This allows short-lived functions to be on top of the graph, and long-lived
 * ones on the bottom, which leads to a less confusing graph */
void
sort_details_serie_foreach(gpointer key, gpointer value, gpointer user_data) {
	AddDetailsArg *arg = (AddDetailsArg *)user_data;

	arg->functions_sorted = g_list_insert_sorted_with_data(
			arg->functions_sorted, key, sort_func_label,
			arg->function_labels);
}

/* Build datastructures for the functions we want to show */
void
build_function_tables(GList *snapshots, GList **snapshot_details, AddDetailsArg *arg) {
	MassifgSnapshot *s = NULL;
	GHashTable *ht = NULL;
	GList *l = snapshots;

	while (l) {
		s = (MassifgSnapshot *)l->data;
		ht = g_hash_table_new(g_str_hash, g_str_equal);
		*snapshot_details = g_list_append(*snapshot_details, ht);
		arg->functions = ht;

		/* Note: we only look at the direct children of the root, because that
		 * is the behaviour that ms_print and massif_grapher has */
		if (s->heap_tree) {
			g_node_children_foreach(s->heap_tree, G_TRAVERSE_ALL,
				add_details_foreach, (gpointer)arg);
		}
		l = l->next;
	}
}

/* Adds all the detailed data series to graph
 * The graph should have been cleared for data series before this is called */
void
massifg_graph_update_detailed(MassifgGraph *graph) {
	/* TODO: these two helper structures should probably be joined into one */
	AddDetailsSerieArg add_dserie_arg;
	AddDetailsArg add_d_arg;

	GHashTable *function_labels = g_hash_table_new(g_str_hash, g_str_equal);
	GList *snapshot_details = NULL;

	/* Build the datastructures neccesary for this view */
	add_d_arg.function_labels = function_labels;
	add_d_arg.functions_sorted = NULL;
	add_d_arg.functions = NULL;
	build_function_tables(graph->data->snapshots, &snapshot_details, &add_d_arg);
	g_hash_table_foreach(function_labels, sort_details_serie_foreach, (gpointer)&add_d_arg);

	/* Create and add the series to the graph */
	add_dserie_arg.graph = graph;
	add_dserie_arg.snapshot_details = snapshot_details;
	g_list_foreach(add_d_arg.functions_sorted, add_details_serie_foreach, (gpointer)&add_dserie_arg);

	/* FIXME: free snapshot_details */
	g_hash_table_destroy(function_labels);
}

void massifg_graph_add_axis_labels(MassifgGraph *graph) {
	GogAxis *axis;
	GOData *label_data;
	GogObject *label;

	gchar *x_axis_str;
	gchar *time_unit = graph->data->time_unit->str;

	/* Get X axis label string */
	if (g_ascii_strcasecmp(time_unit, "ms") == 0) {
		x_axis_str = "Execution time (in milliseconds)";
	}
	else if (g_ascii_strcasecmp(time_unit,"i") == 0) {
		x_axis_str = "Instructions executed";
	}
	else if (g_ascii_strcasecmp(time_unit,"b") == 0) {
		x_axis_str = "Memory allocated (in bytes)";
	}
	else {
		x_axis_str = "Time (unknown unit)";
	}

	/* Add X axis label */
	axis = gog_plot_get_axis(graph->plot, GOG_AXIS_X);
	g_assert(axis);

	label = gog_object_get_child_by_name(GOG_OBJECT(axis), "Label");
	if (!label) {
		label_data = go_data_scalar_str_new(x_axis_str, FALSE);
		label = gog_object_add_by_name(GOG_OBJECT (axis), "Label", NULL);
		gog_dataset_set_dim (GOG_DATASET (label), 0, label_data, NULL);
	}

	/* Add Y axis label */
	axis = gog_plot_get_axis(graph->plot, GOG_AXIS_Y);
	g_assert(axis);

	label = gog_object_get_child_by_name(GOG_OBJECT(axis), "Label");
	if (!label) {
		label_data = go_data_scalar_str_new("Memory usage (in bytes)", FALSE);
		label = gog_object_add_by_name(GOG_OBJECT (axis), "Label", NULL);
		gog_dataset_set_dim (GOG_DATASET (label), 0, label_data, NULL);
	}

}

/* Adjusts the size request to make the widget
 * 1. tall enough to fit the entire legend without having to overflow
 * 2. wide enough to make the legend take up less than 50% of the horizontal size 
 */
void
massifg_graph_adjust_size_request(MassifgGraph *graph) {
	GogChart *chart = go_graph_widget_get_chart(GO_GRAPH_WIDGET(graph->widget));
	GogGraph *gog_graph = go_graph_widget_get_graph(GO_GRAPH_WIDGET(graph->widget));
	GogObject *legend = gog_object_get_child_by_name(GOG_OBJECT(chart), "Legend");

	GtkAllocation allocation;
	gint width, height;

	GogRenderer *rend;
	GogView *view;
	GogView *legend_view;

	if (!legend) {
		return;
	}

	/* */
	gtk_widget_get_allocation(graph->widget, &allocation);
        rend = gog_renderer_new(gog_graph);
        gog_renderer_update(rend, allocation.width, allocation.height*4.);
        g_object_get(G_OBJECT (rend), "view", &view, NULL);
	legend_view = gog_view_find_child_view(view, legend);
	g_return_if_fail(view != NULL);

	/* */
	height = (int)legend_view->residual.h/4.;
	width = (int)legend_view->residual.w;

	g_message("width=%d, height=%d", width, height);
	width = (int)width*2;
	height = (int)height; /* Should be ~812 */

/*	gtk_widget_set_size_request(graph->widget, width, height);*/
/*	gtk_widget_set_size_request(graph->widget, 1000, 1000);*/

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
	massifg_graph_add_axis_labels(graph);
	massifg_graph_adjust_size_request(graph);
}

/* Public functions */

/**
 * massifg_graph_init:
 *
 * Initialize what is neccesary to use the graph.
 * Note: Must be called before the first call to massifg_graph_new() 
 */
void
massifg_graph_init(void) {
	libgoffice_init();
	go_plugins_init(NULL, NULL, NULL, NULL, TRUE, GO_TYPE_PLUGIN_LOADER_MODULE);
}

/**
 * massifg_graph_new:
 * @Returns: A new #MassifgGraph. Free with massifg_graph_free()
 *
 * Create a new #MassifgGraph.
 */
MassifgGraph *
massifg_graph_new(void) {
	GogChart *chart = NULL;

	MassifgGraph *graph = (MassifgGraph *)g_malloc(sizeof(MassifgGraph));

	/* Initialize members */
	graph->data = NULL;
	graph->error = NULL;

	graph->has_legend = FALSE;
	graph->detailed = FALSE;

	/* Create a graph widget, and get the embedded graph and chart */
	graph->widget = go_graph_widget_new(NULL);
	chart = go_graph_widget_get_chart(GO_GRAPH_WIDGET(graph->widget));

	/* Create a plot and add it to the chart */
	graph->plot = (GogPlot *)gog_plot_new_by_name("GogAreaPlot");
	g_object_set (G_OBJECT (graph->plot), "type", "stacked", NULL);

	gog_object_add_by_name(GOG_OBJECT (chart), "Plot", GOG_OBJECT(graph->plot));

	/* Set default settings */
	massifg_graph_set_show_details(graph, FALSE);
	massifg_graph_set_show_legend(graph, TRUE);

	return graph;
}

/**
 * massifg_graph_free:
 * @graph: A #MassifgGraph
 *
 * Free a #MassifgGraph.
 */
void massifg_graph_free(MassifgGraph *graph) {

	/* FIXME: actually free the stuff used by graph */
	g_free(graph);
}

/**
 * massifg_graph_set_data:
 * @graph: A #MassifgGraph
 * @data: #MassifgOutputData to visualize in graph
 *
 * Set the data to visualize.
 */
void 
massifg_graph_set_data(MassifgGraph *graph, MassifgOutputData *data) {
	if (graph->data) {
		massifg_output_data_free(graph->data);
	}
	graph->data = data;
	massifg_graph_update(graph);
}


/**
 * massifg_graph_set_show_details:
 * @graph: A #MassifgGraph
 * @show_details: %TRUE for detailed view, %FALSE for simple view
 *
 * Enable/disable detailed graph view.
 */
void
massifg_graph_set_show_details(MassifgGraph *graph, gboolean show_details) {

	graph->detailed = show_details;
	if (graph->data) {
		massifg_graph_update(graph);
	}
}

/**
 * massifg_graph_set_show_legend:
 * @graph: A #MassifgGraph
 * @show_legend: %TRUE to enable, %FALSE to disable
 *
 * Enable/disable display of legend.
 */
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

/**
 * massifg_graph_get_widget:
 * @graph: A #MassifgGraph
 * @Returns: The #GtkWidget that displays the graph
 *
 * Get the widget that displays the graph.
 */
GtkWidget *
massifg_graph_get_widget(MassifgGraph *graph) {
	return graph->widget;
}

/**
 * massifg_graph_get_data:
 * @graph: A #MassifgGraph
 * @Returns: the #MassifgOutputData the graph is visualizing
 *
 * Get the data the graph visualizes.
 */
MassifgOutputData 
*massifg_graph_get_data(MassifgGraph *graph) {
	return graph->data;
}

/**
 * massifg_graph_render_to_cairo:
 * @graph: A #MassifgGraph to render
 * @cr: #cairo_t context to render to
 * @width: width of the rendered output
 * @height: height of the rendered output
 * @Returns: %TRUE on success, %FALSE on failure
 *
 * Render graph to a cairo context.
 */
gboolean
massifg_graph_render_to_cairo(MassifgGraph *graph, cairo_t *cr,
				const guint width, const guint height) {
	gboolean retval;

	GogGraph *go_graph = go_graph_widget_get_graph(GO_GRAPH_WIDGET(graph->widget));
	GogRenderer *renderer = gog_renderer_new(go_graph);

	retval = gog_renderer_render_to_cairo(renderer, cr, width, height);
	g_object_unref(G_OBJECT(renderer));
	return retval;
}

/** 
 * massifg_graph_render_to_png:
 * @graph: A #MassifgGraph to render
 * @filename: Path to file to render to. Will be created if not existing,
 * or overwritten if existing
 * @width: width of the rendered output
 * @height: height of the rendered output
 * @Returns: %TRUE on success, %FALSE on failure
 *
 * Render the graph to a PNG file.
 */
gboolean
massifg_graph_render_to_png(MassifgGraph *graph, const gchar *filename, const guint width, const guint height) {
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cr = cairo_create(surface);

	if (!massifg_graph_render_to_cairo(graph, cr, width, height)) {
		return FALSE;
	}

	/* TODO: propagate error up to caller */
	if (cairo_surface_write_to_png(surface, filename) != CAIRO_STATUS_SUCCESS) {
		return FALSE;
	}

	return TRUE;
}
