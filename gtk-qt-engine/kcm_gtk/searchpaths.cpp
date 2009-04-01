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

#include "searchpaths.h"

#include <QSettings>
#include <QDir>

SearchPaths::SearchPaths(QWidget* parent)
	: QDialog(parent)
{
	m_ui.setupUi(this);
	
	m_settings = new QSettings("gtk-qt-engine", "kcmgtk", this);
	
	QStringList gtkSearchPathsDefault;
	gtkSearchPathsDefault.append("/usr");
	gtkSearchPathsDefault.append("/usr/local");
	gtkSearchPathsDefault.append("/opt/gnome");
	gtkSearchPathsDefault.append(QDir::homePath() + "/.local");
	m_paths = new QStringListModel(m_settings->value("GtkSearchPaths", gtkSearchPathsDefault).toStringList(), this);
	
	m_ui.searchPaths->setModel(m_paths);
	
	connect(m_ui.pathBox, SIGNAL(textEdited(const QString&)), SLOT(textChanged(const QString&)));
	connect(m_ui.pathBox, SIGNAL(returnPressed()), SLOT(add()));
	connect(m_ui.searchPaths, SIGNAL(clicked(const QModelIndex&)), SLOT(itemClicked(const QModelIndex&)));
	connect(m_ui.addButton, SIGNAL(clicked()), SLOT(add()));
	connect(m_ui.removeButton, SIGNAL(clicked()), SLOT(remove()));
}

int SearchPaths::exec()
{
	QStringList rollback(m_paths->stringList());
	int ret = QDialog::exec();
	if (ret == QDialog::Rejected)
		m_paths->setStringList(rollback);
	else
		m_settings->setValue("GtkSearchPaths", m_paths->stringList());
	
	return ret;
}

void SearchPaths::textChanged(const QString& text)
{
	m_ui.addButton->setEnabled(!text.isEmpty());
}

void SearchPaths::add()
{
	m_paths->setStringList(m_paths->stringList() << m_ui.pathBox->text());
	m_ui.pathBox->clear();
}

void SearchPaths::remove()
{
	m_paths->removeRow(m_ui.searchPaths->currentIndex().row());
	
	m_ui.removeButton->setEnabled(m_paths->rowCount() > 0);
}

void SearchPaths::itemClicked(const QModelIndex&)
{
	m_ui.removeButton->setEnabled(m_ui.searchPaths->currentIndex().isValid());
}

