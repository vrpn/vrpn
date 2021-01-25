#ifndef LOADERUNIX_H
#define LOADERUNIX_H

#include <Antilatency.InterfaceContract.h>
#include <dlfcn.h>

namespace Antilatency {
    namespace InterfaceContract {
        template<typename LibraryInterface>
        class LibraryLoader : public Antilatency::InterfaceContract::Library<LibraryInterface> {
        public:
            LibraryLoader(const char* libraryName){
                _library = dlopen(libraryName, RTLD_LAZY);
            }
            typename Library<LibraryInterface>::LibraryEntryPoint getEntryPoint() override{
                if(_library != nullptr){
                    return reinterpret_cast<typename Library<LibraryInterface>::LibraryEntryPoint>(dlsym(_library, "getLibraryInterface"));
                }
                return nullptr;
            }
        protected:
            void unloadLibraryImpl() override{
                if(_library != nullptr){
                    dlclose(_library);
                    _library = nullptr;
                }
            }
        private:
            void* _library = nullptr;
        };
    }
}

#endif // LOADERUNIX_H
