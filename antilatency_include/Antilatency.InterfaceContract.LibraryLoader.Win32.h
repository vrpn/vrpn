#ifndef LOADERWIN32_H
#define LOADERWIN32_H

#include <Antilatency.InterfaceContract.h>
#ifdef __GNUG__
#   include <windows.h>
#else
#   include <Windows.h>
#endif

namespace Antilatency {
    namespace InterfaceContract {
        template<typename LibraryInterface>
        class LibraryLoader : public Antilatency::InterfaceContract::Library<LibraryInterface> {
        public:
            LibraryLoader(const char* libraryName){
                _library = LoadLibraryA(libraryName);
            }
            typename Library<LibraryInterface>::LibraryEntryPoint getEntryPoint() override{
                if(_library != nullptr){
                    return reinterpret_cast<typename Library<LibraryInterface>::LibraryEntryPoint>(GetProcAddress(_library, "getLibraryInterface"));
                }
                return nullptr;
            }
        protected:
            void unloadLibraryImpl() override {
                FreeLibrary(_library);
                _library = nullptr;
            }
        private:
            HMODULE _library = nullptr;
        };
    }
}

#endif // LOADERWIN32_H
