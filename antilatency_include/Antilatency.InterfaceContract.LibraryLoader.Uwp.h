#pragma once

#include <string>
#include <Antilatency.InterfaceContract.h>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace Antilatency {
    namespace InterfaceContract {
        template<typename LibraryInterface>
        class LibraryLoader : public Antilatency::InterfaceContract::Library<LibraryInterface> {
        public:
            LibraryLoader(const char* libraryName) {
                std::string s(libraryName);
                std::wstring ws(s.begin(), s.end());
                _library = LoadPackagedLibrary(ws.c_str(), 0);
            }
            typename Library<LibraryInterface>::LibraryEntryPoint getEntryPoint() override{
                if(_library != nullptr){
                    return reinterpret_cast<typename Library<LibraryInterface>::LibraryEntryPoint>(GetProcAddress(_library, "getLibraryInterface"));
                }
                return nullptr;
            }
        protected:
            void unloadLibraryImpl() override{
                //TODO: implement
            }
        private:
            HMODULE _library = nullptr;
        };
    }
}
