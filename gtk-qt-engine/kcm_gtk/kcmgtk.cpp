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

#include "kcmgtk.h"
#include "gtkrcfile.h"
#include "firefoxfix.h"
#include "searchpaths.h"

#include <kgenericfactory.h>
#include <kaboutdata.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kfontdialog.h>

#include <QDir>
#include <QSettings>
#include <QMessageBox>

#include <sys/stat.h>


const QString KcmGtk::k_gtkRcFileName(QDir::homePath() + "/.gtkrc-2.0-kde4");
const QString KcmGtk::k_envFileName(KGlobal::dirs()->localkdedir() + "/env/gtk-qt-engine.rc.sh");
const QString KcmGtk::k_qtThemeName("Qt4");

K_PLUGIN_FACTORY(KcmGtkFactory, registerPlugin<KcmGtk>();)
K_EXPORT_PLUGIN(KcmGtkFactory("kcmgtk4"))


KcmGtk::KcmGtk(QWidget* parent, const QVariantList&)
	: KCModule(KcmGtkFactory::componentData(), parent)
{
	m_ui.setupUi(this);
	connect(m_ui.fontChange, SIGNAL(clicked()), SLOT(fontChangeClicked()));
	connect(m_ui.fontKde, SIGNAL(clicked(bool)), SLOT(fontKdeClicked()));
	connect(m_ui.styleBox, SIGNAL(activated(int)), SLOT(styleChanged()));
	connect(m_ui.styleKde, SIGNAL(clicked(bool)), SLOT(styleKdeClicked(bool)));
	connect(m_ui.firefoxFix, SIGNAL(clicked()), SLOT(firefoxFixClicked()));
	
	m_gtkRc = new GtkRcFile(k_gtkRcFileName);
	
	m_searchPaths = new SearchPaths(this);
	connect(m_searchPaths, SIGNAL(accepted()), SLOT(getInstalledThemes()));
	connect(m_ui.warning3, SIGNAL(clicked()), m_searchPaths, SLOT(exec()));
	
	// Load icons
	KIconLoader* loader = KIconLoader::global();
	m_ui.styleIcon->setPixmap(loader->loadIcon("preferences-desktop-theme", KIconLoader::Desktop));
	m_ui.fontIcon->setPixmap(loader->loadIcon("preferences-desktop-font", KIconLoader::Desktop));
	m_ui.firefoxIcon->setPixmap(loader->loadIcon("firefox", KIconLoader::Desktop));
	
	// Set KCM stuff
	setAboutData(new KAboutData(I18N_NOOP("kcm_gtk4"), 0,
                                    ki18n("GTK Styles and Fonts"),0,KLocalizedString(),KAboutData::License_GPL,
                                    ki18n("(C) 2008 David Sansome")));
	setQuickHelp(i18n("Change the appearance of GTK applications"));
	
	// Load GTK settings
	getInstalledThemes();
	load();
	setButtons(Apply);
}

KcmGtk::~KcmGtk()
{
	delete m_gtkRc;
}

void KcmGtk::load()
{
	m_gtkRc->load();
	
	m_ui.styleKde->setChecked(m_gtkRc->themeName() == k_qtThemeName);
	m_ui.styleOther->setChecked(m_gtkRc->themeName() != k_qtThemeName);
	m_ui.styleBox->setCurrentIndex(m_themes.keys().indexOf(m_gtkRc->themeName()));
	
	QFont defaultFont;
	bool usingKdeFont = (m_gtkRc->font().family() == defaultFont.family() &&
	                     m_gtkRc->font().pointSize() == defaultFont.pointSize() &&
	                     m_gtkRc->font().bold() == defaultFont.bold() &&
	                     m_gtkRc->font().italic() == defaultFont.italic());
	m_ui.fontKde->setChecked(usingKdeFont);
	m_ui.fontOther->setChecked(!usingKdeFont);
	
	updateFontPreview();
}

