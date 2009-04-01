# Both these macros are taken from KDE3Macros.cmake
# Apart from the names, the only modification is that they take a destination argument

MACRO(GTKQT_KDE3_ADD_KPART _target_NAME _destination _with_PREFIX)
#is the first argument is "WITH_PREFIX" then keep the standard "lib" prefix, otherwise SET the prefix empty
   IF (${_with_PREFIX} STREQUAL "WITH_PREFIX")
      SET(_first_SRC)
   ELSE (${_with_PREFIX} STREQUAL "WITH_PREFIX")
      SET(_first_SRC ${_with_PREFIX})
   ENDIF (${_with_PREFIX} STREQUAL "WITH_PREFIX")

   IF (KDE3_ENABLE_FINAL)
      KDE3_CREATE_FINAL_FILE(${_target_NAME}_final.cpp ${_first_SRC} ${ARGN})
      ADD_LIBRARY(${_target_NAME} MODULE  ${_target_NAME}_final.cpp)
   ELSE (KDE3_ENABLE_FINAL)
      ADD_LIBRARY(${_target_NAME} MODULE ${_first_SRC} ${ARGN})
   ENDIF (KDE3_ENABLE_FINAL)

   IF(_first_SRC)
      SET_TARGET_PROPERTIES(${_target_NAME} PROPERTIES PREFIX "")
   ENDIF(_first_SRC)

   GTKQT_KDE3_INSTALL_LIBTOOL_FILE(${_target_NAME} ${_destination})

ENDMACRO(GTKQT_KDE3_ADD_KPART)

MACRO(GTKQT_KDE3_INSTALL_LIBTOOL_FILE _target _destination)
   GET_TARGET_PROPERTY(_target_location ${_target} LOCATION)

   GET_FILENAME_COMPONENT(_laname ${_target_location} NAME_WE)
   GET_FILENAME_COMPONENT(_soname ${_target_location} NAME)
   SET(_laname ${CMAKE_CURRENT_BINARY_DIR}/${_laname}.la)

   FILE(WRITE ${_laname} "# ${_laname} - a libtool library file, generated by cmake \n")
   FILE(APPEND ${_laname} "# The name that we can dlopen(3).\n")
   FILE(APPEND ${_laname} "dlname='${_soname}'\n")
   FILE(APPEND ${_laname} "# Names of this library\n")
   FILE(APPEND ${_laname} "library_names='${_soname} ${_soname} ${_soname}'\n")
   FILE(APPEND ${_laname} "# The name of the static archive\n")
   FILE(APPEND ${_laname} "old_library=''\n")
   FILE(APPEND ${_laname} "# Libraries that this one depends upon.\n")
   FILE(APPEND ${_laname} "dependency_libs=''\n")
#   FILE(APPEND ${_laname} "dependency_libs='${${_target}_LIB_DEPENDS}'\n")
   FILE(APPEND ${_laname} "# Version information.\ncurrent=0\nage=0\nrevision=0\n")
   FILE(APPEND ${_laname} "# Is this an already installed library?\ninstalled=yes\n")
   FILE(APPEND ${_laname} "# Should we warn about portability when linking against -modules?\nshouldnotlink=yes\n")
   FILE(APPEND ${_laname} "# Files to dlopen/dlpreopen\ndlopen=''\ndlpreopen=''\n")
   FILE(APPEND ${_laname} "# Directory that this library needs to be installed in:\n")
   FILE(APPEND ${_laname} "libdir='${CMAKE_INSTALL_PREFIX}/lib/kde3'\n")

   INSTALL(
      FILES ${_laname}
      DESTINATION ${_destination}
   )

ENDMACRO(GTKQT_KDE3_INSTALL_LIBTOOL_FILE)