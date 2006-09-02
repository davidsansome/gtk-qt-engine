/***************************************************************************
 *   Copyright (C) 2004 by David Sansome                                   *
 *   david@dave-linux                                                      *
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

#include <kcmodule.h>
#include <kaboutdata.h>

#include "kcmgtkwidget.h"

class GtkRcParser
{
public:
	GtkRcParser() {}
	~GtkRcParser() {}
	
	void parse(QString fileName);
	
	QString font;
	int fontSize;
	QString style;
};

class KcmGtk: public KCModule
{
	Q_OBJECT

public:
	KcmGtk( QWidget *parent=0, const char *name=0, const QStringList& = QStringList() );
	~KcmGtk();
	
	virtual void load();
	virtual void save();
	virtual int buttons();
	virtual QString quickHelp() const;
	virtual const KAboutData *aboutData()const
	{ return myAboutData; };

public slots:
	void styleChanged();
	void fontChangeClicked();
	void itemChanged();
	void firefoxFixClicked();

private:
	void updateFontPreview();
	void getProfiles(const QString& basePath, int type);
	void fixProfile(const QString& path);
	QString scrollBarCSS();
	void writeFirefoxCSS(const QString& path, const QString& data);
	
	QString env(QString key);
	
	KcmGtkWidget* widget;
	QMap<QString,QString> themes;
	GtkRcParser parser;
	KAboutData *myAboutData;
	QFont font;
	QMap<QString,QString> profiles;
};

#endif
