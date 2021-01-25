#pragma once

#include <Antilatency.Api.h>

#if defined(__has_include) && __has_include(<QLibrary>)
    #include "Antilatency.InterfaceContract.LibraryLoader.Qt.h"
#elif defined(_WIN32)
    #if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        #include "Antilatency.InterfaceContract.LibraryLoader.Win32.h"
    #elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
        #include "Antilatency.InterfaceContract.LibraryLoader.Uwp.h"
    #endif
#elif defined(__ANDROID__) || defined(__linux__)
    #include "Antilatency.InterfaceContract.LibraryLoader.Unix.h"
#endif

namespace Antilatency {
    namespace InterfaceContract {
        template<typename LibraryInterface>
        LibraryInterface getLibraryInterface(const char* libraryName){
            return (new LibraryLoader<LibraryInterface>(libraryName))->getLibraryInterface();
        }
    }
}
