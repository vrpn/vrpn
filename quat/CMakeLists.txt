CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(quatlib)

# Build the library itself and declare what bits need to be installed
ADD_LIBRARY(quat matrix.c quat.c vector.c xyzquat.c)
INSTALL(FILES quat.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include) 
INSTALL(TARGETS quat ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib) 

# Include directory needed by all of the files
INCLUDE_DIRECTORIES(${quatlib_SOURCE_DIR})

# Build the testapps if required
OPTION(BUILD_QUAT_LIB_EXAMPLES "Enable to build quatlib examples" OFF)

IF(BUILD_QUAT_LIB_EXAMPLES)
  # A little utility helper macro to setup default linkages and filenames
  MACRO(TEST_APPLICATION APPLICATION_NAME)
    SET(TARGET_NAME ${APPLICATION_NAME})
    ADD_EXECUTABLE(${APPLICATION_NAME} testapps/${APPLICATION_NAME}.c)
    TARGET_LINK_LIBRARIES(${APPLICATION_NAME} quat)
    if(WIN32) # MS-Windows, both 32 and 64 bits
      # nothing here yet
    elseif(APPLE) # Apple
      target_link_libraries(${APPLICATION_NAME} m) # Math library
    elseif(UNIX) # other UNIXes
      target_link_libraries(${APPLICATION_NAME} m) # Math library
    endif()
  ENDMACRO(TEST_APPLICATION)

  # Declaration of the actual test applications themselves
  TEST_APPLICATION(eul)
  TEST_APPLICATION(matrix_to_posquat)
  TEST_APPLICATION(qmake)
  TEST_APPLICATION(qmult)
  TEST_APPLICATION(qxform)
  TEST_APPLICATION(timer)
ENDIF(BUILD_QUAT_LIB_EXAMPLES)