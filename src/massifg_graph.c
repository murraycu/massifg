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
#include <glib.h>

#include "massifg_parser.h"

#define SURF_SIZE_H 400
#define SURF_SIZE_W 800

/* Data structures */

/* Used as an argument to get_max_values () foreach function 
 * to prevent having to iterate trough the list twice, 
 * once for x values and once for y values */
typedef struct {
	gint x;
	gint y;
} MaxValues;

/* */
typedef enum {
	GRAPH_SERIES_HEAP,
	GRAPH_SERIES_HEAP_EXTRA,
	GRAPH_SERIES_STACKS,
} MassifgGraphSeries;

/* Used as an argument to the draw_snapshot_point () foreach function */
typedef struct {
	cairo_t *cr;
	MassifgGraphSeries series;
} MassifgGraphDrawSeries;

/* Private functions */
/* */
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

/* Draw a single data point of a data series */
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
draw_snapshot_serie(cairo_t *context, MassifgOutputData *data, MassifgGraphSeries serie) {
	GList *last_elem = g_list_last(data->snapshots);
	MassifgSnapshot *last_snapshot = (MassifgSnapshot *)last_elem->data;
	MassifgGraphDrawSeries ds;
	ds.cr = context;
	ds.series = serie;

	g_list_foreach(data->snapshots, draw_snapshot_point, &ds);
	cairo_line_to(context, (double)last_snapshot->time, 0.);
	cairo_close_path(context);

	cairo_fill(context);
}

/* Draw all the data series */
static void
draw_snapshot_series(cairo_t *context, MassifgOutputData *data) {
	MaxValues max;
	max.y = -1;
	max.x = -1;

	g_debug("Drawing snapshot series graph");

	/* Transform from positive y-axis downwards to positive y-axis upwards */
	g_debug("Transforming to traditional human y-axis conversion.");
	cairo_scale(context, 1.0, -1.0);
	cairo_translate(context, 0, -SURF_SIZE_H);

	/* Scale to match the boundries of the image */
	g_list_foreach(data->snapshots, get_max_values, &max);
	cairo_scale(context, SURF_SIZE_W/(double)max.x, SURF_SIZE_H/(double)max.y);
	g_debug("Scaling by factor: x=%e, y=%e", SURF_SIZE_W/(double)max.x, SURF_SIZE_H/(double)max.y);

	/* Draw the path */
	/* NOTE: order here matters, because the series that are drawn later
	 * are composited over the previous ones. Thus, for correct operation,
	 * the graph with the largest y values must be drawn first.
	 * The code deciding which series that is can found in draw_snapshot_point ()
	 */
	cairo_set_source_rgba(context, 0.0, 0.0, 1.0, 1.0);
	draw_snapshot_serie(context, data, GRAPH_SERIES_STACKS);
	cairo_set_source_rgba(context, 0.0, 1.0, 0.0, 1.0);
	draw_snapshot_serie(context, data, GRAPH_SERIES_HEAP_EXTRA);
	cairo_set_source_rgba(context, 1.0, 0.0, 0.0, 1.0);
	draw_snapshot_serie(context, data, GRAPH_SERIES_HEAP);
}


/* Public functions */
/* TODO: use another way to return the graph than writing a png to disk */
/* */
void massifg_draw_graph(MassifgOutputData *data) {
	cairo_surface_t *surface = NULL;
	cairo_t *context = NULL;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SURF_SIZE_W, SURF_SIZE_H);
	context = cairo_create(surface);

	/* Set up cairo transform. Need to find the max values for the data for this */

	/* Draw each data series; heap, heap_extra, stack */
	draw_snapshot_series(context, data);

	/* Output */
	cairo_surface_write_to_png(surface, "massifg-graph-test.png");

	/* Cleanup */
	cairo_surface_destroy(surface);
	cairo_destroy(context);

}

