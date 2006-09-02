#ifndef QTENGINE_QT_WRAPPER_H
#define QTENGINE_QT_WRAPPER_H


#include <gtk/gtknotebook.h>
#include <gdk/gdkgc.h>
#include <gtk/gtkstyle.h>
#include <gtk/gtkprogressbar.h>

#ifdef __cplusplus
extern "C" {

void mapColour(GdkColor* g, QColor q);

QString doIconMapping(const QString& stockName, const QString& path, int sizes = 7);

void initKdeSettings();
QString kdeConfigValue(const QString& section, const QString& name, const QString& def);
QString kdeFindDir(const QString& suffix, const QString& file1, const QString& file2);

Atom kipcCommAtom;
Atom desktopWindowAtom;
GdkFilterReturn gdkEventFilter(GdkXEvent *xevent, GdkEvent *event, gpointer data);

#endif

void createQApp();
void destroyQApp();
void setColors(GtkStyle* style);
void setRcProperties(GtkRcStyle* rc_style, int forceRecreate);
void drawButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawSquareButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
int findCachedButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawToolButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawTab(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void initDrawTabNG(int count);
void drawTabNG(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int width, int height, GtkNotebook *notebook);
void drawVLine(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int ySource, int yDest);
void drawHLine(GdkWindow * window, GtkStyle * style, GtkStateType state, int y, int xSource, int xDest);
void drawLineEdit(GdkWindow * window, GtkStyle * style, GtkStateType state, int hasFocus, int x, int y, int w, int h);
void drawComboBox(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawFrame(GdkWindow * window, GtkStyle * style, GtkStateType state, GtkShadowType shadow_type, int x, int y, int w, int h);
void drawToolbar(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawMenubar(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h);
void drawCheckBox(GdkWindow * window, GtkStyle * style, GtkStateType state, int on, int x, int y, int w, int h);
void drawMenuCheck(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawRadioButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int on, int x, int y, int w, int h);
void drawScrollBar(GdkWindow * window, GtkStyle * style, GtkStateType state, int orientation, GtkAdjustment* adj, int x, int y, int w, int h);
void drawScrollBarSlider(GdkWindow * window, GtkStyle * style, GtkStateType state, int orientation, GtkAdjustment* adj, int x, int y, int w, int h, int offset, int totalExtent);
void drawSplitter(GdkWindow * window, GtkStyle * style, GtkStateType state, int orientation, int x, int y, int w, int h);
void drawMenuBarItem(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawMenuItem(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawMenu(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawTabFrame(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h, GtkPositionType pos);
void drawProgressBar(GdkWindow * window, GtkStyle * style, GtkStateType state, GtkProgressBarOrientation orientation, gfloat percentage, int x, int y, int w, int h);
void drawProgressChunk(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawSlider(GdkWindow * window, GtkStyle * style, GtkStateType state, GtkAdjustment* adj, int x, int y, int w, int h, GtkOrientation orientation, int inverted);
void drawSpinButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int direction, int x, int y, int w, int h);
void drawArrow(GdkWindow * window, GtkStyle * style, GtkStateType state, GtkArrowType direction, int x, int y, int w, int h);
void drawListHeader(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);
void drawListViewItem(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h);

void getTextColor(GdkColor* g, GtkStateType state);

void setFillPixmap(GdkPixbuf* buf);
GdkGC* alternateBackgroundGc(GtkStyle* style);

int isBaghira;
int isKeramik;
int isAlloy;
int isDomino;
int openOfficeFix;
int gtkQtDebug;

#ifdef  __cplusplus
}
#endif

#endif
