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

#include <qlayout.h>

#include <klocale.h>
#include <kglobal.h>
#include <kfontdialog.h>
#include <kiconloader.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qdir.h>
#include <qcombobox.h>
#include <qmap.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <kurllabel.h>
#include <kapplication.h>
#include <kfontcombo.h>
#include <qspinbox.h>
#include <ksqueezedtextlabel.h>
#include <stdlib.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <qstyle.h>
#include <qheader.h>
#include <klistview.h>

#include "kcmgtk.h"
#include "mozillaprofile.h"

/*typedef KGenericFactory<KcmGtk, QWidget> KcmGtkFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_gtk, KcmGtkFactory("gtk"))*/

extern "C"
{
	KCModule *create_kcmgtk( QWidget * parent, const char * name )
	{
		KGlobal::locale()->insertCatalogue( "gtkqtengine" );
		return new KcmGtk( parent, "kcmgtk" );
	}
}

void GtkRcParser::parse(QString fileName)
{
	QFile file(fileName);
	file.open(IO_ReadOnly);
	QTextStream stream(&file);
	
	QRegExp includeRe("include\\s*\"([^\"]*)\"");
	QRegExp fontRe("font_name\\s*=\\s*\"([^\"]*)\"");
	
	QStringList includes;
	
	while (1)
	{
		QString line = stream.readLine();
		if (line.isNull())
			break;
		if (line.startsWith("#"))
			continue;
		
		line = line.stripWhiteSpace();
		
		if (line.startsWith("include"))
		{
			if (includeRe.search(line) == -1)
				continue;
			if (includeRe.cap(1).endsWith("/gtk-2.0/gtkrc"))
				style = includeRe.cap(1);
		}
		if (line.startsWith("font_name"))
		{
			if (fontRe.search(line) == -1)
				continue;
			// Assume there will only be one font line
			font = fontRe.cap(1);
		}
	}
	
	file.close();
	
	int lastSpacePos = font.findRev(' ');
	if (lastSpacePos != -1)
	{
		bool ok;
		fontSize = font.right(font.length() - lastSpacePos).toInt(&ok);
		if (!ok)
			fontSize = 12;
		else
			font = font.left(lastSpacePos);
	}
}

KcmGtk::KcmGtk(QWidget *parent, const char *name, const QStringList&)
    : KCModule(parent, name), myAboutData(0)
{
	KGlobal::locale()->insertCatalogue("gtkqtengine");
	
	QStringList gtkSearchPaths;
	gtkSearchPaths.append("/usr");
	gtkSearchPaths.append("/usr/local");
	gtkSearchPaths.append("/opt/gnome");
	gtkSearchPaths.append(QDir::homeDirPath() + "/.local");
	// We should probably add some other paths here too...

	// Get a list of installed themes
	for ( QStringList::Iterator it = gtkSearchPaths.begin(); it != gtkSearchPaths.end(); ++it )
	{
		QString path = (*it) + "/share/themes/";
		QDir dir(path);
		QStringList entryList = dir.entryList(QDir::Dirs, QDir::Unsorted);
		for ( QStringList::Iterator it2 = entryList.begin(); it2 != entryList.end(); ++it2 )
		{
			if ((*it2).startsWith("."))
				continue;
			if (themes.find(*it2) != themes.end())
				continue;
			if (!QFile::exists(path + (*it2) + "/gtk-2.0/gtkrc"))
				continue;
			themes.insert((*it2), path + (*it2) + "/gtk-2.0/gtkrc");
		}
	}
	
	// Add the widget to our layout
	QBoxLayout* l = new QVBoxLayout(this);
	widget = new KcmGtkWidget(this);
	l->addWidget(widget);
	
	// Load the icons
	KIconLoader iconLoader;
	widget->styleIcon->setPixmap(iconLoader.loadIcon("style", KIcon::Desktop));
	widget->fontIcon->setPixmap(iconLoader.loadIcon("fonts", KIcon::Desktop));
	widget->firefoxIcon->setPixmap(iconLoader.loadIcon("firefox", KIcon::Desktop));
	
	widget->styleBox->insertStringList(themes.keys());
	load();
	
	// Connect some signals
	connect(widget->warning2, SIGNAL(leftClickedURL(const QString&)), KApplication::kApplication(), SLOT(invokeBrowser(const QString&)));
	connect(widget->styleGroup, SIGNAL(clicked(int)), SLOT(itemChanged()));
	connect(widget->fontGroup, SIGNAL(clicked(int)), SLOT(itemChanged()));
	connect(widget->styleBox, SIGNAL(activated(int)), SLOT(itemChanged()));
	connect(widget->styleBox, SIGNAL(activated(int)), SLOT(styleChanged()));
	connect(widget->fontChange, SIGNAL(clicked()), SLOT(fontChangeClicked()));
	connect(widget->firefoxFix, SIGNAL(clicked()), SLOT(firefoxFixClicked()));
}

