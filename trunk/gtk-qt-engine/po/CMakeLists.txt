# .po to .gmo stuff
FILE(GLOB _pofiles *.po)

FOREACH(_file ${_pofiles})
	GET_FILENAME_COMPONENT(_file_we ${_file} NAME_WE)
	SET(_out "${_file_we}.gmo")
	SET(_in "${_file_we}.po")
	
	ADD_CUSTOM_COMMAND(
		OUTPUT ${_out}
		COMMAND ${MSGFMT_EXECUTABLE} -o ${_out} ${_in}
		DEPENDS ${_in}
	)
	
	INSTALL(
		FILES ${_out}
		DESTINATION share/locale/${_file_we}/LC_MESSAGES/
		RENAME gtkqtengine.mo
	)
	
	SET(_outputs ${_outputs} ${_out})
ENDFOREACH(_file)

ADD_CUSTOM_TARGET(
	pofiles ALL
	DEPENDS ${_outputs}
)


# Stuff to generate the .pot
FILE(GLOB POT_UIFILES ../kcm_gtk/*.ui)
SET(POT_SOURCES ../kcm_gtk/kcmgtk.cpp ../kcm_gtk/firefoxfix.cpp ../kcm_gtk/gtkrcfile.cpp ../kcm_gtk/searchpaths.cpp)
SET(POT_OUTPUT gtkqtengine.pot)

ADD_CUSTOM_COMMAND(
	OUTPUT _${POT_OUTPUT}
	COMMAND ${EXTRACTRC_PATH} ${POT_UIFILES} > rc.cpp
	COMMAND ${XGETTEXT_PATH} --foreign-user -C -ci18n -ki18n -ktr2i18n
		-kI18N_NOOP -kI18N_NOOP2 -kaliasLocale -o ${POT_OUTPUT} rc.cpp ${POT_SOURCES}
	COMMAND rm rc.cpp
)

ADD_CUSTOM_TARGET(
	extract_messages
	DEPENDS _${POT_OUTPUT}
)
