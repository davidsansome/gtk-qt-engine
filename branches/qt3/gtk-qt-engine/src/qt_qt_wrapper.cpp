#include <qstyle.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qimage.h>
#include <qstylefactory.h>
#include <qtabbar.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qscrollbar.h>
#include <qmenubar.h>
#include <qcombobox.h>
#include <qprogressbar.h>
#include <qslider.h>
#include <qtoolbutton.h>
#include <qapplication.h>
#include <qdir.h>
#include <qregexp.h>
#include <gdk/gdkx.h>

#include <cstdlib>
#include <fcntl.h>

#ifdef USE_FREEBSD
#include <kvm.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/file.h>
#endif

#ifdef USE_SOLARIS
#include <procfs.h>
#endif

#include <gtk/gtkgc.h>

#include "qt_qt_wrapper.h"
#include "qt_style.h"

#define RC_CACHE_VERSION QString("2")


bool gtkQtEnable = false;
bool mozillaFix = false;
bool qAppOwner = false;

QStringList appDirList;
typedef QMap<QString, QString> IconMap;
IconMap iconMap[4];
extern int errno;

QScrollBar* scrollBar = 0;
QWidget* meepWidget = 0;
QWidget* meepWidgetP = 0;
QSlider* meepSlider = 0;
QTabBar* meepTabBar = 0;
GdkGC* altBackGC = 0;
QWidget* smw = 0;

GtkRcStyle* gtkRcStyle = 0;

QStringList kdeSearchPaths;
QString iconTheme;
QStringList iconThemeDirs;
QColor alternateBackgroundColour;
int showIconsOnButtons;
int toolbarStyle;

const QPixmap* backgroundTile;
GdkPixmap* backgroundTileGdk;
QPixmap* menuBackgroundPixmap;
GdkPixmap* menuBackgroundPixmapGdk;

QPixmap* fillPixmap;

// How much room between the ends of the scrollbar and the slider groove
// Only set on the Domino style
int scrollBarSpacingLeft = 0;
int scrollBarSpacingRight = 0;

int isBaghira;
int isKeramik;
int isAlloy;
int isDomino;
int isPolyester;
int eclipseFix;
int openOfficeFix;
int gtkQtDebug;

Atom kipcCommAtom;
Atom desktopWindowAtom;

void setFillPixmap(GdkPixbuf* buf)
{
	if (!gtkQtEnable)
		return;
	
	// This code isn't very robust.  It doesn't handle depths other than 24 bits.
	// It sure is fast though!
	int depth = gdk_pixbuf_get_n_channels(buf) * gdk_pixbuf_get_bits_per_sample(buf);
	int width = gdk_pixbuf_get_width(buf);
	int height = gdk_pixbuf_get_height(buf);
	int excess = gdk_pixbuf_get_rowstride(buf) - (width*3);
	
	if (depth != 24)
		return;
	
	QImage fillImage(width, height, 32);
	
	uchar* source = gdk_pixbuf_get_pixels(buf);
	uchar* dest = fillImage.bits();
	
	for (int y=0 ; y<height ; y++)
	{
		for (int x=0 ; x<width ; x++)
		{
			// TODO: Make good on other endiannesses
			dest[0] = source[2];
			dest[1] = source[1];
			dest[2] = source[0];
			dest[3] = '\0';
		
			dest += 4;
			source += 3;
		}
		source += excess;
	}
	
	if (fillPixmap)
		delete fillPixmap;
	fillPixmap = 0;
	fillPixmap = new QPixmap();
	fillPixmap->convertFromImage(fillImage);
	return;
}

		
/* Now to get rid of a ton of un-needed new's across the board.  `new' and `delete' are 
 * non-trivial operations.  You normally just don't notice it; until you're painting a window
 * with 50 widgets, with each paint operation requiring 3-4 news and 3-4 delete's.  The cost
 * of indirection is `not insubstantial'. */

 
static int dummy_x_errhandler( Display *dpy, XErrorEvent *err )
{
	return 0;
}
static int dummy_xio_errhandler( Display * )
{
	return 0;
}

void createQApp()
{
	int argc = 1;
	char** argv;
	// Supply it with fake data to keep KApplication happy
	argv = (char**) malloc(sizeof(char*));
	argv[0] = (char*) malloc(sizeof(char) * 19);
	strncpy(argv[0], "gtk-qt-application", 19);
	
	QString cmdLine;
	
#ifdef USE_FREEBSD
/* 
   procfs(5) is deprecated on FreeBSD.
   We use the kvm(3) library to get argv[0] of the current pid.
*/
	int cnt = 0;
	int ret = 0;
	kvm_t *kd;
	struct kinfo_proc *pbase;
	char ** arg;
	const char *msg = "";

	kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
	if (kd == NULL ) 
	{
		msg = "kvm_open failed\n";
		ret = -1;
	} 
	else
	{
		pbase = kvm_getprocs(kd, KERN_PROC_PID, getpid(), &cnt);
		if (( pbase == NULL ) || ( cnt != 1 )) 
		{ 
			msg = "kvm_getprocs failed\n";
			ret = -1;
		}
		else
		{
			arg = kvm_getargv(kd, pbase, 1024);
			if (arg == NULL) 
			{
				msg = "kvm_getargv failed\n";
				ret = -1;
			}       
			else
			{
				cmdLine += arg[0];
			}
		}
		kvm_close(kd);
	}
	if (ret == -1) 
	{
		printf("Gtk-Qt theme engine warning:\n");
		printf(msg);
		printf("  This may cause problems for the GNOME window manager\n");
	}       
#endif // USE_FREEBSD
	
#ifdef USE_SOLARIS
	int pid=getpid();
	char filen[256];
	psinfo_t pfi;
	uintptr_t addr;
	uintptr_t addr2;
	int  i,count,readl, ret=0;
	const char *msg;
	
	sprintf(filen, "/proc/%d/psinfo",pid);
	int fd=open(filen, O_RDONLY);
	if (fd == -1)
	{
		msg = "Open of psinfo failed\n";
		ret = -1;
	}
	else
	{
		readl=read(fd, (void *)&pfi, sizeof(psinfo_t));
		if (readl < 0)
		{
			msg = "Read on as failed\n";
			close(fd);
			ret = -1;
		}
		else
		{
			addr=pfi.pr_argv;
			count=pfi.pr_argc;
		}
		close(fd);
	}
	/* if read of psinfo was success */
	if (!ret)
	{
		sprintf(filen, "/proc/%d/as",pid);
		fd=open(filen, O_RDONLY);
		if (fd == -1)
		{
			msg = "Open of as failed\n";
			ret = -1;
		}
		else
		{
			for (i=0;i<count;i++)
			{
				lseek(fd, addr, SEEK_SET);
				if (pfi.pr_dmodel == PR_MODEL_ILP32)
				{
					addr=addr+4;
					readl=read(fd, (void *)&addr2, 4);
				}
				else
				{
					addr=addr+8;
					readl=read(fd, (void *)&addr2,8);
				}
				if (readl < 0)
				{
					msg = "Read on as failed\n";
					close(fd);
					ret = -1;
					break;
				}
				if (addr2 != 0)
				{
					cmdLine += (char *)addr2;
					cmdLine += " ";
				}
				else
					break;
			}
			close(fd);
		}
	}
	
	if (ret == -1)
	{
		printf("Gtk-Qt theme engine warning:\n");
		printf(msg);
		printf("  This may cause problems for the GNOME window manager\n");
	}
#else // USE_SOLARIS
#ifndef USE_FREEBSD
	QCString cmdlinePath;
	cmdlinePath.sprintf("/proc/%d/cmdline", getpid());
	int fd = open(cmdlinePath, O_RDONLY);
	if (fd == -1)
	{
		printf("Gtk-Qt theme engine warning:\n");
		printf("  Could not open %s\n", (const char*)cmdlinePath);
		printf("  This may cause problems for the GNOME window manager\n");
	}
	else
	{
		while (1)
		{
			char data[80];
			int len = read(fd, data, 80);
			if (len == 0)
				break;
			cmdLine += data;
		}
		close(fd);
	}
#endif // USE_FREEBSD
#endif // USE_SOLARIS

	mozillaFix = (cmdLine.contains("mozilla") || cmdLine.contains("firefox"));
	
	openOfficeFix = (cmdLine.endsWith("soffice.bin"))
	              | (cmdLine.endsWith("swriter.bin"))
	              | (cmdLine.endsWith("scalc.bin"))
	              | (cmdLine.endsWith("sdraw.bin"))
	              | (cmdLine.endsWith("spadmin.bin"))
	              | (cmdLine.endsWith("simpress.bin"));

	eclipseFix = cmdLine.contains("eclipse");
	
	gtkQtDebug = (getenv("GTK_QT_ENGINE_DEBUG") != NULL) ? 1 : 0;
	
	if (gtkQtDebug)
		printf("createQApp()\n");
	
	char* sessionEnv = getenv("SESSION_MANAGER");
	if (QString(sessionEnv).endsWith(QString::number(getpid())) || cmdLine.contains("gnome-wm") || cmdLine.contains("metacity") || cmdLine.contains("xfwm4") || (getenv("GTK_QT_ENGINE_DISABLE") != NULL) ||
	((qApp) && (qApp->type() == QApplication::Tty)))
	{
		printf("Not initializing the Gtk-Qt theme engine\n");
	}
	else
	{
		int (*original_x_errhandler)( Display *dpy, XErrorEvent * );
		int (*original_xio_errhandler)( Display *dpy );
		original_x_errhandler = XSetErrorHandler( dummy_x_errhandler );
		original_xio_errhandler = XSetIOErrorHandler( dummy_xio_errhandler );
		
#ifndef USE_SOLARIS
		unsetenv("SESSION_MANAGER");
#else
		putenv("SESSION_MANAGER=");
#endif
		
		initKdeSettings();
		
		if (!qApp)
		{
			new QApplication(gdk_x11_get_default_xdisplay());
			qAppOwner = true;
		}

#ifndef USE_SOLARIS
		setenv("SESSION_MANAGER", sessionEnv, 1);
#else
		char *tempEnv=(char *)malloc(strlen(sessionEnv)+strlen("SESSION_MANAGER")+2);
		sprintf(tempEnv, "SESSION_MANAGER=%s", sessionEnv);
		putenv(tempEnv);
#endif
		
		XSetErrorHandler( original_x_errhandler );
		XSetIOErrorHandler( original_xio_errhandler );
		
		gtkQtEnable = true;
	}
	
	free(argv[0]);
	free(argv);
	
	if (!gtkQtEnable)
		return;
	
	isBaghira   = (QString(qApp->style().name()).lower() == "baghira");
	isKeramik   = (QString(qApp->style().name()).lower() == "keramik");
	isAlloy     = (QString(qApp->style().name()).lower() == "alloy");
	isDomino    = (QString(qApp->style().name()).lower() == "domino");
	isPolyester = (QString(qApp->style().name()).lower() == "polyester");
	
	if (isDomino)
	{
		QScrollBar sbar(NULL);
		sbar.setOrientation(Qt::Horizontal);
		sbar.setValue(1);
		sbar.resize(200,25);
		
		QRect rect = qApp->style().querySubControlMetrics(QStyle::CC_ScrollBar, &sbar, QStyle::SC_ScrollBarGroove);
		scrollBarSpacingLeft = rect.x();
		scrollBarSpacingRight = 200 - rect.x() - rect.width();
	}
	
	// Set Gtk fonts and icons
	/*setGnomeFonts();
	setGnomeIcons();*/
	
	if (!cmdLine.contains("xfce-mcs-manager"))
	{
		// Get KDE related atoms from the X server
		kipcCommAtom = XInternAtom ( gdk_x11_get_default_xdisplay() , "KIPC_COMM_ATOM" , false );
		desktopWindowAtom = XInternAtom ( gdk_x11_get_default_xdisplay() , "KDE_DESKTOP_WINDOW" , false );
		
		// Create a new window, and set the KDE_DESKTOP_WINDOW property on it
		// This window will then receive events from KDE when the style changes
		smw = new QWidget(0,0);
		long data = 1;
		XChangeProperty(gdk_x11_get_default_xdisplay(), smw->winId(),
			desktopWindowAtom, desktopWindowAtom,
			32, PropModeReplace, (unsigned char *)&data, 1);
		
		// This filter will intercept those events
		gdk_window_add_filter( NULL, gdkEventFilter, 0);
	}
	
	meepWidgetP = new QWidget(0);
	meepWidget = new QWidget(meepWidgetP);
	meepSlider = new QSlider(meepWidget);
	meepWidget->polish();
	
	meepTabBar = new QTabBar(meepWidget);
	
	menuBackgroundPixmap = NULL;
	backgroundTile = meepWidget->paletteBackgroundPixmap();
	if (backgroundTile != NULL)
		backgroundTileGdk = gdk_pixmap_foreign_new(backgroundTile->handle());
}

