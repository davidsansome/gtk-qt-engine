#include <math.h>
#include <string.h>
#include <gtk/gtkprogressbar.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/glist.h>
/*#include <libbonobo.h>
#include <libbonoboui.h>*/
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "qt_style.h"
#include "qt_rc_style.h"
#include "qt_qt_wrapper.h"

#define DETAIL(xx) ((detail) && (!strcmp(xx, detail)))
#define DETAILHAS(xx) ((detail) && (strstr(detail, xx)))
#define PARENT(xx) ((parent) && (!strcmp(xx, gtk_widget_get_name(parent))))
#ifndef max
#define max(x,y) ((x)>=(y)?(x):(y))
#endif
#ifndef min
#define min(x,y) ((x)<=(y)?(x):(y))
#endif

static void qtengine_style_init       (QtEngineStyle      *style);
static void qtengine_style_class_init (QtEngineStyleClass *klass);

static GtkNotebook *notebook = NULL;
static int nb_num_pages = 0;

static GtkStyleClass *parent_class = NULL;

static PangoLayout*
get_insensitive_layout (GdkDrawable *drawable,
                        PangoLayout *layout);


static GtkShadowType
get_shadow_type (GtkStyle* style, const char *detail, GtkShadowType requested)
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

static void sanitize_size(GdkWindow* window, gint* width, gint* height)
{
	if ((*width == -1) && (*height == -1))
		gdk_window_get_size (window, width, height);
	else if (*width == -1)
		gdk_window_get_size (window, width, NULL);
	else if (*height == -1)
		gdk_window_get_size (window, NULL, height);
}


static void
draw_hline(GtkStyle* style,
           GdkWindow* window,
           GtkStateType state_type,
           GdkRectangle* area,
           GtkWidget* widget,
           const gchar* detail,
           gint x1,
           gint x2,
           gint y)
{
	if (gtkQtDebug)
		printf("HLINE (%d,%d,%d) Widget: %s  Detail: %s\n",x1,y1,y,gtk_widget_get_name(widget),detail);

	if (DETAIL("vscale"))
		return;

	drawHLine(window,style,state_type,y,x1,x2);
}


static void
draw_vline(GtkStyle* style,
           GdkWindow* window,
           GtkStateType state_type,
           GdkRectangle* area,
           GtkWidget* widget,
           const gchar* detail,
           gint ySource,
           gint yDest,
           gint x)
{
	if (gtkQtDebug)
		printf("VLINE (%d,%d,%d) Widget: %s  Detail: %s\n",ySource ,yDest ,x,gtk_widget_get_name(widget),detail);


	if (DETAIL("hscale"))
		return;
	drawVLine(window,style,state_type,x,ySource,yDest);
}

