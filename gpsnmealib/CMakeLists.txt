if(WIN32)
	ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
endif()

set(GPSNMEALIB_PUBLIC_HEADERS
	nmeaParser.h
	utmCoord.h
	latLonCoord.h)

set(GPSNMEALIB_SOURCES
	latLonCoord.C
	nmeaParser.C
	typedCoord.h
	typedCoord.C
	utmCoord.C)

add_library(gpsnmea ${GPSNMEALIB_PUBLIC_HEADERS} ${GPSNMEALIB_SOURCES})
set_property(TARGET
	gpsnmea
	PROPERTY
	PUBLIC_HEADER
	${GPSNMEALIB_PUBLIC_HEADERS})
set_property(TARGET gpsnmea PROPERTY FOLDER "Library")

if(VRPN_INSTALL)
	install(TARGETS
		gpsnmea
		RUNTIME DESTINATION bin COMPONENT serversdk
		LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT serversdk
		ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT serversdk
		PUBLIC_HEADER DESTINATION include COMPONENT serversdk)
endif()
