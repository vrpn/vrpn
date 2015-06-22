/** @file
    @brief Header containing vrpn_Thread, vrpn_Semaphore (formerly in
   vrpn_Shared.h), as well as a lock-guard class.

    Semaphore and Thread classes derived from Hans Weber's classes from UNC.
    Don't let the existence of a Thread class fool you into thinking
    that VRPN is thread-safe.  This and the Semaphore are included as
    building blocks towards making your own code thread-safe.  They are
    here to enable the vrpn_Imager_Logger class to do its thing.

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_vrpn_Thread_h_GUID_A455652F_72CE_4F8A_859E_543489012D01
#define INCLUDED_vrpn_Thread_h_GUID_A455652F_72CE_4F8A_859E_543489012D01

// Internal Includes
#include "vrpn_Configure.h" // for VRPN_API

// Library/third-party includes
// - none

// Standard includes

#if defined(sgi) || (defined(_WIN32) && !defined(__CYGWIN__)) ||               \
    defined(linux) || defined(__APPLE__)
#define vrpn_THREADS_AVAILABLE
#else
#undef vrpn_THREADS_AVAILABLE
#endif

// multi process stuff
#if defined(sgi)
#include <task.h>
#include <ulocks.h>
#elif defined(_WIN32)
#include "vrpn_WindowsH.h"
#include <process.h>
#else
#include <pthread.h>   // for pthread_t
#include <semaphore.h> // for sem_t
#endif

// make the SGI compile without tons of warnings
#ifdef sgi
#pragma set woff 1110, 1424, 3201
#endif

// and reset the warnings
#ifdef sgi
#pragma reset woff 1110, 1424, 3201
#endif

class VRPN_API vrpn_Semaphore {
public:
    /// @brief constructor - mutex by default (0 is a sync primitive)
    vrpn_Semaphore(int cNumResources = 1);

    /// @brief destructor
    ~vrpn_Semaphore();

    /// @brief routine to reset it (true on success, false on failure)
    /// (may create new semaphore)
    bool reset(int cNumResources = 1);

    /// @brief Blocking acquire of resource. ("down")
    /// @return 1 when it has acquired the resource, -1 on fail
    int p();

    /// @brief Release of resource. ("up")
    /// @return 0 when it has released the resource, -1 on fail
    int v();

    /// @brief Non-blocking attempt to acquire resource ("down")
    /// @return 0 if it could not access the resource
    /// and 1 if it could (-1 on fail)
    int condP();

    /// @brief read values
    int numResources();

private:
    /// @brief non-copyable
    vrpn_Semaphore(const vrpn_Semaphore &);
    /// @brief non-assignable
    vrpn_Semaphore & operator=(const vrpn_Semaphore &);
    /// @name common init and destroy routines
    /// @{
    bool init();
    bool destroy();
    /// @}

    int cResources;

    // arch specific details
#ifdef sgi
    // single mem area for dynamically alloced shared mem
    static usptr_t *ppaArena;
    static void allocArena();

    // the semaphore struct in the arena
    usema_t *ps;
    ulock_t l;
    bool fUsingLock;
#elif defined(_WIN32)
    HANDLE hSemaphore;
#else
    sem_t *semaphore; // Posix
#endif
};

namespace vrpn {
    struct try_to_lock_t {
    };

    /// @brief Dummy variable to pass to SemaphoreGuard to indicate we only want
    /// a conditional lock.
    const try_to_lock_t try_to_lock = {};
    /// @brief An RAII lock/guard class for vrpn_Semaphore
    class VRPN_API SemaphoreGuard {
    public:
        /// @brief Constructor that locks (p) the semaphore
        explicit SemaphoreGuard(vrpn_Semaphore &sem);

        /// @brief overload that only tries to lock (condP) - doesn't block.
        SemaphoreGuard(vrpn_Semaphore &sem, try_to_lock_t);

        /// @brief Destructor that unlocks if we've locked.
        ~SemaphoreGuard();

        /// @brief Checks to see if we locked.
        bool locked() const { return locked_; }

        /// @brief Locks the semaphore, if we haven't locked it already.
        void lock();

        /// @brief Tries to lock - returns true if we locked it.
        bool try_to_lock();

        /// @brief Unlocks the resource, if we have locked it.
        void unlock();

    private:
        void handleLockResult_(int result);
        /// @brief non-copyable
        SemaphoreGuard(SemaphoreGuard const &);
        /// @brief non-assignable
        SemaphoreGuard &operator=(SemaphoreGuard const &);
        bool locked_;
        vrpn_Semaphore &sem_;
    };

} // namespace vrpn

// A ptr to this struct will be passed to the
// thread function.  The user data ptr will be in pvUD.
// (There used to be a non-functional semaphore object
// also in this structure, but it was removed.  This leaves
// a struct with only one element, which is a pain but
// at least it doesn't break existing code.  If we need
// to add something else later, there is a place for it.

// The user should create and manage any semaphore needed
// to handle access control to the userdata.

struct VRPN_API vrpn_ThreadData {
    void *pvUD;
};

typedef void(*vrpn_THREAD_FUNC)(vrpn_ThreadData &threadData);

// Don't let the existence of a Thread class fool you into thinking
// that VRPN is thread-safe.  This and the Semaphore are included as
// building blocks towards making your own code thread-safe.  They are
// here to enable the vrpn_Imager_Stream_Buffer class to do its thing.
class VRPN_API vrpn_Thread {
public:
    // args are the routine to run in the thread
    // a ThreadData struct which will be passed into
    // the thread (it will be passed as a void *).
    vrpn_Thread(vrpn_THREAD_FUNC pfThread, vrpn_ThreadData td);
    ~vrpn_Thread();

#if defined(sgi)
    typedef unsigned long thread_t;
#elif defined(_WIN32)
    typedef uintptr_t thread_t;
#else
    typedef pthread_t thread_t;
#endif

    // start/kill the thread (true on success, false on failure)
    bool go();
    bool kill();

    // thread info: check if running, get proc id
    bool running();
    thread_t pid();

    // run-time user function to test if threads are available
    // (same value as #ifdef THREADS_AVAILABLE)
    static bool available();

    // Number of processors available on this machine.
    static unsigned number_of_processors();

    // This can be used to change the ThreadData user data ptr
    // between calls to go (ie, when a thread object is used
    // many times with different args).  This will take
    // effect the next time go() is called.
    void userData(void *pvNewUserData);
    void *userData();

protected:
    // user func and data ptrs
    void(*pfThread)(vrpn_ThreadData &ThreadData);
    vrpn_ThreadData td;

    // utility func for calling the specified function.
    static void threadFuncShell(void *pvThread);

    // Posix version of the utility function, makes the
    // function prototype match.
    static void *threadFuncShellPosix(void *pvThread);

    // the process id
    thread_t threadID;
};

// Returns true if they work and false if they do not.
extern bool vrpn_test_threads_and_semaphores(void);



#endif // INCLUDED_vrpn_Thread_h_GUID_A455652F_72CE_4F8A_859E_543489012D01