void destroyQApp()
{
	if (!gtkQtEnable)
		return;
	delete meepWidget;
	delete meepWidgetP;
	delete menuBackgroundPixmap;
	delete smw;
	if (qAppOwner)
	{
		delete qApp;
		qApp = 0;
	}
	if (altBackGC != 0)
		gtk_gc_release(altBackGC);
}

GdkFilterReturn gdkEventFilter(GdkXEvent *xevent, GdkEvent *gevent, gpointer data)
{
	XEvent* event = (XEvent*) xevent;

	// Is the event a KIPC message?
	if ((event->type == ClientMessage) && (event->xclient.message_type == kipcCommAtom))
	{
		// This data variable contains the type of KIPC message
		// As defined in kdelibs/kdecore/kipc.h, 2 = StyleChanged
		if (event->xclient.data.l[0] != 2)
			return GDK_FILTER_REMOVE;
		
		if (gtkQtDebug)
			printf("StyleChanged IPC message\n");
		
		// Find out the new widget style
		QString styleName = kdeConfigValue("General", "widgetStyle", "");
		QStyle* style = QStyleFactory::create(styleName);
		if (!style)
			return GDK_FILTER_REMOVE;
		
		// Tell the QApplication about this new style
		qApp->setStyle(style);
		
		// Now we need to update GTK's properties
		setRcProperties(gtkRcStyle, 1); // Rewrite our cache file
		gtk_rc_reparse_all(); // Tell GTK to parse the cache file
		
		return GDK_FILTER_REMOVE;
	}
	return GDK_FILTER_CONTINUE;
}

QString kdeConfigValue(const QString& section, const QString& name, const QString& def)
{
	for ( QStringList::Iterator it = kdeSearchPaths.begin(); it != kdeSearchPaths.end(); ++it )
	{
		if (!QFile::exists((*it) + "/share/config/kdeglobals"))
			continue;
		
		QFile file((*it) + "/share/config/kdeglobals");
		if (!file.open( IO_ReadOnly ))
			continue;
		
		QTextStream stream( &file );
		QString line;
		QString sec;
		int i = 1;
		while ( !stream.atEnd() )
		{
			line = stream.readLine();
			if (line.startsWith("["))
			{
				sec = line.mid(1, line.length() - 2);
				continue;
			}
			if (sec != section)
				continue;
			QRegExp parser("([\\S]*)\\s*=\\s*([\\S]*)");
			if (parser.search(line) == -1)
				continue;
			if (parser.cap(1) == name)
				return parser.cap(2);
		}
		file.close();
	}
	return def;
}

QString kdeFindDir(const QString& suffix, const QString& file1, const QString& file2)
{
	for ( QStringList::Iterator it = kdeSearchPaths.begin(); it != kdeSearchPaths.end(); ++it )
	{
		if ((QFile::exists((*it) + suffix + file1)) || (QFile::exists((*it) + suffix + file2)))
			return (*it) + suffix;
	}
	return QString::null;
}

QString runCommand(const QString& command)
{
	FILE* p = popen(command.latin1(), "r");
	if ((p == NULL) || (p < 0))
		return QString::null;
	
	QString ret;
	while (!feof(p))
	{
		char buffer[256];
		int n = fread(buffer, 1, 255, p);
		buffer[n] = '\0';
		ret += buffer;
	}
	pclose(p);
	
	return ret.stripWhiteSpace();
}

void initKdeSettings()
{
	kdeSearchPaths.clear();
	
	QString kdeHome = getenv("KDEHOME");
	QString kdeDirs = getenv("KDEDIRS");
	QString kdeDir = getenv("KDEDIR");
	
	if (!kdeHome.isEmpty())
		kdeSearchPaths.append(kdeHome);
	kdeSearchPaths.append(runCommand("kde-config --localprefix"));
	
	if (!kdeDirs.isEmpty())
		kdeSearchPaths += QStringList::split(':', kdeDirs);
	if (!kdeDir.isEmpty())
		kdeSearchPaths.append(kdeDir);
	kdeSearchPaths.append(runCommand("kde-config --prefix"));
	
	iconTheme = kdeConfigValue("Icons", "Theme", "crystalsvg");
	QStringList back = QStringList::split(',', kdeConfigValue("General", "alternateBackground", "238,246,255"));
	alternateBackgroundColour.setRgb(back[0].toInt(), back[1].toInt(), back[2].toInt());

	showIconsOnButtons = (kdeConfigValue("KDE", "ShowIconsOnPushButtons", "true").lower() == "true");
	
	
	QString tmp = kdeConfigValue("Toolbar style", "IconText", "true").lower();
	if (tmp == "icononly")
		toolbarStyle = 0;
	else if (tmp == "icontextright")
		toolbarStyle = 3;
	else if (tmp == "textonly")
		toolbarStyle = 1;
	else if (tmp == "icontextbottom")
		toolbarStyle = 2;
	else // Should never happen, but just in case we fallback to KDE's default "icononly"
		toolbarStyle = 0;
}

QStyle::SFlags stateToSFlags(GtkStateType state)
{
	switch (state)
	{
		case GTK_STATE_ACTIVE:
			return QStyle::Style_Enabled | QStyle::Style_Down;
		case GTK_STATE_PRELIGHT:
			return QStyle::Style_Enabled | QStyle::Style_MouseOver | QStyle::Style_Raised;
		case GTK_STATE_SELECTED:
			return QStyle::Style_Enabled | QStyle::Style_HasFocus | QStyle::Style_Raised;
		case GTK_STATE_INSENSITIVE:
			return QStyle::Style_Default | QStyle::Style_Raised;
		default:
			return QStyle::Style_Enabled | QStyle::Style_Raised;
	}
}

