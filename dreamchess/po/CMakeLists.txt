if(WIN32)
	set(LOCALEDIR locale)
else()
	set(LOCALEDIR ${CMAKE_INSTALL_LOCALEDIR})
endif()

GettextTranslate()
