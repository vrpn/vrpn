#include "vrpn_Configure.h"
#ifdef  VRPN_USE_HID
#ifdef  VRPN_USE_LOCAL_HIDAPI
	
#if defined(_WIN32) || defined(__CYGWIN__)

// I had to include these extras to get the hid.cpp file to compile
// under Visual Studio 2005.  Hopefully they don't conflict with the environments
// of others.  In future versions, if HIDP_PREPARSED_DATA is located by
// the compiler then we can remove the second of these definitions.  Also, if
// it compiles without the DDK in the future, we can remove the first one.
#define HIDAPI_USE_DDK
#ifndef HIDP_PREPARSED_DATA
typedef struct _HIDP_PREPARSED_DATA HIDP_PREPARSED_DATA;
#endif
#pragma comment( lib, "Hid.lib" )
#pragma comment( lib, "Setupapi.lib" )

#include "submodules/hidapi/windows/hid.cpp"

#elif defined(linux)
// On linux, we need to compile this code as C code rather than C++ code
// because otherwise the lack of casts from void* keeps it from compiling.
// Hopefully this will be fixed in a future version.  If so, we can then remove
// the special "compile this as C" line from the Makefile and the following
// check.
#ifdef __cplusplus
#error This code must be compiled as C code, rather than C++.  Use the '-x c' option to the compiler.
#endif

// I had to include these stubs to make the code compile under Linux.
// They will disable the hid_send_feature_report() and hid_get_feature_report()
// functions, unfortunately, but it did not link otherwise.
#include <stdio.h>
static int HIDIOCSFEATURE(size_t length) {
  fprintf(stderr,"HIDIOCSFEATURE not found during link, so hacked into vrpn_Local_HIDAPI.C.  This implementation does nothing.\n");
  return 0;
}
static int HIDIOCGFEATURE(size_t length) {
  fprintf(stderr,"HIDIOCGFEATURE not found during link, so hacked into vrpn_Local_HIDAPI.C.  This implementation does nothing.\n");
  return 0;
}
#include "submodules/hidapi/linux/hid.c"

#elif defined(__APPLE__)
// On the mac, we need to compile this code as C code rather than C++ code
// because otherwise the lack of casts from void* keeps it from compiling.
// The inclusion of this file is handled in CMake.
//#include "submodules/hidapi/mac/hid.c"

#else
#error HIDAPI is not configured for this architecture.  If the current version works on this architecture, describe how to find it in this file.

#endif

#endif  // VRPN_USE_LOCAL_HIDAPI
#endif  // VRPN_USE_HID