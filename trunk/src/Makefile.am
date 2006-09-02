# set the include path for X, qt and KDE
INCLUDES = $(all_includes) $(GTK2_CFLAGS) $(LIBBONOBOUI_CFLAGS)

# these are the headers for your project
noinst_HEADERS = qt_qt_wrapper.h qt_rc_style.h qt_style.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/gtkqtengine.pot

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

enginedir = $(GTK_LIBPREFIX)/gtk-2.0/$(GTK_VERSION)/engines
engine_LTLIBRARIES = libqtengine.la

libqtengine_la_SOURCES = qt_main.c qt_rc_style.c qt_theme_draw.c qt_qt_wrapper.cpp
# the application source, library search path, and link libraries
libqtengine_la_LDFLAGS = -module -avoid-version -no-undefined $(GTK2_CFLAGS) $(all_libraries)
libqtengine_la_LIBADD = $(GTK2_LIBS) $(LIB_QT) $(LIBBONOBOUI_LIBS) 


dummy =
themedir = $(GTK_PREFIX)/share/themes/Qt/gtk-2.0
theme_DATA = gtkrc

appdir = $(GTK_PREFIX)/share/gtk-qt-engine
app_DATA = kde-index.theme

EXTRA_DIST = $(theme_DATA) $(app_DATA)
