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

#include "qt_rc_style.h"
#include "qt_style.h"
#include "wrapper.h"



static void      qtengine_rc_style_init         (QtEngineRcStyle      *style);
static void      qtengine_rc_style_class_init   (QtEngineRcStyleClass *klass);
static void      qtengine_rc_style_finalize     (GObject             *object);
static guint     qtengine_rc_style_parse        (GtkRcStyle          *rc_style,
					       GtkSettings          *settings,
					       GScanner             *scanner);
static void      qtengine_rc_style_merge       (GtkRcStyle           *dest,
					       GtkRcStyle           *src);

static GtkStyle *qtengine_rc_style_create_style (GtkRcStyle          *rc_style);

static GtkRcStyleClass *parent_class;

GType qtengine_type_rc_style = 0;


void qtengine_rc_style_register_type (GTypeModule *module)
{
	static const GTypeInfo object_info =
	{
		sizeof (QtEngineRcStyleClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) qtengine_rc_style_class_init,
		NULL,           /* class_finalize */
		NULL,           /* class_data */
		sizeof (QtEngineRcStyle),
		0,              /* n_preallocs */
		(GInstanceInitFunc) qtengine_rc_style_init,
	};
	
	qtengine_type_rc_style = g_type_module_register_type (module, GTK_TYPE_RC_STYLE, "QtEngineRcStyle", &object_info, 0);
}

static void qtengine_rc_style_init (QtEngineRcStyle *style)
{
}

static void qtengine_rc_style_class_init (QtEngineRcStyleClass *klass)
{
	GtkRcStyleClass *rc_style_class = GTK_RC_STYLE_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	
	rc_style_class->parse = qtengine_rc_style_parse;
	rc_style_class->merge = qtengine_rc_style_merge;
	rc_style_class->create_style = qtengine_rc_style_create_style;
}




static guint
qtengine_rc_style_parse (GtkRcStyle *rc_style, GtkSettings *settings, GScanner *scanner)
{
	static GQuark       scope_id = 0;
	guint               old_scope;
	guint               token;
	
	/* The rest of this keeps GTK happy - therefore I don't care what it does */
	if (!scope_id)
		scope_id = g_quark_from_string("theme_engine");
	
	old_scope = g_scanner_set_scope(scanner, scope_id);
	
	token = g_scanner_get_next_token(scanner);
	while (token != G_TOKEN_RIGHT_CURLY)
	{
		token = g_scanner_get_next_token(scanner);
	}
	
	g_scanner_set_scope(scanner, old_scope);
	
	return G_TOKEN_NONE;
}

static void
qtengine_rc_style_merge (GtkRcStyle * dest,
			GtkRcStyle * src)
{
	parent_class->merge(dest, src);
}

/* Create an empty style suitable to this RC style
 */
static GtkStyle *
qtengine_rc_style_create_style (GtkRcStyle *rc_style)
{
	void *ptr = GTK_STYLE (g_object_new (QTENGINE_TYPE_STYLE, NULL));
	return (GtkStyle *)ptr;
}
