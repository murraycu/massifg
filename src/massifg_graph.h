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

#include <cairo.h>

#include "massifg_parser.h"

/* Data structures */
/* Enum that represents the different possible data series */
typedef enum {
	GRAPH_SERIES_HEAP,
	GRAPH_SERIES_HEAP_EXTRA,
	GRAPH_SERIES_STACKS,
	GRAPH_SERIES_LAST /* NOTE: only used to calculate the number of elements in the enum */
} MassifgGraphSeries;

/* Represent a RGB color with alpha channel */
typedef struct {
	double r, g, b, a;
} RGBAColor;

/* Holds the various variables that decides how the graph should look */
typedef struct {
	/* The colors of the different data series. Array indexed by the MassifgGraphSeries enum
 	 * NOTE: this might not be the best datastructure for this */
	RGBAColor *data_series_color[GRAPH_SERIES_LAST];
	RGBAColor *text_color;
	GString *text_font;
	RGBAColor *axes_color;
	double legend_pos_x;
	double legend_pos_y;
	double legend_entry_box_size;
	double data_series_pos_x;
	double data_series_pos_y;
	double data_series_padding_left;
	double data_series_padding_top;
} MassifgGraphFormat;

/* Holds the shared state used in the graph */
typedef struct {
	cairo_t *cr;
	cairo_matrix_t *aux_matrix;
	double context_width;
	double context_height;
	MassifgOutputData *data;
	MassifgGraphFormat *format;
} MassifgGraph;

/* Public functions */
MassifgGraph *massifg_graph_new(void);
void massifg_graph_free(MassifgGraph *graph);

void massifg_graph_redraw(MassifgGraph *graph);
void massifg_graph_update(MassifgGraph *graph,
		cairo_t *context, MassifgOutputData *data,
		int width, int height);