static void
draw_shadow(GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state_type,
	    GtkShadowType shadow_type,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x,
	    gint          y,
	    gint          width,
	    gint          height)
{
	GdkGC *gc1 = NULL;		/* Initialize to quiet GCC */
	GdkGC *gc2 = NULL;
	GdkGC *gc3 = NULL;
	GdkGC *gc4 = NULL;
	
	gint thickness_light;
	gint thickness_dark;
	gint i;
	
	sanitize_size(window, &width, &height);
	
	if (gtkQtDebug)
		printf("Shadow (%d,%d,%d,%d) Widget: %s Detail: %s\n",x,y,width,height,gtk_widget_get_name(widget),detail);

	
	if (DETAIL("menuitem"))
		return;
	if (DETAIL("menu"))
		return;
	if (DETAIL("entry"))
	{
		drawLineEdit(window,style,state_type,gtk_widget_is_focus(widget),x,y,width,height);
		return;
	}
	if (DETAIL("frame") || DETAIL("trough") || DETAIL("viewport"))
	{
		if (!GTK_IS_SCALE(widget))
		{
			/*printf("Frame (%d,%d) %dx%d   %d %d\n", x,y,width,height,state_type, shadow_type);*/
			drawFrame(window,style,state_type,shadow_type,x,y,width,height);
			return;
		}
	}
	
	/* The remainder of this function was borrowed from the "Metal" theme/
	   I don't really want to use Qt to draw these frames as there are too
	   many of them (it would slow down the theme engine even more).
	   TODO: Make them use the Qt color palette */
	 
	switch (shadow_type)
	{
	case GTK_SHADOW_NONE:
	case GTK_SHADOW_IN:
	case GTK_SHADOW_ETCHED_IN:
		gc1 = style->light_gc[state_type];
		gc2 = style->dark_gc[state_type];
		gc3 = style->black_gc;
		gc4 = style->bg_gc[state_type];
		break;
	case GTK_SHADOW_OUT:
	case GTK_SHADOW_ETCHED_OUT:
		gc1 = style->dark_gc[state_type];
		gc2 = style->light_gc[state_type];
		gc3 = style->black_gc;
		gc4 = style->bg_gc[state_type];
		break;
	}
	
	if (area)
	{
		gdk_gc_set_clip_rectangle (gc1, area);
		gdk_gc_set_clip_rectangle (gc2, area);
		gdk_gc_set_clip_rectangle (gc3, area);
		gdk_gc_set_clip_rectangle (gc4, area);
	}
	
	switch (shadow_type)
	{
	case GTK_SHADOW_NONE:
		break;
	case GTK_SHADOW_IN:
		gdk_draw_line (window, gc1,
				x, y + height - 1, x + width - 1, y + height - 1);
		gdk_draw_line (window, gc1,
				x + width - 1, y, x + width - 1, y + height - 1);
		
		gdk_draw_line (window, gc4,
				x + 1, y + height - 2, x + width - 2, y + height - 2);
		gdk_draw_line (window, gc4,
				x + width - 2, y + 1, x + width - 2, y + height - 2);
		
		gdk_draw_line (window, gc3,
				x + 1, y + 1, x + width - 2, y + 1);
		gdk_draw_line (window, gc3,
				x + 1, y + 1, x + 1, y + height - 2);
		
		gdk_draw_line (window, gc2,
				x, y, x + width - 1, y);
		gdk_draw_line (window, gc2,
				x, y, x, y + height - 1);
		break;
	
	case GTK_SHADOW_OUT:
		gdk_draw_line (window, gc1,
				x + 1, y + height - 2, x + width - 2, y + height - 2);
		gdk_draw_line (window, gc1,
				x + width - 2, y + 1, x + width - 2, y + height - 2);
		
		gdk_draw_line (window, gc2,
				x, y, x + width - 1, y);
		gdk_draw_line (window, gc2,
				x, y, x, y + height - 1);
		
		gdk_draw_line (window, gc4,
				x + 1, y + 1, x + width - 2, y + 1);
		gdk_draw_line (window, gc4,
				x + 1, y + 1, x + 1, y + height - 2);
		
		gdk_draw_line (window, gc3,
				x, y + height - 1, x + width - 1, y + height - 1);
		gdk_draw_line (window, gc3,
				x + width - 1, y, x + width - 1, y + height - 1);
		break;
	case GTK_SHADOW_ETCHED_IN:
	case GTK_SHADOW_ETCHED_OUT:
		thickness_light = 1;
		thickness_dark = 1;
	
		for (i = 0; i < thickness_dark; i++)
		{
			gdk_draw_line (window, gc1,
					x + i,
					y + height - i - 1,
					x + width - i - 1,
					y + height - i - 1);
			gdk_draw_line (window, gc1,
					x + width - i - 1,
					y + i,
					x + width - i - 1,
					y + height - i - 1);
		
			gdk_draw_line (window, gc2,
					x + i,
					y + i,
					x + width - i - 2,
					y + i);
			gdk_draw_line (window, gc2,
					x + i,
					y + i,
					x + i,
					y + height - i - 2);
		}
	
		for (i = 0; i < thickness_light; i++)
		{
			gdk_draw_line (window, gc1,
					x + thickness_dark + i,
					y + thickness_dark + i,
					x + width - thickness_dark - i - 1,
					y + thickness_dark + i);
			gdk_draw_line (window, gc1,
					x + thickness_dark + i,
					y + thickness_dark + i,
					x + thickness_dark + i,
					y + height - thickness_dark - i - 1);
		
			gdk_draw_line (window, gc2,
					x + thickness_dark + i,
					y + height - thickness_light - i - 1,
					x + width - thickness_light - 1,
					y + height - thickness_light - i - 1);
			gdk_draw_line (window, gc2,
					x + width - thickness_light - i - 1,
					y + thickness_dark + i,
					x + width - thickness_light - i - 1,
					y + height - thickness_light - 1);
		}
		break;
	}
	
	if (area)
	{
		gdk_gc_set_clip_rectangle (gc1, NULL);
		gdk_gc_set_clip_rectangle (gc2, NULL);
		gdk_gc_set_clip_rectangle (gc3, NULL);
		gdk_gc_set_clip_rectangle (gc4, NULL);
	}
	
	return;
}

