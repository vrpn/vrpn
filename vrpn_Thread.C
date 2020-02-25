/** @file
    @brief Implementation

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/


// Copyright 2015 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "vrpn_Thread.h"
#include "vrpn_Shared.h"

// Library/third-party includes
// - none

// Standard includes
#include <stdio.h>  // for fprintf, stderr, perror, etc
#include <string.h> // for memcpy, strlen, strcpy, etc
#ifndef _WIN32
#include <errno.h>  // for EAGAIN, errno
#include <signal.h> // for pthread_kill, SIGKILL
#endif

#ifdef __APPLE__
#include <unistd.h>
#endif

#define ALL_ASSERT(exp, msg)                                                   \
    if (!(exp)) {                                                              \
        fprintf(stderr, "\nAssertion failed! \n %s (%s, %d)\n", msg, __FILE__, \
                __LINE__);                                                     \
        }

// init all fields in init()
vrpn_Semaphore::vrpn_Semaphore(int cNumResources)
    : cResources(cNumResources)
{
    init();
}

#ifdef sgi
bool vrpn_Semaphore::init()
{
    if (vrpn_Semaphore::ppaArena == NULL) {
        vrpn_Semaphore::allocArena();
    }
    if (cResources == 1) {
        fUsingLock = true;
        ps = NULL;
        // use lock instead of semaphore
        if ((l = usnewlock(vrpn_Semaphore::ppaArena)) == NULL) {
            fprintf(stderr, "vrpn_Semaphore::vrpn_Semaphore: error allocating "
                "lock from arena.\n");
            return false;
        }
    }
    else {
        fUsingLock = false;
        l = NULL;
        if ((ps = usnewsema(vrpn_Semaphore::ppaArena, cResources)) == NULL) {
            fprintf(stderr, "vrpn_Semaphore::vrpn_Semaphore: error allocating "
                "semaphore from arena.\n");
            return false;
        }
    }
    return true;
}
#elif defined(_WIN32)
bool vrpn_Semaphore::init()
{
    // args are security, initial count, max count, and name
    // TCH 20 Feb 2001 - Make the PC behavior closer to the SGI behavior.
    int numMax = cResources;
    if (numMax < 1) {
        numMax = 1;
    }
    hSemaphore = CreateSemaphore(NULL, cResources, numMax, NULL);
    if (!hSemaphore) {
        // get error info from windows (from FormatMessage help page)
        LPVOID lpMsgBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // Default language
            (LPTSTR)&lpMsgBuf, 0, NULL);
        fprintf(stderr,
            "vrpn_Semaphore::vrpn_Semaphore: error creating semaphore, "
            "WIN32 CreateSemaphore call caused the following error: %s\n",
            (LPTSTR)lpMsgBuf);
        // Free the buffer.
        LocalFree(lpMsgBuf);
        return false;
    }
    return true;
}
#elif defined(__APPLE__)
bool vrpn_Semaphore::init()
{
    // We need to use sem_open on the mac because sem_init is not implemented
    int numMax = cResources;
    if (numMax < 1) {
        numMax = 1;
    }
    char tempname[100];
    sprintf(tempname, "/tmp/vrpn_sem.XXXXXXX");
    semaphore = sem_open(mktemp(tempname), O_CREAT, 0600, numMax);
    if (semaphore == SEM_FAILED) {
        perror("vrpn_Semaphore::vrpn_Semaphore: error opening semaphore");
        return false;
    }
    return true;
}
#else

bool vrpn_Semaphore::init()
{
    // Posix threads are the default.
    // We use sem_init on linux (instead of sem_open).
    int numMax = cResources;
    if (numMax < 1) {
        numMax = 1;
    }
    try { semaphore = new sem_t; }
    catch (...) { return false; }
    if (sem_init(semaphore, 0, numMax) != 0) {
        perror("vrpn_Semaphore::vrpn_Semaphore: error initializing semaphore");
        return false;
    }
    return true;
}
#endif

#ifdef sgi
bool vrpn_Semaphore::destroy()
{
    if (fUsingLock) {
        usfreelock(l, vrpn_Semaphore::ppaArena);
    }
    else {
        usfreesema(ps, vrpn_Semaphore::ppaArena);
    }
    return true;
}
#elif defined(_WIN32)
bool vrpn_Semaphore::destroy()
{
    if (!CloseHandle(hSemaphore)) {
        // get error info from windows (from FormatMessage help page)
        LPVOID lpMsgBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // Default language
            (LPTSTR)&lpMsgBuf, 0, NULL);
        fprintf(stderr,
            "vrpn_Semaphore::destroy: error destroying semaphore, "
            "WIN32 CloseHandle call caused the following error: %s\n",
            (LPTSTR)lpMsgBuf);
        // Free the buffer.
        LocalFree(lpMsgBuf);
        return false;
    }
    return true;
 }
#else
bool vrpn_Semaphore::destroy()
{
    // Posix threads are the default.
#ifdef __APPLE__
    if (sem_close(semaphore) != 0) {
        perror("vrpn_Semaphore::destroy: error destroying semaphore.");
        return false;
    }
#else
    if (sem_destroy(semaphore) != 0) {
        fprintf(stderr,
            "vrpn_Semaphore::destroy: error destroying semaphore.\n");
        return false;
    }
    try {
      delete semaphore;
    } catch (...) {
      fprintf(stderr, "vrpn_Semaphore::destroy(): delete failed\n");
      return false;
    }
#endif
    semaphore = NULL;
    return true;
}
#endif

vrpn_Semaphore::~vrpn_Semaphore()
{
    if (!destroy()) {
        fprintf(
            stderr,
            "vrpn_Semaphore::~vrpn_Semaphore: error destroying semaphore.\n");
    }
}

// routine to reset it
bool vrpn_Semaphore::reset(int cNumResources)
{
    cResources = cNumResources;

    // Destroy the old semaphore and then create a new one with the correct
    // value.
    if (!destroy()) {
        fprintf(stderr, "vrpn_Semaphore::reset: error destroying semaphore.\n");
        return false;
    }
    if (!init()) {
        fprintf(stderr,
            "vrpn_Semaphore::reset: error initializing semaphore.\n");
        return false;
    }
    return true;
}

// routines to use it (p blocks, cond p does not)
// 1 on success, -1 fail
int vrpn_Semaphore::p()
{
#ifdef sgi
    if (fUsingLock) {
        if (ussetlock(l) != 1) {
            perror("vrpn_Semaphore::p: ussetlock:");
            return -1;
        }
    }
    else {
        if (uspsema(ps) != 1) {
            perror("vrpn_Semaphore::p: uspsema:");
            return -1;
        }
    }
#elif defined(_WIN32)
    switch (WaitForSingleObject(hSemaphore, INFINITE)) {
    case WAIT_OBJECT_0:
        // got the resource
        break;
    case WAIT_TIMEOUT:
        ALL_ASSERT(0, "vrpn_Semaphore::p: infinite wait time timed out!");
        return -1;
        break;
    case WAIT_ABANDONED:
        ALL_ASSERT(0, "vrpn_Semaphore::p: thread holding resource died");
        return -1;
        break;
    case WAIT_FAILED:
        // get error info from windows (from FormatMessage help page)
        LPVOID lpMsgBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // Default language
            (LPTSTR)&lpMsgBuf, 0, NULL);
        fprintf(stderr,
            "vrpn_Semaphore::p: error waiting for resource, "
            "WIN32 WaitForSingleObject call caused the following error: %s",
            (LPTSTR)lpMsgBuf);
        // Free the buffer.
        LocalFree(lpMsgBuf);
        return -1;
        break;
    default:
        ALL_ASSERT(0, "vrpn_Semaphore::p: unknown return code");
        return -1;
    }
#else
    // Posix by default
    if (sem_wait(semaphore) != 0) {
        perror("vrpn_Semaphore::p: ");
        return -1;
    }
#endif
    return 1;
}

// 0 on success, -1 fail
int vrpn_Semaphore::v()
{
#ifdef sgi
    if (fUsingLock) {
        if (usunsetlock(l)) {
            perror("vrpn_Semaphore::v: usunsetlock:");
            return -1;
        }
    }
    else {
        if (usvsema(ps)) {
            perror("vrpn_Semaphore::v: uspsema:");
            return -1;
        }
    }
#elif defined(_WIN32)
    if (!ReleaseSemaphore(hSemaphore, 1, NULL)) {
        // get error info from windows (from FormatMessage help page)
        LPVOID lpMsgBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // Default language
            (LPTSTR)&lpMsgBuf, 0, NULL);
        fprintf(stderr,
            "vrpn_Semaphore::v: error v'ing semaphore, "
            "WIN32 ReleaseSemaphore call caused the following error: %s",
            (LPTSTR)lpMsgBuf);
        // Free the buffer.
        LocalFree(lpMsgBuf);
        return -1;
    }
#else
    // Posix by default
    if (sem_post(semaphore) != 0) {
        perror("vrpn_Semaphore::p: ");
        return -1;
    }
#endif
    return 0;
}

// 0 if it can't get the resource, 1 if it can
// -1 if fail
int vrpn_Semaphore::condP()
{
    int iRetVal = 1;
#ifdef sgi
    if (fUsingLock) {
        // don't spin at all
        iRetVal = uscsetlock(l, 0);
        if (iRetVal <= 0) {
            perror("vrpn_Semaphore::condP: uscsetlock:");
            return -1;
        }
    }
    else {
        iRetVal = uscpsema(ps);
        if (iRetVal <= 0) {
            perror("vrpn_Semaphore::condP: uscpsema:");
            return -1;
        }
    }
#elif defined(_WIN32)
    switch (WaitForSingleObject(hSemaphore, 0)) {
    case WAIT_OBJECT_0:
        // got the resource
        break;
    case WAIT_TIMEOUT:
        // resource not free
        iRetVal = 0;
        break;
    case WAIT_ABANDONED:
        ALL_ASSERT(0, "vrpn_Semaphore::condP: thread holding resource died");
        return -1;
        break;
    case WAIT_FAILED:
        // get error info from windows (from FormatMessage help page)
        LPVOID lpMsgBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // Default language
            (LPTSTR)&lpMsgBuf, 0, NULL);
        fprintf(stderr,
            "Semaphore::condP: error waiting for resource, "
            "WIN32 WaitForSingleObject call caused the following error: %s",
            (LPTSTR)lpMsgBuf);
        // Free the buffer.
        LocalFree(lpMsgBuf);
        return -1;
        break;
    default:
        ALL_ASSERT(0, "vrpn_Semaphore::p: unknown return code");
        return -1;
    }
#else
    // Posix by default
    iRetVal = sem_trywait(semaphore);
    if (iRetVal == 0) {
        iRetVal = 1;
    }
    else if (errno == EAGAIN) {
        iRetVal = 0;
    }
    else {
        perror("vrpn_Semaphore::condP: ");
        iRetVal = -1;
    }
#endif
    return iRetVal;
}

int vrpn_Semaphore::numResources() { return cResources; }

// static var definition
#ifdef sgi
usptr_t *vrpn_Semaphore::ppaArena = NULL;

#include <sys/stat.h>
// for umask stuff
#include <sys/types.h>
#include <sys/stat.h>

void vrpn_Semaphore::allocArena()
{
    // /dev/zero is a special file which can only be shared between
    // processes/threads which share file descriptors.
    // It never shows up in the file system.
    if ((ppaArena = usinit("/dev/zero")) == NULL) {
        perror("vrpn_Thread::allocArena: usinit:");
    }
}
#endif

namespace vrpn {
    SemaphoreGuard::SemaphoreGuard(vrpn_Semaphore &sem)
        : locked_(false)
        , sem_(sem)
    {
        lock();
    }

    // @brief overload that only tries to lock - doesn't block.
    SemaphoreGuard::SemaphoreGuard(vrpn_Semaphore &sem, try_to_lock_t)
        : locked_(false)
        , sem_(sem)
    {
        try_to_lock();
    }

    /// @brief Destructor that unlocks if we've locked.
    SemaphoreGuard::~SemaphoreGuard() { unlock(); }

    /// @brief Locks the semaphore, if we haven't locked it already.
    void SemaphoreGuard::lock()
    {
        if (locked_) {
            return;
        }
        int result = sem_.p();
        handleLockResult_(result);
    }

    /// @brief Tries to lock - returns true if we locked it.
    bool SemaphoreGuard::try_to_lock()
    {
        if (locked_) {
            return true;
        }
        int result = sem_.condP();
        handleLockResult_(result);
        return locked_;
    }

    /// @brief Unlocks the resource, if we have locked it.
    void SemaphoreGuard::unlock()
    {
        if (locked_) {
            int result = sem_.v();
            ALL_ASSERT(result == 0, "failed to unlock semaphore!");
            locked_ = false;
        }
    }
    void SemaphoreGuard::handleLockResult_(int result)
    {
        ALL_ASSERT(result >= 0, "Lock error!");
        if (result == 1) {
            locked_ = true;
        }
    }

} // namespace vrpn

vrpn_Thread::vrpn_Thread(void(*pfThreadparm)(vrpn_ThreadData &ThreadData),
    vrpn_ThreadData tdparm)
    : pfThread(pfThreadparm)
    , td(tdparm)
    , threadID(0)
{
}

bool vrpn_Thread::go()
{
    if (threadID != 0) {
        fprintf(stderr, "vrpn_Thread::go: already running\n");
        return false;
    }

#ifdef sgi
    if ((threadID = sproc(&threadFuncShell, PR_SALL, this)) ==
        ((unsigned long)-1)) {
        perror("vrpn_Thread::go: sproc");
        return false;
    }
    // Threads not defined for the CYGWIN environment yet...
#elif defined(_WIN32) && !defined(__CYGWIN__)
    // pass in func, let it pick stack size, and arg to pass to thread
    if ((threadID = _beginthread(&threadFuncShell, 0, this)) ==
        ((unsigned long)-1)) {
        perror("vrpn_Thread::go: _beginthread");
        return false;
    }
#else
    // Pthreads by default
    if (pthread_create(&threadID, NULL, &threadFuncShellPosix, this) != 0) {
        perror("vrpn_Thread::go:pthread_create: ");
        return false;
    }
#endif
    return true;
}

bool vrpn_Thread::kill()
{
    // kill the os thread
#if defined(sgi) || defined(_WIN32)
    if (threadID > 0) {
#ifdef sgi
        if (::kill((long)threadID, SIGKILL) < 0) {
            perror("vrpn_Thread::kill: kill:");
            return false;
        }
#elif defined(_WIN32)
        // Return value of -1 passed to TerminateThread causes a warning.
        if (!TerminateThread((HANDLE)threadID, 1)) {
            fprintf(stderr,
                "vrpn_Thread::kill: problem with terminateThread call.\n");
            return false;
        }
#endif
#else
    if (threadID) {
        // Posix by default.  Detach so that the thread's resources will be
        // freed automatically when it is killed.
        if (pthread_detach(threadID) != 0) {
            perror("vrpn_Thread::kill:pthread_detach: ");
            return false;
        }
        if (pthread_kill(threadID, SIGKILL) != 0) {
            perror("vrpn_Thread::kill:pthread_kill: ");
            return false;
        }
#endif
    }
    else {
        fprintf(stderr, "vrpn_Thread::kill: thread is not currently alive.\n");
        return false;
    }
    threadID = 0;
    return true;
    }

bool vrpn_Thread::running() { return threadID != 0; }

vrpn_Thread::thread_t vrpn_Thread::pid() { return threadID; }

bool vrpn_Thread::available()
{
#ifdef vrpn_THREADS_AVAILABLE
    return true;
#else
    return false;
#endif
}

void vrpn_Thread::userData(void *pvNewUserData) { td.pvUD = pvNewUserData; }

void *vrpn_Thread::userData() { return td.pvUD; }

void vrpn_Thread::threadFuncShell(void *pvThread)
{
    vrpn_Thread *pth = static_cast<vrpn_Thread *>(pvThread);
    pth->pfThread(pth->td);
    // thread has stopped running
#if !defined(sgi) && !defined(_WIN32)
    // Pthreads; need to detach the thread so its resources will be freed.
    if (pthread_detach(pth->threadID) != 0) {
        perror("vrpn_Thread::threadFuncShell:pthread_detach: ");
    }
#endif
    pth->threadID = 0;
}

// This is a Posix-compatible function prototype that
// just calls the other function.
void *vrpn_Thread::threadFuncShellPosix(void *pvThread)
{
    threadFuncShell(pvThread);
    return NULL;
}

vrpn_Thread::~vrpn_Thread()
{
    if (running()) {
        kill();
    }
}

// For the code to get the number of processor cores.
#ifdef __APPLE__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

unsigned vrpn_Thread::number_of_processors()
{
#ifdef _WIN32
    // Copy the hardware information to the SYSTEM_INFO structure.
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    return siSysInfo.dwNumberOfProcessors;
#elif linux
    // For Linux, we look at the /proc/cpuinfo file and count up the number
    // of "processor	:" entries (tab between processor and colon) in
    // the file to find out how many we have.
    FILE *f = fopen("/proc/cpuinfo", "r");
    int count = 0;
    if (f == NULL) {
        perror("vrpn_Thread::number_of_processors:fopen: ");
        return 1;
    }

    char line[512];
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "processor\t:", strlen("processor\t:")) == 0) {
            count++;
        }
    }

    fclose(f);
    if (count == 0) {
        fprintf(stderr,
            "vrpn_Thread::number_of_processors: Found zero, returning 1\n");
        count = 1;
    }
    return count;

#elif __APPLE__
    int count;
    size_t size = sizeof(count);
    if (sysctlbyname("hw.ncpu", &count, &size, NULL, 0)) {
        return 1;
    }
    else {
        return static_cast<unsigned>(count);
    }

#else
    fprintf(stderr, "vrpn_Thread::number_of_processors: Not yet implemented on "
        "this architecture.\n");
    return 1;
#endif
}

// Thread function to call from within vrpn_test_threads_and_semaphores().
// In this case, the userdata pointer is a pointer to a semaphore that
// the thread should call v() on so that it will free up the main program
// thread.
static void vrpn_test_thread_body(vrpn_ThreadData &threadData)
{
    if (threadData.pvUD == NULL) {
        fprintf(stderr, "vrpn_test_thread_body(): pvUD is NULL\n");
        return;
    }
    vrpn_Semaphore *s = static_cast<vrpn_Semaphore *>(threadData.pvUD);
    s->v();

    return;
}

bool vrpn_test_threads_and_semaphores(void)
{
    //------------------------------------------------------------
    // Make a semaphore to test in single-threaded mode.  First run its count
    // all the way
    // down to zero, then bring it back to the full complement and then bring it
    // down
    // again.  Check that all of the semaphores are available and also that
    // there are no
    // more than expected available.
    const unsigned sem_count = 5;
    vrpn_Semaphore s(sem_count);
    unsigned i;
    for (i = 0; i < sem_count; i++) {
        if (s.condP() != 1) {
            fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore ran "
                "out of counts\n");
            return false;
        }
    }
    if (s.condP() != 0) {
        fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore had too "
            "many counts\n");
        return false;
    }
    for (i = 0; i < sem_count; i++) {
        if (s.v() != 0) {
            fprintf(stderr, "vrpn_test_threads_and_semaphores(): Could not "
                "release Semaphore\n");
            return false;
        }
    }
    for (i = 0; i < sem_count; i++) {
        if (s.condP() != 1) {
            fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore ran "
                "out of counts, round 2\n");
            return false;
        }
    }
    if (s.condP() != 0) {
        fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore had too "
            "many counts, round 2\n");
        return false;
    }

    //------------------------------------------------------------
    // Get a semaphore and use it to construct a thread data structure and then
    // a thread.  Use that thread to test whether threading is enabled (if not,
    // then
    // this completes our testing) and to find out how many processors there
    // are.
    vrpn_ThreadData td;
    td.pvUD = NULL;
    vrpn_Thread t(vrpn_test_thread_body, td);

    // If threading is not enabled, then we're done.
    if (!t.available()) {
        return true;
    }

    // Find out how many processors we have.
    unsigned num_procs = t.number_of_processors();
    if (num_procs == 0) {
        fprintf(stderr, "vrpn_test_threads_and_semaphores(): "
            "vrpn_Thread::number_of_processors() returned zero\n");
        return false;
    }

    //------------------------------------------------------------
    // Now make sure that we can actually run a thread.  Do this by
    // creating a semaphore with one entry and calling p() on it.
    // Then make sure we can't p() it again and then run a thread
    // that will call v() on it when it runs.
    vrpn_Semaphore sem;
    if (sem.p() != 1) {
        fprintf(stderr, "vrpn_test_threads_and_semaphores(): thread-test "
            "Semaphore had no count\n");
        return false;
    }
    if (sem.condP() != 0) {
        fprintf(stderr, "vrpn_test_threads_and_semaphores(): thread-test "
            "Semaphore had too many counts\n");
        return false;
    }
    t.userData(&sem);
    if (!t.go()) {
        fprintf(stderr,
            "vrpn_test_threads_and_semaphores(): Could not start thread\n");
        return false;
    }
    struct timeval start;
    struct timeval now;
    vrpn_gettimeofday(&start, NULL);
    while (true) {
        if (sem.condP() == 1) {
            // The thread must have run; we got the semaphore!
            break;
        }

        // Time out after three seconds if we haven't had the thread run to
        // reset the semaphore.
        vrpn_gettimeofday(&now, NULL);
        struct timeval diff = vrpn_TimevalDiff(now, start);
        if (diff.tv_sec >= 3) {
            fprintf(stderr,
                "vrpn_test_threads_and_semaphores(): Thread didn't run\n");
            return false;
        }

        vrpn_SleepMsecs(1);
    }

    return true;
}