void KcmGtk::itemChanged()
{
	// In KDE < 3.3 there is no changed() slot - only a signal.
	emit changed(true);
}

void KcmGtk::fontChangeClicked()
{
	if ( KFontDialog::getFont( font ) == KFontDialog::Accepted )
	{
		updateFontPreview();
		widget->fontGroup->setButton(widget->fontGroup->id(widget->fontOther));
		itemChanged();
	}
}

void KcmGtk::styleChanged()
{
	widget->styleGroup->setButton(widget->styleGroup->id(widget->styleOther));
	itemChanged();
}

QString KcmGtk::env(QString key)
{
	char* value = getenv(key.latin1());
	if (value == NULL)
		return QString::null;
	else
		return QString(value);
}

void KcmGtk::updateFontPreview()
{
	widget->fontPreview->setFont(font);
	widget->fontPreview->setText( 
		i18n("%1 (size %2)").arg(font.family()).arg(QString::number(font.pointSize())));
	widget->fontPreview2->setFont(font);
}


KcmGtk::~KcmGtk()
{
}


void KcmGtk::load()
{
	parser.parse(QDir::homeDirPath() + "/.gtkrc-2.0");
	
	bool usingQtEngine = false;
	if (!parser.style.isEmpty())
	{
		for ( QMapIterator<QString, QString> it = themes.begin(); it != themes.end(); ++it )
		{
			if (it.data() != parser.style)
				continue;
			
			if (it.key() == "Qt")
				usingQtEngine = true;
			
			for (int i=0 ; i<widget->styleBox->count() ; ++i)
			{
				if (widget->styleBox->text(i) == it.key())
				{
					widget->styleBox->setCurrentItem(i);
					break;
				}
			}
			
			break;
		}
	}
	else
		usingQtEngine = true;
	
	if (usingQtEngine)
		widget->styleGroup->setButton(widget->styleGroup->id(widget->styleKde));
	else
		widget->styleGroup->setButton(widget->styleGroup->id(widget->styleOther));
	
	if (themes.find("Qt") == themes.end())
		widget->styleKde->setEnabled(false);
	else
	{
		widget->warning1->hide();
		widget->warning2->hide();
	}
	
	if (!parser.font.isEmpty())
	{
		font.setFamily(parser.font);
		font.setPointSize(parser.fontSize);
		
		if ((QApplication::font().family() == parser.font) && (QApplication::font().pointSize() == parser.fontSize))
			widget->fontGroup->setButton(widget->fontGroup->id(widget->fontKde));
		else
			widget->fontGroup->setButton(widget->fontGroup->id(widget->fontOther));
	}
	else
	{
		widget->fontGroup->setButton(widget->fontGroup->id(widget->fontKde));
		font = QApplication::font();
	}
	
	updateFontPreview();
}


