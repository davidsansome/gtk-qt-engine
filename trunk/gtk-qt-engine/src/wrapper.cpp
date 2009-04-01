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

#include "colormapper.h"
#include "engine.h"
#include "utilities.h"

#include <QColor>
#include <QMap>

#include "wrapper.h"
#include "qt_style.h"

#define GTK_QT_SETUP_STATE \
	if (!s_engine->isEnabled() || !s_engine->setRect(x, y, w, h)) \
	{ \
		s_engine->unsetFillPixmap(); \
		return; \
	} \
	s_engine->setWindow(window, style, state); \
	s_engine->setHasFocus(hasFocus); \

Engine* s_engine;


void setFillPixmap(GdkPixbuf* buf)
{
	if (!s_engine->isEnabled())
		return;
	
	// This code isn't very robust.  It doesn't handle depths other than 24 bits.
	int depth = gdk_pixbuf_get_n_channels(buf) * gdk_pixbuf_get_bits_per_sample(buf);
	int width = gdk_pixbuf_get_width(buf);
	int height = gdk_pixbuf_get_height(buf);
	int excess = gdk_pixbuf_get_rowstride(buf) - (width*3);
	
	if (depth != 24)
		return;
	
	QImage fillImage(width, height, QImage::Format_RGB32);
	
	uchar* source = gdk_pixbuf_get_pixels(buf);
	QRgb* dest = (QRgb*) fillImage.bits();
	
	for (int y=0 ; y<height ; y++)
	{
		for (int x=0 ; x<width ; x++)
		{
			*dest = qRgb(source[0], source[1], source[2]);
			
			dest ++;
			source += 3;
		}
		source += excess;
	}
	
	s_engine->setFillPixmap(QPixmap::fromImage(fillImage));
}


void gtkQtInit()
{
	s_engine = Engine::instance();
}

void gtkQtDestroy()
{
	delete s_engine;
	s_engine = NULL;
}

int gtkQtDebug()
{
	return Engine::instance()->isDebug() ? 1 : 0;
}

int gtkQtOpenOfficeHack()
{
	return Engine::instance()->usingWorkaround(Engine::OpenOffice) ? 1 : 0;
}


void drawButton(GTK_QT_WRAPPER_ARGS, int defaultButton)
{
	GTK_QT_SETUP_STATE
	s_engine->drawButton(defaultButton);
}

void drawToolbar(GTK_QT_WRAPPER_ARGS)
{
	// TODO
}

void drawMenubar(GTK_QT_WRAPPER_ARGS)
{
	// TODO
}

void drawTab(GTK_QT_WRAPPER_ARGS, int tabCount, int selectedTab, int tab, int upsideDown)
{
	if (selectedTab != tab)
	{
		// GTK draws the active tab bigger than the others.  We correct this by drawing other tabs bigger.
		y -= style->ythickness;
		h += style->ythickness;
	}
	
	GTK_QT_SETUP_STATE
	s_engine->drawTab(tabCount, selectedTab, tab, upsideDown);
}

void drawVLine(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawVLine();
}

void drawHLine(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawHLine();
}

void drawLineEdit(GTK_QT_WRAPPER_ARGS, int editable)
{
	// Lineedit drawing is split into two functions.  drawLineEditFrame is called first with the whole area, then drawLineEdit
	// is called with just the interior.  It is also clipped to the interior area so we can't overdraw.
	x -= style->xthickness;
	y -= style->ythickness;
	w += style->xthickness * 2;
	h += style->ythickness * 2;
	
	GTK_QT_SETUP_STATE
	s_engine->drawLineEdit(editable);
}

void drawLineEditFrame(GTK_QT_WRAPPER_ARGS, int editable)
{
	GTK_QT_SETUP_STATE
	s_engine->drawLineEdit(editable);
}

void drawFrame(GTK_QT_WRAPPER_ARGS, int type)
{
	GTK_QT_SETUP_STATE
	s_engine->drawFrame(type);
}

void drawComboBox(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawComboBox();
}

