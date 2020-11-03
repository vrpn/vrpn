#include "vrpn_Configure.h"
#ifdef  VRPN_USE_HID
#ifdef  VRPN_USE_LOCAL_HIDAPI
	
#if defined(_WIN32) || defined(__CYGWIN__)

// I had to include this definition to get the hid.c file to compile
// under Visual Studio 2005.  Hopefully this won't conflict with the environments
// of others.  In future versions, if NTSTATUS is located by
// the compiler then we can remove this definition.

#include "submodules/hidapi/windows/hid.c"

#pragma comment( lib, "Setupapi.lib" )

#elif defined(linux)
// On linux, we need to compile this code as C code rather than C++ code
// because otherwise the lack of casts from void* keeps it from compiling.
// Hopefully this will be fixed in a future version.  If so, we can then remove
// the special "compile this as C" line from the Makefile and the following
// check.
#ifdef __cplusplus
#error This code must be compiled as C code, rather than C++.  Use the '-x c' option to the compiler.
#endif

#include "submodules/hidapi/libusb/hid.c"

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