void KcmGtk::save()
{
	// First write out the gtkrc file
	QFile file(QDir::homeDirPath() + "/.gtkrc-2.0");
	file.open(IO_WriteOnly);
	QTextStream stream(&file);
	
	QString fontName = widget->fontKde->isChecked()
	  ? QApplication::font().family() + " " + QString::number(QApplication::font().pointSize())
	  : font.family() + " " + QString::number(font.pointSize());
	QString themeName = widget->styleKde->isChecked() ? themes["Qt"] : themes[widget->styleBox->currentText()];
	QString themeNameShort = widget->styleKde->isChecked() ? QString("Qt") : widget->styleBox->currentText();
	
	stream << "# This file was written by KDE\n";
	stream << "# You can edit it in the KDE control center, under \"GTK Styles and Fonts\"\n";
	stream << "\n";
	stream << "include \"" << themeName << "\"\n";
	stream << "\n";
	stream << "style \"user-font\"\n";
	stream << "{\n";
	stream << "\tfont_name=\"" << fontName << "\"\n";
	stream << "}\n";
	stream << "widget_class \"*\" style \"user-font\"\n";
	stream << "\n";
	stream << "gtk-theme-name=\"" << themeNameShort << "\"\n";
	stream << "gtk-font-name=\"" << fontName << "\"\n";
	
	file.close();
	
	// Now we check if that file is actually being loaded.
	file.setName(QDir::homeDirPath() + "/.bashrc");
	file.open(IO_ReadWrite);
	stream.setDevice(&file);
	bool found = false;
	for (;;)
	{
		QString line = stream.readLine();
		if (line.isNull())
			break;
		
		if (line.stripWhiteSpace().startsWith("export GTK2_RC_FILES=$HOME/.gtkrc-2.0"))
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		stream << "\n";
		stream << "\n";
		stream << "# This line was appended by KDE\n";
		stream << "# Make sure our customised gtkrc file is loaded.\n";
		stream << "export GTK2_RC_FILES=$HOME/.gtkrc-2.0\n";
	}
	file.close();
	
	emit changed(true);
}


int KcmGtk::buttons()
{
	return KCModule::Apply;
}

QString KcmGtk::quickHelp() const
{
	return i18n("");
}


void KcmGtk::firefoxFixClicked()
{
	profiles.clear();
	getProfiles(QDir::homeDirPath() + "/.mozilla/firefox/", 0);
	getProfiles(QDir::homeDirPath() + "/.thunderbird/", 1);
	
	QString profilePath;
	if (profiles.count() == 0)
	{
		KMessageBox::error(this, i18n("No Mozilla profiles found"), i18n("Could not load Mozilla profiles"));
		return;
	}
	else if (profiles.count() == 1)
	{
		fixProfile(profiles.begin().data());
	}
	else
	{
		KDialogBase* dialog = new KDialogBase(this, "", true, i18n("Mozilla profile"), KDialogBase::Ok | KDialogBase::Cancel);
		MozillaProfileWidget* w = new MozillaProfileWidget(dialog);
		w->profilesList->header()->hide();
		w->profilesList->hideColumn(1);
		
		QPixmap icon = KGlobal::iconLoader()->loadIcon("kuser", KIcon::Small);
		
		for ( QMapIterator<QString,QString> it = profiles.begin(); it != profiles.end(); ++it )
		{
			KListViewItem* i = new KListViewItem(w->profilesList);
			i->setPixmap(0, icon);
			i->setText(0, it.key());
			i->setText(1, it.data());
		}
		
		dialog->setMainWidget(w);
		if (dialog->exec() == QDialog::Rejected)
		{
			delete dialog;
			return;
		}
		
		QListViewItemIterator it2(w->profilesList, QListViewItemIterator::Selected);
		while (it2.current())
		{
			KListViewItem* i = (KListViewItem*) it2.current();
			++it2;
			
			fixProfile(i->text(1));
		}
		delete dialog;
	}
	
	KMessageBox::information(this, i18n("Your Mozilla profile was updated sucessfully.  You must close and restart all Firefox and Thunderbird windows for the changes to take effect"), i18n("Mozilla profile"));
}

