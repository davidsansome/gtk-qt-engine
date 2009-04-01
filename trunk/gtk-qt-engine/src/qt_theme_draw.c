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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtkprogressbar.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/glist.h>

#ifdef HAVE_BONOBO
#include <libbonobo.h>
#include <libbonoboui.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "qt_style.h"
#include "qt_rc_style.h"
#include "wrapper.h"

#define DETAIL(xx)    ((detail) && (!strcmp(xx, detail)))
#define DETAILHAS(xx) ((detail) && (strstr(detail, xx)))
#define PARENT(xx)    ((parent) && (!strcmp(xx, gtk_widget_get_name(parent))))
#define WIDGET(xx)    (GTK_IS_WIDGET(widget) && (!strcmp(xx, gtk_widget_get_name(widget))))

#define GTK_QT_STANDARD_ARGS window, style, stateType, x, y, w, h, GTK_IS_WIDGET(widget) && gtk_widget_is_focus(widget)

#ifndef max
#define max(x,y) ((x)>=(y)?(x):(y))
#endif
#ifndef min
#define min(x,y) ((x)<=(y)?(x):(y))
#endif

static void qtengine_style_init       (QtEngineStyle      *style);
static void qtengine_style_class_init (QtEngineStyleClass *klass);

static GtkStyleClass *parent_class = NULL;

static PangoLayout*
get_insensitive_layout (GdkDrawable *drawable,
                        PangoLayout *layout);


static GtkShadowType
get_shadowType (GtkStyle* style, const char *detail, GtkShadowType requested)
{
    GtkShadowType retval = GTK_SHADOW_NONE;

    if (requested != GTK_SHADOW_NONE) {
        retval = GTK_SHADOW_ETCHED_IN;
    }

    if (DETAIL ("dockitem") || DETAIL ("handlebox_bin") || DETAIL ("spinbutton_up") || DETAIL ("spinbutton_down")) {
        retval = GTK_SHADOW_NONE;
    } else if (DETAIL ("button") || DETAIL ("togglebutton") || DETAIL ("notebook") || DETAIL ("optionmenu")) {
        retval = requested;
    } else if (DETAIL ("menu")) {
        retval = GTK_SHADOW_ETCHED_IN;
    }

    return retval;
}

static void sanitize_size(GdkWindow* window, gint* w, gint* h)
{
	if ((*w == -1) && (*h == -1))
		gdk_window_get_size (window, w, h);
	else if (*w == -1)
		gdk_window_get_size (window, w, NULL);
	else if (*h == -1)
		gdk_window_get_size (window, NULL, h);
}

void grabFillPixmap(GtkWidget* widget, int x, int y, int w, int h)
{
	if (x < 0 || y < 0 || w <= 1 || h <= 1)
		return;
	
	if (gdk_window_is_viewable(gtk_widget_get_parent_window(widget)))
	{
		GdkPixbuf* gpix = gdk_pixbuf_get_from_drawable(NULL, gtk_widget_get_parent_window(widget), NULL, x, y, 0, 0,  w, h);
		setFillPixmap(gpix);
		g_object_unref(gpix);
	}
}


static void
draw_hline(GtkStyle* style,
           GdkWindow* window,
           GtkStateType stateType,
           GdkRectangle* area,
           GtkWidget* widget,
           const gchar* detail,
           gint x1,
           gint x2,
           gint y)
{
	int x = min(x1, x2);
	int w = abs(x2 - x1);
	int h = 2;
	
	if (gtkQtDebug())
		printf("HLINE (%d,%d,%d) Widget: %s  Detail: %s\n",x1,x2,y,gtk_widget_get_name(widget),detail);

	if (DETAIL("vscale"))
		return;

	drawHLine(GTK_QT_STANDARD_ARGS);
}


static void
draw_vline(GtkStyle* style,
           GdkWindow* window,
           GtkStateType stateType,
           GdkRectangle* area,
           GtkWidget* widget,
           const gchar* detail,
           gint y1,
           gint y2,
           gint x)
{
	int y = min(y1, y2);
	int h = abs(y2 - y1);
	int w = 2;
	if (gtkQtDebug())
		printf("VLINE (%d,%d,%d) Widget: %s  Detail: %s\n", y1, y2, x, gtk_widget_get_name(widget), detail);

	if (DETAIL("hscale"))
		return;
		
	drawVLine(GTK_QT_STANDARD_ARGS);
}

static void
draw_shadow(GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  stateType,
	    GtkShadowType shadowType,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x,
	    gint          y,
	    gint          w,
	    gint          h)
{
	GdkGC *gc1 = NULL;		/* Initialize to quiet GCC */
	GdkGC *gc2 = NULL;
	GdkGC *gc3 = NULL;
	GdkGC *gc4 = NULL;
	
	gint thickness_light;
	gint thickness_dark;
	gint i;
	
	sanitize_size(window, &w, &h);
	
	if (gtkQtDebug())
		printf("Shadow (%d,%d,%d,%d) Widget: %s Detail: %s\n",x,y,w,h,gtk_widget_get_name(widget),detail);

	
	if (DETAIL("menuitem"))
		return;
	if (DETAIL("menu"))
		return;
	if (DETAIL("entry"))
	{
		int editable = 1;
		if (GTK_IS_ENTRY(widget) && !GTK_ENTRY(widget)->editable)
			editable = 0;
		
		drawLineEditFrame(GTK_QT_STANDARD_ARGS, editable);
		return;
	}
	if (DETAIL("frame") || DETAIL("trough") || DETAIL("viewport"))
	{
		if (!GTK_IS_SCALE(widget))
		{
			/*printf("Frame (%d,%d) %dx%d   %d %d\n", x,y,w,h,stateType, shadowType);*/
			drawFrame(GTK_QT_STANDARD_ARGS, 0);
			return;
		}
	}
	
	if (shadowType == GTK_SHADOW_NONE)
		return;

	if (WIDGET("GimpFgBgEditor"))
		return;
	
	drawFrame(GTK_QT_STANDARD_ARGS, 1);
}

