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

#include <QDir> // This has to be above any GTK headers

#include "utilities.h"
#include "rcproperties.h"
#include "engine.h"

#include <QtDebug>
#include <QToolTip>
#include <QFile>
#include <QSettings>
#include <QFileInfo>

#define INITDUMMY(className) \
	dummy.setPalette(QApplication::palette()); \
	dummy.setObjectName(className); \
	Engine::instance()->m_qtStyle->polish(&dummy);

#define WCOLOR(class, role) \
	GtkQtUtilities::colorString(dummy.palette().color(QPalette::class, QPalette::role))


bool RcProperties::s_scrollBarHasBack1;
bool RcProperties::s_scrollBarHasForward1;
bool RcProperties::s_scrollBarHasBack2;
bool RcProperties::s_scrollBarHasForward2;
int RcProperties::s_scrollBarButtonSize;
int RcProperties::s_scrollBarButtonCount;

QStringList RcProperties::s_iconThemeDirs;
QStringList RcProperties::s_kdeSearchPaths;


void RcProperties::setRcProperties()
{
	GTK_QT_DEBUG_FUNC
	
	initKdeSettings();
	
	setWidgetProperties();
	setColorProperties();
	setIconProperties();
	
	// KDE's "Apply colors to non-KDE apps" setting is broken.
	// It applies colors to the "*" class which overrides colors set by the style
	// We take the (rather extreme) measure of removing its gtkrc file
	
	char** rcFiles = gtk_rc_get_default_files();
	while (*rcFiles != NULL)
	{
		QString fileName(*(rcFiles++));
		
		if (!fileName.endsWith("/share/config/gtkrc-2.0"))
			continue;
		
		// Open the file and check for a known string just to make
		// sure we're not accidentally deleting something else
		QFile file(fileName);
		if (!file.exists())
			continue;
		
		file.open(QIODevice::ReadOnly);
		if (!file.readLine(20).startsWith("# created by KDE"))
			continue;
		
		file.close();
		file.remove();
	}
}