void KcmGtk::updateFontPreview()
{
	m_ui.fontPreview->setFont(m_gtkRc->font());
	m_ui.fontPreview->setText(i18n("%1 (size %2)", m_gtkRc->font().family(), QString::number(m_gtkRc->font().pointSize())));
	m_ui.fontPreview2->setFont(m_gtkRc->font());
}

void KcmGtk::save()
{
	// Write ~/.gtkrc-2.0-kde4
	m_gtkRc->save();
	
	// Write ~/.kde/env/gtk-qt-engine.rc.sh
	bool envFileDidNotExist = !QFile::exists(k_envFileName);
	
	// Make sure the directories exist
	QDir dir;
	dir.mkpath(QFileInfo(k_envFileName).path());
	
	// Now write the file itself
	QFile file(k_envFileName);
	file.open(QIODevice::WriteOnly);
	QTextStream stream(&file);
	stream << "#!/bin/bash\n\n";
	stream << "# Make sure our customised gtkrc file is loaded.\n";
	stream << "export GTK2_RC_FILES=" + k_gtkRcFileName + "\n";
	file.close();
	
	// Make it executable
	chmod(k_envFileName.toAscii(), 0755);
	
	if (envFileDidNotExist)
		QMessageBox::information(this, "Restart KDE", "Your changes have been saved, but you will have to restart KDE for them to take effect.", QMessageBox::Ok);
	
	// Write the KDE paths to a config file that gets picked up by the theme engine
	// It used to popen kde-config, but that broke sometimes and seemed ugly
	QSettings settings("gtk-qt-engine", "gtk-qt-engine");
	settings.setValue("KDELocalPrefix", KGlobal::dirs()->localkdedir());
	settings.setValue("KDEPrefix", KGlobal::dirs()->installPath("kdedir"));
}

void KcmGtk::defaults()
{
}

void KcmGtk::getInstalledThemes()
{
	m_themes.clear();
	Q_FOREACH (QString path, m_searchPaths->paths())
	{
		path += "/share/themes/";
		Q_FOREACH (QString subdir, QDir(path).entryList(QDir::Dirs, QDir::Unsorted))
		{
			if (subdir.startsWith("."))
				continue;
			if (m_themes.contains(subdir))
				continue;
			if (!QFile::exists(path + subdir + "/gtk-2.0/gtkrc"))
				continue;
			m_themes[subdir] = path + subdir + "/gtk-2.0/gtkrc";
		}
	}
	
	m_ui.styleBox->clear();
	m_ui.styleBox->addItems(m_themes.keys());
	
	bool installed = m_themes.contains(k_qtThemeName);
	m_ui.styleKde->setEnabled(installed);
	m_ui.warning1->setHidden(installed);
	m_ui.warning2->setHidden(installed);
	m_ui.warning3->setHidden(installed);
}

void KcmGtk::fontChangeClicked()
{
	QFont font(m_gtkRc->font());
	if (KFontDialog::getFont(font) != KFontDialog::Accepted)
		return;
	
	m_gtkRc->setFont(font);
	updateFontPreview();
	m_ui.fontOther->setChecked(true);
	changed(true);
}

void KcmGtk::fontKdeClicked()
{
	m_gtkRc->setFont(QFont());
	updateFontPreview();
	changed(true);
}

void KcmGtk::styleChanged()
{
	m_gtkRc->setTheme(m_themes[m_ui.styleBox->currentText()]);
	m_ui.styleOther->setChecked(true);
	changed(true);
}

void KcmGtk::styleKdeClicked(bool checked)
{
	if (checked)
	{
		m_gtkRc->setTheme(m_themes[k_qtThemeName]);
		m_ui.styleBox->setCurrentIndex(m_themes.keys().indexOf(k_qtThemeName));
	}
	else
		m_gtkRc->setTheme(m_themes[m_ui.styleBox->currentText()]);
	changed(true);
}

void KcmGtk::firefoxFixClicked()
{
	FirefoxFix::go();
}