static void
draw_polygon(GtkStyle* style,
             GdkWindow* window,
             GtkStateType stateType,
             GtkShadowType shadowType,
             GdkRectangle* area,
             GtkWidget* widget,
             const gchar* detail,
             GdkPoint* points,
             gint npoints,
             gint fill)
{
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */
#ifndef M_PI_4
#define M_PI_4  0.78539816339744830962
#endif /* M_PI_4 */

  static const gdouble pi_over_4 = M_PI_4;
  static const gdouble pi_3_over_4 = M_PI_4 * 3;

  GdkGC              *gc1;
  GdkGC              *gc2;
  GdkGC              *gc3;
  GdkGC              *gc4;
  gdouble             angle;
  gint                xadjust;
  gint                yadjust;
  gint                i;

  g_return_if_fail(style != NULL);
  g_return_if_fail(window != NULL);
  g_return_if_fail(points != NULL);

  switch (shadowType)
    {
    case GTK_SHADOW_IN:
      gc1 = style->light_gc[stateType];
      gc2 = style->dark_gc[stateType];
      gc3 = style->light_gc[stateType];
      gc4 = style->dark_gc[stateType];
      break;
    case GTK_SHADOW_ETCHED_IN:
      gc1 = style->light_gc[stateType];
      gc2 = style->dark_gc[stateType];
      gc3 = style->dark_gc[stateType];
      gc4 = style->light_gc[stateType];
      break;
    case GTK_SHADOW_OUT:
      gc1 = style->dark_gc[stateType];
      gc2 = style->light_gc[stateType];
      gc3 = style->dark_gc[stateType];
      gc4 = style->light_gc[stateType];
      break;
    case GTK_SHADOW_ETCHED_OUT:
      gc1 = style->dark_gc[stateType];
      gc2 = style->light_gc[stateType];
      gc3 = style->light_gc[stateType];
      gc4 = style->dark_gc[stateType];
      break;
    default:
      return;
    }

  if (area)
    {
      gdk_gc_set_clip_rectangle(gc1, area);
      gdk_gc_set_clip_rectangle(gc2, area);
      gdk_gc_set_clip_rectangle(gc3, area);
      gdk_gc_set_clip_rectangle(gc4, area);
    }

  if (fill)
    gdk_draw_polygon(window, style->bg_gc[stateType], TRUE, points, npoints);

  npoints--;

  for (i = 0; i < npoints; i++)
    {
      if ((points[i].x == points[i + 1].x) &&
          (points[i].y == points[i + 1].y))
        {
          angle = 0;
        }
      else
        {
          angle = atan2(points[i + 1].y - points[i].y,
                        points[i + 1].x - points[i].x);
        }

      if ((angle > -pi_3_over_4) && (angle < pi_over_4))
        {
          if (angle > -pi_over_4)
            {
              xadjust = 0;
              yadjust = 1;
            }
          else
            {
              xadjust = 1;
              yadjust = 0;
            }

          gdk_draw_line(window, gc1,
                        points[i].x - xadjust, points[i].y - yadjust,
                        points[i + 1].x - xadjust, points[i + 1].y - yadjust);
          gdk_draw_line(window, gc3,
                        points[i].x, points[i].y,
                        points[i + 1].x, points[i + 1].y);
        }
      else
        {
          if ((angle < -pi_3_over_4) || (angle > pi_3_over_4))
            {
              xadjust = 0;
              yadjust = 1;
            }
          else
            {
              xadjust = 1;
              yadjust = 0;
            }

          gdk_draw_line(window, gc4,
                        points[i].x + xadjust, points[i].y + yadjust,
                        points[i + 1].x + xadjust, points[i + 1].y + yadjust);
          gdk_draw_line(window, gc2,
                        points[i].x, points[i].y,
                        points[i + 1].x, points[i + 1].y);
        }
    }
  if (area)
    {
      gdk_gc_set_clip_rectangle(gc1, NULL);
      gdk_gc_set_clip_rectangle(gc2, NULL);
      gdk_gc_set_clip_rectangle(gc3, NULL);
      gdk_gc_set_clip_rectangle(gc4, NULL);
    }
}