void RcProperties::setWidgetProperties()
{
	const QStyle* qtStyle = Engine::instance()->m_qtStyle;
	
	// Buttons
	GtkQtUtilities::parseRcString("GtkButton::child_displacement_x = " + QString::number(qtStyle->pixelMetric(QStyle::PM_ButtonShiftHorizontal)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkButton::child_displacement_y = " + QString::number(qtStyle->pixelMetric(QStyle::PM_ButtonShiftVertical)), "*", GtkQtUtilities::WidgetClass);
	
	// Check boxes
	// We add 2 to the size to account for the box around the check indicator
	GtkQtUtilities::parseRcString("GtkCheckButton::indicator-size = " + QString::number(qtStyle->pixelMetric(QStyle::PM_IndicatorHeight) + 2), "*", GtkQtUtilities::WidgetClass);
	
	// Tabs
	GtkQtUtilities::parseRcString("GtkButton::tab-curvature = 0", "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkNotebook::tab-overlap = " + QString::number(qtStyle->pixelMetric(QStyle::PM_TabBarTabOverlap)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("xthickness = 2", "*.GtkNotebook", GtkQtUtilities::WidgetClass); // Hardcode values that look good
	GtkQtUtilities::parseRcString("ythickness = 3", "*.GtkNotebook", GtkQtUtilities::WidgetClass);
	
	// Line edits
	GtkQtUtilities::parseRcString("xthickness = 5", "GtkEntry", GtkQtUtilities::Class);
	GtkQtUtilities::parseRcString("ythickness = 5", "GtkEntry", GtkQtUtilities::Class);
	
	// Menus
	GtkQtUtilities::parseRcString("GtkMenu::vertical-padding = " + QString::number(qtStyle->pixelMetric(QStyle::PM_MenuPanelWidth)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkMenu::horizontal-padding = " + QString::number(qtStyle->pixelMetric(QStyle::PM_MenuPanelWidth)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkCheckMenuItem::indicator-size = " +  QString::number(qtStyle->pixelMetric(QStyle::PM_IndicatorWidth) + 2), "*", GtkQtUtilities::WidgetClass);
	
	// Sliders
	GtkQtUtilities::parseRcString("GtkScale::slider-length = " + QString::number(qtStyle->pixelMetric(QStyle::PM_SliderLength)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScale::slider-width = " + QString::number(qtStyle->pixelMetric(QStyle::PM_SliderThickness)), "*", GtkQtUtilities::WidgetClass);
	
	// Scrollbars
	findScrollBarButtons();
	GtkQtUtilities::parseRcString("GtkScrollbar::min-slider-length = " + QString::number(qtStyle->pixelMetric(QStyle::PM_ScrollBarSliderMin)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScrollbar::slider-width = " + QString::number(qtStyle->pixelMetric(QStyle::PM_ScrollBarExtent)), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScrollbar::stepper-size= " + QString::number(s_scrollBarButtonSize), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScrollbar::has-backward-stepper = " + QString(s_scrollBarHasBack1 ? "1" : "0"), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScrollbar::has-forward-stepper = " + QString(s_scrollBarHasForward2 ? "1" : "0"), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScrollbar::has-secondary-backward-stepper = " + QString(s_scrollBarHasBack2 ? "1" : "0"), "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GtkScrollbar::has-secondary-forward-stepper = " + QString(s_scrollBarHasForward1 ? "1" : "0"), "*", GtkQtUtilities::WidgetClass);
	
	// Put a bigger border around tab icons in the gimp
	GtkQtUtilities::parseRcString("GimpColorNotebook::tab-border = 2", "*", GtkQtUtilities::WidgetClass);
	GtkQtUtilities::parseRcString("GimpDockbook::tab-border = 2", "*", GtkQtUtilities::WidgetClass);
	
}

void RcProperties::findScrollBarButtons()
{
	// I'm really quite ashamed of this function.
	// It loops over the ends of a scrollbar looking to see if each pixel is part of a button.
	
	const QScrollBar* scrollBar = Engine::instance()->m_dummyScrollBar;
	const QStyle* qtStyle = Engine::instance()->m_qtStyle;
	
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
	
	s_scrollBarButtonSize = 0;
	
	int availableSize = 200 - qtStyle->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarGroove, scrollBar).width();
	s_scrollBarButtonCount = s_scrollBarHasForward1 + s_scrollBarHasBack1 + s_scrollBarHasForward2 + s_scrollBarHasBack2; // Evil
	if (s_scrollBarButtonCount != 0)
		s_scrollBarButtonSize = availableSize / s_scrollBarButtonCount;
}

QString RcProperties::gtkPaletteString(const QPalette& pal, const QString& bgfg, QPalette::ColorRole role)
{
	return bgfg + "[NORMAL] = "      + GtkQtUtilities::colorString(pal.color(QPalette::Inactive, role)) + "\n"
	     + bgfg + "[ACTIVE] = "      + GtkQtUtilities::colorString(pal.color(QPalette::Active, role))   + "\n"
	     + bgfg + "[PRELIGHT] = "    + GtkQtUtilities::colorString(pal.color(QPalette::Active, role))   + "\n"
	     + bgfg + "[INSENSITIVE] = " + GtkQtUtilities::colorString(pal.color(QPalette::Disabled, role)) + "\n";
}

void RcProperties::setColorProperties()
{
	// Normal
	mapColor("fg[NORMAL]",        QPalette::Active,   QPalette::WindowText);
	mapColor("bg[NORMAL]",        QPalette::Active,   QPalette::Window);
	mapColor("text[NORMAL]",      QPalette::Active,   QPalette::Text);
	mapColor("base[NORMAL]",      QPalette::Active,   QPalette::Base);
	
	// Active (on)
	mapColor("fg[ACTIVE]",        QPalette::Active,   QPalette::WindowText);
	mapColor("bg[ACTIVE]",        QPalette::Active,   QPalette::Window);
	mapColor("text[ACTIVE]",      QPalette::Active,   QPalette::Text);
	mapColor("base[ACTIVE]",      QPalette::Active,   QPalette::Base);
	
	// Mouseover
	mapColor("fg[PRELIGHT]",      QPalette::Active,   QPalette::WindowText);
	mapColor("bg[PRELIGHT]",      QPalette::Active,   QPalette::Highlight);
	mapColor("text[PRELIGHT]",    QPalette::Active,   QPalette::HighlightedText);
	mapColor("base[PRELIGHT]",    QPalette::Active,   QPalette::Base);
	
	// Selected
	mapColor("fg[SELECTED]",      QPalette::Active,   QPalette::HighlightedText);
	mapColor("bg[SELECTED]",      QPalette::Active,   QPalette::Highlight);
	mapColor("text[SELECTED]",    QPalette::Active,   QPalette::HighlightedText);
	mapColor("base[SELECTED]",    QPalette::Active,   QPalette::Highlight);
	
	// Disabled
	mapColor("fg[INSENSITIVE]",   QPalette::Disabled, QPalette::WindowText);
	mapColor("bg[INSENSITIVE]",   QPalette::Disabled, QPalette::Window);
	mapColor("text[INSENSITIVE]", QPalette::Disabled, QPalette::Text);
	mapColor("base[INSENSITIVE]", QPalette::Disabled, QPalette::Base);
	
	// Tooltips
	QColor tooltipBg(convertColor(kdeConfigValue("/share/config/kdeglobals", "Colors:Tooltip/BackgroundNormal", QToolTip::palette().color(QPalette::Active, QPalette::Base), true).second));
	QColor tooltipFg(convertColor(kdeConfigValue("/share/config/kdeglobals", "Colors:Tooltip/ForegroundNormal", QToolTip::palette().color(QPalette::Active, QPalette::Text), true).second));
	GtkQtUtilities::parseRcString("bg[NORMAL] = " + GtkQtUtilities::colorString(tooltipBg), "gtk-tooltip*", GtkQtUtilities::Widget);
	GtkQtUtilities::parseRcString("fg[NORMAL] = " + GtkQtUtilities::colorString(tooltipFg), "gtk-tooltip*", GtkQtUtilities::Widget);
	
	// Lists
	QString activeListColor = GtkQtUtilities::colorString(QApplication::palette().color(QPalette::Active, QPalette::HighlightedText));
	GtkQtUtilities::parseRcString("text[ACTIVE] = " + activeListColor, "GtkTreeView", GtkQtUtilities::Class);
	
	// Patch from Thomas Lübking:
	// Ask the Qt style for color settings of a (growing?) bunch of widget types.
	// We pass a dummy widget to QStyle::polish(QWidget*) with the widget's object
	// name reflecting (a faked) QObject::className alongside with the apps
	// default palette - if the style changes the palette, we add a gtk style
	// for the matching gtk widgets
	// NOTICE using a dummy widget objectName rather than a matching QWidget
	// here does not only allow us to skip couple of widget headers here for no
	// sense, but also is REQUIRED! because the Qt styles tend to do freaky things
	// on certain widget types, which we need to prevent...
	//
	// as the GTK and Qt palette models do not really fit, we'll translate
	// QPalette::Inactive -> GTK_STATE_NORMAL
	// QPalette::Active -> GTK_STATE_PRELIGHT
	// QPalette::Active -> GTK_STATE_ACTIVE
	// QPalette::Disabled -> GTK_STATE_INSENSITIVE
	
	QWidget dummy;
	
	INITDUMMY("QPushButton")
	GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "bg", QPalette::Button), "GtkButton", GtkQtUtilities::Class);
	GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "fg", QPalette::ButtonText), "*.GtkButton*GtkLabel", GtkQtUtilities::WidgetClass);
	
	INITDUMMY("QComboBox")
	GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "bg", QPalette::Button), "GtkOptionMenu", GtkQtUtilities::Class);

	INITDUMMY("QCheckBox")
	GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "bg", QPalette::Button), "GtkCheckButton", GtkQtUtilities::Class);
	
	INITDUMMY("QRadioButton")
	GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "bg", QPalette::Button), "GtkRadioButton", GtkQtUtilities::Class);
	
	INITDUMMY("QTabBar")
	if (dummy.palette() != QApplication::palette())
	{
		// SIC! GTK thinks all tabs but the active are active...
		GtkQtUtilities::parseRcString(
			"fg[ACTIVE] = "      + WCOLOR(Inactive, WindowText) + "\n" +
			"fg[NORMAL] = "      + WCOLOR(Active, WindowText) + "\n" +
			"fg[PRELIGHT] = "    + WCOLOR(Active, WindowText) + "\n" +
			"fg[INSENSITIVE] = " + WCOLOR(Disabled, WindowText),
			"*.GtkNotebook.GtkLabel", GtkQtUtilities::WidgetClass);
	}

	INITDUMMY("QMenu")
	if (dummy.palette() != QApplication::palette())
	{
		GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "bg", QPalette::Window), "GtkMenu", GtkQtUtilities::Class);
		GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "fg", QPalette::WindowText), "*.GtkMenuItem.GtkLabel", GtkQtUtilities::Class);
	}

	INITDUMMY("QMenuBar")
	if (dummy.palette() != QApplication::palette())
	{
		GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "bg", QPalette::Window), "GtkMenuBar", GtkQtUtilities::Class);
		GtkQtUtilities::parseRcString(gtkPaletteString(dummy.palette(), "fg", QPalette::WindowText), "*.GtkMenuBar.GtkMenuItem.GtkLabel", GtkQtUtilities::WidgetClass);
	}
}

