#ifndef LOADERQT_H
#define LOADERQT_H

#include <Antilatency.Api.h>
#include <QLibrary>

namespace Antilatency {
    namespace InterfaceContract {
        template<typename LibraryInterface>
        class LibraryLoader : public Antilatency::InterfaceContract::Library<LibraryInterface> {
        public:
            LibraryLoader(const char* libraryName){
                _library.setFileName(libraryName);
                _library.load();
            }
            typename Library<LibraryInterface>::LibraryEntryPoint getEntryPoint() override{
                if(_library.isLoaded()){
                    return reinterpret_cast<typename Library<LibraryInterface>::LibraryEntryPoint>(_library.resolve("getLibraryInterface"));
                }
                return nullptr;
            }
        protected:
            void unloadLibraryImpl() override{

            }
        private:
            QLibrary _library;
        };
    }
}

#endif // LOADERQT_H
