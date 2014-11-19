include_directories(${DIRECTSHOW_INCLUDE_DIRS})
file(GLOB
	BASECLASSES
	"${DIRECTSHOW_BASECLASS_DIR}/*.cpp"
	"${DIRECTSHOW_BASECLASS_DIR}/*.h")

# Separate static library so that unresolved externals are OK - when
# we link with it below, we'll only grab what we need.
add_library(DirectShow_BaseClasses STATIC ${BASECLASSES})

add_executable(directshow_video_server
	directx_video_imager_server.cpp
	directx_camera_server.cpp
	directx_camera_server.h)
target_link_libraries(directshow_video_server
	${VRPN_SERVER_LIBRARY}
	DirectShow_BaseClasses
	${DIRECTSHOW_LIBRARIES})

set_target_properties(directshow_video_server
	DirectShow_BaseClasses
	PROPERTIES
	FOLDER
	Servers)

if(VRPN_INSTALL)
	install(TARGETS
		directshow_video_server
		RUNTIME
		DESTINATION
		bin
		COMPONENT
		servers)
endif()