void RcProperties::mapColor(const QString& name, QPalette::ColorGroup group, QPalette::ColorRole role)
{
	QPalette palette(QApplication::palette());
	QString color(GtkQtUtilities::colorString(palette.color(group, role)));
	
	GtkQtUtilities::parseRcString(name + " = " + color, "GtkWidget", GtkQtUtilities::Class);
}

PathAndValue RcProperties::kdeConfigValue(const QString& file, const QString& key, const QVariant& def, bool searchAllFiles)
{
	Q_FOREACH (QString searchPath, s_kdeSearchPaths)
	{
		QString fileName(searchPath + file);
		
		if (!QFile::exists(fileName))
			continue;
		
		QSettings settings(fileName, QSettings::IniFormat);
		if (!settings.contains(key))
		{
			if (searchAllFiles)
				continue;
			else
				return PathAndValue(fileName, def);
		}
		
		return PathAndValue(fileName, settings.value(key));
	}
	
	return PathAndValue(QString::null, def);
}

void RcProperties::initKdeSettings()
{
	// Initialise the list of search paths
	s_kdeSearchPaths.clear();
	
	QSettings settings("gtk-qt-engine", "gtk-qt-engine");
	QString kdeHome(getenv("KDEHOME"));
	QString kdeDirs(getenv("KDEDIRS"));
	QString kdeDir(getenv("KDEDIR"));
	
	if (!kdeHome.isEmpty())
		s_kdeSearchPaths << kdeHome;
	s_kdeSearchPaths << settings.value("KDELocalPrefix").toString();
	
	if (!kdeDirs.isEmpty())
		s_kdeSearchPaths << kdeDirs.split(':');
	if (!kdeDir.isEmpty())
		s_kdeSearchPaths << kdeDir;
	s_kdeSearchPaths << settings.value("KDEPrefix").toString();
	
	s_kdeSearchPaths << QDir::homePath() + "/.kde4";
	s_kdeSearchPaths << QDir::homePath() + "/.kde";
	s_kdeSearchPaths << "/usr/local";
	s_kdeSearchPaths << "/usr";
}

