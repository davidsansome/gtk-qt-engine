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

#ifndef GTKRCFILE_H
#define GTKRCFILE_H

#include <QString>
#include <QFont>

class GtkRcFile
{
public:
	GtkRcFile(const QString& fileName);
	
	void load();
	void save();
	
	QString fileName() const { return m_fileName; }
	QString themeName() const { return m_themeName; }
	QString themePath() const { return m_themePath; }
	QFont font() const { return m_font; }
	
	void setTheme(const QString& path);
	void setFont(const QFont& font) { m_font = font; }
	void setFont(const QString& family, int pointSize, bool bold, bool italic);
	
private:
	void parseFont(QString fontString);
	
	QString m_fileName;
	QString m_themeName;
	QString m_themePath;
	QFont m_font;
	
	static QRegExp k_themeNameRe;
};

#endif

