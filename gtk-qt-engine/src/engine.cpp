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

#include "engine.h"
#include "dummyeventdispatcher.h"
#include "dummyapplication.h"
#include "utilities.h"
#include "colormapper.h"
#include "rcproperties.h"

// Creates an offscreen pixmap of the right size and opens a QPainter on it.
#define GTK_QT_SETUP(OptionClassName, isButton) \
	QPalette palette(ColorMapper::mapGtkToQt(m_style, m_state, isButton)); \
	QPixmap offscreenPixmap; \
	if (m_fillPixmap) \
		offscreenPixmap = QPixmap(*m_fillPixmap); \
	else \
		offscreenPixmap = QPixmap(m_size); \
	QPalette::ColorGroup colorGroup = (m_state == GTK_STATE_INSENSITIVE) ? QPalette::Inactive : QPalette::Normal; \
	if (!m_fillPixmap) \
		offscreenPixmap.fill(palette.color(colorGroup, QPalette::Window)); \
	QPainter p(&offscreenPixmap); \
	\
	OptionClassName option; \
	setupOption(&option, palette);

#define GTK_QT_END_PAINTING \
	p.end();

#define GTK_QT_DRAW_PIXMAP \
	GdkPixmap* gdkPix = gdk_pixmap_foreign_new(offscreenPixmap.handle()); \
	gdk_draw_drawable(m_window, m_style->bg_gc[m_state], gdkPix, 0, 0, m_topLeft.x(), m_topLeft.y(), m_size.width(), m_size.height()); \
	g_object_unref(gdkPix); \
	unsetFillPixmap();

// Copies the offscreen pixmap back onto the GTK widget
#define GTK_QT_FINISH \
	GTK_QT_END_PAINTING \
	GTK_QT_DRAW_PIXMAP



static int dummyErrorHandler(Display*, XErrorEvent*)
{
	return 0;
}

static int dummyIOErrorHandler(Display*)
{
	return 0;
}



Engine* Engine::s_instance = NULL;

Engine::Engine()
	: m_enabled(true),
	  m_debug(false),
	  m_workaround(None),
	  m_window(NULL),
	  m_style(NULL),
	  m_fillPixmap(NULL)
{
	// If this environment variable is set then lots of debugging info will be printed to the console
	m_debug = (getenv("GTK_QT_ENGINE_DEBUG") != NULL) ? 1 : 0;
	
	s_instance = this; // Don't put GTK_QT_DEBUG* calls before this line

	GTK_QT_DEBUG_FUNC
	
	// Get argv[0] so we can work out which hacks to enable :(
	QString commandLine(GtkQtUtilities::getCommandLine());
	GTK_QT_DEBUG("Command line:" << commandLine)
	
	if (commandLine.contains("mozilla") || commandLine.contains("firefox"))
		m_workaround |= Mozilla;
	else if (commandLine.contains("soffice.bin") ||
	         commandLine.contains("swriter.bin") ||
	         commandLine.contains("scalc.bin") ||
	         commandLine.contains("sdraw.bin") ||
	         commandLine.contains("spadmin.bin") ||
	         commandLine.contains("simpress.bin"))
		m_workaround |= OpenOffice;
	else if (commandLine.contains("eclipse"))
		m_workaround |= Eclipse;
	
	// We don't want to enable the theme engine in the session manager's process.  Bad things will happen
	// Disable the theme engine in various window managers.  Seems to cause them to not start.  Not sure why
	QString sessionEnv(getenv("SESSION_MANAGER"));
	if (sessionEnv.endsWith(QString::number(getpid())) ||
	    commandLine.contains("gnome-wm") ||
	    commandLine.contains("metacity") ||
	    commandLine.contains("xfwm4") ||
	    commandLine.contains("gnome-settings-daemon") ||
	    commandLine.contains("gnome-panel"))
	{
		m_enabled = false;
		qDebug() << "Disabling the GTK-Qt Theme Engine for" << commandLine;
	}
	
	// Also disable the engine if the user has requested it
	if (getenv("GTK_QT_ENGINE_DISABLE") != NULL)
	{
		m_enabled = false;
		qDebug() << "Disabling the GTK-Qt Theme Engine as requested by GTK_QT_ENGINE_DISABLE";
	}
	
	if (!m_enabled)
		return;
	
	// Styles can check for this if they need to enable workarounds for gtk-qt
	setenv("GTK_QT_ENGINE_ACTIVE", "1", true);
	
	// Create a QApplication if there isn't one already
	if (QApplication::instance() == NULL)
	{
		// The QApplication constructor sets its own X11 error handlers.
		// We want to save the current (GTK) ones so we can restore them after
		int (*originalErrorHandler)(Display*, XErrorEvent*) = XSetErrorHandler(dummyErrorHandler);
		int (*originalIOErrorHandler)(Display*) = XSetIOErrorHandler(dummyIOErrorHandler);
	
		// Stop Qt from integrating with the Glib event loop and stealing events from GTK
		new DummyEventDispatcher();
		
		// Construct our Qt application - needed to use QPainter
		// The dummy application has empty implementations for Qt's session management functions
		// We don't need them, and they try to use Qt's event loop (which doesn't work)
		new DummyApplication(GDK_DISPLAY());
		
		// Restore GTK's error handlers
		XSetErrorHandler(originalErrorHandler);
		XSetIOErrorHandler(originalIOErrorHandler);
	}
	
	// Get a pointer to the default Qt style
	m_qtStyle = QApplication::style();
	
	// Create some Qt widgets here so we don't have to recreate them in every draw* function
	m_dummyWidget = new QWidget(0);
	m_dummyButton = new QPushButton(m_dummyWidget);
	m_dummyCheckBox = new QCheckBox(m_dummyWidget);
	m_dummyRadioButton = new QRadioButton(m_dummyWidget);
	m_dummyTabBar = new QTabBar(m_dummyWidget);
	m_dummyTabWidget = new QTabWidget(m_dummyWidget);
	m_dummyLineEdit = new QLineEdit(m_dummyWidget);
	m_dummyMenu = new QMenu(m_dummyWidget);
	m_dummyComboBox = new QComboBox(m_dummyWidget);
	m_dummySlider = new QSlider(m_dummyWidget);
	m_dummyScrollBar = new QScrollBar(m_dummyWidget);
	
	// Initialize various GTK RC properties
	RcProperties::setRcProperties();
	
	// Grab the menu background image
	initMenuBackground();
}

