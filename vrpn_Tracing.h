/** @file
    @brief Header

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

//               Copyright Sensics, Inc. 2015.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_vrpn_Tracing_h_GUID_AB7863D8_FF81_4A99_0348_41018326D4F5
#define INCLUDED_vrpn_Tracing_h_GUID_AB7863D8_FF81_4A99_0348_41018326D4F5

// Internal Includes
#include "vrpn_Configure.h"
#include "vrpn_Types.h"

// Library/third-party includes
// - none

// Standard includes
// - none

#ifdef VRPN_USE_ETWPROVIDERS
#include <vrpn_WindowsH.h>
#include <ETWProviders/etwprof.h>
#include <cstring>
#include <cmath>
#endif

namespace vrpn {
    namespace tracing {

#ifdef VRPN_USE_ETWPROVIDERS
        class Region {
        protected:
            Region(const char name[])
                : name_(name)
            {
                stamp_ = ETWBegin(name);
            }

        public:
            ~Region() { ETWEnd(name_, stamp_); }
        private:
            // Noncopyable
            Region(Region const&);
            Region& operator=(Region const&);

            const char* name_;
            int64 stamp_;
        };
        inline void mark(const char message[]) { ETWMark(message); }

        // For scientific purposes only!
        inline int shovePointerIntoInt(void* ptr)
        {

            int ret = 0;
            std::memcpy(reinterpret_cast<char*>(&ret),
                        reinterpret_cast<const char*>(&ptr),
                        sizeof(int) < sizeof(void*) ? sizeof(int)
                                                    : sizeof(void*));
            return ret;
        }
#else
        class Region {
        protected:
            Region(const char[]) {}

        public:
            ~Region() {}
        private:
            // Noncopyable
            Region(Region const&);
            Region& operator=(Region const&);
        };

        inline void mark(const char[]) {}
#endif

        inline void markConnectionManagerDestructor()
        {
            mark("vrpn_ConnectionManager::~vrpn_ConnectionManager()");
        }
        inline void markConnectionManagerGetInstance()
        {
            mark("vrpn_ConnectionManager::instance()");
        }

        inline void markConnectionManagerAddConnection(void* conn,
                                                       const char* name)
        {
#ifdef VRPN_USE_ETWPROVIDERS
            ETWMarkPrintf("vrpn_ConnectionManager::addConnection(%x, \"%s\")",
                          conn, name);
#endif
        }
        inline void markConnectionManagerDeletingConnection(void* conn)
        {
#ifdef VRPN_USE_ETWPROVIDERS
            ETWMark1I("vrpn_ConnectionManager is deleting a connection",
                      shovePointerIntoInt(conn));
#endif
        }

        inline void markGetConnectionByName(const char* name, bool force)
        {
#ifdef VRPN_USE_ETWPROVIDERS
            ETWMarkPrintf("vrpn_get_connection_by_name(\"%s\", ..., %s)", name,
                          (force ? "true" : "false"));
#endif
        }

        inline void markRequestSemaphore(void* guardPtr, void* semaphorePtr)
        {
#ifdef VRPN_USE_ETWPROVIDERS
            ETWMark2I("SemaphoreGuard: attempting to lock",
                      shovePointerIntoInt(guardPtr),
                      shovePointerIntoInt(semaphorePtr));
#endif
        }
        inline void markAcquiredSemaphore(void* guardPtr, void* semaphorePtr)
        {
#ifdef VRPN_USE_ETWPROVIDERS
            ETWMark2I("SemaphoreGuard: locked", shovePointerIntoInt(guardPtr),
                      shovePointerIntoInt(semaphorePtr));
#endif
        }
        inline void markReleasedSemaphore(void* guardPtr, void* semaphorePtr)
        {
#ifdef VRPN_USE_ETWPROVIDERS
            ETWMark2I("SemaphoreGuard: unlocked", shovePointerIntoInt(guardPtr),
                      shovePointerIntoInt(semaphorePtr));
#endif
        }
    } // namespace tracing
} // namespace vrpn

#endif // INCLUDED_vrpn_Tracing_h_GUID_AB7863D8_FF81_4A99_0348_41018326D4F5