QColor gdkColorToQColor(GdkColor* c)
{
	return QColor(c->red / 256, c->green / 256, c->blue / 256);
}


// The drawing functions follow the same pattern:
//  * Set the appropriate flags
//  * Ask QT to paint the widget to a pixmap
//  * Create a GdkPixmap that points to our QPixmap
//  * Paint the pixmap on the window


void drawButton(GdkWindow* window, GtkStyle* style, GtkStateType state, int defaultButton, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QPixmap     pixmap(w, h);
	QPainter    painter(&pixmap);
	QPushButton button(meepWidget);	
	button.setBackgroundOrigin(QWidget::ParentOrigin);
	button.setGeometry(x, y, w, h);
	if (style->rc_style->bg[GTK_STATE_NORMAL].pixel != 0)
		button.setPaletteBackgroundColor(gdkColorToQColor(&style->rc_style->bg[GTK_STATE_NORMAL]));
	QPoint p = button.backgroundOffset();
	QPoint pos = button.pos();

	QStyle::SFlags sflags = stateToSFlags(state);
	
	if (defaultButton)
		sflags |= QStyle::Style_ButtonDefault;
	button.setDefault(defaultButton);

	painter.fillRect(0, 0, w, h, qApp->palette().active().background());

	qApp->style().drawControl(QStyle::CE_PushButton, &painter, &button, 
	                          QRect(0,0,w,h), button.palette().active(), sflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

// Thanks Peter Hartshorn <peter@dimtech.com.au>
void drawToolbar(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	int w1, h1;
	QStyle::SFlags sflags = stateToSFlags(state) | QStyle::Style_Raised;


	// Keramik hack...
	// Keramik only draws the toolbar border, and not the gradient
	// so we also draw a separator, but make sure the line is off the
	// widget
	
	if (w > h)
	{
		sflags |= QStyle::Style_Horizontal;
		w1 = w * 3;
		h1 = h;
	}
	else
	{
		w1 = h;
		h1 = h * 3;
	}

	if ((w1 < 1) || (h1 < 1) ||
	    (w < 1) || (h < 1))
		return;

	QPixmap     pixmap(w1, h1);
	QPixmap     p(w, h);
	QPainter    painter(&pixmap);

        if ((backgroundTile) && (!backgroundTile->isNull()))
                painter.fillRect(0, 0, w1, h1, QBrush(QColor(255,255,255), *backgroundTile));
        else
                painter.fillRect(0, 0, w1, h1, qApp->palette().active().brush(QColorGroup::Background));
	
	qApp->style().drawPrimitive(QStyle::PE_PanelDockWindow, &painter,
			QRect(0,0,w1,h1), qApp->palette().active(),sflags);

	if (isKeramik)
	{
		qApp->style().drawPrimitive(QStyle::PE_DockWindowSeparator, &painter,
				QRect(0,0,w1,h1), qApp->palette().active(),sflags);
	}

	bitBlt(&p, 0, 0, &pixmap, 0, 0, w, h);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawMenubar(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	int w1, h1;
	QStyle::SFlags sflags = stateToSFlags(state);


	// Keramik hack...
	// Keramik only draws the toolbar border, and not the gradient
	// so we also draw a separator, but make sure the line is off the
	// widget
	
	if (w > h)
	{
		sflags |= QStyle::Style_Horizontal;
		w1 = w * 3;
		h1 = h;
	}
	else
	{
		w1 = h;
		h1 = h * 3;
	}

	if ((w1 < 1) || (h1 < 1) ||
	    (w < 1) || (h < 1))
		return;

	QPixmap     pixmap(w1, h1);
	QPixmap     p(w, h);
	QPainter    painter(&pixmap);

        if ((backgroundTile) && (!backgroundTile->isNull()))
                painter.fillRect(0, 0, w1, h1, QBrush(QColor(255,255,255), *backgroundTile));
        else
                painter.fillRect(0, 0, w1, h1, qApp->palette().active().brush(QColorGroup::Background));

	qApp->style().drawPrimitive(QStyle::PE_PanelMenuBar, &painter,
			QRect(0,0,w1,h1), qApp->palette().active(),sflags);

	bitBlt(&p, 0, 0, &pixmap, 0, 0, w, h);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawTab(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);
	
	// GTK doesn't tell us if our tab is on the left, right, or middle of the tabbar
	// So, let's always assume it's in the middle - it looks pretty
	QTab* tab = new QTab;
	meepTabBar->insertTab(tab,1);

	QStyle::SFlags sflags = stateToSFlags(state);

	if (state != GTK_STATE_ACTIVE)
		sflags = QStyle::Style_Selected;

	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawControl(QStyle::CE_TabBarTab, &painter, meepTabBar, QRect(0,0,w,h), qApp->palette().active(), sflags, QStyleOption(tab));
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
	
	meepTabBar->removeTab(tab);
}

void drawVLine(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int ySource, int yDest)
{
	if (!gtkQtEnable)
		return;
	
	int width = style->xthickness;
	int height = abs(ySource-yDest);
	
	if (width < 2) width = 2;

	if ((width < 1) || (height < 1))
		return;

	QPixmap pixmap(width, height);
	QPainter painter(&pixmap);

	painter.fillRect(2, 0, width - 2, height, qApp->palette().active().brush(QColorGroup::Background));
	painter.setPen( qApp->palette().active().mid() );
	painter.drawLine( 0, 0, 0, height );
	painter.setPen( qApp->palette().active().light() );
	painter.drawLine( 1, 0, 1, height );

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, ySource, width, height);
	g_object_unref(pix);
}

void drawHLine(GdkWindow* window, GtkStyle* style, GtkStateType state, int y, int xSource, int xDest)
{
	if (!gtkQtEnable)
		return;
	
	int width = abs(xSource-xDest);
	int height = style->ythickness;

	if ((width < 1) || (height < 1))
		return;

	QPixmap pixmap(width, height);
	QPainter painter(&pixmap);

	painter.fillRect(0, 2, width, height-2, qApp->palette().active().brush(QColorGroup::Background));
	painter.setPen(qApp->palette().active().mid() );
	painter.drawLine(0, 0, width, 0);
	painter.setPen(qApp->palette().active().light());
	painter.drawLine(0, 1, width, 1);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, xSource, y, width, height);
	g_object_unref(pix);
}

void drawLineEdit(GdkWindow* window, GtkStyle* style, GtkStateType state, int hasFocus, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w, h);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state);
	if (hasFocus)
		sflags |= QStyle::Style_HasFocus;
	
	painter.fillRect(0, 0, w, h, qApp->palette().active().base());
	qApp->style().drawPrimitive(QStyle::PE_PanelLineEdit, &painter, QRect(0, 0, w, h), qApp->palette().active(), sflags, QStyleOption(1,1));

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawFrame(GdkWindow* window, GtkStyle* style, GtkStateType state, GtkShadowType shadow_type, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1)) // Caused crash in gaim file transfers window
		return;

	QPixmap pixmap(w, h);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state);
	if ((shadow_type == GTK_SHADOW_IN) || (shadow_type == GTK_SHADOW_ETCHED_IN))
		sflags |= QStyle::Style_Sunken;

	if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));

	qApp->style().drawPrimitive(QStyle::PE_Panel, &painter, QRect(0, 0, w, h), qApp->palette().active(), sflags, QStyleOption(2,2) );
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawComboBox(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);
	QComboBox cb(false, 0);
	cb.resize(w,h);
	
	QStyle::SFlags sflags = stateToSFlags(state);
	QStyle::SCFlags scflags = QStyle::SC_ComboBoxArrow | QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxListBoxPopup;
	QStyle::SCFlags activeflags = QStyle::SC_None;

	if (state == GTK_STATE_PRELIGHT)
		activeflags = QStyle::Style_MouseOver;

	painter.fillRect(0,0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawComplexControl(QStyle::CC_ComboBox, &painter, &cb, QRect(0, 0, w, h), qApp->palette().active(), sflags, scflags, activeflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawCheckBox(GdkWindow* window, GtkStyle* style, GtkStateType state, int checked, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	int realH = qApp->style().pixelMetric(QStyle::PM_IndicatorHeight);
	int realW = qApp->style().pixelMetric(QStyle::PM_IndicatorWidth);

	if ((realW < 1) || (realH < 1))
		return;

	QPixmap pixmap(realW, realH);
	QPainter painter(&pixmap);
	QCheckBox checkbox(0);

	QStyle::SFlags sflags = stateToSFlags(state);
	sflags |= (checked ? QStyle::Style_On : QStyle::Style_Off);

	painter.fillRect(0, 0, realW, realH, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawControl(QStyle::CE_CheckBox, &painter, &checkbox, QRect(0, 0, realW, realH), qApp->palette().active(), sflags);

	// Qt checkboxes are usually bigger than GTK wants.
	// We cheat, and draw them over the expected area.
	int xOffset = (realW - w) / 2;
	int yOffset = (realH - h) / 2;
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x - xOffset, y - yOffset, realW, realH);
	g_object_unref(pix);
}

void drawMenuCheck(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	QCheckBox checkbox(0);
	
	/* A previous version of the function followed the sizehints exclusively 
	   Now follow w and h provided by GTK, but if the checkmark is too big we might have to scale it */
	/*
	int w1 = checkbox.sizeHint().width();
	int h1 = checkbox.sizeHint().height(); */

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state);
	sflags |= QStyle::Style_On;

	if (fillPixmap && (!fillPixmap->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *fillPixmap));
	else if ((backgroundTile) && (!backgroundTile->isNull()))
                painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
        else
	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawPrimitive(QStyle::PE_CheckMark, &painter, QRect(0, 0, w, h), qApp->palette().active(), sflags);
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawRadioButton(GdkWindow* window, GtkStyle* style, GtkStateType state, int checked, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	int realH = qApp->style().pixelMetric(QStyle::PM_IndicatorHeight);
	int realW = qApp->style().pixelMetric(QStyle::PM_IndicatorWidth);

	if ((realW < 1) || (realH < 1))
		return;

	QPixmap pixmap(realH, realW);
	QPainter painter(&pixmap);
	QRadioButton radio(0);

	QStyle::SFlags sflags = stateToSFlags(state);
	sflags |= checked ? QStyle::Style_On : QStyle::Style_Off;

	if (fillPixmap && (!fillPixmap->isNull()))
		painter.fillRect(0, 0, realW, realH, QBrush(QColor(255,255,255), *fillPixmap));
	else if ((backgroundTile) && (!backgroundTile->isNull()))
                painter.fillRect(0, 0, realW, realH, QBrush(QColor(255,255,255), *backgroundTile));
        else
                painter.fillRect(0, 0, realW, realH, qApp->palette().active().brush(QColorGroup::Background));

	qApp->style().drawControl(QStyle::CE_RadioButton, &painter, &radio, QRect(0,0,realH,realW), qApp->palette().active(), sflags);

	// Qt checkboxes are usually bigger than GTK wants.
	// We cheat, and draw them over the expected area.
	int xOffset = (realW - w) / 2;
	int yOffset = (realH - h) / 2;
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x - xOffset, y - yOffset, realW, realH);
	g_object_unref(pix);
}


void drawScrollBarSlider(GdkWindow* window, GtkStyle* style, GtkStateType state, int orientation, GtkAdjustment* adj, int x, int y, int w, int h, int offset, int totalExtent)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	int wCorrected = w;
	int hCorrected = h;
	if (isDomino)
	{
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
		    wCorrected = w + 14;
		else
		    hCorrected = h + 14;
	}
	QPixmap pixmap(wCorrected, hCorrected);
	QPainter painter(&pixmap);
	
	QStyle::SFlags sflags = stateToSFlags(state);
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		sflags |= QStyle::Style_Horizontal;
	
	qApp->style().drawPrimitive(QStyle::PE_ScrollBarSlider, &painter, QRect(0,0,wCorrected,hCorrected), qApp->palette().active(), sflags);
	
	// The domino style doesn't draw the entire slider in PE_ScrollBarSlider
	// We have to draw PE_ScrollBarAddPage and PE_ScrollBarSubPage and piece the bits together
	if (isDomino && !mozillaFix && !eclipseFix)
	{
		QPixmap leftPix, rightPix;
		QRect leftRect, rightRect;
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			leftRect = QRect(0, 0, offset-scrollBarSpacingLeft, h);
			rightRect = QRect(6, 0, totalExtent-offset-w-scrollBarSpacingRight+2, h);
			leftPix.resize(6 + leftRect.width(), h);
			rightPix.resize(6 + rightRect.width(), h);
		}
		else
		{
			leftRect = QRect(0, 0, w, offset-scrollBarSpacingLeft);
			rightRect = QRect(0, 6, w, totalExtent-offset-h-scrollBarSpacingRight+2);
			leftPix.resize(w, 6 + leftRect.height());
			rightPix.resize(w, 6 + rightRect.height());
		}
		printf("Offset %d\n", offset);
		printf("%d %d %d\n", offset, scrollBarSpacingLeft, scrollBarSpacingRight);
		printf("%dx%d, %dx%d\n", leftRect.width(), leftRect.height(), rightRect.width(), rightRect.height());
		
		printf("1\n");
		QPainter dominoPainter(&leftPix);
		//qApp->style().drawPrimitive(QStyle::PE_ScrollBarSubPage, &dominoPainter, leftRect, qApp->palette().active(), sflags);
		printf("2\n");
		dominoPainter.fillRect(0, 0, leftPix.width(), leftPix.height(), Qt::red);
		printf("3\n");
		dominoPainter.end();
		printf("4\n");
		dominoPainter.begin(&rightPix);
		//qApp->style().drawPrimitive(QStyle::PE_ScrollBarAddPage, &dominoPainter, rightRect, qApp->palette().active(), sflags);
		printf("6\n");
		dominoPainter.fillRect(0, 0, rightPix.width(), rightPix.height(), Qt::blue);
		printf("7\n");
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			bitBlt(&pixmap, 1, 0, &leftPix, leftRect.width(), 0, 6, h, Qt::CopyROP, true);
			bitBlt(&pixmap, w-7, 0, &rightPix, 0, 0, 7, h, Qt::CopyROP, true);
		}
		else
		{
			bitBlt(&pixmap, 0, 1, &leftPix, 0, leftRect.height(), w, 6, Qt::CopyROP, true);
			bitBlt(&pixmap, 0, h-7, &rightPix, 0, 0, w, 7, Qt::CopyROP, true);
		}
	}
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	
	if (isDomino)
	{
		int endsSkip = mozillaFix ? 7 : 1;
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
			gdk_draw_drawable(window, style->bg_gc[state], pix, endsSkip, 0, x, y, w-1, h);
		else
			gdk_draw_drawable(window, style->bg_gc[state], pix, 0, endsSkip, x, y, w, h-1);
	}
	else
		gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawScrollBar(GdkWindow* window, GtkStyle* style, GtkStateType state, int orientation, GtkAdjustment* adj, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	if (scrollBar != 0)
		delete scrollBar;
	scrollBar = new QScrollBar(NULL);
	
	scrollBar->resize(w,h);

	// Patch from Chad Kitching <chadk@cmanitoba.com>
	// Patch from Peter Hartshorn <peter@dimtech.com.au>

	// another check for mozilla is step_increment and page_increment
	// are set to zero for mozilla, and have values set by all other
	// gtk applications I've tested this with.
	
	// Why oh why couldn't mozilla use native widgets instead of
	// handling everything in cross platform.
	
	scrollBar->setOrientation(orientation ? Qt::Vertical : Qt::Horizontal);
	
	
	QStyle::SFlags sflags = stateToSFlags(state);
	if (sflags |= QStyle::Style_Down) sflags = QStyle::Style_Enabled;
	if (orientation == GTK_ORIENTATION_HORIZONTAL) sflags |= QStyle::Style_Horizontal;
	
	QPixmap pixmap(w,h);
	
	scrollBar->setMinValue(0);
	scrollBar->setMaxValue(65535);
	scrollBar->setValue(32767);
	scrollBar->setPageStep(1);

	int offset = 0;
	int thumbSize = 0;


	if (orientation == GTK_ORIENTATION_VERTICAL) {
		QRect r;
		r = qApp->style().querySubControlMetrics(QStyle::CC_ScrollBar,
				scrollBar, QStyle::SC_ScrollBarSlider);
		offset = r.y();
		thumbSize = r.height();
                if (thumbSize < 0)
                  thumbSize = -thumbSize;
		
		if (!r.isValid()) // Fix a crash bug in Eclipse where it was trying to draw tiny scrollbars.
			return;

		QPixmap tmpPixmap(w, h + thumbSize);
		QPainter painter2(&tmpPixmap);
		scrollBar->resize(w, h + thumbSize);

		painter2.fillRect(0, 0, w, h + thumbSize,
				qApp->palette().active().brush(QColorGroup::Background));
		qApp->style().drawComplexControl(QStyle::CC_ScrollBar,
				&painter2, scrollBar, QRect(0, 0, w, h+thumbSize),
				qApp->palette().active(), sflags);

		bitBlt(&pixmap, 0, 0, &tmpPixmap, 0, 0, w, offset, Qt::CopyROP);
		bitBlt(&pixmap, 0, offset, &tmpPixmap, 0, offset + thumbSize,
				w, h - offset, Qt::CopyROP);
	} else {
		QRect r;
		r = qApp->style().querySubControlMetrics(QStyle::CC_ScrollBar,
				scrollBar, QStyle::SC_ScrollBarSlider);
		offset = r.x();
		thumbSize = r.width();
		if (thumbSize < 0)
		  thumbSize = -thumbSize;

		if (!r.isValid()) // Fix a crash bug in Eclipse when it was trying to draw tiny scrollbars.
			return;
		
		QPixmap tmpPixmap(w + thumbSize, h);
		QPainter painter2(&tmpPixmap);
		scrollBar->resize(w + thumbSize, h);

		painter2.fillRect(0, 0, w + thumbSize, h,
				qApp->palette().active().brush(QColorGroup::Background));
		qApp->style().drawComplexControl(QStyle::CC_ScrollBar,
				&painter2, scrollBar, QRect(0, 0, w+thumbSize, h),
				qApp->palette().active(), sflags);

		bitBlt(&pixmap, 0, 0, &tmpPixmap, 0, 0, offset, h, Qt::CopyROP);
		bitBlt(&pixmap, offset, 0, &tmpPixmap, offset + thumbSize, 0,
				w - offset, h, Qt::CopyROP);
	}
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawToolButton(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QToolButton button(NULL);
	button.resize(w, h);
	
	/* 
	int realW = button.sizeHint().width();
	int realH = button.sizeHint().height();	 */
	
	QStyle::SFlags sflags = stateToSFlags(state);
	QStyle::SCFlags activeflags = QStyle::SC_None;
	if (state == GTK_STATE_ACTIVE)
	{
		sflags |= QStyle::Style_AutoRaise;
		activeflags = QStyle::SC_ToolButton;
	}
	else
		sflags |= QStyle::Style_AutoRaise | QStyle::Style_Raised;
	
	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);
	
	if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawComplexControl(QStyle::CC_ToolButton, &painter, &button, QRect(0, 0, w, h), qApp->palette().active(), sflags, QStyle::SC_ToolButton, activeflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawMenuBarItem(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w, h);
	QPainter painter(&pixmap);
	QMenuItem mi;
	QMenuBar mb(0);
	
	QStyle::SFlags sflags = QStyle::Style_Down | QStyle::Style_Enabled | QStyle::Style_Active | QStyle::Style_HasFocus;

	qApp->style().drawControl(QStyle::CE_MenuBarItem, &painter, &mb, QRect(0, 0, w, h), qApp->palette().active(), sflags, QStyleOption(&mi));
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawMenuItem(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);
	QPopupMenu pm;
	QMenuData md;
	QMenuItem* mi = md.findItem(md.insertItem(""));
	
	QStyleOption opt(mi, 16, 16);
	QStyle::SFlags sflags = QStyle::Style_Active | QStyle::Style_Enabled;

	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawControl(QStyle::CE_PopupMenuItem, &painter, &pm, QRect(0,0,w,h), qApp->palette().active(), sflags, opt);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawSplitter(GdkWindow* window, GtkStyle* style, GtkStateType state, int orientation, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state);
	// No idea why this works...
	if (orientation != GTK_ORIENTATION_HORIZONTAL) sflags |= QStyle::Style_Horizontal;

	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawPrimitive(QStyle::PE_Splitter, &painter, QRect(0,0,w,h), qApp->palette().active(), sflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawTabFrame(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h, GtkPositionType pos)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QStyle::SFlags sflags = stateToSFlags(state);
		
	QPixmap pixmap(w, h);
	QPainter painter(&pixmap);
	QStyleOption opt(2, 2); // line width

	if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawPrimitive(QStyle::PE_PanelTabWidget, &painter, QRect(0,0,w,h), qApp->palette().active(), sflags, opt);
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
	
	// Drawing tab base
        int th = qApp->style().pixelMetric(QStyle::PM_TabBarBaseHeight, meepTabBar);
	int tw = w;
	
	if ((tw < 1) || (th < 1))
		return;

        QPixmap pixmap1(tw,th);
        QPainter painter1(&pixmap1);
	if ((backgroundTile) && (!backgroundTile->isNull()))
		painter1.fillRect(0, 0, tw, th, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter1.fillRect(0, 0, tw, th, qApp->palette().active().brush(QColorGroup::Background));
        

        qApp->style().drawPrimitive(QStyle::PE_TabBarBase, &painter1, QRect(0, 0, tw, th), qApp->palette().active(), sflags, QStyleOption(1,1));
	if (pos == GTK_POS_BOTTOM)
	{
		QWMatrix m;
		m.scale(1, -1);
		pixmap1 = pixmap1.xForm(m);
		
		GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap1.handle());
		gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y+h+qApp->style().pixelMetric(QStyle::PM_TabBarBaseOverlap, meepTabBar), tw, th);
		g_object_unref(pix);
	}
	else
	{
		GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap1.handle());
		gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y-qApp->style().pixelMetric(QStyle::PM_TabBarBaseOverlap, meepTabBar), tw, th);
		g_object_unref(pix);
	}
}

