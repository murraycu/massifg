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

#include <cairo.h>
#include <pango/pangocairo.h>
#include <glib.h>

#include "massifg_parser.h"
#include "massifg_graph.h"

/* Data structures */

/* Used as an argument to get_max_values () foreach function 
 * to prevent having to iterate trough the list twice, 
 * once for x values and once for y values */
typedef struct {
	gint x;
	gint y;
} MaxValues;

/* Used as an argument to the draw_snapshot_point () foreach function */
typedef struct {
	cairo_t *cr;
	cairo_matrix_t *aux_matrix;
	MassifgGraphSeries series;
} MassifgGraphDrawSeries;

/* Private functions */
/* Free with g_free () */
RGBAColor *
massifg_rgbacolor_new(double r, double g, double b, double a) {
	RGBAColor *color = (RGBAColor *)g_malloc(sizeof(RGBAColor));
	color->r =r;
	color->g =g;
	color->b =b;
	color->a =a;
	return color;
}

/* Free with massifg_graphformat_free () */
MassifgGraphFormat *
massifg_graphformat_new(void) {
	MassifgGraphFormat *graph_format = (MassifgGraphFormat *)g_malloc(sizeof(MassifgGraphFormat));

	/* Initialize with default values */
	graph_format->data_series_color[GRAPH_SERIES_STACKS] = massifg_rgbacolor_new(0.0, 0.0, 1.0, 1.0);
	graph_format->data_series_color[GRAPH_SERIES_HEAP_EXTRA] = massifg_rgbacolor_new(0.0, 1.0, 0.0, 1.0);
	graph_format->data_series_color[GRAPH_SERIES_HEAP] = massifg_rgbacolor_new(1.0, 0.0, 0.0, 1.0);

	return graph_format;
}

void
massifg_graphformat_free(MassifgGraphFormat *graph_format) {
	int i;
	for (i=0; i<GRAPH_SERIES_LAST; i++) {
		g_free(graph_format->data_series_color[i]);
	}
	g_free(graph_format);
}

/* Get the maximum x and y values, and put them in the MaxValues struct
 * passed in via user_data
 * Meant to be used as a parameter to a g_(s)list_foreach call */
static void
get_max_values(gpointer data, gpointer user_data) {
	MassifgSnapshot *s = (MassifgSnapshot *)data;
	MaxValues *max = (MaxValues *)user_data;

	/* y value*/
	gint y = s->mem_heap_B + s->mem_heap_extra_B + s->mem_stacks_B;
	if (y > max->y)
		max->y = y;
	/* x value */
	if (s->time > max->x)
		max->x = s->time;
}

/* Draw a single data point of a data series 
 * Meant to be used as a parameter to a g_(s)list_foreach call */
static void
draw_snapshot_point(gpointer data, gpointer user_data) {
	MassifgSnapshot *snapshot = (MassifgSnapshot *)data;
	MassifgGraphDrawSeries *draw_series = (MassifgGraphDrawSeries *)user_data;
	double x, x_dev;
	double y, y_dev;

	/* Get the x, y values for the point */
	x = (double)snapshot->time;
	switch (draw_series->series) {
	case GRAPH_SERIES_HEAP:
		y = (double)snapshot->mem_heap_B;
		break;
	case GRAPH_SERIES_HEAP_EXTRA:
		y = (double)snapshot->mem_heap_B +
		    (double)snapshot->mem_heap_extra_B;
		break;
	case GRAPH_SERIES_STACKS:
		y = (double)snapshot->mem_heap_B +
		    (double)snapshot->mem_heap_extra_B +
		    (double)snapshot->mem_stacks_B;
		break;
	}

	/* Draw the data point */
	cairo_matrix_transform_point(draw_series->aux_matrix, &x, &y);
	cairo_line_to(draw_series->cr, x, y);

	/* Print debugging information */ 
	x_dev = x;
	y_dev = y;
	cairo_user_to_device(draw_series->cr, &x_dev, &y_dev);
	g_debug("Drawing snapshot point: x=%e, y=%e <=> x_dev=%f, y_dev=%f",
			x, y, x_dev, y_dev);

}

