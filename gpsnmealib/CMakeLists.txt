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

install(TARGETS
	gpsnmea
	ARCHIVE
	DESTINATION
	lib
	COMPONENT
	serversdk
	PUBLIC_HEADER
	DESTINATION
	include
	COMPONENT
	serversdk)
