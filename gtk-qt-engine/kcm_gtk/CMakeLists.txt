# Sources
SET(kcm_gtk4_SOURCES
	kcmgtk.cpp
	gtkrcfile.cpp
	firefoxfix.cpp
	searchpaths.cpp
)

# UI files
SET(kcm_gtk4_UI
	kcmgtkwidget.ui
	searchpaths.ui
)

KDE4_ADD_UI_FILES(kcm_gtk4_SOURCES ${kcm_gtk4_UI})

KDE4_ADD_PLUGIN(kcm_gtk4 ${kcm_gtk4_SOURCES})

INCLUDE_DIRECTORIES(${KDE4_INCLUDE_DIR})
LINK_DIRECTORIES(${KDE4_LIB_DIR})

TARGET_LINK_LIBRARIES(kcm_gtk4 ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})

INSTALL(
	TARGETS kcm_gtk4
	DESTINATION ${INSTALL_PATH_KCONTROL_MODULES}/kde4
)

INSTALL(
	FILES kcmgtk4.desktop
	DESTINATION ${XDG_APPS_INSTALL_DIR}
)

INSTALL(
	FILES kcmgtk.png
	DESTINATION ${ICON_INSTALL_DIR}
)