void drawMenu(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w, h);
	QPainter painter(&pixmap);
	QStyle::SFlags sflags = stateToSFlags(state);

	if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawPrimitive(QStyle::PE_PanelPopup, &painter, QRect(0,0,w,h), qApp->palette().active(), sflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

// Note: In GTK, the drawing of a progress bar is in two parts:
// First the progress "container" is drawn
// Second the actually percent bar is drawn
// Mozilla requires this to be done in two steps as it first
// asks gtk to draw an EMPTY progress bar, then it asks to draw
// the contents, or percent bar, over the empty progress bar.
// So, although this function is not required for any gtk application
// except mozilla based apps, doing it this way is following the gtk
// theme structure more.
//
// See also drawScrollbar/drawScrollbarSlider pair.
//
// Peter Hartshorn (peter@dimtech.com.au)

void drawProgressChunk(GdkWindow * window, GtkStyle * style, GtkStateType state, int x, int y, int w, int h)
{
	// This is only for Mozilla/Firefox
	if (!mozillaFix || !gtkQtEnable)
		return;
	
	if ((w<=1) || (h<=1))
		return; // Trying to draw something that small caused a segfault
	
	// Dirty hack: When using the Alloy style, tweak the position and size of progress bar "filling"
	int w2 = isAlloy ? w+4 : w;
	int h2 = isAlloy ? h+4 : h;

	QProgressBar bar(100, NULL);
	bar.resize(w2, h2);
	bar.setProgress(100);
	bar.setCenterIndicator(false);
	bar.setPercentageVisible(false);
	bar.setFrameStyle(QFrame::NoFrame);
	
	if ((w2 < 1) || (h2 < 1))
		return;

	QPixmap pixmap(w2, h2);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state);
	
	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));

	qApp->style().drawControl(QStyle::CE_ProgressBarContents, &painter, &bar, QRect(0,0,w2,h2), qApp->palette().active(), sflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	if (isAlloy)
		gdk_draw_drawable(window, style->bg_gc[state], pix, 4, 4, x+2, y+2, w-3, h-3);
	else
		gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawProgressBar(GdkWindow * window, GtkStyle * style, GtkStateType state, GtkProgressBarOrientation orientation, gfloat percentage, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w<=1) || (h<=1))
		return; // Trying to draw something that small caused a segdault
	
	QProgressBar bar(100, NULL);
	if ((orientation == GTK_PROGRESS_BOTTOM_TO_TOP) || (orientation == GTK_PROGRESS_TOP_TO_BOTTOM))
		bar.resize(h, w);
	else
		bar.resize(w, h);
	bar.setProgress((int)(percentage*100.0));
	bar.setCenterIndicator(false);
	bar.setPercentageVisible(false);

	QPixmap pixmap = QPixmap::grabWidget(&bar);
	
	QWMatrix matrix;
	switch (orientation)
	{
		case GTK_PROGRESS_RIGHT_TO_LEFT: matrix.rotate(180); break;
		case GTK_PROGRESS_BOTTOM_TO_TOP: matrix.rotate(270);  break;
		case GTK_PROGRESS_TOP_TO_BOTTOM: matrix.rotate(90); break;
		default: break;
	}
	
	if (orientation != GTK_PROGRESS_LEFT_TO_RIGHT)
		pixmap = pixmap.xForm(matrix);
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawSlider(GdkWindow * window, GtkStyle * style, GtkStateType state, GtkAdjustment *adj, int x, int y, int w, int h, GtkOrientation orientation, int inverted)
{
	if (!gtkQtEnable)
		return;

	meepSlider->setBackgroundOrigin(QWidget::ParentOrigin);
	
	meepSlider->setOrientation((orientation == GTK_ORIENTATION_HORIZONTAL) ? Qt::Horizontal : Qt::Vertical);
	meepSlider->setEnabled(state != GTK_STATE_INSENSITIVE);

	meepSlider->setGeometry(x, y, w, h);
	meepSlider->setMinValue(0);
	meepSlider->setMaxValue(100);
	
	if (!inverted) // Normal sliders
		meepSlider->setValue((int)((adj->value-adj->lower)/(adj->upper-adj->lower)*100));
	else // Inverted sliders... where max is at the left/top and min is at the right/bottom
		meepSlider->setValue(100-(int)((adj->value-adj->lower)/(adj->upper-adj->lower)*100));

	QPixmap pixmap = QPixmap::grabWidget(meepSlider);
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawSpinButton(GdkWindow * window, GtkStyle * style, GtkStateType state, int direction, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w, h);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state);
	
	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawPrimitive((direction ? QStyle::PE_SpinWidgetDown : QStyle::PE_SpinWidgetUp), &painter, QRect(0,0,w,h), qApp->palette().active(), sflags);
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawListHeader(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);

	QStyle::SFlags sflags = stateToSFlags(state) | QStyle::Style_Horizontal;

	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawPrimitive(QStyle::PE_HeaderSection, &painter, QRect(0,0,w,h), qApp->palette().active(), sflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}