void drawCheckBox(GTK_QT_WRAPPER_ARGS, int checked)
{
	GTK_QT_SETUP_STATE
	s_engine->drawCheckBox(checked);
}

void drawMenuCheck(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawMenuCheck();
}

void drawRadioButton(GTK_QT_WRAPPER_ARGS, int checked)
{
	GTK_QT_SETUP_STATE
	s_engine->drawRadioButton(checked);
}


void drawScrollBarSlider(GTK_QT_WRAPPER_ARGS, GtkOrientation orientation)
{
	if (s_engine->usingWorkaround(Engine::Mozilla))
	{
		GTK_QT_SETUP_STATE
		s_engine->drawScrollBarSlider(orientation);
	}
}

void drawScrollBar(GTK_QT_WRAPPER_ARGS, GtkOrientation orientation, GtkAdjustment* adj)
{
	GTK_QT_SETUP_STATE
	s_engine->drawScrollBar(orientation, adj);
}

void drawToolButton(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawButton(false);
}

void drawMenuBarItem(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawMenuBarItem();
}

void drawMenuItem(GTK_QT_WRAPPER_ARGS, int type)
{
	GTK_QT_SETUP_STATE
	s_engine->drawMenuItem(type);
}

void drawSplitter(GTK_QT_WRAPPER_ARGS, GtkOrientation orientation)
{
	GTK_QT_SETUP_STATE
	s_engine->drawSplitter(orientation);
}

void drawTabFrame(GTK_QT_WRAPPER_ARGS)
{
	if (!s_engine->isEnabled())
		return;
	
	int overlap = s_engine->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap);
	y -= overlap;
	h += overlap;
	
	GTK_QT_SETUP_STATE
	s_engine->drawTabFrame();
}

void drawMenu(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawMenu();
}

void drawProgressChunk(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawProgressChunk();
}

void drawProgressBar(GTK_QT_WRAPPER_ARGS, GtkProgressBarOrientation orientation, double percentage)
{
	GTK_QT_SETUP_STATE
	s_engine->drawProgressBar(orientation, percentage);
}

void drawSlider(GTK_QT_WRAPPER_ARGS, GtkAdjustment* adj, GtkOrientation orientation, int inverted)
{
	GTK_QT_SETUP_STATE
	s_engine->drawSlider(adj, orientation, inverted);
}

void drawSpinButton(GTK_QT_WRAPPER_ARGS, int direction)
{
	GTK_QT_SETUP_STATE
	s_engine->drawSpinButton(direction);
}

void drawListHeader(GTK_QT_WRAPPER_ARGS)
{
	GTK_QT_SETUP_STATE
	s_engine->drawListHeader();
}

void drawSquareButton(GTK_QT_WRAPPER_ARGS)
{
	
}

void drawArrow(GTK_QT_WRAPPER_ARGS, GtkArrowType direction)
{
	GTK_QT_SETUP_STATE
	s_engine->drawArrow(direction);
}

GdkGC* alternateBackgroundGc(GtkStyle* style, int enabled)
{
	static GdkGC* enabledGc = NULL;
	static GdkGC* disabledGc = NULL;
	GdkGC** gc = enabled ? &enabledGc : &disabledGc;
	
	if (*gc == NULL)
	{
		QColor color = QApplication::palette().color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::AlternateBase);
		
		GdkGCValues gc_values;
		GdkGCValuesMask gc_values_mask;
		gc_values_mask = GDK_GC_FOREGROUND;
		gc_values.foreground = GtkQtUtilities::convertColor(color, style);
		
		*gc = (GdkGC*) gtk_gc_get(style->depth, style->colormap, &gc_values, gc_values_mask);
	}
	
	return *gc;
}

GdkPixmap* menuBackground()
{
	if (!s_engine->isEnabled())
		return NULL;
	
	static GdkPixmap* ret = NULL;
	if (ret == NULL)
		ret = gdk_pixmap_foreign_new(s_engine->menuBackground().handle());
	
	return ret;
}