void RcProperties::setIconProperties()
{
	// Get the theme's directory and those of its parents
	QString iconThemeName(kdeConfigValue("/share/config/kdeglobals", "Icons/Theme", "oxygen", true).second.toString());
	traverseIconThemeDir(iconThemeName);
	
	GtkQtUtilities::parseRcString("pixmap_path \"" + s_iconThemeDirs.join(":") + "\"");
	
	QString iconMappings = "style \"KDE-icons\" {\n";
	iconMappings += doIconMapping("gtk-about", "actions/help-about.png");
	iconMappings += doIconMapping("gtk-add", "actions/list-add.png");
	iconMappings += doIconMapping("gtk-apply", "actions/dialog-ok-apply.png");
	iconMappings += doIconMapping("gtk-bold", "actions/format-text-bold.png");
	iconMappings += doIconMapping("gtk-cancel", "actions/dialog-cancel.png");
	iconMappings += doIconMapping("gtk-cdrom", "devices/media-optical.png");
	iconMappings += doIconMapping("gtk-clear", "actions/edit-clear.png");
	iconMappings += doIconMapping("gtk-close", "actions/dialog-close.png");
	iconMappings += doIconMapping("gtk-color-picker", "actions/color-picker.png");
	iconMappings += doIconMapping("gtk-copy", "actions/edit-copy.png");
	//iconMappings += doIconMapping("gtk-convert", "actions/gtk-convert.png"); ??
	iconMappings += doIconMapping("gtk-connect", "actions/network-connect.png");
	iconMappings += doIconMapping("gtk-cut", "actions/edit-cut.png");
	iconMappings += doIconMapping("gtk-delete", "actions/edit-delete.png");
	iconMappings += doIconMapping("gtk-dialog-authentication", "actions/document-decrypt.png");
	iconMappings += doIconMapping("gtk-dialog-error", "status/dialog-error.png");
	iconMappings += doIconMapping("gtk-dialog-info", "status/dialog-information.png");
	iconMappings += doIconMapping("gtk-dialog-question", "actions/help-contextual.png");
	iconMappings += doIconMapping("gtk-dialog-warning", "status/dialog-warning.png");
	iconMappings += doIconMapping("gtk-directory", "mimetypes/inode-directory.png");
	iconMappings += doIconMapping("gtk-disconnect", "actions/network-disconnect.png");
	iconMappings += doIconMapping("gtk-dnd", "mimetypes/application-x-zerosize.png");
	iconMappings += doIconMapping("gtk-dnd-multiple", "mimetypes/application-x-zerosize.png");
	iconMappings += doIconMapping("gtk-edit", "actions/edit-rename.png");  //2.6 
	iconMappings += doIconMapping("gtk-execute", "actions/system-run.png");
	iconMappings += doIconMapping("gtk-file", "mimetypes/application-x-zerosize.png");
	iconMappings += doIconMapping("gtk-find", "actions/edit-find.png");
	iconMappings += doIconMapping("gtk-find-and-replace", "actions/edit-find.png");
	iconMappings += doIconMapping("gtk-floppy", "devices/media-floppy.png");
	iconMappings += doIconMapping("gtk-fullscreen", "actions/view-fullscreen.png");
	iconMappings += doIconMapping("gtk-goto-bottom", "actions/go-bottom.png");
	iconMappings += doIconMapping("gtk-goto-first", "actions/go-first.png");
	iconMappings += doIconMapping("gtk-goto-last", "actions/go-last.png");
	iconMappings += doIconMapping("gtk-goto-top", "actions/go-top.png");
	iconMappings += doIconMapping("gtk-go-back", "actions/go-previous.png");
	iconMappings += doIconMapping("gtk-go-down", "actions/go-down.png");
	iconMappings += doIconMapping("gtk-go-forward", "actions/go-next.png");
	iconMappings += doIconMapping("gtk-go-up", "actions/go-up.png");
	iconMappings += doIconMapping("gtk-harddisk", "devices/drive-harddisk.png");
	iconMappings += doIconMapping("gtk-help", "actions/help-contents.png");
	iconMappings += doIconMapping("gtk-home", "actions/go-home.png");
	iconMappings += doIconMapping("gtk-indent", "actions/format-indent-more.png");
	iconMappings += doIconMapping("gtk-index", "actions/go-top.png");
	//iconMappings += doIconMapping("gtk-info", "??");
	iconMappings += doIconMapping("gtk-italic", "actions/format-text-italic.png");
	iconMappings += doIconMapping("gtk-jump-to", "actions/go-jump.png");
	iconMappings += doIconMapping("gtk-justify-center", "actions/format-justify-center.png");
	iconMappings += doIconMapping("gtk-justify-fill", "actions/format-justify-fill.png");
	iconMappings += doIconMapping("gtk-justify-left", "actions/format-justify-left.png");
	iconMappings += doIconMapping("gtk-justify-right", "actions/format-justify-right.png");
	iconMappings += doIconMapping("gtk-leave-fullscreen", "actions/view-fullscreen.png");
	iconMappings += doIconMapping("gtk-media-forward", "actions/media-seek-forward.png");
	iconMappings += doIconMapping("gtk-media-next", "actions/media-skip-forward.png");
	iconMappings += doIconMapping("gtk-media-pause", "actions/media-playback-pause.png");
	iconMappings += doIconMapping("gtk-media-previous", "actions/media-skip-backward.png");
	iconMappings += doIconMapping("gtk-media-record", "actions/media-record.png");
	iconMappings += doIconMapping("gtk-media-rewind", "actions/media-seek-backward.png");
	iconMappings += doIconMapping("gtk-media-stop", "actions/media-playback-stop.png");
	iconMappings += doIconMapping("gtk-missing-image", "mimetypes/unknown.png");
	iconMappings += doIconMapping("gtk-network", "places/folder-remote.png");
	iconMappings += doIconMapping("gtk-new", "actions/document-new.png");
	iconMappings += doIconMapping("gtk-no", "actions/dialog-close.png");
	iconMappings += doIconMapping("gtk-ok", "actions/dialog-ok.png");
	iconMappings += doIconMapping("gtk-open", "actions/document-open.png");
	//iconMappings += doIconMapping("gtk-orientation-landscape", "??");
	//iconMappings += doIconMapping("gtk-orientation-portrait", "??");
	//iconMappings += doIconMapping("gtk-orientation-reverse-landscape", "??");
	//iconMappings += doIconMapping("gtk-orientation-reverse-portrait", "??");
	iconMappings += doIconMapping("gtk-paste", "actions/edit-paste.png");
	iconMappings += doIconMapping("gtk-preferences", "categories/preferences-other.png");
	iconMappings += doIconMapping("gtk-print", "actions/document-print.png");
	iconMappings += doIconMapping("gtk-print-preview", "actions/document-print-preview.png");
	iconMappings += doIconMapping("gtk-properties", "actions/document-properties.png");
	iconMappings += doIconMapping("gtk-quit", "actions/application-exit.png");
	iconMappings += doIconMapping("gtk-redo", "actions/edit-redo.png");
	iconMappings += doIconMapping("gtk-refresh", "actions/view-refresh.png");
	iconMappings += doIconMapping("gtk-remove", "actions/list-remove.png");
	iconMappings += doIconMapping("gtk-revert-to-saved", "actions/document-revert.png");
	iconMappings += doIconMapping("gtk-save", "actions/document-save.png");
	iconMappings += doIconMapping("gtk-save-as", "actions/document-save-as.png");
	iconMappings += doIconMapping("gtk-select-all", "actions/edit-select-all.png");
	iconMappings += doIconMapping("gtk-select-color", "actions/color-picker.png");
	iconMappings += doIconMapping("gtk-select-font", "actions/format-font-size-more.png");
	//iconMappings += doIconMapping("gtk-sort-ascending", "??");
	//iconMappings += doIconMapping("gtk-sort-descending", "??");
	iconMappings += doIconMapping("gtk-spell-check", "actions/tools-check-spelling.png");
	iconMappings += doIconMapping("gtk-stop", "actions/process-stop.png");
	iconMappings += doIconMapping("gtk-strikethrough", "actions/format-text-strikethrough.png");
	iconMappings += doIconMapping("gtk-undelete", "actions/edit-undo.png");
	iconMappings += doIconMapping("gtk-underline", "actions/format-text-underline.png");
	iconMappings += doIconMapping("gtk-undo", "actions/edit-undo.png");
	iconMappings += doIconMapping("gtk-unindent", "actions/format-indent-less.png");
	iconMappings += doIconMapping("gtk-yes", "actions/dialog-ok.png");
	iconMappings += doIconMapping("gtk-zoom-100", "actions/zoom-original.png");
	iconMappings += doIconMapping("gtk-zoom-fit", "actions/zoom-fit-best.png");
	iconMappings += doIconMapping("gtk-zoom-in", "actions/zoom-in.png");
	iconMappings += doIconMapping("gtk-zoom-out", "actions/zoom-out.png");
	iconMappings += "} class \"GtkWidget\" style \"KDE-icons\"";
	
	GtkQtUtilities::parseRcString(iconMappings);
}

