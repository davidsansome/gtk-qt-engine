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

#ifndef SEARCHPATHS_H
#define SEARCHPATHS_H

#include <QDialog>
#include <QStringListModel>

#include "ui_searchpaths.h"

class QSettings;

class SearchPaths : public QDialog
{
	Q_OBJECT
public:
	SearchPaths(QWidget* parent = 0);
	
	QStringList paths() const { return m_paths->stringList(); }

public Q_SLOTS:
	int exec();

private Q_SLOTS:
	void textChanged(const QString& text);
	void add();
	void remove();
	void itemClicked(const QModelIndex&);

private:
	Ui_SearchPaths m_ui;
	
	QStringListModel* m_paths;
	QSettings* m_settings;
};

#endif

