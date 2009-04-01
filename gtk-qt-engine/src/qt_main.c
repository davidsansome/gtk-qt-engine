/***************************************************************************
 *   Copyright (C) 2008 by David Sansome                                   *
 *   me@davidsansome.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <gmodule.h>
#include <stdio.h>
#include "qt_rc_style.h"
#include "qt_style.h"

#include "wrapper.h"

G_MODULE_EXPORT void theme_init (GTypeModule *module);
G_MODULE_EXPORT void theme_exit (void);
G_MODULE_EXPORT GtkRcStyle * theme_create_rc_style (void);

G_MODULE_EXPORT void theme_init (GTypeModule *module)
{
	gtkQtInit();
	qtengine_rc_style_register_type (module);
	qtengine_style_register_type (module);
}

G_MODULE_EXPORT void theme_exit (void)
{
	gtkQtDestroy();
}

G_MODULE_EXPORT GtkRcStyle * theme_create_rc_style (void)
{
	void *ptr = GTK_RC_STYLE (g_object_new (QTENGINE_TYPE_RC_STYLE, NULL));  
	return (GtkRcStyle *)ptr;
}

