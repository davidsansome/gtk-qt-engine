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

#include "colormapper.h"
#include "utilities.h"

#include <QApplication>
#include <QToolTip>


QPalette ColorMapper::mapGtkToQt(GtkStyle* style, GtkStateType state, bool isButton)
{
	QPalette palette(QApplication::palette());
	
	mapGtkToQt(style->fg[GTK_STATE_NORMAL],   &palette, QPalette::Active, QPalette::WindowText);
	mapGtkToQt(style->base[GTK_STATE_NORMAL], &palette, QPalette::Active, QPalette::Base);
	mapGtkToQt(style->fg[GTK_STATE_SELECTED], &palette, QPalette::Active, QPalette::HighlightedText);
	mapGtkToQt(style->bg[GTK_STATE_SELECTED], &palette, QPalette::Active, QPalette::Highlight);
	
	mapGtkToQt(style->fg[GTK_STATE_INSENSITIVE],   &palette, QPalette::Disabled, QPalette::WindowText);
	mapGtkToQt(style->base[GTK_STATE_INSENSITIVE], &palette, QPalette::Disabled, QPalette::Base);
	mapGtkToQt(style->fg[GTK_STATE_INSENSITIVE],   &palette, QPalette::Disabled, QPalette::HighlightedText);
	mapGtkToQt(style->bg[GTK_STATE_INSENSITIVE],   &palette, QPalette::Disabled, QPalette::Highlight);
	
	if (isButton)
	{
		mapGtkToQt(style->bg[state], &palette, QPalette::Active, QPalette::Button);
		mapGtkToQt(style->fg[state], &palette, QPalette::Active, QPalette::ButtonText);
		mapGtkToQt(style->bg[GTK_STATE_INSENSITIVE], &palette, QPalette::Disabled, QPalette::Button);
		mapGtkToQt(style->fg[GTK_STATE_INSENSITIVE], &palette, QPalette::Disabled, QPalette::ButtonText);
	}
	else
	{
		mapGtkToQt(style->bg[GTK_STATE_NORMAL], &palette, QPalette::Active, QPalette::Window);
		mapGtkToQt(style->fg[GTK_STATE_NORMAL], &palette, QPalette::Active, QPalette::Text);
		mapGtkToQt(style->bg[GTK_STATE_INSENSITIVE], &palette, QPalette::Disabled, QPalette::Window);
		mapGtkToQt(style->fg[GTK_STATE_INSENSITIVE], &palette, QPalette::Disabled, QPalette::Text);
	}
	
	return palette;
}

void ColorMapper::mapGtkToQt(const GdkColor& color, QPalette* palette, QPalette::ColorGroup group, QPalette::ColorRole role)
{
	QColor qcolor(GtkQtUtilities::convertColor(color));
	
	palette->setColor(group, role, qcolor);
	
	// The inactive group should be set the same as the active group
	if (group == QPalette::Active)
		mapGtkToQt(color, palette, QPalette::Inactive, role);
}
