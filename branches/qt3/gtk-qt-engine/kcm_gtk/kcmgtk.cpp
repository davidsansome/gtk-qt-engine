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
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "kcmgtk.h"
#include "mozillaprofile.h"

const QString KcmGtk::GTK_RC_FILE(".gtkrc-2.0-kde");
const QString KcmGtk::KDE_RC_DIR(KGlobal::dirs()->localkdedir() + "/env/");
const QString KcmGtk::GTK_QT_RC_FILE("gtk-qt-engine.rc.sh");

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

GtkRcParser::GtkRcParser()
 : emacs(false)
{
}

void GtkRcParser::parse(QString fileName)
{
	QFile file(fileName);
	file.open(IO_ReadOnly);
	QTextStream stream(&file);
	
	QRegExp includeRe("include\\s*\"([^\"]*)\"");
	QRegExp fontRe("font_name\\s*=\\s*\"([^\"]*)\"");
	QRegExp keyThemeRe("gtk-key-theme-name\\s*=\\s*\"([^\"]*)\"");
	
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
			QString themePath = includeRe.cap(1);
			if (themePath.endsWith("/gtk-2.0/gtkrc") && !themePath.startsWith("/etc"))
				style = includeRe.cap(1);
		}
		if (line.startsWith("font_name"))
		{
			if (fontRe.search(line) == -1)
				continue;
			// Assume there will only be one font line
			font = parseFont(fontRe.cap(1));
		}
		if (line.startsWith("gtk-key-theme-name"))
		{
			if (keyThemeRe.search(line) == -1)
				continue;
			if (keyThemeRe.cap(1).lower() == "emacs")
				emacs = true;
		}
	}
	
	file.close();
}

QFont GtkRcParser::parseFont(QString fontString)
{
	QFont ret;
	while (true)
	{
		int lastSpacePos = fontString.findRev(' ');
		if (lastSpacePos == -1)
			break;

		QString lastWord = fontString.right(fontString.length() - lastSpacePos).stripWhiteSpace();

		if (lastWord.lower() == "bold")
			ret.setBold(true);
		else if (lastWord.lower() == "italic")
			ret.setItalic(true);
		else
		{
			bool ok;
			int fontSize = lastWord.toInt(&ok);
			if (!ok)
				break;
			ret.setPointSize(fontSize);
		}

		fontString = fontString.left(lastSpacePos);
	}
	ret.setFamily(fontString);
	return ret;
}

KcmGtk::KcmGtk(QWidget *parent, const char *name, const QStringList&)
    : KCModule(parent, name),
      myAboutData(0),
      emacsDetailsDialog(NULL),
      searchPathsDialog(NULL)
{
	KGlobal::locale()->insertCatalogue("gtkqtengine");
	
	config = new KConfig("kcmgtkrc");

	QStringList gtkSearchPathsDefault;
	gtkSearchPathsDefault.append("/usr");
	gtkSearchPathsDefault.append("/usr/local");
	gtkSearchPathsDefault.append("/opt/gnome");
	gtkSearchPathsDefault.append(QDir::homeDirPath() + "/.local");

	gtkSearchPaths = config->readListEntry("gtkSearchPaths", gtkSearchPathsDefault);

	
	// Add the widget to our layout
	QBoxLayout* l = new QVBoxLayout(this);
	widget = new KcmGtkWidget(this);
	l->addWidget(widget);
	
	// Load the icons
	KIconLoader iconLoader;
	widget->styleIcon->setPixmap(iconLoader.loadIcon("style", KIcon::Desktop));
	widget->fontIcon->setPixmap(iconLoader.loadIcon("fonts", KIcon::Desktop));
	widget->firefoxIcon->setPixmap(iconLoader.loadIcon("firefox", KIcon::Desktop));
	widget->keyboardIcon->setPixmap(iconLoader.loadIcon("keyboard", KIcon::Desktop));
	
	getInstalledThemes();
	load();
	
	// Connect some signals
	connect(widget->warning2, SIGNAL(leftClickedURL(const QString&)), KApplication::kApplication(), SLOT(invokeBrowser(const QString&)));
	connect(widget->styleGroup, SIGNAL(clicked(int)), SLOT(itemChanged()));
	connect(widget->fontGroup, SIGNAL(clicked(int)), SLOT(itemChanged()));
	connect(widget->styleBox, SIGNAL(activated(int)), SLOT(itemChanged()));
	connect(widget->styleBox, SIGNAL(activated(int)), SLOT(styleChanged()));
	connect(widget->emacsBox, SIGNAL(toggled(bool)), SLOT(itemChanged()));
	connect(widget->fontChange, SIGNAL(clicked()), SLOT(fontChangeClicked()));
	connect(widget->firefoxFix, SIGNAL(clicked()), SLOT(firefoxFixClicked()));
	connect(widget->emacsDetails, SIGNAL(clicked()), SLOT(emacsDetailsClicked()));
	connect(widget->warning3, SIGNAL(clicked()), SLOT(searchPathsClicked()));
}