void RcProperties::traverseIconThemeDir(const QString& themeName)
{
	// TODO: Fix infinite recursion
	
	// Find the theme's path and get it's parent
	PathAndValue pathAndParents(kdeConfigValue("/share/icons/" + themeName + "/index.theme", "Icon Theme/Inherits", QVariant(QString::null), false));
	
	// Add this theme to the list
	QFileInfo themeIndex(pathAndParents.first);
	
	if (!themeIndex.exists()) // The theme wasn't found
		return;
	
	s_iconThemeDirs << themeIndex.path() + "/";

	// Do it again for any parent themes
	QStringList parents = pathAndParents.second.toString().split(',', QString::SkipEmptyParts);
	Q_FOREACH (QString parent, parents)
		traverseIconThemeDir(parent);
}

QString RcProperties::doIconMapping(const QString& stockName, const QString& path)
{
	bool has16 = false, has22 = false, has32 = false;

	Q_FOREACH (QString iconThemeDir, s_iconThemeDirs)
	{
		if (QFile::exists(iconThemeDir + "16x16/" + path))
			has16 = true;
		if (QFile::exists(iconThemeDir + "22x22/" + path))
			has22 = true;
		if (QFile::exists(iconThemeDir + "32x32/" + path))
			has32 = true;
	}

	if (!has16 && !has22 && !has32)
		return QString::null;
	
	QStringList lines;
	if (has22)
		lines << "{ \"22x22/" + path +"\", *, *, \"gtk-large-toolbar\" }";

	if (has32)
	{
		lines << "{ \"32x32/" + path +"\", *, *, \"gtk-dnd\" }";
		lines << "{ \"32x32/" + path +"\", *, *, \"gtk-dialog\" }";
	}

	if (has16)
	{
		lines << "{ \"16x16/" + path +"\", *, *, \"gtk-button\" }";
		lines << "{ \"16x16/" + path +"\", *, *, \"gtk-menu\" }";
		lines << "{ \"16x16/" + path +"\", *, *, \"gtk-small-toolbar\" }";
	}
	
	if (has22)
		lines << "{ \"22x22/" + path +"\" }";
	else if (has32)
		lines << "{ \"32x32/" + path +"\" }";
	else
		lines << "{ \"16x16/" + path +"\" }";
	
	
	return "stock[\"" + stockName + "\"] = {" + lines.join(",") + "}\n";
}

QColor RcProperties::convertColor(const QVariant& variant)
{
	if (variant.value<QColor>().isValid())
		return variant.value<QColor>();
	
	// KDE seems to store colors in its config files as QStringList(r, g, b), which you can't convert
	// with QVariant::value<QColor>
	QStringList list(variant.toStringList());
	if (list.count() == 3)
		return QColor(list[0].toInt(), list[1].toInt(), list[2].toInt());
	else if (list.count() == 4)
		return QColor(list[0].toInt(), list[1].toInt(), list[2].toInt(), list[3].toInt());
	return QColor();
}

