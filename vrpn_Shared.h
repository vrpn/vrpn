#ifndef VRPN_SHARED_H
#define VRPN_SHARED_H

#include "vrpn_Types.h"

// jan 2000: jeff changing the way sockets are used with cygwin.  I made this
// change because I realized that we were using winsock stuff in some places,
// and cygwin stuff in others.  Discovered this when our code wouldn't compile
// in cygwin-1.0 (but it did in cygwin-b20.1).

// let's start with a clean slate
#undef VRPN_USE_WINSOCK_SOCKETS

// Does cygwin use winsock sockets or unix sockets
#define VRPN_CYGWIN_USES_WINSOCK_SOCKETS

#if defined(_WIN32) \
    && (!defined(__CYGWIN__) || defined(VRPN_CYGWIN_USES_WINSOCK_SOCKETS))
#define VRPN_USE_WINSOCK_SOCKETS
#endif

#ifndef	VRPN_USE_WINSOCK_SOCKETS
// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#define	INVALID_SOCKET	-1
#define	SOCKET		int
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

// {{{ timeval defines
#if (!defined(VRPN_USE_WINSOCK_SOCKETS))
#include <sys/time.h>    // for timeval, timezone, gettimeofday
#else  // winsock sockets

  #include <windows.h>
  #include <sys/timeb.h>
  #include <winsock.h>   // timeval is defined here

  // If compiling under Cygnus Solutions Cygwin then these get defined by
  // including sys/time.h.  So, we will manually define only for _WIN32

  #ifndef _STRUCT_TIMEVAL
    #define _STRUCT_TIMEVAL
    /* from HP-UX */
    struct timezone {
        int     tz_minuteswest; /* minutes west of Greenwich */
        int     tz_dsttime;     /* type of dst correction */
    };
  #endif

  // manually define this too.  _WIN32 sans cygwin doesn't have gettimeofday
  extern "C" int gettimeofday(struct timeval *tp, struct timezone *tzp);

#endif
// }}} timeval defines

// {{{ vrpn_* timeval utility functions
//     --------------------------------

// IMPORTANT: timevals must be normalized to make any sense
//
//  * normalized means abs(tv_usec) is less than 1,000,000
//
//  * TimevalSum and TimevalDiff do not do the right thing if
//    their inputs are not normalized
//
//  * TimevalScale now normalizes it's results [9/1999 it didn't before]
//
//  * added a function vrpn_TimevalNormalize


// make sure tv_usec is less than 1,000,000
timeval vrpn_TimevalNormalize( const timeval & );

extern struct timeval vrpn_TimevalSum( const struct timeval& tv1, 
				  const struct timeval& tv2 );
extern struct timeval vrpn_TimevalDiff( const struct timeval& tv1, 
				   const struct timeval& tv2 );
extern struct timeval vrpn_TimevalScale (const struct timeval & tv,
                                         double scale);
extern int vrpn_TimevalGreater (const struct timeval & tv1,
                                const struct timeval & tv2);
extern double vrpn_TimevalMsecs( const struct timeval& tv1 );

extern struct timeval vrpn_MsecsTimeval( const double dMsecs );

extern void vrpn_SleepMsecs( double dMsecs );

// }}}
// {{{ vrpn_* buffer util functions
//     ----------------------------

// xform a double to/from network order -- like htonl and htons
extern vrpn_float64 htond( vrpn_float64 d );
extern vrpn_float64 ntohd( vrpn_float64 d );

extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_int8 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_int32 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_uint32 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_float32 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_float64 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const timeval t);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const char * string, vrpn_int32 length);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_bool value);
extern int vrpn_unbuffer (const char ** buffer, vrpn_int8 * cval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_int32 * lval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_uint32 * lval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_float32 * fval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_float64 * dval);
extern int vrpn_unbuffer (const char ** buffer, timeval * t);
extern int vrpn_unbuffer (const char ** buffer, char * string,
                           vrpn_int32 length);
extern int vrpn_unbuffer (const char ** buffer, vrpn_bool * lval);
// }}}

// XXX should this be done in cygwin?
#ifdef	_WIN32	// No sleep() function, but Sleep(DWORD) defined in winbase.h
#define	sleep(x)	Sleep( DWORD(1000.0 * x) )
#endif

#endif  // VRPN_SHARED_H
