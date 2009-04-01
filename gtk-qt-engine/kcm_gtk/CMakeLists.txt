INCLUDE(KDE3Macros)
INCLUDE(GtkQt)

# Sources
SET(kcm_kcmgtk_SOURCES
	kcmgtk.cpp)

KDE3_AUTOMOC(${kcm_kcmgtk_SOURCES})

# UI files
SET(kcm_kcmgtk_UI
	kcmgtkwidget.ui
	mozillaprofile.ui
	emacsdetails.ui
	searchpaths.ui)

KDE3_ADD_UI_FILES(kcm_kcmgtk_SOURCES ${kcm_kcmgtk_UI})

GTKQT_KDE3_ADD_KPART(kcm_kcmgtk "${INSTALL_PATH_KCONTROL_MODULES}/kde3" ${kcm_kcmgtk_SOURCES})

INSTALL(
	TARGETS kcm_kcmgtk
	DESTINATION ${INSTALL_PATH_KCONTROL_MODULES}/kde3
)

INSTALL(
	FILES kcmgtk.desktop
	DESTINATION share/applications/
)

