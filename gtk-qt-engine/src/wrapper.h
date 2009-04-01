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

#ifndef WRAPPER_H
#define WRAPPER_H


#include <gtk/gtknotebook.h>
#include <gdk/gdkgc.h>
#include <gtk/gtkstyle.h>
#include <gtk/gtkprogressbar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_QT_WRAPPER_ARGS GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h, int hasFocus

void gtkQtInit();
void gtkQtDestroy();

void drawButton         (GTK_QT_WRAPPER_ARGS, int defaultButton);
void drawSquareButton   (GTK_QT_WRAPPER_ARGS);
void drawToolButton     (GTK_QT_WRAPPER_ARGS);
void drawTab            (GTK_QT_WRAPPER_ARGS, int tabCount, int selectedTab, int tab, int upsideDown);
void drawVLine          (GTK_QT_WRAPPER_ARGS);
void drawHLine          (GTK_QT_WRAPPER_ARGS);
void drawLineEdit       (GTK_QT_WRAPPER_ARGS, int editable);
void drawLineEditFrame  (GTK_QT_WRAPPER_ARGS, int editable);
void drawComboBox       (GTK_QT_WRAPPER_ARGS);
void drawFrame          (GTK_QT_WRAPPER_ARGS, int type);
void drawToolbar        (GTK_QT_WRAPPER_ARGS);
void drawMenubar        (GTK_QT_WRAPPER_ARGS);
void drawCheckBox       (GTK_QT_WRAPPER_ARGS, int on);
void drawMenuCheck      (GTK_QT_WRAPPER_ARGS);
void drawRadioButton    (GTK_QT_WRAPPER_ARGS, int on);
void drawScrollBar      (GTK_QT_WRAPPER_ARGS, GtkOrientation orientation, GtkAdjustment* adj);
void drawScrollBarSlider(GTK_QT_WRAPPER_ARGS, GtkOrientation orientation);
void drawSplitter       (GTK_QT_WRAPPER_ARGS, GtkOrientation orientation);
void drawMenuBarItem    (GTK_QT_WRAPPER_ARGS);
void drawMenuItem       (GTK_QT_WRAPPER_ARGS, int type);
void drawMenu           (GTK_QT_WRAPPER_ARGS);
void drawTabFrame       (GTK_QT_WRAPPER_ARGS);
void drawProgressBar    (GTK_QT_WRAPPER_ARGS, GtkProgressBarOrientation orientation, double percentage);
void drawProgressChunk  (GTK_QT_WRAPPER_ARGS);
void drawSlider         (GTK_QT_WRAPPER_ARGS, GtkAdjustment* adj, GtkOrientation orientation, int inverted);
void drawSpinButton     (GTK_QT_WRAPPER_ARGS, int direction);
void drawArrow          (GTK_QT_WRAPPER_ARGS, GtkArrowType direction);
void drawListHeader     (GTK_QT_WRAPPER_ARGS);

void setFillPixmap(GdkPixbuf* buf);
GdkGC* alternateBackgroundGc(GtkStyle* style, int enabled);
GdkPixmap* menuBackground();

int gtkQtDebug();
int gtkQtOpenOfficeHack();

#ifdef __cplusplus
}
#endif

#endif