/* Draw a single data series */
static void
draw_snapshot_serie(MassifgGraph *graph, MassifgGraphSeries serie) {
	GList *last_elem = g_list_last(graph->data->snapshots);
	MassifgSnapshot *last_snapshot = (MassifgSnapshot *)last_elem->data;
	MassifgGraphDrawSeries ds;
	ds.cr = graph->cr;
	ds.series = serie;
	ds.aux_matrix = graph->aux_matrix;
	RGBAColor *serie_color = graph->format->data_series_color[serie];

	double x = (double)last_snapshot->time;
	double y = 0.0;

	/* Create the path for the serie */
	g_list_foreach(graph->data->snapshots, draw_snapshot_point, &ds);
	cairo_matrix_transform_point(graph->aux_matrix, &x, &y);
	cairo_line_to(graph->cr, x, y);
	cairo_close_path(graph->cr);

	cairo_set_source_rgba(graph->cr, serie_color->r, serie_color->g, serie_color->b, serie_color->a);
	cairo_fill(graph->cr);
}

/* Draw all the data series */
static void
draw_snapshot_series(MassifgGraph *graph, int width, int height) {
	MaxValues max;
	max.y = -1;
	max.x = -1;
	cairo_matrix_t tmp_matrix;

	g_debug("Drawing snapshot series graph");

	/* Scale to match the boundries of the image */
	g_list_foreach(graph->data->snapshots, get_max_values, &max);

	tmp_matrix = *graph->aux_matrix;
	cairo_matrix_scale(graph->aux_matrix, width/(double)max.x, height/(double)max.y);
	g_debug("Scaling by factor: x=%e, y=%e", width/(double)max.x, height/(double)max.y);

	/* Draw the path */
	/* NOTE: order here matters, because the series that are drawn later
	 * are composited over the previous ones. Thus, for correct operation,
	 * the graph with the largest y values must be drawn first.
	 * The code deciding which series that is can found in draw_snapshot_point ()
	 */
	draw_snapshot_serie(graph, GRAPH_SERIES_STACKS);
	draw_snapshot_serie(graph, GRAPH_SERIES_HEAP_EXTRA);
	draw_snapshot_serie(graph, GRAPH_SERIES_HEAP);

	*graph->aux_matrix = tmp_matrix;
}

/* Draw text at (human convention) coordinates x, y 
 * This is a generic function, designed for being used by the more concrete 
 * elements of the graph */
static void
draw_text(MassifgGraph *graph, gchar *text, double x, double y) {
	PangoLayout *layout;
	PangoFontDescription *desc;

	layout = pango_cairo_create_layout(graph->cr);

	/* Set cairo options */
	cairo_matrix_transform_point(graph->aux_matrix, &x, &y);
	cairo_translate(graph->cr, x, y);
	cairo_set_source_rgba(graph->cr, 0.0, 0.0, 0.0, 1.0); /* TODO: read from format */

	/* Set Pango options */
	pango_layout_set_text(layout, text, -1); 
	desc = pango_font_description_from_string("Sans 10"); /* TODO: read from format */
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	/* Draw the text */
	pango_cairo_update_layout(graph->cr, layout);
	/* Valgrind memcheck thinks this is leaking memory, TODO: investigate */
	pango_cairo_show_layout(graph->cr, layout);

	/* Cleanup */
	cairo_translate(graph->cr, -x, -y);
	g_object_unref(layout);

}

static void
draw_legend_entry(MassifgGraph *graph, MassifgGraphSeries serie) {
	RGBAColor *color = NULL;
	gchar *text = NULL;
	/* FIXME: these should be read from the format */
	double rect_w = 12.;
	double rect_h = rect_w;
	double base_x = 50.;
	double base_y = 150.;

	double entry_x = base_x;
	double entry_y = base_y + rect_h*serie;
	/* FIXME: these should be stored in the format */
	gchar *legend_texts[] = {"Heap", "Heap Extra", "Stacks"};

	color = graph->format->data_series_color[serie];
	text = legend_texts[serie];

	draw_text(graph, text, entry_x+rect_w, entry_y);

	cairo_set_source_rgba(graph->cr, color->r, color->g, color->b, color->a);
	cairo_matrix_transform_point(graph->aux_matrix, &entry_x, &entry_y);
	cairo_rectangle(graph->cr, entry_x, entry_y, rect_w, rect_h);
	cairo_fill(graph->cr);

}

/* Legend consists of a box showing the meaning of each data
 * series in the graph. */
static void
draw_legend(MassifgGraph *graph) {
	/* TODO: add a background, so the legend is visible even if it on top of data */

	draw_legend_entry(graph, GRAPH_SERIES_HEAP);
	draw_legend_entry(graph, GRAPH_SERIES_HEAP_EXTRA);
	draw_legend_entry(graph, GRAPH_SERIES_STACKS);

}

