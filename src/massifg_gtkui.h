/*
 *  MassifG - massifg_gtkui.h
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

#ifndef __MASSIFG_GTKUI_H__
#define __MASSIFG_GTKUI_H__

#include "massifg_application.h"

gint massifg_gtkui_init(MassifgApplication *app);
void massifg_gtkui_start(MassifgApplication *app);

void massifg_gtkui_errormsg(MassifgApplication *app, const gchar *msg_format, ...);

#endif /* __MASSIFG_GTKUI_H__ */
