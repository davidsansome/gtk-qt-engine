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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <QString>
#include <QColor>

#include <gtk/gtk.h>

#define GTK_QT_DEBUG(expression) \
	if (Engine::instance()->isDebug()) \
		qDebug() << expression;

#define GTK_QT_DEBUG_FUNC \
	GTK_QT_DEBUG(__PRETTY_FUNCTION__)

namespace GtkQtUtilities
{
	enum StyleType
	{
		Class,
		WidgetClass,
		Widget
	};
	
	QString getCommandLine();
	
	QString colorString(const QColor& color);
	QColor convertColor(const GdkColor& color);
	GdkColor convertColor(const QColor& color, GtkStyle* style);
	
	void parseRcString(const QString& string);
	void parseRcString(const QString& defs, const QString& pattern, StyleType type);
	QString generateRcString(const QString& defs, const QString& pattern, StyleType type);
};

#endif