Engine::~Engine()
{
	GTK_QT_DEBUG_FUNC
	
	if (!m_enabled)
		return;
	
	delete m_dummyWidget; // This will also clean up all the other dummy widgets which are its children
	delete m_menuBackground;
	
	delete QApplication::instance();
	
	s_instance = NULL;
}

void Engine::initMenuBackground()
{
	GTK_QT_DEBUG_FUNC
	// Get the menu background image
	m_menuBackground = new QPixmap(1024, 25); // Meh
	m_menuBackground->fill(QApplication::palette().color(QPalette::Active, QPalette::Window));
	QPainter p(m_menuBackground);
	
	QStyleOptionMenuItem option;
	m_state = GTK_STATE_NORMAL;
	setupOption(&option, QApplication::palette());
	
	m_qtStyle->drawControl(QStyle::CE_MenuItem, &option, &p);
}

void Engine::setupOption(QStyleOption* option, const QPalette& palette) const
{
	option->direction = m_dummyWidget->layoutDirection();
	option->rect = QRect(QPoint(0, 0), m_size);
	option->palette = palette;
	option->fontMetrics = m_dummyWidget->fontMetrics();
	
	// Setup state
	option->state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Raised;
	switch (m_state)
	{
		case GTK_STATE_ACTIVE:
			option->state &= ~QStyle::State_Raised;
			option->state |= QStyle::State_Sunken;
			break;
		case GTK_STATE_PRELIGHT:
			option->state |= QStyle::State_MouseOver;
			break;
		case GTK_STATE_SELECTED:
			option->state |= QStyle::State_HasFocus;
			break;
		case GTK_STATE_INSENSITIVE:
			option->state &= ~QStyle::State_Enabled;
			break;
		case GTK_STATE_NORMAL:
		default:
			break;
	}
	
	if (m_hasFocus)
		option->state |= QStyle::State_HasFocus;
}

void Engine::drawButton(bool defaultButton)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionButton, true)
	
	// Handle default buttons (ones that get activated when you press enter)
	option.features = defaultButton ? QStyleOptionButton::DefaultButton : QStyleOptionButton::None;
	
	m_qtStyle->drawControl(QStyle::CE_PushButton, &option, &p, m_dummyButton);
	
	GTK_QT_FINISH
}

