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

/* */
static void
draw_snapshot_point(gpointer data, gpointer user_data) {
	MassifgSnapshot *snapshot = (MassifgSnapshot *)data;
	cairo_t *cr = (cairo_t *)user_data;

	double x = (double)snapshot->time;
	double y = (double)snapshot->mem_heap_B; /* FIXME */
	double x_dev = x;
	double y_dev = y;

	cairo_line_to(cr, x, y);

	cairo_user_to_device(cr, &x_dev, &y_dev);
	g_debug("Drawing snapshot point: x=%e, y=%e <=> x_dev=%f, y_dev=%f",
			x, y, x_dev, y_dev);

}

/* TODO: make generic, so we can draw the different series */
/* Draw a single data series */
static void
draw_snapshot_serie(cairo_t *context, MassifgOutputData *data) {
	GList *last_elem = g_list_last(data->snapshots);
	MassifgSnapshot *last_snapshot = (MassifgSnapshot *)last_elem->data;

	g_list_foreach(data->snapshots, draw_snapshot_point, context);
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
	draw_snapshot_serie(context, data);
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

