#ifndef VRPN_SHARED_H
#define VRPN_SHARED_H

#include "vrpn_Types.h"

// {{{ timeval defines
#if defined (_WIN32) && !defined (__CYGWIN__)

// {{{ win32 specific stuff

#include <windows.h>
#include <sys/timeb.h>
#include <winsock.h>   // timeval is defined here


// If compiling under Cygnus Solutions Cygwin
// then this should already be defined.
#ifndef _STRUCT_TIMEVAL
#define _STRUCT_TIMEVAL
  /* from HP-UX */
struct timezone {
    int     tz_minuteswest; /* minutes west of Greenwich */
    int     tz_dsttime;     /* type of dst correction */
};
#endif

extern int gettimeofday(struct timeval *tp, struct timezone *tzp);

// This has been moved to connection.C so that users of this
// lib can still use fstream and other objects with close functions.
// #define close closesocket

#else  // not _WIN32, but including CYGWIN

#if defined(__CYGWIN__)
#include <windows.h>
#include <sys/timeb.h>
#include <winsock.h>   // timeval is defined here
#endif
// }}} end win32 specific

#include <sys/time.h>    // for struct timeval

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

extern long vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_int32 value);
extern long vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_float32 value);
extern long vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_float64 value);
extern long vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const timeval t);
extern long vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const char * string, vrpn_uint32 length);
extern long vrpn_unbuffer (const char ** buffer, vrpn_int32 * lval);
extern long vrpn_unbuffer (const char ** buffer, vrpn_float32 * fval);
extern long vrpn_unbuffer (const char ** buffer, vrpn_float64 * dval);
extern long vrpn_unbuffer (const char ** buffer, timeval * t);
extern long vrpn_unbuffer (const char ** buffer, char * string,
                           vrpn_uint32 length);
// }}}

#ifdef	_WIN32	// No sleep() function, but Sleep(DWORD) defined in winbase.h
#define	sleep(x)	Sleep( DWORD(1000.0 * x) )
#endif

#endif  // VRPN_SHARED_H

