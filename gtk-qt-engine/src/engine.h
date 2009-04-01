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

#ifndef ENGINE_H
#define ENGINE_H

// Note: All Qt includes should go before the GTK ones to avoid name clashes
#include <QWidget>
#include <QtDebug>
#include <QPainter>
#include <QApplication>
#include <QPushButton>
#include <QStyleOption>
#include <QCheckBox>
#include <QRadioButton>
#include <QTabBar>
#include <QTabWidget>
#include <QLineEdit>
#include <QMenu>
#include <QComboBox>
#include <QSlider>
#include <QScrollBar>

#include <gtk/gtk.h>
#include <gtk/gtkstyle.h>
#include <gdk/gdkx.h>

// The GTK headers suck
#ifdef None
#undef None
#endif

class RcProperties;

class Engine
{
	friend class RcProperties;

public:
	enum Workaround
	{
		None = 0x00,
		OpenOffice = 0x01,
		Mozilla = 0x02,
		Eclipse = 0x04
	};
	
	~Engine();
	
	inline static Engine* instance();
	
	inline bool isEnabled() const { return m_enabled; }
	inline bool isDebug() const { return m_debug; }
	inline QFlags<Workaround> workaround() const { return m_workaround; }
	inline bool usingWorkaround(Workaround workaround) const { return m_workaround & workaround; }
	
	inline QStyle* style() const { return m_qtStyle; }
	
	inline void setWindow(GdkWindow* window, GtkStyle* style, GtkStateType state);
	inline bool setRect(int x, int y, int w, int h);
	inline void setHasFocus(bool hasFocus) { m_hasFocus = hasFocus; }
	inline void setFillPixmap(const QPixmap& fillPixmap);
	inline void unsetFillPixmap();
	
	inline const QPixmap menuBackground() const { return *m_menuBackground; }
	
	void drawButton(bool defaultButton);
	void drawCheckBox(bool checked);
	void drawRadioButton(bool checked);
	void drawTab(int tabCount, int selectedTab, int tab, bool upsideDown);
	void drawTabFrame();
	void drawLineEdit(bool editable);
	void drawFrame(int type);
	void drawArrow(GtkArrowType direction);
	void drawListHeader();
	void drawMenuBarItem();
	void drawMenu();
	void drawMenuItem(int type);
	void drawMenuCheck();
	void drawHLine();
	void drawVLine();
	void drawComboBox();
	void drawProgressBar(GtkProgressBarOrientation orientation, double percentage);
	void drawProgressChunk();
	void drawSpinButton(int direction);
	void drawSlider(GtkAdjustment* adj, GtkOrientation orientation, int inverted);
	void drawScrollBar(GtkOrientation orientation, GtkAdjustment* adj);
	void drawScrollBarSlider(GtkOrientation orientation);
	void drawSplitter(GtkOrientation orientation);
	
private:
	Engine();
	void setupOption(QStyleOption* option, const QPalette& palette) const;
	void initMenuBackground();
	
	// Singleton
	static Engine* s_instance;
	
	// Global GTK-Qt Engine state
	bool m_enabled;
	bool m_debug;
	QFlags<Workaround> m_workaround;
	QStyle* m_qtStyle;
	
	// Current GTK state
	GdkWindow* m_window;
	GtkStyle* m_style;
	GtkStateType m_state;
	QPoint m_topLeft;
	QSize m_size;
	bool m_hasFocus;
	QPixmap* m_fillPixmap;
	
	// Dummy widgets
	QWidget* m_dummyWidget;
	QPushButton* m_dummyButton;
	QCheckBox* m_dummyCheckBox;
	QRadioButton* m_dummyRadioButton;
	QTabBar* m_dummyTabBar;
	QTabWidget* m_dummyTabWidget;
	QLineEdit* m_dummyLineEdit;
	QMenu* m_dummyMenu;
	QComboBox* m_dummyComboBox;
	QSlider* m_dummySlider;
	QScrollBar* m_dummyScrollBar;
	
	QPixmap* m_menuBackground;
};

Engine* Engine::instance()
{
	if (s_instance == NULL)
		new Engine();
	return s_instance;
}

void Engine::setWindow(GdkWindow* window, GtkStyle* style, GtkStateType state)
{
	m_window = window;
	m_style = style;
	m_state = state;
}

bool Engine::setRect(int x, int y, int w, int h)
{
	if (x < 0 || y < 0 || w <= 1 || h <= 1)
		return false;
	
	m_topLeft = QPoint(x, y);
	m_size = QSize(w, h);
	
	return true;
}

void Engine::setFillPixmap(const QPixmap& fillPixmap)
{
	delete m_fillPixmap;
	m_fillPixmap = new QPixmap(fillPixmap);
}

void Engine::unsetFillPixmap()
{
	delete m_fillPixmap;
	m_fillPixmap = NULL;
}

#endif