/* Draw the axes. This does not include tics for values */
static void
draw_axes(MassifgGraph *graph, int width, int height) {
	double origo_x = 0.0;
	double origo_y = 0.0;

	cairo_set_source_rgba(graph->cr, 0.0, 0.0, 0.0, 1.0); /* TODO: read from format */
	/* TODO: add a nice arrow at the end, or similar */
	/* X-axis line */
	cairo_new_path(graph->cr);
	cairo_matrix_transform_point(graph->aux_matrix, &origo_x, &origo_y);
	cairo_move_to(graph->cr, origo_x, origo_y);
	cairo_line_to(graph->cr, width, origo_y);
	cairo_stroke(graph->cr);

	/* Y-axis line */
	cairo_new_path(graph->cr);
	cairo_matrix_transform_point(graph->aux_matrix, &origo_x, &origo_y);
	cairo_move_to(graph->cr, origo_x, origo_y);
	cairo_line_to(graph->cr, origo_x, height);
	cairo_stroke(graph->cr);

}

/* Public functions */
MassifgGraph *
massifg_graph_new(void) {

	MassifgGraph *graph = (MassifgGraph *)g_malloc(sizeof(MassifgGraph));
	graph->aux_matrix = (cairo_matrix_t *)g_malloc(sizeof(cairo_matrix_t));
	graph->format = massifg_graphformat_new();

	graph->data = NULL;
	graph->cr = NULL;
	graph->context_width = 0.0;
	graph->context_height = 0.0;

	return graph;
}

void massifg_graph_free(MassifgGraph *graph) {
	massifg_graphformat_free(graph->format);
	g_free(graph->aux_matrix);
	g_free(graph);

}

/* FIXME: these should be part of the format data structure, so that it can be set/adjusted at runtime */
#define MASSIFG_GRAPH_DATA_SERIES_OFFSET_X 2
#define MASSIFG_GRAPH_DATA_SERIES_OFFSET_Y 2

#define MASSIFG_GRAPH_DATA_SERIES_PADDING_X 5
#define MASSIFG_GRAPH_DATA_SERIES_PADDING_Y 5

/* Redraw the graph. When this is called, it is important that the graph datastructure
 * members are up to date, especially the cairo context, the context_width and _height, and the data  */
void massifg_graph_redraw(MassifgGraph *graph) {
	/* Update the transformation matrix for converting between
	 * traditional human coordinate system (origo bottom left, positive y-values up)
	 * and cairo coordinate system (origo top left, positive y-values down) */
	cairo_get_matrix(graph->cr, graph->aux_matrix);
	cairo_matrix_scale(graph->aux_matrix, 1.0, -1.0);
	cairo_matrix_translate(graph->aux_matrix, 0, -graph->context_height);


	/* Draw each data series; heap, heap_extra, stack */
	/* FIXME: the position and size of the data series element should be handled that function,
	 * or a generic function that can do this for any element */
	cairo_translate(graph->cr, MASSIFG_GRAPH_DATA_SERIES_OFFSET_X, MASSIFG_GRAPH_DATA_SERIES_OFFSET_Y);
	draw_snapshot_series(graph,
			graph->context_width-(MASSIFG_GRAPH_DATA_SERIES_OFFSET_X+MASSIFG_GRAPH_DATA_SERIES_PADDING_X),
			graph->context_height-(MASSIFG_GRAPH_DATA_SERIES_OFFSET_Y+MASSIFG_GRAPH_DATA_SERIES_PADDING_Y));
	cairo_translate(graph->cr, -MASSIFG_GRAPH_DATA_SERIES_OFFSET_X, -MASSIFG_GRAPH_DATA_SERIES_OFFSET_Y);

	draw_axes(graph, graph->context_width, graph->context_height);

	/* Draw the various labels */
	draw_legend(graph);
}

/* Utility function that updates the graph data members, and then redraws it */
void 
massifg_graph_update(MassifgGraph *graph, 
			cairo_t *context, MassifgOutputData *data, 
			int width, int height) {

	/* Update graph members */
	graph->data = data;
	graph->cr = context;
	graph->context_width = width;
	graph->context_height = height;

	/* Do the actual drawing */
	massifg_graph_redraw(graph);

}

