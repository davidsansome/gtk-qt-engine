INCLUDES    = $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_kcmgtk.la

kcm_kcmgtk_la_SOURCES = kcmgtkwidget.ui kcmgtk.cpp mozillaprofile.ui
kcm_kcmgtk_la_LIBADD = $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT)
kcm_kcmgtk_la_LDFLAGS = -module -avoid-version $(all_libraries) -no-undefined


kcmgtk_DATA = kcmgtk.desktop
kcmgtkdir = $(kde_appsdir)/Settings/LookNFeel

kcmgtkdebian_DATA = kcmgtk.menu
kcmgtkdebiandir = /usr/lib/menu

kcmgtkxdg_DATA = kcmgtk-xdg.desktop
kcmgtkxdgdir = /usr/share/applications


noinst_HEADERS =  kcmgtk.h

messages:
	$(XGETTEXT) *.h *.cpp -o $(podir)/gtkqtengine.pot