void Engine::drawCheckBox(bool checked)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionButton, true)
	
	// Handle checked state
	m_dummyCheckBox->setChecked(checked);
	option.state |= checked ? QStyle::State_On : QStyle::State_Off;
	
	// Without this, clicking a checkbox looks weird as you lose the mouseover highlight for a second
	if (m_state == GTK_STATE_ACTIVE)
		option.state |= QStyle::State_MouseOver;
	
	m_qtStyle->drawControl(QStyle::CE_CheckBox, &option, &p, m_dummyCheckBox);
	
	GTK_QT_FINISH
}

void Engine::drawRadioButton(bool checked)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionButton, true)
	
	// Handle checked state
	m_dummyRadioButton->setChecked(checked);
	option.state |= checked ? QStyle::State_On : QStyle::State_Off;
	
	// Without this, clicking a radiobutton looks weird as you lose the mouseover highlight for a second
	if (m_state == GTK_STATE_ACTIVE)
		option.state |= QStyle::State_MouseOver;
	
	m_qtStyle->drawControl(QStyle::CE_RadioButton, &option, &p, m_dummyRadioButton);
	
	GTK_QT_FINISH
}

void Engine::drawTab(int tabCount, int selectedTab, int tab, bool upsideDown)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionTabV2, false)
	option.cornerWidgets = QStyleOptionTab::NoCornerWidgets;
	option.shape = QTabBar::RoundedNorth;
	
	if (tabCount == -1)
	{
		// Tab information was unavailable (eg in Firefox), so just draw a middle tab
		option.position = QStyleOptionTab::Middle;
		option.selectedPosition = QStyleOptionTab::NotAdjacent;
	}
	else
	{
		if (tabCount == 1)
			option.position = QStyleOptionTab::OnlyOneTab;
		else if (tab == 0)
			option.position = QStyleOptionTab::Beginning;
		else if (tab == tabCount - 1)
			option.position = QStyleOptionTab::End;
		else
			option.position = QStyleOptionTab::Middle;
		
		if (selectedTab == tab + 1)
			option.selectedPosition = QStyleOptionTab::NextIsSelected;
		else if (selectedTab == tab - 1)
			option.selectedPosition = QStyleOptionTab::PreviousIsSelected;
		else
			option.selectedPosition = QStyleOptionTab::NotAdjacent;
	}
	
	// All GTK tabs are drawn in the ACTIVE state except the active tab which is drawn in the NORMAL state...
	if (m_state == GTK_STATE_NORMAL)
		option.state |= QStyle::State_Selected;
	
	m_qtStyle->drawControl(QStyle::CE_TabBarTab, &option, &p, m_dummyTabBar);
	
	GTK_QT_END_PAINTING
	if (upsideDown)
	{
		QMatrix m;
		m.scale(1, -1);
		offscreenPixmap = offscreenPixmap.transformed(m);
	}
	GTK_QT_DRAW_PIXMAP
}

void Engine::drawTabFrame()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionTabWidgetFrame, false)
	option.shape = QTabBar::RoundedNorth;
	option.leftCornerWidgetSize = QSize(0, 0);
	option.rightCornerWidgetSize = QSize(0, 0);
	option.tabBarSize = QSize(0, 0);
	
	m_qtStyle->drawPrimitive(QStyle::PE_FrameTabWidget, &option, &p, m_dummyTabWidget);
	
	GTK_QT_FINISH
}

void Engine::drawLineEdit(bool editable)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionFrameV2, false)
	option.lineWidth = m_qtStyle->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, m_dummyLineEdit);
	option.midLineWidth = 0;
	option.state |= QStyle::State_Sunken;
	option.state &= ~QStyle::State_Raised;
	if (!editable)
		option.state |= QStyle::State_ReadOnly;
	
	m_qtStyle->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &p, m_dummyLineEdit);
	
	GTK_QT_FINISH
}

void Engine::drawFrame(int type)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionFrameV2, false)
	option.state |= QStyle::State_Sunken;
	option.state &= ~QStyle::State_Raised;
	
	QStyle::PrimitiveElement element;
	switch (type)
	{
		case 0:  element = QStyle::PE_FrameGroupBox; break;
		case 1:
		default: element = QStyle::PE_Frame;         break;
	}
	
	m_qtStyle->drawPrimitive(element, &option, &p, m_dummyWidget);
	
	GTK_QT_FINISH
}