void KcmGtk::getProfiles(const QString& basePath, int type)
{
	QString fileName = basePath + "/profiles.ini";
	if (QFile::exists(fileName))
	{
		KConfig config(fileName, true, false);
		QStringList groups = config.groupList();
		
		for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
		{
			if (!(*it).lower().startsWith("profile"))
				continue;
			
			config.setGroup(*it);
			QString name = (type ? i18n("Thunderbird") : i18n("Firefox")) + " - " + config.readEntry("Name");
			QString path = config.readEntry("Path");
			if (!path.startsWith("/"))
				path = basePath + path;
			profiles.insert(name, path);
		}
	}
}

void KcmGtk::fixProfile(const QString& path)
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

QString KcmGtk::scrollBarCSS()
{
	// The following code determines how many buttons are on a scrollbar
	// It works by looking at each pixel of the scrollbar's area not taken up by the groove,
	// and asking the style which subcontrol is at that location.
	QScrollBar sbar(NULL);
	sbar.setOrientation(Qt::Horizontal);
	sbar.setValue(1);
	sbar.resize(200,25);
	
	QRect rect = qApp->style().querySubControlMetrics(QStyle::CC_ScrollBar, &sbar, QStyle::SC_ScrollBarGroove);
	
	bool back1 = false;
	bool forward1 = false;
	bool back2 = false;
	bool forward2 = false;
	
	QStyle::SubControl sc = QStyle::SC_None;
	for (QPoint pos(0,7) ; pos.x()<rect.x() ; pos.setX(pos.x()+1))
	{
		QStyle::SubControl sc2 = qApp->style().querySubControl(QStyle::CC_ScrollBar, &sbar, pos);
		if (sc != sc2)
		{
			if (sc2 == QStyle::SC_ScrollBarAddLine) forward1 = true;
			if (sc2 == QStyle::SC_ScrollBarSubLine) back1 = true;
			sc = sc2;
		}
	}
	sc = QStyle::SC_None;
	for (QPoint pos(rect.x()+rect.width(),7) ; pos.x()<200 ; pos.setX(pos.x()+1))
	{
		QStyle::SubControl sc2 = qApp->style().querySubControl(QStyle::CC_ScrollBar, &sbar, pos);
		if (sc != sc2)
		{
			if (sc2 == QStyle::SC_ScrollBarAddLine) forward2 = true;
			if (sc2 == QStyle::SC_ScrollBarSubLine) back2 = true;
			sc = sc2;
		}
	}
	
	QString upTop = (back1 ? "-moz-box" : "none");
	QString downTop = (forward1 ? "-moz-box" : "none");
	QString upBottom = (back2 ? "-moz-box" : "none");
	QString downBottom = (forward2 ? "-moz-box" : "none");
	
	QString data;
	data += "# The following four lines were added by KDE\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-up-top\"] { display: " + upTop + " !important; }\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-down-top\"] { display: " + downTop + " !important; }\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-up-bottom\"] { display: " + upBottom + " !important; }\n";
	data += "scrollbarbutton[sbattr=\"scrollbar-down-bottom\"] { display: " + downBottom + " !important; }\n";
	
	return data;
}

void KcmGtk::writeFirefoxCSS(const QString& path, const QString& data)
{
	QString fileData;
	QFile file(path);
	if (file.open(IO_ReadOnly))
	{
		QTextStream stream(&file);
		for (;;)
		{
			QString line = stream.readLine();
			if (line.isNull())
				break;
			
			if (line == "# The following four lines were added by KDE")
			{
				for (int i=0 ; i<4 ; i++)
					stream.readLine();
				continue;
			}
			
			fileData += line + "\n";
		}
		file.close();
	}
	
	if (!file.open(IO_WriteOnly | IO_Truncate))
	{
		KMessageBox::error(this, i18n("Could not write to %1").arg(path), i18n("Mozilla profile"));
		return;
	}
	QTextStream stream(&file);
	stream << fileData << data;
	file.close();
	
	return;
}


#include "kcmgtk.moc"