void KcmGtk::getInstalledThemes()
{
	themes.clear();
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
	
	widget->styleBox->clear();
	widget->styleBox->insertStringList(themes.keys());
	
	bool installed = (themes.find("Qt") != themes.end());
	widget->styleKde->setEnabled(installed);
	widget->warning1->setHidden(installed);
	widget->warning2->setHidden(installed);
	widget->warning3->setHidden(installed);
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
	delete config;
}


void KcmGtk::load()
{
	parser.parse(QDir::homeDirPath() + "/" + GTK_RC_FILE);
	
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
		
		if (usingQtEngine)
			widget->styleGroup->setButton(widget->styleGroup->id(widget->styleKde));
		else
			widget->styleGroup->setButton(widget->styleGroup->id(widget->styleOther));
	}

	font = parser.font;
	if (QApplication::font().family() == font.family() &&
	    QApplication::font().pointSize() == font.pointSize() &&
	    QApplication::font().bold() == font.bold() &&
	    QApplication::font().italic() == font.italic())
		widget->fontGroup->setButton(widget->fontGroup->id(widget->fontKde));
	else
		widget->fontGroup->setButton(widget->fontGroup->id(widget->fontOther));
	
	widget->emacsBox->setChecked(parser.emacs);
	
	updateFontPreview();
}


