#pragma once

// Horrible hack for old HPUX compiler
#ifdef hpux
#ifndef true
#define bool int
#define true 1
#define false 0
#endif
#endif

#include "vrpn_Configure.h" // for VRPN_API
#include "vrpn_Types.h"     // for vrpn_int32, vrpn_float64, etc
#include <string.h>         // for memcpy()
#include <stdio.h>          // for fprintf()

#if defined(__ANDROID__)
#include <bitset>
#endif

// IWYU pragma: no_include <bits/time.h>

// Oct 2000: Sang-Uok changed because vrpn code was compiling but giving
// runtime errors with cygwin 1.1. I changed the code so it only uses unix
// code. I had to change includes in various files.

// jan 2000: jeff changing the way sockets are used with cygwin.  I made this
// change because I realized that we were using winsock stuff in some places,
// and cygwin stuff in others.  Discovered this when our code wouldn't compile
// in cygwin-1.0 (but it did in cygwin-b20.1).

// let's start with a clean slate
#undef VRPN_USE_WINSOCK_SOCKETS

// Does cygwin use winsock sockets or unix sockets
//#define VRPN_CYGWIN_USES_WINSOCK_SOCKETS

#if defined(_WIN32) &&                                                         \
    (!defined(__CYGWIN__) || defined(VRPN_CYGWIN_USES_WINSOCK_SOCKETS))
#define VRPN_USE_WINSOCK_SOCKETS
#endif

#ifndef VRPN_USE_WINSOCK_SOCKETS
// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#define INVALID_SOCKET -1
#define SOCKET int
#endif

#if !(defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS))
#include <sys/select.h> // for select
#include <netinet/in.h> // for htonl, htons
#endif

#ifdef _WIN32_WCE
#define perror(x) fprintf(stderr, "%s\n", x);
#endif

// comment from vrpn_Connection.h reads :
//
//   gethostbyname() fails on SOME Windows NT boxes, but not all,
//   if given an IP octet string rather than a true name.
//   Until we figure out WHY, we have this extra clause in here.
//   It probably wouldn't hurt to enable it for non-NT systems
//   as well.
#ifdef _WIN32
#define VRPN_USE_WINDOWS_GETHOSTBYNAME_HACK
#endif

//--------------------------------------------------------------
// Timeval defines.  These are a bit hairy.  The basic problem is
// that Windows doesn't implement gettimeofday(), nor does it
// define "struct timezone", although Winsock.h does define
// "struct timeval".  The painful solution has been to define a
// vrpn_gettimeofday() function that takes a void * as a second
// argument (the timezone) and have all VRPN code call this function
// rather than gettimeofday().  On non-WINSOCK implementations,
// we alias vrpn_gettimeofday() right back to gettimeofday(), so
// that we are calling the system routine.  On Windows, we will
// be using vrpn_gettimofday().  So far so good, but now user code
// would like to not have to know the difference under windows, so
// we have an optional VRPN configuration setting in vrpn_Configure.h
// that exports vrpn_gettimeofday() as gettimeofday() and also
// exports a "struct timezone" definition.  Yucky, but it works and
// lets user code use the VRPN one as if it were the system call
// on Windows.

#if (!defined(VRPN_USE_WINSOCK_SOCKETS))
#include <sys/time.h> // for timeval, timezone, gettimeofday
#define vrpn_gettimeofday gettimeofday
#else // winsock sockets

// These are a pair of horrible hacks that instruct Windows include
// files to (1) not define min() and max() in a way that messes up
// standard-library calls to them, and (2) avoids pulling in a large
// number of Windows header files.  They are not used directly within
// the VRPN library, but rather within the Windows include files to
// change the way they behave.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef _WIN32_WCE
#include <sys/timeb.h>
#endif
#ifdef VRPN_USE_WINSOCK2
#include <winsock2.h> // struct timeval is defined here
#else
#include <winsock.h> // struct timeval is defined here
#endif

// Whether or not we export gettimeofday, we declare the
// vrpn_gettimeofday() function.
extern "C" VRPN_API int vrpn_gettimeofday(struct timeval *tp, void *tzp);

// If compiling under Cygnus Solutions Cygwin then these get defined by
// including sys/time.h.  So, we will manually define only for _WIN32
// Only do this if the Configure file has set VRPN_EXPORT_GETTIMEOFDAY,
// so that application code can get at it.  All VRPN routines should be
// calling vrpn_gettimeofday() directly.

#if defined(VRPN_EXPORT_GETTIMEOFDAY) && !defined(_STRUCT_TIMEZONE) &&         \
    !defined(_TIMEZONE_DEFINED)