static void
draw_polygon(GtkStyle* style,
             GdkWindow* window,
             GtkStateType state_type,
             GtkShadowType shadow_type,
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

  switch (shadow_type)
    {
    case GTK_SHADOW_IN:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->dark_gc[state_type];
      break;
    case GTK_SHADOW_ETCHED_IN:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case GTK_SHADOW_OUT:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case GTK_SHADOW_ETCHED_OUT:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->dark_gc[state_type];
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
    gdk_draw_polygon(window, style->bg_gc[state_type], TRUE, points, npoints);

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
           GtkStateType state_type,
           GtkShadowType shadow_type,
           GdkRectangle* area,
           GtkWidget* widget,
           const gchar *detail,
           GtkArrowType arrow_type,
           gint fill, gint x, gint y, gint width, gint height)
{
	sanitize_size(window, &width, &height);
	
	if (gtkQtDebug)
		printf("Arrow (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	if (DETAIL("hscrollbar") || DETAIL("vscrollbar"))
		return;
	if (DETAIL("spinbutton"))
		return;
	if (DETAIL("notebook"))
	{
		drawArrow(window, style, state_type, arrow_type, x, y, width, height);
		return;  
	}
	if (DETAIL("arrow"))
	{
		GdkPixbuf *gpix;
		GtkWidget* parent;
		if (gdk_window_is_viewable(gtk_widget_get_parent_window(widget)))
		{
			gpix = gdk_pixbuf_get_from_drawable(NULL, gtk_widget_get_parent_window(widget),NULL, x, y, 0, 0,  width, height);
			setFillPixmap(gpix);
			g_object_unref(gpix);
		}
		
		parent = gtk_widget_get_parent(widget);
		drawArrow(window,style, GTK_WIDGET_STATE(parent), arrow_type, x, y, width, height);
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
	
		switch (shadow_type)
		{
		case GTK_SHADOW_IN:
			gc1 = style->bg_gc[state_type];
			gc2 = style->dark_gc[state_type];
			gc3 = style->light_gc[state_type];
			gc4 = style->black_gc;
			break;
		case GTK_SHADOW_OUT:
			gc1 = style->dark_gc[state_type];
			gc2 = style->light_gc[state_type];
			gc3 = style->black_gc;
			gc4 = style->bg_gc[state_type];
			break;
		case GTK_SHADOW_ETCHED_IN:
			gc2 = style->light_gc[state_type];
			gc1 = style->dark_gc[state_type];
			gc3 = NULL;
			gc4 = NULL;
			break;
		case GTK_SHADOW_ETCHED_OUT:
			gc1 = style->dark_gc[state_type];
			gc2 = style->light_gc[state_type];
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
		
		if (state_type == GTK_STATE_INSENSITIVE)
			draw_black_arrow (window, style->white_gc, area, arrow_type, ax + 1, ay + 1, aw, ah);
		draw_black_arrow (window, style->fg_gc[state_type], area, arrow_type, ax, ay, aw, ah);
		
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
		GdkPixbuf *gpix;
		if (gdk_window_is_viewable(gtk_widget_get_parent_window(widget)))
		{
			gpix = gdk_pixbuf_get_from_drawable(NULL, gtk_widget_get_parent_window(widget),NULL, x, y, 0, 0,  width, height);
			setFillPixmap(gpix);
			g_object_unref(gpix);
		}
		
		drawArrow(window, style, state_type, arrow_type, x, y, width, height);
 		return;  
	}
}



static void
draw_diamond(GtkStyle * style,
             GdkWindow * window,
             GtkStateType state_type,
             GtkShadowType shadow_type,
             GdkRectangle * area,
             GtkWidget * widget,
             const gchar *detail,
             gint x,
             gint y,
             gint width,
             gint height)
{
}



static void
draw_box(GtkStyle * style,
	GdkWindow * window,
	GtkStateType state_type,
	GtkShadowType shadow_type,
	GdkRectangle * area,
	GtkWidget * widget,
	const gchar *detail,
	gint x,
	gint y,
	gint width,
	gint height)
{
	GList *child1;
	GtkWidget *child;
	GtkNotebook *nb;
	int nbpages;
	sanitize_size(window, &width, &height);
	
	if (gtkQtDebug)
		printf("Box (%d,%d,%d,%d) Widget: %s  Detail: %s\n",x,y,width,height,gtk_widget_get_name(widget),detail);

	if (GTK_IS_SCROLLBAR(widget))
	{
		if (DETAIL("trough"))
		{
			GtkAdjustment* adj = (GtkAdjustment*)gtk_range_get_adjustment(GTK_RANGE(widget));
			int orientation = ((width>height) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
			
			drawScrollBar(window, style, state_type, orientation, adj, x, y, width, height);
		}
		return;
	}
	if (DETAIL("menuitem"))
	{
		/* Crude way of checking if it's a menu item, or a menubar item */
		if (x != 0)
			drawMenuBarItem(window,style,state_type,x,y,width,height);
		else
			drawMenuItem(window,style,state_type,x,y,width,height);
		return;
	}
	if (DETAIL("menubar"))
	{
		if (openOfficeFix == 1)
			 parent_class->draw_box (style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
		else
			drawMenubar(window,style,state_type,x,y,width,height);
		return;
	}
	if (DETAIL("menu"))
	{
		if (openOfficeFix == 1)
			 parent_class->draw_box (style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
		else
		{
			if ((x >= 0) && (y >= 0)) /* Work around weirdness in firefox */
				drawMenu(window,style,state_type,x,y,width,height);
		}
		return;
	}
	if (GTK_IS_PROGRESS(widget) && DETAIL("trough"))
	{
		double fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(widget));
		GtkProgressBarOrientation orientation = gtk_progress_bar_get_orientation(GTK_PROGRESS_BAR(widget));
		
		drawProgressBar(window,style,state_type,orientation,fraction,x,y,width,height);
		return;
	}
	if (GTK_IS_PROGRESS(widget) && DETAIL("bar"))
	{
		if (area) gdk_gc_set_clip_rectangle(style->bg_gc[state_type], area);
		
		drawProgressChunk(window,style,state_type,x,y,width,height);
		
		if (area) gdk_gc_set_clip_rectangle(style->bg_gc[state_type], NULL);
		return;
	}
	if (GTK_IS_SCALE(widget) && DETAIL("trough"))
	{
		GtkAdjustment* adj;
		int inverted;
		GValue *val = (GValue*)g_malloc( sizeof(GValue) );
		if (gdk_window_is_viewable(gtk_widget_get_parent_window(widget)))
		{
			GdkPixbuf *gpix;
			gpix = gdk_pixbuf_get_from_drawable(NULL, gtk_widget_get_parent_window(widget),NULL, x, y, 0, 0,  width, height);
			setFillPixmap(gpix);
			g_object_unref(gpix);
		}
		
		memset( val, 0, sizeof(GValue) );
		g_value_init( val, G_TYPE_BOOLEAN );
		g_object_get_property(widget, "inverted", val);
		inverted = g_value_get_boolean(val);
		g_value_unset(val);
		g_free(val);
		
		adj = gtk_range_get_adjustment((GtkRange *) widget);
		drawSlider(window,style,state_type,adj,x,y,width,height, (GTK_RANGE(widget))->orientation, inverted);
		return;
	}
	if (DETAIL("button"))
	{
		GtkWidget *parent;
		int toolbutton = 0;
		parent = gtk_widget_get_parent(widget);

		if (parent && (GTK_IS_CLIST(parent) || GTK_IS_LIST(parent) || GTK_IS_TREE_VIEW(parent)))
		{
			drawListHeader(window,style,state_type,x,y,width,height);
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

		if (toolbutton)
			drawToolButton(window,style,state_type,x,y,width,height);
		else
		{ 
			/* Baghira hack -- rounded buttons really ugly when they are small like 
			   on a dropdown entry box -- eg. search/replace in gedit */	
			/* Draw square buttons only if number of children in the hbox is 2 and 
	 		 * the first child is a entry view (GtkEntry)*/ 
			
			if (isBaghira && GTK_IS_BOX(parent) && (g_list_length(GTK_BOX(parent)->children) == 2))
			{
				child1 = g_list_first((GTK_BOX(parent)->children));
				child = ((GtkBoxChild *)child1->data)->widget;
				if (GTK_IS_ENTRY(child))
				{
					drawSquareButton(window,style,state_type,x,y,width,height);
					return;
				}
				
				child1 = g_list_last((GTK_BOX(parent)->children));
				child = ((GtkBoxChild *)child1->data)->widget;
				if (GTK_IS_ENTRY(child))
				{
					drawSquareButton(window,style,state_type,x,y,width,height);
					return;
				}

			}
			
			drawButton(window,style,state_type,x,y,width,height);
		}
		return;
	}
	if (DETAIL("tab"))
	{
		if (GTK_IS_NOTEBOOK(widget))
		{
			nb = (GtkNotebook *)widget;
			nbpages = g_list_length(nb->children);
			/* THIS IS WHAT WORKS NOW --
				Tabs and tabbarbase will be drawn properly according to the QT style
				But the tabs won't be aligned according to QT. GTK+ does not have 
				an option for alignment of tabs. So if were to do this not only do we have to 
				calculate the x,y position of the tab ourselves, which is difficult in Qt unless
				we are displaying the tab (can be done by subclassing QTabBar/QTabWidget)
				but also have to position the tab bar label ourselves in gtk. 
			*/
			
			/* Check if we have seen this notebook before */
			if ((nb != notebook) || (nbpages != nb_num_pages))
			{
				notebook = nb;
				nb_num_pages = nbpages;
				initDrawTabNG(nbpages);
			}
			
			/* Now draw the tab -- tab position is also calculated in this function
			   checkout drawTabFrame() for drawing tabbarbase. */
			drawTabNG(window,style,state_type,x, y, width - 2, height, nb );
		}
		else
			drawTab(window,style,state_type,x,y,width-2,height);
		return;
	}
	if (DETAIL("optionmenu"))
	{
		drawComboBox(window,style,state_type,x,y,width,height);
		return;
	}
	if (DETAIL("toolbar"))
	{
		if (openOfficeFix == 1)
			parent_class->draw_box (style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
		else
			drawToolbar(window,style,state_type,x,y,width,height);
		return;
	}
	if (DETAIL("spinbutton_up"))
	{
		drawSpinButton(window, style, state_type, 0, x, y, width, height);
		return;
	}
	if (DETAIL("spinbutton_down"))
	{
		drawSpinButton(window, style, state_type, 1, x, y, width, height);
		return;
	}
	if (DETAIL("spinbutton"))
		return;
	
	if (DETAIL("optionmenutab") || DETAIL("buttondefault"))
		return;
	
	drawFrame(window,style,state_type,shadow_type,x,y,width,height);
}



static void
draw_flat_box(GtkStyle * style,
              GdkWindow * window,
              GtkStateType state_type,
              GtkShadowType shadow_type,
              GdkRectangle * area,
              GtkWidget * widget,
              const gchar *detail,
              gint x,
              gint y,
              gint width,
              gint height)
{
	sanitize_size(window, &width, &height);
	
	if (gtkQtDebug)
		printf("Flat Box (%d,%d,%d,%d) Widget: %s  Detail: %s %d %d\n",x,y,width,height,gtk_widget_get_name(widget),detail, state_type, GTK_STATE_SELECTED);

	if (DETAIL("tooltip"))
	{
		GdkColor tooltipColor;
		GdkGCValues gc_values;
		GdkGCValuesMask gc_values_mask;
		GdkGC* tooltipGc;
		tooltipColor.red = 255*257;
		tooltipColor.green = 255*257;
		tooltipColor.blue = 220*257;
		gdk_colormap_alloc_color(style->colormap, &tooltipColor, FALSE, TRUE);
		
		gc_values_mask = GDK_GC_FOREGROUND;
		gc_values.foreground = tooltipColor;
		
		tooltipGc = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
		gdk_draw_rectangle(window, tooltipGc, TRUE, x, y, width, height);
		gdk_draw_rectangle(window, style->black_gc, FALSE, x, y, width - 1, height - 1);
		
		gtk_gc_release(tooltipGc);
	}
	
	if ((DETAILHAS("cell_even") || DETAILHAS("cell_odd")) && (state_type == GTK_STATE_SELECTED))
	{
		drawListViewItem(window,style,state_type,x,y,width,height);
	}
	else if (DETAIL("listitem"))
	{
		drawListViewItem(window,style,state_type,x,y,width,height);
	}
	else if (DETAILHAS("cell_even"))
	{
		gdk_draw_rectangle(window, style->base_gc[GTK_STATE_NORMAL], TRUE, x, y, width, height);
	}
	else if (DETAILHAS("cell_odd"))
	{
		gdk_draw_rectangle(window, alternateBackgroundGc(style), TRUE, x, y, width, height);
	}
}


static void
draw_check(GtkStyle * style,
           GdkWindow * window,
           GtkStateType state_type,
           GtkShadowType shadow_type,
           GdkRectangle * area,
           GtkWidget * widget,
           const gchar *detail,
           gint x,
           gint y,
           gint width,
           gint height)
{
	if (gtkQtDebug)
		printf("Check (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	if (GTK_IS_MENU_ITEM(widget))
	{
		if (shadow_type == GTK_SHADOW_IN)
		{
			if (gdk_window_is_viewable(gtk_widget_get_parent_window(widget)))
			{
				GdkPixbuf *gpix;
				gpix = gdk_pixbuf_get_from_drawable(NULL, gtk_widget_get_parent_window(widget), NULL, x, y, 0, 0, width, height);
				setFillPixmap(gpix);
				g_object_unref(gpix);
			}
		
			drawMenuCheck(window,style,state_type,x,y,width,height);
		}
		return;
	}
	drawCheckBox(window,style,state_type,(shadow_type==GTK_SHADOW_IN),x,y,width,height);
}


/* Thanks to Evan Lawrence */
static void
draw_option(GtkStyle * style,
            GdkWindow * window,
            GtkStateType state_type,
            GtkShadowType shadow_type,
            GdkRectangle * area,
            GtkWidget * widget,
            const gchar *detail,
            gint x,
            gint y,
            gint width,
            gint height)
{
	if (gtkQtDebug)
		printf("Option (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	if (gdk_window_is_viewable(gtk_widget_get_parent_window(widget)))
	{
		GdkPixbuf *gpix;
		gpix = gdk_pixbuf_get_from_drawable(NULL, gtk_widget_get_parent_window(widget),NULL, x, y, 0, 0,  width, height);
		setFillPixmap(gpix);
		g_object_unref(gpix);
	}

	if (GTK_IS_MENU_ITEM(widget))
	{
		if (shadow_type == GTK_SHADOW_IN)
			drawMenuCheck(window,style,state_type,x,y,width,height);
		return;
	}
	drawRadioButton(window,style,state_type,(shadow_type==GTK_SHADOW_IN),x,y,width,height);
}


static void
draw_tab(GtkStyle * style,
         GdkWindow * window,
         GtkStateType state_type,
         GtkShadowType shadow_type,
         GdkRectangle * area,
         GtkWidget * widget,
         const gchar *detail,
         gint x,
         gint y,
         gint width,
         gint height)
{
	if (gtkQtDebug)
		printf("Tab (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	gtk_paint_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}



static void
draw_shadow_gap(GtkStyle * style,
                GdkWindow * window,
                GtkStateType state_type,
                GtkShadowType shadow_type,
                GdkRectangle * area,
                GtkWidget * widget,
                const gchar *detail,
                gint x,
                gint y,
                gint width,
                gint height,
                GtkPositionType gap_side,
                gint gap_x,
                gint gap_width)
{
	GdkGC *gc1 = NULL;
	GdkGC *gc2 = NULL;
	
	g_return_if_fail (window != NULL);
	
	sanitize_size (window, &width, &height);
	shadow_type = get_shadow_type (style, detail, shadow_type);
	
	if (gtkQtDebug)
		printf("Shadow_Gap (%d,%d,%d,%d) Widget: %s  Detail: %s\n",x,y,width,height,gtk_widget_get_name(widget),detail);
	
	switch (shadow_type) {
	case GTK_SHADOW_NONE:
		return;
	case GTK_SHADOW_IN:
		gc1 = style->dark_gc[state_type];
		gc2 = style->light_gc[state_type];
		break;
	case GTK_SHADOW_OUT:
		gc1 = style->light_gc[state_type];
		gc2 = style->dark_gc[state_type];
		break;
	case GTK_SHADOW_ETCHED_IN:
	case GTK_SHADOW_ETCHED_OUT:
		gc1 = style->dark_gc[state_type];
		gc2 = style->dark_gc[state_type];
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
		if ((width - (gap_x + gap_width)) > 0) {
			gdk_draw_line (window, gc1, 
				       x + gap_x + gap_width - 1, y,
				       x + width - 1, y);
		}
		gdk_draw_line (window, gc1, 
			       x, y, 
			       x, y + height - 1);
		gdk_draw_line (window, gc2,
			       x + width - 1, y,
			       x + width - 1, y + height - 1);
		gdk_draw_line (window, gc2,
			       x, y + height - 1,
			       x + width - 1, y + height - 1);
		break;
        case GTK_POS_BOTTOM:
		gdk_draw_line (window, gc1,
			       x, y,
			       x + width - 1, y);
		gdk_draw_line (window, gc1, 
			       x, y, 
			       x, y + height - 1);
		gdk_draw_line (window, gc2,
			       x + width - 1, y,
			       x + width - 1, y + height - 1);

		if (gap_x > 0) {
			gdk_draw_line (window, gc2, 
				       x, y + height - 1, 
				       x + gap_x, y + height - 1);
		}
		if ((width - (gap_x + gap_width)) > 0) {
			gdk_draw_line (window, gc2, 
				       x + gap_x + gap_width - 1, y + height - 1,
				       x + width - 1, y + height - 1);
		}
		
		break;
        case GTK_POS_LEFT:
		gdk_draw_line (window, gc1,
			       x, y,
			       x + width - 1, y);
		if (gap_x > 0) {
			gdk_draw_line (window, gc1, 
				       x, y,
				       x, y + gap_x);
		}
		if ((height - (gap_x + gap_width)) > 0) {
			gdk_draw_line (window, gc1, 
				       x, y + gap_x + gap_width - 1,
				       x, y + height - 1);
		}
		gdk_draw_line (window, gc2,
			       x + width - 1, y,
			       x + width - 1, y + height - 1);
		gdk_draw_line (window, gc2,
			       x, y + height - 1,
			       x + width - 1, y + height - 1);
		break;
        case GTK_POS_RIGHT:
		gdk_draw_line (window, gc1,
			       x, y,
			       x + width - 1, y);
		gdk_draw_line (window, gc1, 
			       x, y, 
			       x, y + height - 1);


		if (gap_x > 0) {
			gdk_draw_line (window, gc2, 
				       x + width - 1, y,
				       x + width - 1, y + gap_x);
		}
		if ((height - (gap_x + gap_width)) > 0) {
			gdk_draw_line (window, gc2, 
				       x + width - 1, y + gap_x + gap_width - 1,
				       x + width - 1, y + height - 1);
		}
		gdk_draw_line (window, gc2,
			       x, y + height - 1,
			       x + width - 1, y + height - 1);

	}
	
	if (area) {
		gdk_gc_set_clip_rectangle (gc1, NULL);
		gdk_gc_set_clip_rectangle (gc2, NULL);
	}
}


static void
draw_box_gap(GtkStyle* style,
             GdkWindow* window,
             GtkStateType state_type,
             GtkShadowType shadow_type,
             GdkRectangle* area,
             GtkWidget* widget,
             const gchar* detail,
             gint x,
             gint y,
             gint width,
             gint height,
             GtkPositionType gap_side,
             gint gap_x,
             gint gap_width)
{
	sanitize_size (window, &width, &height);
	
	if (width<0 || height<0) return;  /* Eclipse really can be this stupid! */
	
	if (gtkQtDebug)
		printf("Box_gap (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	if (DETAIL("notebook"))
		drawTabFrame(window,style,state_type,x,y-2,width,height+2, gtk_notebook_get_tab_pos((GtkNotebook *)widget));
}


static void
draw_extension(GtkStyle * style,
               GdkWindow * window,
               GtkStateType state_type,
               GtkShadowType shadow_type,
               GdkRectangle * area,
               GtkWidget * widget,
               const gchar *detail,
               gint x,
               gint y,
               gint width,
               gint height,
               GtkPositionType gap_side)
{
	g_return_if_fail(style != NULL);
	g_return_if_fail(window != NULL);
	
	sanitize_size (window, &width, &height);
	
	if (gtkQtDebug)
		printf("Extension (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	gtk_paint_box(style, window, state_type, shadow_type, area, widget, detail,
			x, y, width, height);
}


static void
draw_focus (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state_type,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x,
	    gint          y,
	    gint          width,
	    gint          height)
{
	return;
}

static void
draw_slider(GtkStyle * style,
            GdkWindow * window,
            GtkStateType state_type,
            GtkShadowType shadow_type,
            GdkRectangle * area,
            GtkWidget * widget,
            const gchar *detail,
            gint x,
            gint y,
            gint width,
            gint height,
            GtkOrientation orientation)
{
	if (gtkQtDebug)
		printf("Slider (%d,%d,%d,%d) Widget: %s  Detail: %s\n", x, y, width, height,gtk_widget_get_name(widget),detail);
	
	if (DETAIL("slider"))
	{
		GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(widget));
		int widgetX, widgetY;
		
		GtkWidget* parent = widget;
		while (gtk_widget_get_parent(parent) != NULL)
			parent = gtk_widget_get_parent(parent);
		
		gtk_widget_translate_coordinates(widget, parent, 0, 0, &widgetX, &widgetY);
		
		if (orientation == GTK_ORIENTATION_VERTICAL)
			drawScrollBarSlider(window, style, state_type, orientation, adj, x-1, y, width+2, height, y-widgetY, widget->allocation.height);
		else
			drawScrollBarSlider(window, style, state_type, orientation, adj, x, y-1, width, height+2, x-widgetX, widget->allocation.width);
		return;
	}
}

static void
draw_handle(GtkStyle * style,
            GdkWindow * window,
            GtkStateType state_type,
            GtkShadowType shadow_type,
            GdkRectangle * area,
            GtkWidget * widget,
            const gchar *detail,
            gint x,
            gint y,
            gint width,
            gint height,
            GtkOrientation orientation)
{
	g_return_if_fail(style != NULL);
	g_return_if_fail(window != NULL);
	
	sanitize_size(window, &width, &height);
	
	if (gtkQtDebug)
		printf("Handle (%d,%d,%d,%d) Widget: %s  Detail: %s \n",x,y,width,height,gtk_widget_get_name(widget),detail, state_type);
	
	drawSplitter(window,style,state_type,orientation,x,y,width,height);
	return;
}

static
void  draw_layout  (GtkStyle *style,
                        GdkWindow *window,
                        GtkStateType state_type,
                        gboolean use_text,
                        GdkRectangle *area,
                        GtkWidget *widget,
                        const gchar *detail,
                        gint x,
                        gint y,
                        PangoLayout *layout)
{

	GdkColor color;
	GdkGC *gc;
	getTextColor(&color, state_type);
	
	if (gtkQtDebug)
		printf("Layout (%d,%d) Widget: %s  Detail: %s %d \n",x,y,gtk_widget_get_name(widget),detail, state_type);

	if (DETAIL("accellabel") || DETAIL("label") || DETAIL("cellrenderertext"))
	{
		
		GtkWidget* parent = gtk_widget_get_parent(widget);
		GtkWidget* parent1 = gtk_widget_get_parent(parent);

		/* printf("parent's names are %s->%s->%s\n", gtk_widget_get_name(widget), gtk_widget_get_name(parent), gtk_widget_get_name(parent1)); */

		/* In baghira -- even highlight the menu bar items */
		if ((GTK_IS_MENU_ITEM(parent) && (!GTK_IS_MENU_BAR(parent1) || isBaghira)) || GTK_IS_TREE_VIEW(widget))
		{
			PangoAttrList *layoutattr;
		
			const gchar *text;
			gint text_length = 0;
			gint text_bytelen = 0;
			text = pango_layout_get_text (layout);
			if (text != 0)
			{
				PangoAttribute *textcolorattr;
				text_length = g_utf8_strlen (text, -1);
				text_bytelen = strlen (text);

				/* Try to get the attribute list of the layout */
				PangoAttrList* attrlist = pango_layout_get_attributes(layout);
				if(attrlist) {
					/* Now iterate over the attribute list */
					PangoAttrIterator* it = pango_attr_list_get_iterator(attrlist);
					if(it) {
						/* Try to get the first foreground color attribute
						   Note that there can be more than one foreground 
						   color, we use only the first */
						PangoAttrColor* fg = (PangoAttrColor*)pango_attr_iterator_get(it, PANGO_ATTR_FOREGROUND);
						if(fg) {
							/* Use this color to render the text, instead of
							   the default text color */
							color.red	= fg->color.red;
							color.green	= fg->color.green;
							color.blue	= fg->color.blue;
						}
						pango_attr_iterator_destroy(it);
					}
				}
				
				textcolorattr = pango_attr_foreground_new(color.red, color.green, color.blue);
				textcolorattr->start_index = 0;
				textcolorattr->end_index = text_bytelen; 
				
				layoutattr = pango_layout_get_attributes(layout); 
				
				if (layoutattr == NULL)
				{
					layoutattr = pango_attr_list_new();
					pango_attr_list_insert(layoutattr, pango_attribute_copy(textcolorattr));
					pango_layout_set_attributes(layout,layoutattr);
					pango_attr_list_unref(layoutattr);
				}
				else
				{
					pango_attr_list_change(layoutattr, pango_attribute_copy(textcolorattr));
					pango_layout_set_attributes(layout,layoutattr);
				}
				pango_attribute_destroy(textcolorattr);
			}

		}
		/* printf("Drawing an label -- with state %d at %d %d\n", state_type, x, y); */
	}

	g_return_if_fail (window != NULL);

	gc = use_text ? style->text_gc[state_type] : style->fg_gc[state_type];

	if (area)
    		gdk_gc_set_clip_rectangle (gc, area);

  	if (state_type == GTK_STATE_INSENSITIVE)
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
  setColors(style);
  gtk_style_real_realize(style);
}

static void
set_background (GtkStyle *style, GdkWindow *window, GtkStateType state_type)
{
  GdkPixmap *pixmap;
  gint parent_relative;
  GdkPixmap* pix_test;
  
  /* What kind of horrible person would store a pointer to a widget here... */
  GtkWidget* parent = 0;
  gdk_window_get_user_data(window, (void **) &parent);
  if (GTK_IS_MENU(parent))
  {
    pix_test = QTENGINE_STYLE(style)->menuBackground;
  }
  else
    pix_test = style->bg_pixmap[state_type];
  
  if (pix_test)
    {
      if (pix_test == (GdkPixmap*) GDK_PARENT_RELATIVE)
        {
          pixmap = NULL;
          parent_relative = TRUE;
        }
      else
        {
          pixmap = pix_test;
          parent_relative = FALSE;
	  gdk_drawable_set_colormap(pixmap, style->colormap);
        }
      
      if (pixmap && !gdk_drawable_get_colormap (pixmap)) gdk_drawable_set_colormap (pixmap, gdk_drawable_get_colormap (window));
      gdk_window_set_back_pixmap (window, pixmap, parent_relative);
    }
  else
    gdk_window_set_background (window, &style->bg[state_type]);
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


