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

#include "firefoxfix.h"

#include <kmessagebox.h>
#include <klocale.h>

#include <QDir>
#include <QSettings>
#include <QFile>
#include <QDebug>
#include <QScrollBar>
#include <QStyle>
#include <QApplication>
#include <QStyleOptionSlider>

bool FirefoxFix::s_scrollBarHasBack1;
bool FirefoxFix::s_scrollBarHasForward1;
bool FirefoxFix::s_scrollBarHasBack2;
bool FirefoxFix::s_scrollBarHasForward2;

void FirefoxFix::go()
{
	QStringList profiles;
	profiles << getProfiles(QDir::homePath() + "/.mozilla/firefox/");
	profiles << getProfiles(QDir::homePath() + "/.thunderbird/");
	
	if (profiles.count() == 0)
	{
		KMessageBox::error(0, i18n("No Mozilla profiles found"), i18n("Could not load Mozilla profiles"));
		return;
	}
	
	Q_FOREACH (QString profile, profiles)
		fixProfile(profile);
	
	KMessageBox::information(0, i18n("Your Mozilla profile was updated sucessfully.  You must close and restart all Firefox and Thunderbird windows for the changes to take effect"), i18n("Mozilla profile"));
}

QStringList FirefoxFix::getProfiles(const QString& basePath)
{
	QStringList ret;
	
	QString fileName = basePath + "/profiles.ini";
	if (QFile::exists(fileName))
	{
		QSettings settings(fileName, QSettings::IniFormat);
		
		Q_FOREACH (QString group, settings.childGroups())
		{
			if (!group.toLower().startsWith("profile"))
				continue;
			
			settings.beginGroup(group);
			QString path = settings.value("Path").toString();
			settings.endGroup();
			
			if (!path.startsWith("/"))
				path = basePath + path;
			
			ret << path;
		}
	}
	
	return ret;
}

void FirefoxFix::fixProfile(const QString& path)
{
	if (!QFile::exists(path + "/chrome"))
	{
		QDir dir(path);
		dir.mkdir("chrome");
	}
	
	QString data = scrollBarCSS();
	writeFirefoxCSS(path + "/chrome/userChrome.css", data);
	writeFirefoxCSS(path + "/chrome/userContent.css", data);
}

// Nicked from rcproperties.cpp
void FirefoxFix::findScrollBarButtons()
{
	static bool beenHereDoneThat = false;
	if (beenHereDoneThat)
		return;
	beenHereDoneThat = true;
	
	// I'm really quite ashamed of this function.
	// It loops over the ends of a scrollbar looking to see if each pixel is part of a button.
	
	QScrollBar* scrollBar = new QScrollBar(0);
	QStyle* qtStyle = QApplication::style();
	
	QStyleOptionSlider option;
	option.sliderValue = 1;
	option.sliderPosition = 1;
	option.rect = QRect(0, 0, 200, 25);
	option.state = QStyle::State_Horizontal;
	option.orientation = Qt::Horizontal;
	
	QRect rect = qtStyle->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarGroove, scrollBar);
	
	s_scrollBarHasBack1 = false;
	s_scrollBarHasForward1 = false;
	s_scrollBarHasBack2 = false;
	s_scrollBarHasForward2 = false;
	
	for (QPoint pos(0,7) ; pos.x()<rect.x() ; pos.setX(pos.x()+1))
	{
		QStyle::SubControl sc = qtStyle->hitTestComplexControl(QStyle::CC_ScrollBar, &option, pos, scrollBar);
		if (sc == QStyle::SC_ScrollBarAddLine) s_scrollBarHasForward1 = true;
		if (sc == QStyle::SC_ScrollBarSubLine) s_scrollBarHasBack1 = true;
	}
	
	for (QPoint pos(rect.x()+rect.width(),7) ; pos.x()<200 ; pos.setX(pos.x()+1))
	{
		QStyle::SubControl sc = qtStyle->hitTestComplexControl(QStyle::CC_ScrollBar, &option, pos, scrollBar);
		if (sc == QStyle::SC_ScrollBarAddLine) s_scrollBarHasForward2 = true;
		if (sc == QStyle::SC_ScrollBarSubLine) s_scrollBarHasBack2 = true;
	}
	
	delete scrollBar;
}

QString FirefoxFix::scrollBarCSS()
{
	findScrollBarButtons();
	
	QString upTop = (s_scrollBarHasBack1 ? "-moz-box" : "none");
	QString downTop = (s_scrollBarHasForward1 ? "-moz-box" : "none");
	QString upBottom = (s_scrollBarHasBack2 ? "-moz-box" : "none");
	QString downBottom = (s_scrollBarHasForward2 ? "-moz-box" : "none");
	
	QString data;
	data += "/* The following four lines were added by KDE */\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-up-top\"] { display: " + upTop + " !important; }\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-down-top\"] { display: " + downTop + " !important; }\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-up-bottom\"] { display: " + upBottom + " !important; }\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-down-bottom\"] { display: " + downBottom + " !important; }\n";
	
	return data;
}

void FirefoxFix::writeFirefoxCSS(const QString& path, const QString& data)
{
	QString fileData;
	QFile file(path);
	if (file.open(QIODevice::ReadOnly))
	{
		QTextStream stream(&file);
		for (;;)
		{
			QString line = stream.readLine();
			if (line.isNull())
				break;
			
			if ((line == "# The following four lines were added by KDE") ||
			    (line == "/* The following four lines were added by KDE */"))
			{
				for (int i=0 ; i<4 ; i++)
					stream.readLine();
				continue;
			}
			
			fileData += line + "\n";
		}
		file.close();
	}
	
	if (!file.open(QIODevice::WriteOnly))
	{
		KMessageBox::error(0, i18n("Could not write to %1").arg(path), i18n("Mozilla profile"));
		return;
	}
	QTextStream stream(&file);
	stream << fileData << data;
	file.close();
	
	return;
}