static void
draw_arrow(GtkStyle* style,
           GdkWindow* window,
           GtkStateType stateType,
           GtkShadowType shadowType,
           GdkRectangle* area,
           GtkWidget* widget,
           const gchar *detail,
           GtkArrowType arrowType,
           gint fill, gint x, gint y, gint w, gint h)
{
	sanitize_size(window, &w, &h);
	
	if (gtkQtDebug())
		printf("Arrow (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
	
	if (DETAIL("hscrollbar") || DETAIL("vscrollbar"))
		return;
	if (DETAIL("spinbutton"))
		return;
	if (DETAIL("notebook"))
	{
		drawArrow(GTK_QT_STANDARD_ARGS, arrowType);
		return;
	}
	if (DETAIL("arrow"))
	{
		GtkWidget* parent;
		
		grabFillPixmap(widget, x, y, w, h);
		
		parent = gtk_widget_get_parent(widget);
		stateType = GTK_WIDGET_STATE(parent);
		drawArrow(GTK_QT_STANDARD_ARGS, arrowType);
		return;
	}
/*	if (DETAIL("menuitem"))
	{
		GdkGC              *gc1;
		GdkGC              *gc2;
		GdkGC              *gc3;
		GdkGC              *gc4;
		gint                half_width;
		gint                half_height;
		gint                ax, ay, aw, ah;
	
		switch (shadowType)
		{
		case GTK_SHADOW_IN:
			gc1 = style->bg_gc[stateType];
			gc2 = style->dark_gc[stateType];
			gc3 = style->light_gc[stateType];
			gc4 = style->black_gc;
			break;
		case GTK_SHADOW_OUT:
			gc1 = style->dark_gc[stateType];
			gc2 = style->light_gc[stateType];
			gc3 = style->black_gc;
			gc4 = style->bg_gc[stateType];
			break;
		case GTK_SHADOW_ETCHED_IN:
			gc2 = style->light_gc[stateType];
			gc1 = style->dark_gc[stateType];
			gc3 = NULL;
			gc4 = NULL;
			break;
		case GTK_SHADOW_ETCHED_OUT:
			gc1 = style->dark_gc[stateType];
			gc2 = style->light_gc[stateType];
			gc3 = NULL;
			gc4 = NULL;
			break;
		default:
			return;
		}
		
		sanitize_size(window, &width, &height);
		ax = x;
		ay = y;
		aw = width;
		ah = height;
		calculate_arrow_geometry (arrow_type, &ax, &ay, &aw, &ah);
		
		half_width = width / 2;
		half_height = height / 2;
		
		if (area)
		{
			gdk_gc_set_clip_rectangle(gc1, area);
			gdk_gc_set_clip_rectangle(gc2, area);
			if ((gc3) && (gc4))
			{
				gdk_gc_set_clip_rectangle(gc3, area);
				gdk_gc_set_clip_rectangle(gc4, area);
			}
		}
		
		if (stateType == GTK_STATE_INSENSITIVE)
			draw_black_arrow (window, style->white_gc, area, arrow_type, ax + 1, ay + 1, aw, ah);
		draw_black_arrow (window, style->fg_gc[stateType], area, arrow_type, ax, ay, aw, ah);
		
		if (area)
		{
			gdk_gc_set_clip_rectangle(gc1, NULL);
			gdk_gc_set_clip_rectangle(gc2, NULL);
			if (gc3)
			{
				gdk_gc_set_clip_rectangle(gc3, NULL);
				gdk_gc_set_clip_rectangle(gc4, NULL);
			}
		}
	}*/
	else
	{
		grabFillPixmap(widget, x, y, w, h);
		drawArrow(GTK_QT_STANDARD_ARGS, arrowType);
 		return;
	}
}



static void
draw_diamond(GtkStyle * style,
             GdkWindow * window,
             GtkStateType stateType,
             GtkShadowType shadowType,
             GdkRectangle * area,
             GtkWidget * widget,
             const gchar *detail,
             gint x,
             gint y,
             gint w,
             gint h)
{
}



static void
draw_box(GtkStyle * style,
	GdkWindow * window,
	GtkStateType stateType,
	GtkShadowType shadowType,
	GdkRectangle * area,
	GtkWidget * widget,
	const gchar *detail,
	gint x,
	gint y,
	gint w,
	gint h)
{
	sanitize_size(window, &w, &h);
	
	if (gtkQtDebug())
		printf("Box (%d,%d,%d,%d) Widget: %s  Detail: %s\n",x,y,w,h,gtk_widget_get_name(widget),detail);

	if (GTK_IS_SCROLLBAR(widget))
	{
		if (DETAIL("trough"))
		{
			GtkAdjustment* adj = (GtkAdjustment*)gtk_range_get_adjustment(GTK_RANGE(widget));
			GtkOrientation orientation = ((w>h) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
			
			drawScrollBar(GTK_QT_STANDARD_ARGS, orientation, adj);
		}
		return;
	}
	if (DETAIL("menuitem"))
	{
		/* Crude way of checking if it's a menu item, or a menubar item */
		if (x != 0)
			drawMenuBarItem(GTK_QT_STANDARD_ARGS);
		else
		{
			int type = 0;
			if (GTK_IS_SEPARATOR_MENU_ITEM(widget))
				type = 1;
			else if (GTK_IS_TEAROFF_MENU_ITEM(widget))
				type = 2;
			
			drawMenuItem(GTK_QT_STANDARD_ARGS, type);
		}
		return;
	}
	if (DETAIL("menubar"))
	{
		if (gtkQtOpenOfficeHack() == 1)
			parent_class->draw_box (style, window, stateType, shadowType, area, widget, detail, x, y, w, h);
		else
			drawMenubar(GTK_QT_STANDARD_ARGS);
		return;
	}
	if (DETAIL("menu"))
	{
		if (gtkQtOpenOfficeHack() == 1)
			 parent_class->draw_box (style, window, stateType, shadowType, area, widget, detail, x, y, w, h);
		else
		{
			drawMenu(GTK_QT_STANDARD_ARGS);
		}
		return;
	}
	if (GTK_IS_PROGRESS(widget) && DETAIL("trough"))
	{
		double fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(widget));
		GtkProgressBarOrientation orientation = gtk_progress_bar_get_orientation(GTK_PROGRESS_BAR(widget));
		
		drawProgressBar(GTK_QT_STANDARD_ARGS, orientation, fraction);
		return;
	}
	if (GTK_IS_PROGRESS(widget) && DETAIL("bar"))
	{
		drawProgressChunk(GTK_QT_STANDARD_ARGS);
		return;
	}
	if (GTK_IS_SCALE(widget) && DETAIL("trough"))
	{
		GtkAdjustment* adj;
		int inverted;
		GValue *val = (GValue*)g_malloc( sizeof(GValue) );
		
		memset( val, 0, sizeof(GValue) );
		g_value_init( val, G_TYPE_BOOLEAN );
		g_object_get_property((GObject*)widget, "inverted", val);
		inverted = g_value_get_boolean(val);
		g_value_unset(val);
		g_free(val);
		
		adj = gtk_range_get_adjustment((GtkRange *) widget);
		drawSlider(GTK_QT_STANDARD_ARGS, adj, (GTK_RANGE(widget))->orientation, inverted);
		return;
	}
	if (DETAIL("button"))
	{
		GtkWidget *parent;
		int toolbutton = 0;
		parent = gtk_widget_get_parent(widget);

		if (parent && (GTK_IS_CLIST(parent) || GTK_IS_LIST(parent) || GTK_IS_TREE_VIEW(parent)))
		{
			drawListHeader(GTK_QT_STANDARD_ARGS);
			return;
		}

		/* this is a very very bad hack but there seems to be no way to find if a button is on a
		 * toolbar in gtk */
		while (1)
		{
			if (GTK_IS_WIDGET(parent))
			{
#ifdef HAVE_BONOBO
				if (GTK_IS_TOOLBAR(parent) || BONOBO_IS_UI_TOOLBAR(parent))
#else
				if (GTK_IS_TOOLBAR(parent))
#endif
				{
					toolbutton = 1;
					break;
				}
			}
			else
				break;
			parent = gtk_widget_get_parent(parent);
		}
		
		parent = gtk_widget_get_parent(widget);

		grabFillPixmap(widget, x, y, w, h);
		if (toolbutton)
			drawToolButton(GTK_QT_STANDARD_ARGS);
		else
		{
			int defaultButton = GTK_WIDGET_HAS_FOCUS(widget);
			GtkWindow* toplevel;
			
			toplevel = GTK_WINDOW(gtk_widget_get_toplevel(widget));
			if (toplevel && toplevel->default_widget == widget)
				defaultButton = 1;
			
			drawButton(GTK_QT_STANDARD_ARGS, defaultButton);
		}
		return;
	}
	if (DETAIL("tab"))
	{
		if (GTK_IS_NOTEBOOK(widget))
		{
			GtkNotebook* notebook = (GtkNotebook*) widget;
			GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
			int tabCount = g_list_length(notebook->children);
			int selectedTab = gtk_notebook_get_current_page(notebook);
			int tab = 0;
			GtkPositionType tpos = gtk_notebook_get_tab_pos(notebook);
			
			/* Find tab position */
			int sdiff=10000, pos=-1, diff=1, i;
			for (i=0 ; i<tabCount ; i++)
			{
				GtkWidget* tabLabel = gtk_notebook_get_tab_label(notebook, gtk_notebook_get_nth_page(notebook, i));
				if (tabLabel)
					diff = tabLabel->allocation.x - x;
				if ((diff > 0) && (diff < sdiff))
				{
					sdiff = diff; tab = i;
				}
			}
			
			drawTab(GTK_QT_STANDARD_ARGS, tabCount, selectedTab, tab, tpos == GTK_POS_BOTTOM);
		}
		else
			drawTab(GTK_QT_STANDARD_ARGS, -1, -1, -1, 0);
		return;
	}
	if (DETAIL("optionmenu"))
	{
		grabFillPixmap(widget, x, y, w, h);
		drawComboBox(GTK_QT_STANDARD_ARGS);
		return;
	}
	if (DETAIL("toolbar"))
	{
		if (gtkQtOpenOfficeHack() == 1)
			parent_class->draw_box (style, window, stateType, shadowType, area, widget, detail, x, y, w, h);
		else
			drawToolbar(GTK_QT_STANDARD_ARGS);
		return;
	}
	if (DETAIL("spinbutton_up"))
	{
		drawSpinButton(GTK_QT_STANDARD_ARGS, 0);
		return;
	}
	if (DETAIL("spinbutton_down"))
	{
		drawSpinButton(GTK_QT_STANDARD_ARGS, 1);
		return;
	}
	if (DETAIL("spinbutton"))
		return;
	
	if (DETAIL("optionmenutab") || DETAIL("buttondefault"))
		return;
	
	drawFrame(GTK_QT_STANDARD_ARGS, 0);
}



static void
draw_flat_box(GtkStyle * style,
              GdkWindow * window,
              GtkStateType stateType,
              GtkShadowType shadowType,
              GdkRectangle * area,
              GtkWidget * widget,
              const gchar *detail,
              gint x,
              gint y,
              gint w,
              gint h)
{
	sanitize_size(window, &w, &h);
	
	if (gtkQtDebug())
		printf("Flat Box (%d,%d,%d,%d) Widget: %s  Detail: %s %d %d\n",x,y,w,h,gtk_widget_get_name(widget),detail, stateType, GTK_STATE_SELECTED);

	
	if (DETAILHAS("cell_odd") && (stateType != GTK_STATE_SELECTED))
	{
		gdk_draw_rectangle(window, alternateBackgroundGc(style, stateType != GTK_STATE_INSENSITIVE), TRUE, x, y, w, h);
	}
	else if (DETAILHAS("cell_odd") || DETAILHAS("cell_even") || DETAIL("listitem"))
	{
		gdk_draw_rectangle(window, style->base_gc[stateType], TRUE, x, y, w, h);
	}
	else if (DETAIL("tooltip"))
	{
		gdk_draw_rectangle(window, style->bg_gc[stateType], TRUE, x, y, w, h);
		gdk_draw_rectangle(window, style->fg_gc[stateType], FALSE, x, y, w-1, h-1);
	}
	else if (DETAIL("entry_bg"))
	{
		int editable = 1;
		if (GTK_IS_ENTRY(widget) && !GTK_ENTRY(widget)->editable)
			editable = 0;
		
		drawLineEdit(window,style,stateType,x,y,w,h,gtk_widget_is_focus(widget), editable);
	}
}


static void
draw_check(GtkStyle * style,
           GdkWindow * window,
           GtkStateType stateType,
           GtkShadowType shadowType,
           GdkRectangle * area,
           GtkWidget * widget,
           const gchar *detail,
           gint x,
           gint y,
           gint w,
           gint h)
{
	if (gtkQtDebug())
		printf("Check (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
	
	if (GTK_IS_MENU_ITEM(widget))
	{
		if (shadowType == GTK_SHADOW_IN)
		{
			grabFillPixmap(widget, x, y, w, h);
			drawMenuCheck(GTK_QT_STANDARD_ARGS);
		}
		return;
	}
	drawCheckBox(GTK_QT_STANDARD_ARGS, shadowType == GTK_SHADOW_IN);
}


/* Thanks to Evan Lawrence */
static void
draw_option(GtkStyle * style,
            GdkWindow * window,
            GtkStateType stateType,
            GtkShadowType shadowType,
            GdkRectangle * area,
            GtkWidget * widget,
            const gchar *detail,
            gint x,
            gint y,
            gint w,
            gint h)
{
	if (gtkQtDebug())
		printf("Option (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);

	if (GTK_IS_MENU_ITEM(widget))
	{
		if (shadowType == GTK_SHADOW_IN)
		{
			grabFillPixmap(widget, x, y, w, h);
			drawMenuCheck(GTK_QT_STANDARD_ARGS);
		}
		return;
	}
	grabFillPixmap(widget, x, y, w, h);
	drawRadioButton(GTK_QT_STANDARD_ARGS, shadowType == GTK_SHADOW_IN);
}


static void
draw_tab(GtkStyle * style,
         GdkWindow * window,
         GtkStateType stateType,
         GtkShadowType shadowType,
         GdkRectangle * area,
         GtkWidget * widget,
         const gchar *detail,
         gint x,
         gint y,
         gint w,
         gint h)
{
	if (gtkQtDebug())
		printf("Tab (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
	
	gtk_paint_box(style, window, stateType, shadowType, area, widget, detail, x, y, w, h);
}



static void
draw_shadow_gap(GtkStyle * style,
                GdkWindow * window,
                GtkStateType stateType,
                GtkShadowType shadowType,
                GdkRectangle * area,
                GtkWidget * widget,
                const gchar *detail,
                gint x,
                gint y,
                gint w,
                gint h,
                GtkPositionType gap_side,
                gint gap_x,
                gint gap_w)
{
	GdkGC *gc1 = NULL;
	GdkGC *gc2 = NULL;
	
	g_return_if_fail (window != NULL);
	
	sanitize_size (window, &w, &h);
	shadowType = get_shadowType (style, detail, shadowType);
	
	if (gtkQtDebug())
		printf("Shadow_Gap (%d,%d,%d,%d) Widget: %s  Detail: %s\n",x,y,w,h,gtk_widget_get_name(widget),detail);
	
	switch (shadowType) {
	case GTK_SHADOW_NONE:
		return;
	case GTK_SHADOW_IN:
		gc1 = style->dark_gc[stateType];
		gc2 = style->light_gc[stateType];
		break;
	case GTK_SHADOW_OUT:
		gc1 = style->light_gc[stateType];
		gc2 = style->dark_gc[stateType];
		break;
	case GTK_SHADOW_ETCHED_IN:
	case GTK_SHADOW_ETCHED_OUT:
		gc1 = style->dark_gc[stateType];
		gc2 = style->dark_gc[stateType];
	}

	if (area) {
		gdk_gc_set_clip_rectangle (gc1, area);
		gdk_gc_set_clip_rectangle (gc2, area);
	}
	
	switch (gap_side) {
        case GTK_POS_TOP:
		if (gap_x > 0) {
			gdk_draw_line (window, gc1, 
				       x, y,
				       x + gap_x, y);
		}
		if ((w - (gap_x + gap_w)) > 0) {
			gdk_draw_line (window, gc1, 
				       x + gap_x + gap_w - 1, y,
				       x + w - 1, y);
		}
		gdk_draw_line (window, gc1, 
			       x, y, 
			       x, y + h - 1);
		gdk_draw_line (window, gc2,
			       x + w - 1, y,
			       x + w - 1, y + h - 1);
		gdk_draw_line (window, gc2,
			       x, y + h - 1,
			       x + w - 1, y + h - 1);
		break;
        case GTK_POS_BOTTOM:
		gdk_draw_line (window, gc1,
			       x, y,
			       x + w - 1, y);
		gdk_draw_line (window, gc1, 
			       x, y, 
			       x, y + h - 1);
		gdk_draw_line (window, gc2,
			       x + w - 1, y,
			       x + w - 1, y + h - 1);

		if (gap_x > 0) {
			gdk_draw_line (window, gc2, 
				       x, y + h - 1, 
				       x + gap_x, y + h - 1);
		}
		if ((w - (gap_x + gap_w)) > 0) {
			gdk_draw_line (window, gc2, 
				       x + gap_x + gap_w - 1, y + h - 1,
				       x + w - 1, y + h - 1);
		}
		
		break;
        case GTK_POS_LEFT:
		gdk_draw_line (window, gc1,
			       x, y,
			       x + w - 1, y);
		if (gap_x > 0) {
			gdk_draw_line (window, gc1, 
				       x, y,
				       x, y + gap_x);
		}
		if ((h - (gap_x + gap_w)) > 0) {
			gdk_draw_line (window, gc1, 
				       x, y + gap_x + gap_w - 1,
				       x, y + h - 1);
		}
		gdk_draw_line (window, gc2,
			       x + w - 1, y,
			       x + w - 1, y + h - 1);
		gdk_draw_line (window, gc2,
			       x, y + h - 1,
			       x + w - 1, y + h - 1);
		break;
        case GTK_POS_RIGHT:
		gdk_draw_line (window, gc1,
			       x, y,
			       x + w - 1, y);
		gdk_draw_line (window, gc1, 
			       x, y, 
			       x, y + h - 1);


		if (gap_x > 0) {
			gdk_draw_line (window, gc2, 
				       x + w - 1, y,
				       x + w - 1, y + gap_x);
		}
		if ((h - (gap_x + gap_w)) > 0) {
			gdk_draw_line (window, gc2, 
				       x + w - 1, y + gap_x + gap_w - 1,
				       x + w - 1, y + h - 1);
		}
		gdk_draw_line (window, gc2,
			       x, y + h - 1,
			       x + w - 1, y + h - 1);

	}
	
	if (area) {
		gdk_gc_set_clip_rectangle (gc1, NULL);
		gdk_gc_set_clip_rectangle (gc2, NULL);
	}
}


static void
draw_box_gap(GtkStyle* style,
             GdkWindow* window,
             GtkStateType stateType,
             GtkShadowType shadowType,
             GdkRectangle* area,
             GtkWidget* widget,
             const gchar* detail,
             gint x,
             gint y,
             gint w,
             gint h,
             GtkPositionType gap_side,
             gint gap_x,
             gint gap_w)
{
	sanitize_size (window, &w, &h);
	
	if (w<0 || h<0) return;  /* Eclipse really can be this stupid! */
	
	if (gtkQtDebug())
		printf("Box_gap (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
	
	if (DETAIL("notebook"))
		drawTabFrame(GTK_QT_STANDARD_ARGS);
}


static void
draw_extension(GtkStyle * style,
               GdkWindow * window,
               GtkStateType stateType,
               GtkShadowType shadowType,
               GdkRectangle * area,
               GtkWidget * widget,
               const gchar *detail,
               gint x,
               gint y,
               gint w,
               gint h,
               GtkPositionType gap_side)
{
	g_return_if_fail(style != NULL);
	g_return_if_fail(window != NULL);
	
	sanitize_size (window, &w, &h);
	
	if (gtkQtDebug())
		printf("Extension (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
	
	gtk_paint_box(style, window, stateType, shadowType, area, widget, detail, x, y, w, h);
}


static void
draw_focus (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  stateType,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x,
	    gint          y,
	    gint          w,
	    gint          h)
{
	if (gtkQtDebug())
		printf("Focus Rect (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
}

static void
draw_slider(GtkStyle * style,
            GdkWindow * window,
            GtkStateType stateType,
            GtkShadowType shadowType,
            GdkRectangle * area,
            GtkWidget * widget,
            const gchar *detail,
            gint x,
            gint y,
            gint w,
            gint h,
            GtkOrientation orientation)
{
	if (gtkQtDebug())
		printf("Slider (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, w, h,gtk_widget_get_name(widget),detail);
	
	if (DETAIL("slider"))
	{
		GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(widget));
		
		if (orientation == GTK_ORIENTATION_VERTICAL)
			drawScrollBarSlider(GTK_QT_STANDARD_ARGS, orientation);
		else
			drawScrollBarSlider(GTK_QT_STANDARD_ARGS, orientation);
		return;
	}
}

static void
draw_handle(GtkStyle * style,
            GdkWindow * window,
            GtkStateType stateType,
            GtkShadowType shadowType,
            GdkRectangle * area,
            GtkWidget * widget,
            const gchar *detail,
            gint x,
            gint y,
            gint w,
            gint h,
            GtkOrientation orientation)
{
	g_return_if_fail(style != NULL);
	g_return_if_fail(window != NULL);
	
	sanitize_size(window, &w, &h);
	
	if (gtkQtDebug())
		printf("Handle (%d,%d,%d,%d) Widget: %s  Detail: %s \n",x,y,w,h,gtk_widget_get_name(widget),detail, stateType);
	
	drawSplitter(GTK_QT_STANDARD_ARGS, orientation);
	return;
}

static
void  draw_layout  (GtkStyle *style,
                        GdkWindow *window,
                        GtkStateType stateType,
                        gboolean use_text,
                        GdkRectangle *area,
                        GtkWidget *widget,
                        const gchar *detail,
                        gint x,
                        gint y,
                        PangoLayout *layout)
{
	GdkGC *gc;

	if (gtkQtDebug())
		printf("Layout (%d,%d) Widget: %s  Detail: %s %d \n",x,y,gtk_widget_get_name(widget),detail, stateType);

	if (DETAIL("accellabel") || DETAIL("label") || DETAIL("cellrenderertext"))
	{
		
		GtkWidget* parent = gtk_widget_get_parent(widget);
		GtkWidget* parent1 = gtk_widget_get_parent(parent);

		/* printf("parent's names are %s->%s->%s\n", gtk_widget_get_name(widget), gtk_widget_get_name(parent), gtk_widget_get_name(parent1)); */

		/* printf("Drawing an label -- with state %d at %d %d\n", stateType, x, y); */
	}

	g_return_if_fail (window != NULL);

	gc = use_text ? style->text_gc[stateType] : style->fg_gc[stateType];

	if (area)
    		gdk_gc_set_clip_rectangle (gc, area);

  	if (stateType == GTK_STATE_INSENSITIVE)
    	{
      		PangoLayout *ins;
      		ins = get_insensitive_layout (window, layout);
      		gdk_draw_layout (window, gc, x, y, ins);
      		g_object_unref (ins);
    	}
  	else
    	{
      		gdk_draw_layout (window, gc, x, y, layout);
    	}

  	if (area)
    		gdk_gc_set_clip_rectangle (gc, NULL);
}

typedef struct _ByteRange ByteRange;

struct _ByteRange
{
  guint start;
  guint end;
};

static ByteRange*
range_new (guint start,
           guint end)
{
  ByteRange *br = g_new (ByteRange, 1);

  br->start = start;
  br->end = end;

  return br;
}

static PangoLayout*
get_insensitive_layout (GdkDrawable *drawable,
                        PangoLayout *layout)
{
  GSList *embossed_ranges = NULL;
  GSList *stippled_ranges = NULL;
  PangoLayoutIter *iter;
  GSList *tmp_list = NULL;
  PangoLayout *new_layout;
  PangoAttrList *attrs;
  GdkBitmap *stipple = NULL;

  iter = pango_layout_get_iter (layout);

  do
    {
      PangoLayoutRun *run;
      PangoAttribute *attr;
      gboolean need_stipple = FALSE;
      ByteRange *br;

      run = pango_layout_iter_get_run (iter);

      if (run)
        {
          tmp_list = run->item->analysis.extra_attrs;

          while (tmp_list != NULL)
            {
              attr = tmp_list->data;
              switch (attr->klass->type)
                {
                case PANGO_ATTR_FOREGROUND:
                case PANGO_ATTR_BACKGROUND:
                  need_stipple = TRUE;
                  break;

                default:
                  break;
                }

              if (need_stipple)
                break;

              tmp_list = g_slist_next (tmp_list);
            }

          br = range_new (run->item->offset, run->item->offset + run->item->length);

          if (need_stipple)
            stippled_ranges = g_slist_prepend (stippled_ranges, br);
          else
            embossed_ranges = g_slist_prepend (embossed_ranges, br);
        }
    }
  while (pango_layout_iter_next_run (iter));

  pango_layout_iter_free (iter);

  new_layout = pango_layout_copy (layout);

  attrs = pango_layout_get_attributes (new_layout);

  if (attrs == NULL)
    {
      /* Create attr list if there wasn't one */
      attrs = pango_attr_list_new ();
      pango_layout_set_attributes (new_layout, attrs);
      pango_attr_list_unref (attrs);
    }

  tmp_list = embossed_ranges;
  while (tmp_list != NULL)
    {
      PangoAttribute *attr;
      ByteRange *br = tmp_list->data;

      attr = gdk_pango_attr_embossed_new (TRUE);

      attr->start_index = br->start;
      attr->end_index = br->end;

      pango_attr_list_change (attrs, attr);

      g_free (br);

      tmp_list = g_slist_next (tmp_list);
    }

  g_slist_free (embossed_ranges);

  tmp_list = stippled_ranges;
  while (tmp_list != NULL)
    {
      PangoAttribute *attr;
      ByteRange *br = tmp_list->data;

      if (stipple == NULL)
        {
#define gray50_width 2
#define gray50_height 2
          static const char gray50_bits[] = {
            0x02, 0x01
         };

          stipple = gdk_bitmap_create_from_data (drawable,
                                                 gray50_bits, gray50_width,
                                                 gray50_height);
        }

      attr = gdk_pango_attr_stipple_new (stipple);

      attr->start_index = br->start;
      attr->end_index = br->end;

      pango_attr_list_change (attrs, attr);

      g_free (br);

      tmp_list = g_slist_next (tmp_list);
    }

  g_slist_free (stippled_ranges);

  if (stipple)
    g_object_unref (stipple);

  return new_layout;
}


GType qtengine_type_style = 0;

void
qtengine_style_register_type (GTypeModule *module)
{
	static const GTypeInfo object_info =
	{
	sizeof (QtEngineStyleClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) qtengine_style_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (QtEngineStyle),
	0,              /* n_preallocs */
	(GInstanceInitFunc) qtengine_style_init,
	};
	
	qtengine_type_style = g_type_module_register_type (module,
						    GTK_TYPE_STYLE,
						    "QtEngineStyle",
						    &object_info, 0);
}

static void
qtengine_style_init (QtEngineStyle *style)
{
}


/* Copied these functions from gtkstyle.c
   Evil, evil GTK... why isn't this stuff exported? */

#define LIGHTNESS_MULT  1.3
#define DARKNESS_MULT   0.7

static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b)
{
  gdouble min;
  gdouble max;
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble h, l, s;
  gdouble delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;
      
      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;
      
      if (red < blue)
        min = red;
      else
        min = blue;
    }
  
  l = (max + min) / 2;
  s = 0;
  h = 0;
  
  if (max != min)
    {
      if (l <= 0.5)
        s = (max - min) / (max + min);
      else
        s = (max - min) / (2 - max - min);
      
      delta = max -min;
      if (red == max)
        h = (green - blue) / delta;
      else if (green == max)
        h = 2 + (blue - red) / delta;
      else if (blue == max)
        h = 4 + (red - green) / delta;
      
      h *= 60;
      if (h < 0.0)
        h += 360;
    }
  
  *r = h;
  *g = l;
  *b = s;
}

static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s)
{
  gdouble hue;
  gdouble lightness;
  gdouble saturation;
  gdouble m1, m2;
  gdouble r, g, b;

  lightness = *l;
  saturation = *s;

  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;

  if (saturation == 0)
    {
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      hue = *h + 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;

      if (hue < 60)
        r = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        r = m2;
      else if (hue < 240)
        r = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        r = m1;
      
      hue = *h;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        g = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        g = m2;
      else if (hue < 240)
        g = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        g = m1;
      
      hue = *h - 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        b = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        b = m2;
      else if (hue < 240)
        b = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        b = m1;
      
      *h = r;
      *l = g;
      *s = b;
    }
}

static void
gtk_style_shade (GdkColor *a,
                 GdkColor *b,
                 gdouble   k)
{
  gdouble red;
  gdouble green;
  gdouble blue;

  red = (gdouble) a->red / 65535.0;
  green = (gdouble) a->green / 65535.0;
  blue = (gdouble) a->blue / 65535.0;

  rgb_to_hls (&red, &green, &blue);

  green *= k;
  if (green > 1.0)
    green = 1.0;
  else if (green < 0.0)
    green = 0.0;

  blue *= k;
  if (blue > 1.0)
    blue = 1.0;
  else if (blue < 0.0)
    blue = 0.0;

  hls_to_rgb (&red, &green, &blue);

  b->red = red * 65535.0;
  b->green = green * 65535.0;
  b->blue = blue * 65535.0;
}

static void
gtk_style_real_realize (GtkStyle *style)
{
  GdkGCValues gc_values;
  GdkGCValuesMask gc_values_mask;

  gint i;

  for (i = 0; i < 5; i++)
    {
      gtk_style_shade (&style->bg[i], &style->light[i], LIGHTNESS_MULT);
      gtk_style_shade (&style->bg[i], &style->dark[i], DARKNESS_MULT);

      style->mid[i].red = (style->light[i].red + style->dark[i].red) / 2;
      style->mid[i].green = (style->light[i].green + style->dark[i].green) / 2;
      style->mid[i].blue = (style->light[i].blue + style->dark[i].blue) / 2;

      style->text_aa[i].red = (style->text[i].red + style->base[i].red) / 2;
      style->text_aa[i].green = (style->text[i].green + style->base[i].green) / 2;
      style->text_aa[i].blue = (style->text[i].blue + style->base[i].blue) / 2;
    }

  style->black.red = 0x0000;
  style->black.green = 0x0000;
  style->black.blue = 0x0000;
  gdk_colormap_alloc_color (style->colormap, &style->black, FALSE, TRUE);

  style->white.red = 0xffff;
  style->white.green = 0xffff;
  style->white.blue = 0xffff;
  gdk_colormap_alloc_color (style->colormap, &style->white, FALSE, TRUE);

  gc_values_mask = GDK_GC_FOREGROUND;
  
  gc_values.foreground = style->black;
  style->black_gc = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
  
  gc_values.foreground = style->white;
  style->white_gc = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
  
  for (i = 0; i < 5; i++)
    {

      if (!gdk_colormap_alloc_color (style->colormap, &style->fg[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->fg[i].red, style->fg[i].green, style->fg[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->bg[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->bg[i].red, style->bg[i].green, style->bg[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->light[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->light[i].red, style->light[i].green, style->light[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->dark[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->dark[i].red, style->dark[i].green, style->dark[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->mid[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->mid[i].red, style->mid[i].green, style->mid[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->text[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->text[i].red, style->text[i].green, style->text[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->base[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->base[i].red, style->base[i].green, style->base[i].blue);
      if (!gdk_colormap_alloc_color (style->colormap, &style->text_aa[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->text_aa[i].red, style->text_aa[i].green, style->text_aa[i].blue);
      
      gc_values.foreground = style->fg[i];
      style->fg_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->bg[i];
      style->bg_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->light[i];
      style->light_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->dark[i];
      style->dark_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->mid[i];
      style->mid_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->text[i];
      style->text_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->base[i];
      style->base_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->text_aa[i];
      style->text_aa_gc[i] = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
    }
}


static void
realize (GtkStyle* style)
{
  gtk_style_real_realize(style);
}

static void
set_background (GtkStyle* style, GdkWindow* window, GtkStateType stateType)
{
	GdkPixmap* pixmap = NULL;
	gint parent_relative;
	
	GtkWidget* parent = 0;
	gdk_window_get_user_data(window, (void**) &parent);
	
	if (GTK_IS_MENU((GtkWidget*) parent))
		pixmap = menuBackground();
	
	if (pixmap == 0) /* menuBackground() can return null if the theme engine is disabled */
		pixmap = style->bg_pixmap[stateType];
	
	if (pixmap)
	{
		if (pixmap == (GdkPixmap*) GDK_PARENT_RELATIVE)
		{
			pixmap = NULL;
			parent_relative = TRUE;
		}
		else
		{
			parent_relative = FALSE;
			gdk_drawable_set_colormap(pixmap, style->colormap);
		}
	
		if (pixmap && !gdk_drawable_get_colormap(pixmap))
			gdk_drawable_set_colormap(pixmap, gdk_drawable_get_colormap(window));
		gdk_window_set_back_pixmap(window, pixmap, parent_relative);
	}
	else
		gdk_window_set_background(window, &style->bg[stateType]);
}

static void
qtengine_style_class_init (QtEngineStyleClass *klass)
{
	GtkStyleClass *style_class = GTK_STYLE_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	
	style_class->draw_hline = draw_hline;
	style_class->draw_vline = draw_vline;
	style_class->draw_shadow = draw_shadow;
	style_class->draw_polygon = draw_polygon;
	style_class->draw_arrow = draw_arrow;
	style_class->draw_diamond = draw_diamond;
	/*style_class->draw_string = draw_string;*/
	style_class->draw_box = draw_box;
	style_class->draw_flat_box = draw_flat_box;
	style_class->draw_check = draw_check;
	style_class->draw_option = draw_option;
	style_class->draw_tab = draw_tab;
	style_class->draw_shadow_gap = draw_shadow_gap;
	
	/* box around notebooks */
	style_class->draw_box_gap = draw_box_gap;
	/* the tab */
	style_class->draw_extension = draw_extension;
	
	style_class->draw_focus = draw_focus;
	style_class->draw_handle = draw_handle;
	style_class->draw_layout = draw_layout;
	style_class->draw_slider = draw_slider;
	
	style_class->realize = realize;
	
	style_class->set_background = set_background;
}