void Engine::drawArrow(GtkArrowType direction)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOption, false)
	
	QStyle::PrimitiveElement element;
	switch (direction)
	{
		case GTK_ARROW_UP:    element = QStyle::PE_IndicatorArrowUp;    break;
		case GTK_ARROW_LEFT:  element = QStyle::PE_IndicatorArrowLeft;  break;
		case GTK_ARROW_RIGHT: element = QStyle::PE_IndicatorArrowRight; break;
		case GTK_ARROW_DOWN:
		default:              element = QStyle::PE_IndicatorArrowDown;  break;
	}
	
	m_qtStyle->drawPrimitive(element, &option, &p);
	
	GTK_QT_FINISH
}

void Engine::drawListHeader()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionHeader, false)
	option.orientation = Qt::Horizontal;
	option.section = 1;
	option.position = QStyleOptionHeader::Middle;
	option.selectedPosition = QStyleOptionHeader::NotAdjacent;
	option.sortIndicator = QStyleOptionHeader::None;
	option.state &= ~QStyle::State_Raised;
	option.state |= QStyle::State_Sunken;
	
	m_qtStyle->drawControl(QStyle::CE_Header, &option, &p, m_dummyButton);
	
	GTK_QT_FINISH
}

void Engine::drawMenuBarItem()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionMenuItem, false)
	option.state |= QStyle::State_Sunken | QStyle::State_Selected;
	option.state &= ~QStyle::State_Raised;
	
	m_qtStyle->drawControl(QStyle::CE_MenuBarItem, &option, &p);
	
	GTK_QT_FINISH
}

void Engine::drawMenu()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionFrame, false)
	option.state = QStyle::State_None;
	option.lineWidth = style()->pixelMetric(QStyle::PM_MenuPanelWidth);
	option.midLineWidth = 0;
	
	m_qtStyle->drawPrimitive(QStyle::PE_FrameMenu, &option, &p, m_dummyMenu);
	
	GTK_QT_FINISH
}

void Engine::drawMenuItem(int type)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionMenuItem, false)
	option.state |= QStyle::State_Sunken | QStyle::State_Selected;
	option.state &= ~QStyle::State_Raised;
	
	switch (type)
	{
		case 0: option.menuItemType = QStyleOptionMenuItem::Normal;    break;
		case 1: option.menuItemType = QStyleOptionMenuItem::Separator; break;
		case 2: option.menuItemType = QStyleOptionMenuItem::TearOff;   break;
	}
	
	m_qtStyle->drawControl(QStyle::CE_MenuItem, &option, &p);
	
	GTK_QT_FINISH
}

void Engine::drawMenuCheck()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOption, false)
	
	m_qtStyle->drawPrimitive(QStyle::PE_IndicatorMenuCheckMark, &option, &p, m_dummyMenu);
	
	GTK_QT_FINISH
}

void Engine::drawHLine()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOption, false)
	
	qDrawShadeLine(&p, 0, 0, m_size.width(), 0, option.palette);
	
	GTK_QT_FINISH
}

void Engine::drawVLine()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOption, false)
	
	qDrawShadeLine(&p, 0, 0, 0, m_size.height(), option.palette);
	
	GTK_QT_FINISH
}

void Engine::drawComboBox()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionComboBox, true)
	
	m_qtStyle->drawComplexControl(QStyle::CC_ComboBox, &option, &p, m_dummyComboBox);
	
	GTK_QT_FINISH
}

void Engine::drawProgressBar(GtkProgressBarOrientation orientation, double percentage)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionProgressBarV2, false)
	option.maximum = 10000;
	option.minimum = 0;
	option.progress = int(percentage * 10000);
	
	switch (orientation)
	{
		case GTK_PROGRESS_LEFT_TO_RIGHT:
		case GTK_PROGRESS_RIGHT_TO_LEFT:
			option.orientation = Qt::Horizontal;
			break;
		case GTK_PROGRESS_BOTTOM_TO_TOP:
		case GTK_PROGRESS_TOP_TO_BOTTOM:
			option.orientation = Qt::Vertical;
			break;
	}
	
	m_qtStyle->drawControl(QStyle::CE_ProgressBarGroove, &option, &p);
	
	GTK_QT_FINISH
}

void Engine::drawProgressChunk()
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionProgressBarV2, false)
	option.maximum = 10000;
	option.minimum = 0;
	option.progress = 10000;
	
	m_qtStyle->drawControl(QStyle::CE_ProgressBarContents, &option, &p);
	
	GTK_QT_FINISH
}

void Engine::drawSpinButton(int direction)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionSpinBox, false)
	option.frame = false;
	
	QStyle::PrimitiveElement element;
	switch (direction)
	{
		case 0:  element = QStyle::PE_IndicatorSpinUp;   break;
		case 1:
		default: element = QStyle::PE_IndicatorSpinDown; break;
	}
	
	m_qtStyle->drawPrimitive(element, &option, &p);
	
	GTK_QT_FINISH
}