void drawListViewItem(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
        if (!gtkQtEnable)
                return;

	if ((w < 1) || (h < 1))
		return;

        QPixmap     pixmap(w, h);
        QPainter    painter(&pixmap);

	/* Get the brush corresponding to highlight color */
	QBrush brush = qApp->palette().brush(QPalette::Active, QColorGroup::Highlight);
	painter.setBrush(brush);
	painter.setPen(Qt::NoPen);
	painter.drawRect(0, 0, w, h);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawSquareButton(GdkWindow* window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;

	QPixmap     pixmap(20, 20);
	QPainter    painter(&pixmap);
	QPushButton button(0);
		
	QStyle::SFlags sflags = stateToSFlags(state);
	if (fillPixmap && (!fillPixmap->isNull()))
		painter.fillRect(0, 0, 20, 20, QBrush(QColor(255,255,255), *fillPixmap));
	else if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, 20, 20, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter.fillRect(0, 0, 20, 20, qApp->palette().active().brush(QColorGroup::Background));
	
	qApp->style().drawControl(QStyle::CE_PushButton, &painter, &button,
					QRect(0,0,20,20), qApp->palette().active(), sflags);
	
	QImage image = pixmap.convertToImage().smoothScale(w,h);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void initDrawTabNG(int count)
{
	if (!gtkQtEnable)
		return;
	
	delete meepTabBar;
	meepTabBar = 0;
	meepTabBar = new QTabBar(meepWidget);
	
	for ( int i = 0; i < count; i++ )
		meepTabBar->addTab(new QTab);
	
	return;
}

void drawTabNG(GdkWindow *window, GtkStyle* style, GtkStateType state, int x, int y, int w, int h, GtkNotebook *notebook)
{
	if (!gtkQtEnable)
		return;
	
	GtkPositionType tpos = gtk_notebook_get_tab_pos(notebook);
	
	// Find tab position
	int sdiff = 10000, pos = -1, diff = 1;
	for ( int i = 0; i < g_list_length(notebook->children); i++ )
	{
		GtkWidget *tab_label=gtk_notebook_get_tab_label(notebook,gtk_notebook_get_nth_page(notebook,i));
		if (tab_label) diff = tab_label->allocation.x - x;
		if ((diff > 0) && (diff < sdiff))
		{
			sdiff = diff; pos = i;
		}
	}
	
	QTab *tab = meepTabBar->tabAt(pos);
	
	if (!tab)
	{
		// This happens in Firefox.  Just draw a normal tab instead
		if (state == GTK_STATE_ACTIVE)
			drawTab(window, style, state, x, y - 2, w, h + 2);
		else
			drawTab(window, style, state, x, y, w, h);
		return;
	}
	
	QStyle::SFlags sflags = stateToSFlags(state);
	
	if (state != GTK_STATE_ACTIVE)
	{
		sflags = QStyle::Style_Selected;
		if (tpos == GTK_POS_TOP)
			y += 3;
		h -= 3;
	}
	
	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);
	
	if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
	else
	
	painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));
	qApp->style().drawControl(QStyle::CE_TabBarTab, &painter, (QTabBar *)meepTabBar, QRect(0,0,w,h), qApp->palette().active(), sflags, QStyleOption(tab));
	
	painter.end(); // So the pixmap assignment below won't give an error
	// Account for tab position -- if its in the bottom flip the image
	if (tpos == GTK_POS_BOTTOM)
	{
		QWMatrix m;
		m.scale(1, -1);
		pixmap = pixmap.xForm(m);
	}
	
	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawArrow(GdkWindow* window, GtkStyle* style, GtkStateType state, GtkArrowType direction, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QStyle::SFlags sflags = stateToSFlags(state);
	if (state == GTK_STATE_INSENSITIVE)
		sflags |= QStyle::Style_Off;
	else if (state == GTK_STATE_PRELIGHT)
		sflags |= QStyle::Style_On;

	QStyle::PrimitiveElement element;
	switch(direction)
	{
		case GTK_ARROW_UP:    element = QStyle::PE_ArrowUp;    break;
		case GTK_ARROW_DOWN:  element = QStyle::PE_ArrowDown;  break;
		case GTK_ARROW_LEFT:  element = QStyle::PE_ArrowLeft;  break;
		case GTK_ARROW_RIGHT: element = QStyle::PE_ArrowRight; break;
	}

	 
	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);

	if (fillPixmap && (!fillPixmap->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *fillPixmap));
	else if ((backgroundTile) && (!backgroundTile->isNull()))
		painter.fillRect(0, 0, w, h, QBrush(QColor(255,255,255), *backgroundTile));
	else
		painter.fillRect(0, 0, w, h, qApp->palette().active().brush(QColorGroup::Background));

	qApp->style().drawPrimitive(element, &painter, QRect(0,0,w,h), qApp->palette().active(), sflags);

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[state], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

