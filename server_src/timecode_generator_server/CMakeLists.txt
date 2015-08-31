# XXX TODO:
# Do something reasonable with the runtime DLL.

include_directories(${ADRIENNE_INCLUDE_DIRS})

set(TIMECODE_GENSERV_SOURCES vrpn_timecode_generator.cpp)

set(TIMECODE_GENSERV_PUBLIC_HEADERS vrpn_timecode_generator.h)

add_library(vrpn_timecode_generator
	${TIMECODE_GENSERV_SOURCES}
	${TIMECODE_GENSERV_PUBLIC_HEADERS})
set_property(TARGET
	vrpn_timecode_generator
	PROPERTY
	PUBLIC_HEADER
	${TIMECODE_GENSERV_PUBLIC_HEADERS})

target_link_libraries(vrpn_timecode_generator ${ADRIENNE_LIBRARIES})
set_target_properties(vrpn_timecode_generator
	PROPERTIES
	FOLDER
	"Main Server")

if(VRPN_INSTALL)
	install(TARGETS
		vrpn_timecode_generator
		RUNTIME DESTINATION bin COMPONENT serversdk
		LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT serversdk
		ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT serversdk
		PUBLIC_HEADER DESTINATION include COMPONENT serversdk)
endif()
