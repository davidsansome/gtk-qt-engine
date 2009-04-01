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

#ifndef _KCMTEST_H_
#define _KCMTEST_H_

#include <QStringList>
#include <kcmodule.h>

#include "ui_kcmgtkwidget.h"

class KConfig;
class GtkRcFile;
class SearchPaths;

class KcmGtk: public KCModule
{
	Q_OBJECT
public:
	KcmGtk(QWidget* parent = 0, const QVariantList& = QVariantList());
	~KcmGtk();
	
	void load();
	void save();
	void defaults();

private Q_SLOTS:
	void fontChangeClicked();
	void fontKdeClicked();
	void styleChanged();
	void styleKdeClicked(bool checked);
	void firefoxFixClicked();
	void getInstalledThemes();

private:
	void updateFontPreview();
	
	Ui_KcmGtkWidget m_ui;
	
	GtkRcFile* m_gtkRc;
	QMap<QString, QString> m_themes;
	SearchPaths* m_searchPaths;
	
	static const QString k_gtkRcFileName;
	static const QString k_envFileName;
	static const QString k_qtThemeName;
};

#endif