void drawFocusRect(GdkWindow * window, GtkStyle * style, int x, int y, int w, int h)
{
	if (!gtkQtEnable)
		return;
	
	if ((w < 1) || (h < 1))
		return;

	QPixmap pixmap(w,h);
	QPainter painter(&pixmap);
	QColor bg(qApp->palette().active().background());

	painter.fillRect(0,0,w,h,bg);
	qApp->style().drawPrimitive(QStyle::PE_FocusRect, &painter, QRect(0,0,w,h), qApp->palette().active(), QStyle::Style_Default, QStyleOption(bg));

	GdkPixmap* pix = gdk_pixmap_foreign_new(pixmap.handle());
	gdk_draw_drawable(window, style->bg_gc[GTK_STATE_NORMAL], pix, 0, 0, x, y, w, h);
	g_object_unref(pix);
}

GdkGC* alternateBackgroundGc(GtkStyle* style)
{
	// Alternate background color for listviews
	if (altBackGC != 0)
		return altBackGC;
	
	GdkColor altBackColor;
	altBackColor.red = alternateBackgroundColour.red() * 257;
	altBackColor.green = alternateBackgroundColour.green() * 257;
	altBackColor.blue = alternateBackgroundColour.blue() * 257;
	
	gdk_colormap_alloc_color(style->colormap, &altBackColor, FALSE, TRUE);
	
	GdkGCValues gc_values;
	GdkGCValuesMask gc_values_mask;
	gc_values_mask = GDK_GC_FOREGROUND;
	gc_values.foreground = altBackColor;
	
	altBackGC = (GdkGC*) gtk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
	
	return altBackGC;
}


// Thanks Martin Dvorak of metatheme
QString parse_rc_string(const QString& defs, const QString& pattern, bool widgetClass = true)
{
	static int dynamic_counter = 0;
	++dynamic_counter;
	
	return "style \"gtk-qt-dynamic-" + QString::number(dynamic_counter) + "\" { " + defs + " } " + (widgetClass ? "widget_class" : "widget") + " \"" + pattern + "\" style \"gtk-qt-dynamic-" + QString::number(dynamic_counter) + "\"\n";
}

QString doIconMapping(const QString& stockName, const QString& path, int sizes)
{
	QString fullPath;
	bool has16 = false, has22 = false, has32 = false;

	for( QStringList::ConstIterator it = iconThemeDirs.begin();
			it != iconThemeDirs.end();
			++it )
	{
		fullPath = *it + "16x16/" + path;
		if (access(fullPath.latin1(), R_OK) == 0)
			has16 = true;
		fullPath = *it + "22x22/" + path;
		if (access(fullPath.latin1(), R_OK) == 0)
			has22 = true;
		fullPath = *it + "32x32/" + path;
		if (access(fullPath.latin1(), R_OK) == 0)
			has32 = true;
	}

	if (!has16 && !has22 && !has32) return "";
	
	// sizes is an addition of 1=16, 2=22 and 4=32
	QString ret = "stock[\"" + stockName + "\"]={\n";
	
	if (has22)
		ret += "\t{ \"22x22/" + path +"\", *, *, \"gtk-large-toolbar\" },\n";

	if (has32)
	{
		ret += "\t{ \"32x32/" + path +"\", *, *, \"gtk-dnd\" },\n";
		ret += "\t{ \"32x32/" + path +"\", *, *, \"gtk-dialog\" },\n";
	}

	if (has16)
	{
		ret += "\t{ \"16x16/" + path +"\", *, *, \"gtk-button\" },\n";
		ret += "\t{ \"16x16/" + path +"\", *, *, \"gtk-menu\" },\n";
		ret += "\t{ \"16x16/" + path +"\", *, *, \"gtk-small-toolbar\" },\n";
	}
	
	if (has22)
		ret += "\t{ \"22x22/" + path +"\" }\n";
	else if (has32)
		ret += "\t{ \"32x32/" + path +"\" }\n";
	else
		ret += "\t{ \"16x16/" + path +"\" }\n";
	
	ret += "}\n";
	return ret;
}

QString colorString(QColor color)
{
	QString ret = "{";
	ret += QString::number(color.red() * 257) + ", ";
	ret += QString::number(color.green() * 257) + ", ";
	ret += QString::number(color.blue() * 257) + "}";
	
	return ret;
}

void setColour(QString name, QColor color)
{
	gtk_rc_parse_string(parse_rc_string(name + " = " + colorString(color), "*").latin1());
}

