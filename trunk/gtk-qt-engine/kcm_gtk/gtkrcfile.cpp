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

#include "gtkrcfile.h"

#include <QFile>
#include <QIODevice>
#include <QRegExp>
#include <QTextStream>
#include <QStringList>
#include <QtDebug>

QRegExp GtkRcFile::k_themeNameRe("([^/]+)/gtk-2.0/gtkrc$");

GtkRcFile::GtkRcFile(const QString& fileName)
	: m_fileName(fileName)
{
}

void GtkRcFile::load()
{
	QFile file(m_fileName);
	file.open(QIODevice::ReadOnly);
	QTextStream stream(&file);
	
	QRegExp includeRe("include\\s*\"([^\"]*)\"");
	QRegExp fontRe("gtk-font-name\\s*=\\s*\"([^\"]*)\"");
	
	
	QStringList includes;
	
	while (1)
	{
		QString line = stream.readLine();
		if (line.isNull())
			break;
		if (line.startsWith("#"))
			continue;
		
		line = line.trimmed();
		
		if (line.startsWith("include"))
		{
			if (includeRe.indexIn(line) == -1)
				continue;
			QString themePath = includeRe.cap(1);
			if (themePath.startsWith("/etc"))
				continue;
			
			setTheme(themePath);
		}
		if (line.startsWith("gtk-font-name"))
		{
			if (fontRe.indexIn(line) == -1)
				continue;
			// Assume there will only be one font line
			parseFont(fontRe.cap(1));
		}
	}
	
	file.close();
}

void GtkRcFile::parseFont(QString fontString)
{
	QFont font;
	while (true)
	{
		int lastSpacePos = fontString.lastIndexOf(' ');
		if (lastSpacePos == -1)
			break;

		QString lastWord = fontString.right(fontString.length() - lastSpacePos).trimmed();

		if (lastWord.toLower() == "bold")
			font.setBold(true);
		else if (lastWord.toLower() == "italic")
			font.setItalic(true);
		else
		{
			bool ok;
			int fontSize = lastWord.toInt(&ok);
			if (!ok)
				break;
			font.setPointSize(fontSize);
		}

		fontString = fontString.left(lastSpacePos);
	}
	font.setFamily(fontString);
	setFont(font);
}

void GtkRcFile::save()
{
	QFile file(m_fileName);
	file.open(QIODevice::WriteOnly);
	QTextStream stream(&file);
	
	QString fontName = m_font.family() + " " +
		QString(m_font.bold() ? "Bold " : "") +
		QString(m_font.italic() ? "Italic " : "") +
		QString::number(m_font.pointSize());
	
	stream << "# This file was written by KDE\n";
	stream << "# You can edit it in the KDE control center, under \"GTK Styles and Fonts\"\n";
	stream << "\n";
	stream << "include \"" << m_themePath << "\"\n";
	if (QFile::exists("/etc/gtk-2.0/gtkrc"))
		stream << "include \"/etc/gtk-2.0/gtkrc\"\n";
	stream << "\n";
	stream << "style \"user-font\"\n";
	stream << "{\n";
	stream << "\tfont_name=\"" << m_font.family() << "\"\n";
	stream << "}\n";
	stream << "widget_class \"*\" style \"user-font\"\n";
	stream << "\n";
	stream << "gtk-theme-name=\"" << m_themeName << "\"\n";
	stream << "gtk-font-name=\"" << fontName << "\"\n";
}

void GtkRcFile::setFont(const QString& family, int pointSize, bool bold, bool italic)
{
	QFont font(family, pointSize);
	font.setBold(bold);
	font.setItalic(italic);
	
	setFont(font);
}

void GtkRcFile::setTheme(const QString& path)
{
	if (k_themeNameRe.indexIn(path) == -1)
		return;
	
	m_themePath = path;
	m_themeName = k_themeNameRe.cap(1);
}