void KcmGtk::save()
{
	// First write out the gtkrc file
	QFile file(QDir::homeDirPath() + "/" + GTK_RC_FILE);
	file.open(IO_WriteOnly);
	QTextStream stream(&file);

	QFont selectedFont(widget->fontKde->isChecked() ? QApplication::font() : font);
	QString fontName = selectedFont.family() + " " +
		QString(selectedFont.bold() ? "Bold " : "") +
		QString(selectedFont.italic() ? "Italic " : "") +
		QString::number(selectedFont.pointSize());
	
	QString themeName = widget->styleKde->isChecked() ? themes["Qt"] : themes[widget->styleBox->currentText()];
	QString themeNameShort = widget->styleKde->isChecked() ? QString("Qt") : widget->styleBox->currentText();
	
	stream << "# This file was written by KDE\n";
	stream << "# You can edit it in the KDE control center, under \"GTK Styles and Fonts\"\n";
	stream << "\n";
	stream << "include \"" << themeName << "\"\n";
	if (QFile::exists("/etc/gtk-2.0/gtkrc"))
		stream << "include \"/etc/gtk-2.0/gtkrc\"\n";
	stream << "\n";
	stream << "style \"user-font\"\n";
	stream << "{\n";
	stream << "\tfont_name=\"" << fontName << "\"\n";
	stream << "}\n";
	stream << "widget_class \"*\" style \"user-font\"\n";
	stream << "\n";
	stream << "gtk-theme-name=\"" << themeNameShort << "\"\n";
	stream << "gtk-font-name=\"" << fontName << "\"\n";
	
	if (widget->emacsBox->isChecked())
		stream << "gtk-key-theme-name=\"Emacs\"\n";
	
	file.close();
	
	// Now we check if that file is actually being loaded.
	QDir kdeRcDir;
	if (!kdeRcDir.exists(KDE_RC_DIR))
	{
		// Make sure KDE's rc dir exists
		kdeRcDir.mkdir(KDE_RC_DIR);
	}
	file.setName(KDE_RC_DIR + "/" + GTK_QT_RC_FILE);
	
	bool envFileDidNotExist = (!file.exists());
	
	file.open(IO_ReadWrite);
	stream.setDevice(&file);
	bool found = false;
	for (;;)
	{
		QString line = stream.readLine();
		if (line.isNull())
			break;
		
		if (line.stripWhiteSpace().startsWith("export GTK2_RC_FILES=$HOME/" + GTK_RC_FILE))
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		stream << "#!/bin/bash\n\n";
		stream << "# Make sure our customised gtkrc file is loaded.\n";
		stream << "export GTK2_RC_FILES=$HOME/" + GTK_RC_FILE + "\n";
	}
	file.close();
	
	// Make it executable
	if (!found)
		chmod(file.name().utf8(), 0755);
	
	// Tell the user to restart KDE if the environment file was created this time
	if (envFileDidNotExist)
		QMessageBox::information(this, "Restart KDE", "Your changes have been saved, but you will have to restart KDE for them to take effect.", QMessageBox::Ok);
	
	// Older versions of the Gtk-Qt theme engine wrote directly into ~/.gtkrc-2.0
	// If the user has upgraded, that file needs to be deleted so the old settings
	// won't override the new ones set now.
	file.setName(QDir::homeDirPath() + "/.gtkrc-2.0");
	if (file.exists())
	{
		file.open(IO_ReadOnly);
		QString firstLine;
		file.readLine(firstLine, 50);
		file.close();
		
		if (firstLine == "# This file was written by KDE")
			file.remove();
	}
	
	// Simarly, remove the line we added to ~/.bashrc to tell GTK to load ~/.gtkrc-2.0
	file.setName(QDir::homeDirPath() + "/.bashrc");
	if (file.exists())
	{
		file.open(IO_ReadOnly);
		QByteArray fileData = file.readAll();
		file.close();
		
		QString rcLine = "export GTK2_RC_FILES=$HOME/.gtkrc-2.0";
		QString fileDataString(fileData);
		fileDataString.replace("\n" + rcLine, "\n# (This is no longer needed from version 0.8 of the theme engine)\n# " + rcLine);
		
		file.open(IO_WriteOnly);
		stream.setDevice(&file);
		stream << fileDataString;
		file.close();
	}
	
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
	data += "/* The following four lines were added by KDE */\n";
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

void KcmGtk::emacsDetailsClicked()
{
	if (emacsDetailsDialog == NULL)
	{
		emacsDetailsDialog = new EmacsDetails(this);
		emacsDetailsDialog->list->header()->setStretchEnabled(true, 1);
	}
	
	emacsDetailsDialog->show();
}

void KcmGtk::searchPathsClicked()
{
	if (searchPathsDialog == NULL)
	{
		searchPathsDialog = new SearchPaths(this);
		connect(searchPathsDialog->okButton, SIGNAL(clicked()), SLOT(searchPathsOk()));
		connect(searchPathsDialog->pathBox, SIGNAL(textChanged(const QString&)), SLOT(searchPathsTextChanged(const QString&)));
		connect(searchPathsDialog->pathList, SIGNAL(currentChanged(QListBoxItem*)), SLOT(searchPathsCurrentChanged(QListBoxItem*)));
		connect(searchPathsDialog->addButton, SIGNAL(clicked()), SLOT(searchPathsAddClicked()));
		connect(searchPathsDialog->removeButton, SIGNAL(clicked()), SLOT(searchPathsRemoveClicked()));
	}

	searchPathsDialog->pathList->clear();
	for (QStringList::Iterator it = gtkSearchPaths.begin(); it != gtkSearchPaths.end(); ++it )
		new QListBoxText(searchPathsDialog->pathList, *it);

	searchPathsDialog->show();
}

void KcmGtk::searchPathsOk()
{
	gtkSearchPaths.clear();
	int i=0;
	QListBoxItem* item = 0;
	while ((item = searchPathsDialog->pathList->item(i++)))
		gtkSearchPaths.append(item->text());
	
	config->writeEntry("gtkSearchPaths", gtkSearchPaths);
	getInstalledThemes();
}

void KcmGtk::searchPathsTextChanged(const QString& text)
{
	searchPathsDialog->addButton->setDisabled(text.isEmpty());
}

void KcmGtk::searchPathsCurrentChanged(QListBoxItem* item)
{
	searchPathsDialog->removeButton->setEnabled(item != NULL);
}

void KcmGtk::searchPathsAddClicked()
{
	new QListBoxText(searchPathsDialog->pathList, searchPathsDialog->pathBox->text());
	searchPathsDialog->pathBox->clear();
}

void KcmGtk::searchPathsRemoveClicked()
{
	searchPathsDialog->pathList->removeItem(searchPathsDialog->pathList->currentItem());
}


#include "kcmgtk.moc"