static QStringList iconInheritsDirs( const QString& icondir )
{
	QFile index;
	index.setName( icondir + "index.theme" );
	if( !index.open( IO_ReadOnly ))
	{
		index.setName( icondir + "index.desktop" );
		if( !index.open( IO_ReadOnly ))
			return QStringList();
	}
	char buf[ 1024 ];
	QRegExp reg( "^\\s*Inherits=([^\\n]*)" );
	for(;;)
	{
		if( index.readLine( buf, 1023 ) <= 0 )
			break;
		if( reg.search( buf, 0 ) >= 0 )
			return QStringList::split(",", reg.cap(1));
	}
	return QStringList();
}

void setRcProperties(GtkRcStyle* rc_style, int forceRecreate)
{
	if (!gtkQtEnable)
		return;
	
	if (gtkQtDebug)
		printf("setRcProperties()\n");
	
	gtkRcStyle = rc_style;
	
	// Set colors
	// Normal
	setColour("fg[NORMAL]",     qApp->palette().active().text());
	setColour("bg[NORMAL]",     qApp->palette().active().background());
	setColour("text[NORMAL]",   qApp->palette().active().text());
	setColour("base[NORMAL]",   qApp->palette().active().base());
	
	// Active (on)
	setColour("fg[ACTIVE]",     qApp->palette().active().text());
	setColour("bg[ACTIVE]",     qApp->palette().active().background());
	setColour("text[ACTIVE]",   qApp->palette().active().text());
	setColour("base[ACTIVE]",   qApp->palette().active().base());
	
	// Mouseover
	setColour("fg[PRELIGHT]",   qApp->palette().active().text()); // menu items - change?
	setColour("bg[PRELIGHT]",   qApp->palette().active().highlight());
	setColour("text[PRELIGHT]", qApp->palette().active().text());
	setColour("base[PRELIGHT]", qApp->palette().active().base());
	
	// Selected
	setColour("fg[SELECTED]",   qApp->palette().active().highlightedText());
	setColour("bg[SELECTED]",   qApp->palette().active().highlight());
	setColour("text[SELECTED]", qApp->palette().active().highlightedText());
	setColour("base[SELECTED]", qApp->palette().active().highlight());
	
	// Disabled
	setColour("fg[INSENSITIVE]",    qApp->palette().disabled().text());
	setColour("bg[INSENSITIVE]",    qApp->palette().disabled().background());
	setColour("text[INSENSITIVE]",  qApp->palette().disabled().text());
	setColour("base[INSENSITIVE]",  qApp->palette().disabled().background());

	gtk_rc_parse_string(("gtk-button-images = " + QString::number(showIconsOnButtons)).latin1());
	
	gtk_rc_parse_string(("gtk-toolbar-style = " + QString::number(toolbarStyle)).latin1());
	
	// This function takes quite a long time to execute, and is run at the start of every app.
	// In order to speed it up, we can store the results in a file, along with the name of icon
	// theme and style.  This file can then be regenerated when the icon theme or style change.
	
	QString cacheFilePath = QDir::homeDirPath() + "/.gtk_qt_engine_rc";
	QFile cacheFile(cacheFilePath);
	QTextStream stream;
	
	if (!forceRecreate && cacheFile.exists())
	{
		cacheFile.open(IO_ReadOnly);
		stream.setDevice(&cacheFile);
		
		if (stream.readLine() == "# " + iconTheme + ", " + qApp->style().name() + ", " + RC_CACHE_VERSION)
		{
			// This cache matches the current icon theme and style
			// Let's load it and return
			gtk_rc_add_default_file(cacheFilePath.latin1());
			return;
		}
		
		stream.unsetDevice();
		cacheFile.close();
	}
	
	cacheFile.open(IO_WriteOnly | IO_Truncate);
	stream.setDevice(&cacheFile);
	
	stream << "# " << iconTheme << ", " << qApp->style().name() << ", " << RC_CACHE_VERSION << "\n\n";
	stream << "# This file was generated by the Gtk Qt Theme Engine\n";
	stream << "# It will be recreated when you change your KDE icon theme or widget style\n\n";
	
	QScrollBar sbar(NULL);
	sbar.setOrientation(Qt::Horizontal);
	sbar.setValue(1);
	sbar.resize(200,25);
	
	// The following code determines how many buttons are on a scrollbar
	// It works by looking at each pixel of the scrollbar's area not taken up by the groove,
	// and asking the style which subcontrol is at that location.
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
	
	stream << parse_rc_string(QString("GtkScrollbar::has-backward-stepper = ") + (back1 ? "1" : "0"), "*");
	stream << parse_rc_string(QString("GtkScrollbar::has-forward-stepper = ") + (forward2 ? "1" : "0"), "*");
	stream << parse_rc_string(QString("GtkScrollbar::has-secondary-forward-stepper = ") + (forward1 ? "1" : "0"), "*");
	stream << parse_rc_string(QString("GtkScrollbar::has-secondary-backward-stepper = ") + (back2 ? "1" : "0"), "*");
	
	stream << parse_rc_string("GtkScrollbar::stepper-size = " + QString::number(qApp->style().querySubControlMetrics(QStyle::CC_ScrollBar, &sbar, QStyle::SC_ScrollBarSubLine).width() - 1), "*");
	
	stream << parse_rc_string("GtkScrollbar::min-slider-length = " + QString::number(qApp->style().pixelMetric(QStyle::PM_ScrollBarSliderMin)), "*");
	stream << parse_rc_string("GtkScrollbar::slider-width = " + QString::number(qApp->style().pixelMetric(QStyle::PM_ScrollBarExtent)-2), "*");
	stream << parse_rc_string("GtkButton::child-displacement-x = " + QString::number(qApp->style().pixelMetric(QStyle::PM_ButtonShiftHorizontal)), "*");
	stream << parse_rc_string("GtkButton::child-displacement-y = " + QString::number(qApp->style().pixelMetric(QStyle::PM_ButtonShiftVertical)), "*");
	QSlider slider(NULL); // To keep BlueCurve happy
	stream << parse_rc_string("GtkScale::slider-length = " + QString::number(qApp->style().pixelMetric(QStyle::PM_SliderLength, &slider)), "*");
	stream << parse_rc_string("GtkButton::default-border = { 0, 0, 0, 0 }", "*");
	
	stream << parse_rc_string("xthickness = " + QString::number(qApp->style().pixelMetric(QStyle::PM_DefaultFrameWidth)), "*.GtkMenu");
	stream << parse_rc_string("ythickness = " + QString::number(qApp->style().pixelMetric(QStyle::PM_DefaultFrameWidth)), "*.GtkMenu");
	stream << parse_rc_string("ythickness = 3", "*.GtkMenu.Gtk*MenuItem");
	stream << parse_rc_string("xthickness = 3", "*.GtkNotebook");
	stream << parse_rc_string("ythickness = 3", "*.GtkNotebook");
	stream << parse_rc_string("ythickness = 1", "*.GtkButton");
	stream << parse_rc_string("fg[NORMAL] = {0, 0, 0}", "gtk-tooltips.GtkLabel", false);
	
	// This one may not work...
	//insertIntProperty(rc_style, "GtkCheckButton", "indicator-size", qApp->style().pixelMetric(QStyle::PM_IndicatorHeight) );
	
	// For icons

	// Build the list of icon theme directories.
	// This function is recursive - it gets the directories of all the inherited themes as well
	addIconThemeDir(iconTheme);

	if (iconThemeDirs.isEmpty())
	{
		cacheFile.close();
		gtk_rc_add_default_file(cacheFilePath.latin1());
		return;
	}

	stream << "\npixmap_path \"" + iconThemeDirs.join( ":" ) + "\"\n\n";
	
	stream << "style \"KDE-icons\" {\n";
        stream << doIconMapping("gtk-about", "actions/gtk-about.png");
        stream << doIconMapping("gtk-add", "actions/gtk-add.png");
        stream << doIconMapping("gtk-apply", "actions/gtk-apply.png");
	stream << doIconMapping("gtk-bold", "actions/text_bold.png");
	stream << doIconMapping("gtk-cancel", "actions/button_cancel.png");
	stream << doIconMapping("gtk-cdrom", "devices/cdrom_unmount.png");
	stream << doIconMapping("gtk-clear", "actions/editclear.png");
	stream << doIconMapping("gtk-close", "actions/fileclose.png");
	stream << doIconMapping("gtk-color-picker", "actions/colorpicker.png", 3);
	stream << doIconMapping("gtk-copy", "actions/editcopy.png");
	stream << doIconMapping("gtk-convert", "actions/gtk-convert.png");
	//stream << doIconMapping("gtk-connect", ??);
	stream << doIconMapping("gtk-cut", "actions/editcut.png");
	stream << doIconMapping("gtk-delete", "actions/editdelete.png");
	stream << doIconMapping("gtk-dialog-authentication", "status/gtk-dialog-authentication");
	stream << doIconMapping("gtk-dialog-error", "actions/messagebox_critical.png", 4);
	stream << doIconMapping("gtk-dialog-info", "actions/messagebox_info.png", 4);
	stream << doIconMapping("gtk-dialog-question", "actions/help.png");
	stream << doIconMapping("gtk-dialog-warning", "actions/messagebox_warning.png", 4);
	//stream << doIconMapping("gtk-directory", ??);
	//stream << doIconMapping("gtk-disconnect", ??);
	stream << doIconMapping("gtk-dnd", "mimetypes/empty.png");
	stream << doIconMapping("gtk-dnd-multiple", "mimetypes/kmultiple.png");
	stream << doIconMapping("gtk-edit", "actions/gtk-edit.png");  //2.6 
	stream << doIconMapping("gtk-execute", "actions/exec.png");
	stream << doIconMapping("gtk-file", "mimetypes/gtk-file.png");
	stream << doIconMapping("gtk-find", "actions/find.png");
	stream << doIconMapping("gtk-find-and-replace", "actions/gtk-find-and-replace.png");
	stream << doIconMapping("gtk-floppy", "devices/3floppy_unmount.png");
	stream << doIconMapping("gtk-fullscreen", "actions/gtk-fullscreen.png");
	stream << doIconMapping("gtk-goto-bottom", "actions/bottom.png");
	stream << doIconMapping("gtk-goto-first", "actions/start.png");
	stream << doIconMapping("gtk-goto-last", "actions/finish.png");
	stream << doIconMapping("gtk-goto-top", "actions/top.png");
	stream << doIconMapping("gtk-go-back", "actions/back.png");
	stream << doIconMapping("gtk-go-down", "actions/down.png");
	stream << doIconMapping("gtk-go-forward", "actions/forward.png");
	stream << doIconMapping("gtk-go-up", "actions/up.png");
	stream << doIconMapping("gtk-harddisk", "devices/gtk-harddisk.png");
	stream << doIconMapping("gtk-help", "apps/khelpcenter.png");
	stream << doIconMapping("gtk-home", "filesystems/folder_home.png");
	stream << doIconMapping("gtk-indent", "actions/gtk-indent.png");
	stream << doIconMapping("gtk-index", "actions/contents.png");
	//stream << doIconMapping("gtk-info", "??");
	stream << doIconMapping("gtk-italic", "actions/text_italic.png");
	stream << doIconMapping("gtk-jump-to", "actions/goto.png");
	stream << doIconMapping("gtk-justify-center", "actions/text_center.png");
	stream << doIconMapping("gtk-justify-fill", "actions/text_block.png");
	stream << doIconMapping("gtk-justify-left", "actions/text_left.png");
	stream << doIconMapping("gtk-justify-right", "actions/text_right.png");
	stream << doIconMapping("gtk-leave-fullscreen", "actions/gtk-leave-fullscreen.png");
	stream << doIconMapping("gtk-media-forward", "actions/gtk-media-forward-ltr.png");
	stream << doIconMapping("gtk-media-next", "actions/gtk-media-next-ltr.png");
	stream << doIconMapping("gtk-media-pause", "actions/gtk-media-pause.png");
	stream << doIconMapping("gtk-media-previous", "actions/gtk-media-previous-ltr.png");
	stream << doIconMapping("gtk-media-record", "actions/gtk-media-record.png");
	stream << doIconMapping("gtk-media-rewind", "actions/gtk-media-rewind-ltr.png");
	stream << doIconMapping("gtk-media-stop", "actions/gtk-media-stop.png");
	stream << doIconMapping("gtk-missing-image", "mimetypes/unknown.png");
	stream << doIconMapping("gtk-network", "places/gtk_network.png");
	stream << doIconMapping("gtk-new", "actions/filenew.png");
	stream << doIconMapping("gtk-no", "actions/gtk-no.png");
	stream << doIconMapping("gtk-ok", "actions/button_ok.png");
	stream << doIconMapping("gtk-open", "actions/fileopen.png");
	//stream << doIconMapping("gtk-orientation-landscape", "??");
	//stream << doIconMapping("gtk-orientation-portrait", "??");
	//stream << doIconMapping("gtk-orientation-reverse-landscape", "??");
	//stream << doIconMapping("gtk-orientation-reverse-portrait", "??");
	stream << doIconMapping("gtk-paste", "actions/editpaste.png");
	stream << doIconMapping("gtk-preferences", "actions/configure.png");
	stream << doIconMapping("gtk-print", "actions/fileprint.png");
	stream << doIconMapping("gtk-print-preview", "actions/filequickprint.png");
	stream << doIconMapping("gtk-properties", "actions/configure.png");
	stream << doIconMapping("gtk-quit", "actions/exit.png");
	stream << doIconMapping("gtk-redo", "actions/redo.png");
	stream << doIconMapping("gtk-refresh", "actions/reload.png");
	stream << doIconMapping("gtk-remove", "actions/gtk-remove.png");
	stream << doIconMapping("gtk-revert-to-saved", "actions/revert.png");
	stream << doIconMapping("gtk-save", "actions/filesave.png");
	stream << doIconMapping("gtk-save-as", "actions/filesaveas.png");
	stream << doIconMapping("gtk-select-all", "actions/gtk-select-all.png");
	stream << doIconMapping("gtk-select-color", "actions/colorize.png");
	stream << doIconMapping("gtk-select-font", "mimetypes/font.png");
	//stream << doIconMapping("gtk-sort-ascending", "??");
	//stream << doIconMapping("gtk-sort-descending", "??");
	stream << doIconMapping("gtk-spell-check", "actions/spellcheck.png");
	stream << doIconMapping("gtk-stop", "actions/stop.png");
	stream << doIconMapping("gtk-strikethrough", "actions/text_strike.png", 3);
	stream << doIconMapping("gtk-undelete", "actions/gtk-undelete.png");
	stream << doIconMapping("gtk-underline", "actions/text_under.png");
	stream << doIconMapping("gtk-undo", "actions/undo.png");
	stream << doIconMapping("gtk-unindent", "actions/gtk-unindent.png");
	stream << doIconMapping("gtk-yes", "actions/gtk-yes.png");
	stream << doIconMapping("gtk-zoom-100", "actions/viewmag1.png");
	stream << doIconMapping("gtk-zoom-fit", "actions/viewmagfit.png");
	stream << doIconMapping("gtk-zoom-in", "actions/viewmag+.png");
	stream << doIconMapping("gtk-zoom-out", "actions/viewmag-.png");
	stream << "} class \"*\" style \"KDE-icons\"";
	
	cacheFile.close();
	
	gtk_rc_add_default_file(cacheFilePath.latin1());
}

