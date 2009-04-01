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

#ifndef RCPROPERTIES_H
#define RCPROPERTIES_H

#include <QPalette>
#include <QStringList>

typedef QPair<QString, QVariant> PathAndValue;

class KIconTheme;

class RcProperties
{
public:
	static void setRcProperties();
	
	static int scrollBarButtonSize() { return s_scrollBarButtonSize; }
	static int scrollBarButtonCount() { return s_scrollBarButtonCount; }

private:
	RcProperties() {}
	
	static void setWidgetProperties();
	static void setColorProperties();
	static void setIconProperties();
	
	static void findScrollBarButtons();
	
	static PathAndValue kdeConfigValue(const QString& file, const QString& key, const QVariant& def, bool searchAllFiles);
	static void initKdeSettings();
	static void traverseIconThemeDir(const QString& themeName);
	static QString doIconMapping(const QString& stockName, const QString& path);
	
	static void mapColor(const QString& name, QPalette::ColorGroup group, QPalette::ColorRole role);
	static QColor convertColor(const QVariant& variant);
	static QString gtkPaletteString(const QPalette& pal, const QString& bgfg, QPalette::ColorRole role = QPalette::Window);
	
	static bool s_scrollBarHasBack1;
	static bool s_scrollBarHasForward1;
	static bool s_scrollBarHasBack2;
	static bool s_scrollBarHasForward2;
	static int s_scrollBarButtonSize;
	static int s_scrollBarButtonCount;
	
	static QStringList s_kdeSearchPaths;
	static QStringList s_iconThemeDirs;
};

#endif