#define _TIMEZONE_DEFINED
/* from HP-UX */
struct timezone {
    int tz_minuteswest; /* minutes west of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};
#endif
#if defined(VRPN_EXPORT_GETTIMEOFDAY) && !defined(_STRUCT_TIMEZONE)
#define _STRUCT_TIMEZONE

// manually define this too.  _WIN32 sans cygwin doesn't have gettimeofday
#define gettimeofday vrpn_gettimeofday

#endif
#endif

//--------------------------------------------------------------
// vrpn_* timeval utility functions

// IMPORTANT: timevals must be normalized to make any sense
//
//  * normalized means abs(tv_usec) is less than 1,000,000
//
//  * TimevalSum and TimevalDiff do not do the right thing if
//    their inputs are not normalized
//
//  * TimevalScale now normalizes it's results [9/1999 it didn't before]

// make sure tv_usec is less than 1,000,000
extern VRPN_API struct timeval vrpn_TimevalNormalize(const struct timeval &tv);

extern VRPN_API struct timeval vrpn_TimevalSum(const struct timeval &tv1,
                                               const struct timeval &tv2);
extern VRPN_API struct timeval vrpn_TimevalDiff(const struct timeval &tv1,
                                                const struct timeval &tv2);
extern VRPN_API struct timeval vrpn_TimevalScale(const struct timeval &tv,
                                                 double scale);

/// @brief Return number of microseconds between startT and endT.
extern VRPN_API unsigned long vrpn_TimevalDuration(struct timeval endT,
                                                   struct timeval startT);

/// @brief Return the number of seconds between startT and endT as a
/// floating-point value.
extern VRPN_API double vrpn_TimevalDurationSeconds(struct timeval endT,
                                                   struct timeval startT);

extern VRPN_API bool vrpn_TimevalGreater(const struct timeval &tv1,
                                         const struct timeval &tv2);
extern VRPN_API bool vrpn_TimevalEqual(const struct timeval &tv1,
                                       const struct timeval &tv2);

extern VRPN_API double vrpn_TimevalMsecs(const struct timeval &tv1);

extern VRPN_API struct timeval vrpn_MsecsTimeval(const double dMsecs);
extern VRPN_API void vrpn_SleepMsecs(double dMsecs);

//--------------------------------------------------------------
// vrpn_* buffer util functions and endian-ness related
// definitions and functions.

// xform a double to/from network order -- like htonl and htons
extern VRPN_API vrpn_float64 htond(vrpn_float64 d);
extern VRPN_API vrpn_float64 ntohd(vrpn_float64 d);

// From this we get the variable "vrpn_big_endian" set to true if the machine we
// are
// on is big endian and to false if it is little endian.  This can be used by
// custom packing and unpacking code to bypass the buffer and unbuffer routines
// for cases that have to be particularly fast (like video data).  It is also
// used
// internally by the htond() function.

static const int vrpn_int_data_for_endian_test = 1;
static const char *vrpn_char_data_for_endian_test =
    (char *)(void *)(&vrpn_int_data_for_endian_test);
static const bool vrpn_big_endian = (vrpn_char_data_for_endian_test[0] != 1);

// Read and write strings (not single items).
extern VRPN_API int vrpn_buffer(char **insertPt, vrpn_int32 *buflen,
                                const char *string, vrpn_int32 length);
extern VRPN_API int vrpn_unbuffer(const char **buffer, char *string,
                                  vrpn_int32 length);

// Read and write timeval.
extern VRPN_API int vrpn_unbuffer(const char **buffer, timeval *t);
extern VRPN_API int vrpn_buffer(char **insertPt, vrpn_int32 *buflen,
                                const timeval t);

// To read and write the atomic types defined in vrpn_Types, you use the
// templated
// buffer and unbuffer routines below.  These have the same form as the ones for
// timeval, but they use types vrpn_int, vrpn_uint, vrpn_int16, vrpn_uint16,
// vrpn_int32, vrpn_uint32, vrpn_float32, and vrpn_float64.

/**
    @brief Internal header providing unbuffering facilities for a number of
   types.

    @date 2011

    @author
    Ryan Pavlik
    <rpavlik@iastate.edu> and <abiryan@ryand.net>
    http://academic.cleardefinition.com/
    Iowa State University Virtual Reality Applications Center
    Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Tested in the context of vrpn_server and vrpn_print_devices running between
// an SGI running Irix 6.5 MIPS 32-bit (big endian) and Mac OSX intel 64-bit
// (little endian) machine with a NULL tracker and it worked using the SGI
// repaired commits from 3/17/2012.

/// @brief Contains overloaded hton() and ntoh() functions that forward
/// to their correctly-typed implementations.
namespace vrpn_byte_order {
    namespace vrpn_detail {
        /// Traits class to get the uint type of a given size
        template <int TypeSize> struct uint_traits;

        template <> struct uint_traits<1> {
            typedef vrpn_uint8 type;
        };
        template <> struct uint_traits<2> {
            typedef vrpn_uint16 type;
        };
        template <> struct uint_traits<4> {
            typedef vrpn_uint32 type;
        };
    } // end of namespace vrpn_detail

    /// host to network byte order for 8-bit uints is a no-op
    inline vrpn_uint8 hton(vrpn_uint8 hostval) { return hostval; }

    /// network to host byte order for 8-bit uints is a no-op
    inline vrpn_uint8 ntoh(vrpn_uint8 netval) { return netval; }

    /// host to network byte order for 16-bit uints
    inline vrpn_uint16 hton(vrpn_uint16 hostval) { return htons(hostval); }

    /// network to host byte order for 16-bit uints
    inline vrpn_uint16 ntoh(vrpn_uint16 netval) { return ntohs(netval); }

    /// host to network byte order for 32-bit uints
    inline vrpn_uint32 hton(vrpn_uint32 hostval) { return htonl(hostval); }

    /// network to host byte order for 32-bit uints
    inline vrpn_uint32 ntoh(vrpn_uint32 netval) { return ntohl(netval); }

    /// host to network byte order for 64-bit floats, using vrpn htond
    inline vrpn_float64 hton(vrpn_float64 hostval) { return htond(hostval); }

    /// network to host byte order for 64-bit floats, using vrpn ntohd
    inline vrpn_float64 ntoh(vrpn_float64 netval) { return ntohd(netval); }

    /// Templated hton that type-puns to the same-sized uint type
    /// as a fallback for those types not explicitly defined above.
    template <typename T> inline T hton(T input)
    {
        union {
            T asInput;
            typename vrpn_detail::uint_traits<sizeof(T)>::type asInt;
        } inVal, outVal;
        inVal.asInput = input;
        outVal.asInt = hton(inVal.asInt);
        return outVal.asInput;
    }

    /// Templated ntoh that type-puns to the same-sized uint type
    /// as a fallback for those types not explicitly defined above.
    template <typename T> inline T ntoh(T input)
    {
        union {
            T asInput;
            typename vrpn_detail::uint_traits<sizeof(T)>::type asInt;
        } inVal, outVal;
        inVal.asInput = input;
        outVal.asInt = ntoh(inVal.asInt);
        return outVal.asInput;
    }
} // end of namespace vrpn_byte_order

namespace vrpn_detail {
    template <typename T> struct remove_const {
        typedef T type;
    };

    template <typename T> struct remove_const<const T> {
        typedef T type;
    };

    template <bool Condition> struct vrpn_static_assert {
    };
    /// @brief Each static assertion needs its message in this enum, or it will
    /// always fail.
    template <> struct vrpn_static_assert<true> {
        enum { SIZE_OF_BUFFER_ITEM_IS_NOT_ONE_BYTE };
    };
} // end of namespace vrpn_detail

#ifdef VRPN_USE_STATIC_ASSERTIONS
/// @brief Static assertion macro for limited sets of messages.
/// Inspired by http://eigen.tuxfamily.org/dox/TopicAssertions.html
#if defined(__GXX_EXPERIMENTAL_CXX0X__) ||                                     \
    (defined(_MSC_VER) && (_MSC_VER >= 1600))
#define VRPN_STATIC_ASSERT(CONDITION, MESSAGE)                                 \
    static_assert(CONDITION, #MESSAGE)
#else
#define VRPN_STATIC_ASSERT(CONDITION, MESSAGE)                                 \
    (void)(::vrpn_detail::vrpn_static_assert<CONDITION>::MESSAGE)
#endif
#else
/// Fall back to normal asserts.
#include <assert.h>
#define VRPN_STATIC_ASSERT(CONDITION, MESSAGE) assert((CONDITION) && #MESSAGE)
#endif

/// Function template to unbuffer values from a buffer stored in little-
/// endian byte order. Specify the type to extract T as a template parameter.
/// The templated buffer type ByteT will be deduced automatically.
/// The input pointer will be advanced past the unbuffered value.
template <typename T, typename ByteT>
static inline T vrpn_unbuffer_from_little_endian(ByteT *&input)
{
    using namespace vrpn_byte_order;

    VRPN_STATIC_ASSERT(sizeof(ByteT) == 1, SIZE_OF_BUFFER_ITEM_IS_NOT_ONE_BYTE);

    /// Union to allow type-punning
    union {
        typename ::vrpn_detail::remove_const<ByteT>::type bytes[sizeof(T)];
        T typed;
    } value;

    /// Swap known little-endian into big-endian (aka network byte order)
    for (unsigned int i = 0, j = sizeof(T) - 1; i < sizeof(T); ++i, --j) {
        value.bytes[i] = input[j];
    }

    /// Advance input pointer
    input += sizeof(T);

    /// return value in host byte order
    return ntoh(value.typed);
}

/// Function template to unbuffer values from a buffer stored in network
/// byte order. Specify the type to extract T as a template parameter.
/// The templated buffer type ByteT will be deduced automatically.
/// The input pointer will be advanced past the unbuffered value.
template <typename T, typename ByteT> inline T vrpn_unbuffer(ByteT *&input)
{
    using namespace vrpn_byte_order;

    VRPN_STATIC_ASSERT(sizeof(ByteT) == 1, SIZE_OF_BUFFER_ITEM_IS_NOT_ONE_BYTE);

    /// Union to allow type-punning and ensure alignment
    union {
        typename ::vrpn_detail::remove_const<ByteT>::type bytes[sizeof(T)];
        T typed;
    } value;

    /// Copy bytes into union
    memcpy(value.bytes, input, sizeof(T));

    /// Advance input pointer
    input += sizeof(T);

    /// return value in host byte order
    return ntoh(value.typed);
}

/// Function template to buffer values to a buffer stored in network
/// byte order. Specify the type to buffer T as a template parameter.
/// The templated buffer type ByteT will be deduced automatically.
/// The input pointer will be advanced past the unbuffered value.
template <typename T, typename ByteT>
inline int vrpn_buffer(ByteT **insertPt, vrpn_int32 *buflen, const T inVal)
{
    using namespace vrpn_byte_order;

    VRPN_STATIC_ASSERT(sizeof(ByteT) == 1, SIZE_OF_BUFFER_ITEM_IS_NOT_ONE_BYTE);

    if ((insertPt == NULL) || (buflen == NULL)) {
        fprintf(stderr, "vrpn_buffer: NULL pointer\n");
        return -1;
    }

    if (sizeof(T) > static_cast<size_t>(*buflen)) {
        fprintf(stderr, "vrpn_buffer: buffer not large enough\n");
        return -1;
    }

    /// Union to allow type-punning and ensure alignment
    union {
        typename ::vrpn_detail::remove_const<ByteT>::type bytes[sizeof(T)];
        T typed;
    } value;

    /// Populate union in network byte order
    value.typed = hton(inVal);

    /// Copy bytes into buffer
    memcpy(*insertPt, value.bytes, sizeof(T));

    /// Advance insert pointer
    *insertPt += sizeof(T);
    /// Decrement buffer length
    *buflen -= sizeof(T);

    return 0;
}

template <typename T, typename ByteT>
inline int vrpn_unbuffer(ByteT **input, T *lvalue)
{
    *lvalue = ::vrpn_unbuffer<T, ByteT>(*input);
    return 0;
}

// Semaphore and Thread classes derived from Hans Weber's classes from UNC.
// Don't let the existence of a Thread class fool you into thinking
// that VRPN is thread-safe.  This and the Semaphore are included as
// building blocks towards making your own code thread-safe.  They are
// here to enable the vrpn_Imager_Logger class to do its thing.

#if defined(sgi) || (defined(_WIN32) && !defined(__CYGWIN__)) ||               \
    defined(linux) || defined(__APPLE__)
#define vrpn_THREADS_AVAILABLE
#else
#undef vrpn_THREADS_AVAILABLE
#endif

// multi process stuff
#ifdef sgi
#include <task.h>
#include <ulocks.h>
#elif defined(_WIN32)
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
    // mutex by default (0 is a sync primitive)
    vrpn_Semaphore(int cNumResources = 1);

    // This does not copy the state of the semaphore, just creates
    // a new one with the same resource count
    vrpn_Semaphore(const vrpn_Semaphore &s);
    ~vrpn_Semaphore();

    // routine to reset it (true on success, false on failure)
    // (may create new semaphore)
    bool reset(int cNumResources = 1);

    // routines to use it (p blocks, condP does not)
    // p returns 1 when it has acquired the resource, -1 on fail
    // v returns 0 when it has released the resource, -1 on fail
    // condP returns 0 if it could not access the resource
    // and 1 if it could (-1 on fail)
    int p();
    int v();
    int condP();

    // read values
    int numResources();

protected:
    // common init and destroy routines
    bool init();
    bool destroy();

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

typedef void (*vrpn_THREAD_FUNC)(vrpn_ThreadData &threadData);

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
    void (*pfThread)(vrpn_ThreadData &ThreadData);
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