void addIconThemeDir(const QString& theme)
{
	// Try to find this theme's directory
	QString icondir = kdeFindDir("/share/icons/" + theme + "/", "index.theme", "index.desktop");
	if(icondir.isEmpty())
		return;
	if (iconThemeDirs.contains(icondir))
		return;

	// Add this theme to the list
	iconThemeDirs.append(icondir);

	// Do it again for any parent themes
	QStringList parents = iconInheritsDirs(icondir);
	for ( QStringList::Iterator it=parents.begin() ; it!=parents.end(); ++it)
		addIconThemeDir((*it).stripWhiteSpace());
}

void setMenuBackground(GtkStyle* style)
{
	if (!gtkQtEnable)
		return;
	
	if (menuBackgroundPixmap == NULL)
	{
		// Get the menu background image
		menuBackgroundPixmap = new QPixmap(1024, 25); // Meh
		QPainter painter(menuBackgroundPixmap);
		QPopupMenu pm;
		QMenuData md;
		QMenuItem* mi = md.findItem(md.insertItem(""));
		
		qApp->style().polish(&pm);
		
		QStyleOption opt(mi, 16, 16);
		QStyle::SFlags sflags = QStyle::Style_Default;
	
		if ((backgroundTile) && (!backgroundTile->isNull()))
			painter.fillRect(0, 0, 1024, 25, QBrush(QColor(255,255,255), *backgroundTile));
		else
			painter.fillRect(0, 0, 1024, 25, qApp->palette().active().brush(QColorGroup::Background));
		qApp->style().drawControl(QStyle::CE_PopupMenuItem, &painter, &pm, QRect(0,0,1024,25), qApp->palette().active(), sflags, opt);
		
		menuBackgroundPixmapGdk = gdk_pixmap_foreign_new(menuBackgroundPixmap->handle());
	}

	QTENGINE_STYLE(style)->menuBackground = menuBackgroundPixmapGdk;
	g_object_ref(menuBackgroundPixmapGdk);
}

// It has a 'u' damnit
void setColour(GdkColor* g, QColor q)
{
	g->red = q.red() * 257;
	g->green = q.green() * 257;
	g->blue = q.blue() * 257;
}

void setColors(GtkStyle* style)
{
	if (!gtkQtEnable)
		return;
	
	if (gtkQtDebug)
		printf("setColors()\n");
	
	/*gtkStyle = style;*/
	
	bool useBg = ((backgroundTile) && (!backgroundTile->isNull()));

	if (useBg)
	{
		style->bg_pixmap[GTK_STATE_NORMAL] = backgroundTileGdk;
		g_object_ref(backgroundTileGdk);
	}
	
	setMenuBackground(style);
}

void getTextColor(GdkColor *color, GtkStateType state_type)
{
	if (!gtkQtEnable)
		return;
	
	if ((state_type == GTK_STATE_PRELIGHT) || (state_type == GTK_STATE_ACTIVE) || (state_type == GTK_STATE_SELECTED))
		setColour(color, qApp->palette().active().highlightedText());
	else if (state_type == GTK_STATE_NORMAL)
		setColour(color, qApp->palette().active().text());
	else if (state_type == GTK_STATE_INSENSITIVE)
		setColour(color, qApp->palette().disabled().text());
}
