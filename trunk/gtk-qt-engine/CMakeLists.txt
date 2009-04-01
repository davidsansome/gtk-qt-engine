PROJECT (gtk-qt-engine)
INCLUDE (UsePkgConfig)

FIND_PACKAGE(X11 REQUIRED)
FIND_PACKAGE(KDE4)
FIND_PACKAGE(Qt4 REQUIRED)

# Find GTK
PKGCONFIG(gtk+-2.0 GTK_INCLUDE_DIR GTK_LIB_DIR GTK_LINK_FLAGS GTK_CFLAGS)
IF (NOT GTK_INCLUDE_DIR)
	MESSAGE(FATAL_ERROR "GTK was not found.  Ensure you have the GTK development libraries installed, and GTK's pkgconfig files are in your pkgconfig search path.")
ENDIF (NOT GTK_INCLUDE_DIR)
MESSAGE(STATUS "Found GTK include dir: ${GTK_INCLUDE_DIR}")
MESSAGE(STATUS "Found GTK library dir: ${GTK_LIB_DIR}")

# Find Bonobo
PKGCONFIG(libbonoboui-2.0 BONOBO_INCLUDE_DIR BONOBO_LIB_DIR BONOBO_LINK_FLAGS BONOBO_CFLAGS)
IF (NOT BONOBO_INCLUDE_DIR)
	MESSAGE(STATUS "bonoboui not found.  Some features of the theme engine will not work as intended.")
ELSE (NOT BONOBO_INCLUDE_DIR)
	MESSAGE(STATUS "Found Bonobo include dir: ${BONOBO_INCLUDE_DIR}")
	MESSAGE(STATUS "Found Bonobo library dir: ${BONOBO_LIB_DIR}")
	SET(BONOBO_DEFINITIONS -D HAVE_BONOBO)
ENDIF (NOT BONOBO_INCLUDE_DIR)

# Find msgfmt
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
FIND_PACKAGE(Msgfmt REQUIRED)

# Find xgettext
FIND_PROGRAM(
	XGETTEXT_PATH
	NAMES "xgettext"
	PATHS "/usr/bin"
)
IF(${XGETTEXT_PATH} STREQUAL "XGETTEXT_PATH-NOTFOUND")
	MESSAGE(STATUS "xgettext not found.  You will not be able to run 'make extract_messages' in the 'po' directory.")
ELSE(${XGETTEXT_PATH} STREQUAL "XGETTEXT_PATH-NOTFOUND")
	MESSAGE(STATUS "Found xgettext: ${XGETTEXT_PATH}")
ENDIF(${XGETTEXT_PATH} STREQUAL "XGETTEXT_PATH-NOTFOUND")

# Find extractrc
FIND_PROGRAM(
	EXTRACTRC_PATH
	NAMES "extractrc"
	PATHS "/usr/bin"
)
IF(${EXTRACTRC_PATH} STREQUAL "EXTRACTRC_PATH-NOTFOUND")
	MESSAGE(STATUS "extractrc not found.  You will not be able to run 'make extract_messages' in the 'po' directory.")
ELSE(${EXTRACTRC_PATH} STREQUAL "EXTRACTRC_PATH-NOTFOUND")
	MESSAGE(STATUS "Found extractrc: ${EXTRACTRC_PATH}")
ENDIF(${EXTRACTRC_PATH} STREQUAL "EXTRACTRC_PATH-NOTFOUND")

# Work out where to install stuff
EXEC_PROGRAM(
	${PKGCONFIG_EXECUTABLE}
	ARGS --variable=gtk_binary_version gtk+-2.0
	OUTPUT_VARIABLE GTK_VERSION)
SET(INSTALL_PATH_GTK_ENGINES "${GTK_LIB_DIR}/gtk-2.0/${GTK_VERSION}/engines" CACHE PATH "The directory in which to install the theme engine library (eg. /usr/lib/gtk-2.0/2.4.0/engines)")
MESSAGE(STATUS "Installing the GTK theme engine library to: ${INSTALL_PATH_GTK_ENGINES}")

EXEC_PROGRAM(
	${PKGCONFIG_EXECUTABLE}
	ARGS --variable=prefix gtk+-2.0
	OUTPUT_VARIABLE GTK_PREFIX)
SET(INSTALL_PATH_GTK_THEMES "${GTK_PREFIX}/share/themes" CACHE PATH "The directory in which to install the theme data (eg. /usr/share/themes)")
MESSAGE(STATUS "Installing the GTK theme engine data to: ${INSTALL_PATH_GTK_THEMES}")

SET(INSTALL_PATH_KCONTROL_MODULES "${KDE4_LIB_DIR}" CACHE PATH "The directory in which to install KControl plugins (eg. /usr/lib)")
MESSAGE(STATUS "Installing the KControl plugins to: ${INSTALL_PATH_KCONTROL_MODULES}")

MESSAGE(STATUS "Installing everything else to: ${CMAKE_INSTALL_PREFIX}")

IF(NOT DEFINED KDE4_FOUND)
	MESSAGE(STATUS "WARNING: The KDE4 development package was not found.  The KDE Control Center module will not be built")
ENDIF(NOT DEFINED KDE4_FOUND)

# Setup definitions and directories to be used in all subdirectories
ADD_DEFINITIONS(${QT_DEFINITIONS} ${BONOBO_DEFINITIONS} -DQT_NO_KEYWORDS)
LINK_DIRECTORIES(${GTK_LIB_DIR} ${BONOBO_LIB_DIR} ${QT_LIBRARY_DIR})
INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIR} ${BONOBO_INCLUDE_DIR} ${QT_INCLUDES} ${X11_X11_INCLUDE_PATH})

# Subdirectories
ADD_SUBDIRECTORY(src)

IF (DEFINED KDE4_FOUND)
	ADD_SUBDIRECTORY(kcm_gtk)
ENDIF (DEFINED KDE4_FOUND)

ADD_SUBDIRECTORY(po)

# Uninstall support
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