void Engine::drawSlider(GtkAdjustment* adj, GtkOrientation orientation, int inverted)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionSlider, false)
	option.minimum = 0;
	option.maximum = 10000;
	option.orientation = (orientation == GTK_ORIENTATION_HORIZONTAL) ? Qt::Horizontal : Qt::Vertical;
	
	if (!inverted) // Normal sliders
		option.sliderValue = int((adj->value-adj->lower) / (adj->upper-adj->lower) * 10000.0);
	else // Inverted sliders... where max is at the left/top and min is at the right/bottom
		option.sliderValue = 10000 - int((adj->value-adj->lower) / (adj->upper-adj->lower) * 10000.0);
	
	option.sliderPosition = option.sliderValue;
	
	m_qtStyle->drawComplexControl(QStyle::CC_Slider, &option, &p, m_dummySlider);
	
	GTK_QT_FINISH
}

void Engine::drawScrollBar(GtkOrientation orientation, GtkAdjustment* adj)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionSlider, false)
	option.minimum = int(adj->lower);
	option.maximum = int(adj->upper - adj->page_size);
	option.singleStep = int(adj->step_increment);
	option.pageStep = int(adj->page_increment);
	option.sliderValue = int(adj->value);
	option.sliderPosition = int(adj->value);
	option.orientation = (orientation == GTK_ORIENTATION_HORIZONTAL) ? Qt::Horizontal : Qt::Vertical;
	
	if (usingWorkaround(Mozilla))
		option.subControls &= ~QStyle::SC_ScrollBarSlider;
	
	// Fixes a divide by 0 error in QCommonStyle if the range (upper-lower) is 0
	if (option.maximum <= option.minimum)
		option.maximum = option.minimum + 1;
	
	// KStyle looks at this instead of option.orientation
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		option.state |= QStyle::State_Horizontal;
	
	m_qtStyle->drawComplexControl(QStyle::CC_ScrollBar, &option, &p, m_dummyScrollBar);
	
	GTK_QT_FINISH
}

void Engine::drawScrollBarSlider(GtkOrientation orientation)
{
	// Make the drawing area bigger to accomodate the buttons on either end of the slider
	int extraSpace = RcProperties::scrollBarButtonCount() * RcProperties::scrollBarButtonSize();
	QSize oldSize = m_size;
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		m_size = QSize(m_size.width() + extraSpace, m_size.height());
	else
		m_size = QSize(m_size.width(), m_size.height() + extraSpace);

	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOptionSlider, false)
	option.minimum = 0;
	option.maximum = 1;
	option.singleStep = 10000;
	option.pageStep = 10000;
	option.orientation = (orientation == GTK_ORIENTATION_HORIZONTAL) ? Qt::Horizontal : Qt::Vertical;
	option.subControls = QStyle::SC_ScrollBarSlider;
	
	if (m_state == GTK_STATE_PRELIGHT)
		option.activeSubControls = QStyle::SC_ScrollBarSlider;
	
	// KStyle looks at this instead of option.orientation
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		option.state |= QStyle::State_Horizontal;
	
	m_qtStyle->drawComplexControl(QStyle::CC_ScrollBar, &option, &p, m_dummyScrollBar);
	
	GTK_QT_END_PAINTING
	
	// Figure out where the slider is.  Hope this rect is the same size as oldSize
	QRect sliderRect = m_qtStyle->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, m_dummyScrollBar);
	if (sliderRect.width() < 0 || sliderRect.height() < 0) // Firefox again :(
		return;
	
	// Get rid of the buttons so we're left with just the scrollbar slider of the size we were asked to draw
	offscreenPixmap = offscreenPixmap.copy(sliderRect);
	
	GTK_QT_DRAW_PIXMAP
}

void Engine::drawSplitter(GtkOrientation orientation)
{
	GTK_QT_DEBUG_FUNC
	GTK_QT_SETUP(QStyleOption, false)
	
	// By "horizontal", GTK means "bar goes from left to right" and Qt means "the two panes are to the left and right of the bar"
	if (orientation != GTK_ORIENTATION_HORIZONTAL)
		option.state |= QStyle::State_Horizontal;
	
	m_qtStyle->drawControl(QStyle::CE_Splitter, &option, &p, m_dummyWidget);
	
	GTK_QT_FINISH
}
