#ifndef VRPN_SHARED_H
#define VRPN_SHARED_H

// Horrible hack for old HPUX compiler
#ifdef	hpux
#ifndef	true
#define	bool	int
#define	true	1
#define	false	0
#endif
#endif

#include "vrpn_Types.h"

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

#if defined(_WIN32) \
    && (!defined(__CYGWIN__) || defined(VRPN_CYGWIN_USES_WINSOCK_SOCKETS))
#define VRPN_USE_WINSOCK_SOCKETS
#endif

#ifndef	VRPN_USE_WINSOCK_SOCKETS
// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#define	INVALID_SOCKET	-1
#define	SOCKET		int
#endif

#ifdef	_WIN32_WCE
#define perror(x) fprintf(stderr,"%s\n",x);
#define	VRPN_NO_STREAMS
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
// argument (the timezone) and have all VPRN code call this function
// rather than gettimeofday().  On non-WINSOCK implementations,
// we alias vrpn_gettimeofday() right back to gettimeofday(), so
// that we are calling the system routine.  On Windows, we will
// be using vrpn_gettimofday().  So far so good, but now user code
// would like to no have to know the difference under windows, so
// we have an optional VRPN configuration setting in vrpn_Configure.h
// that exports vrpn_gettimeofday() as gettimeofday() and also
// exports a "struct timezone" definition.  Yucky, but it works and
// lets user code use the VRPN one as if it were the system call
// on Windows.

#if (!defined(VRPN_USE_WINSOCK_SOCKETS))
#include <sys/time.h>    // for timeval, timezone, gettimeofday
#define vrpn_gettimeofday gettimeofday
#else  // winsock sockets

  #include <windows.h>
#ifndef _WIN32_WCE
  #include <sys/timeb.h>
#endif
  #include <winsock.h>    // struct timeval is defined here

  // Whether or not we export gettimeofday, we declare the
  // vrpn_gettimeofday() function.
  extern "C" int vrpn_gettimeofday(struct timeval *tp, void *tzp);

  // If compiling under Cygnus Solutions Cygwin then these get defined by
  // including sys/time.h.  So, we will manually define only for _WIN32
  // Only do this if the Configure file has set VRPN_EXPORT_GETTIMEOFDAY,
  // so that application code can get at it.  All VRPN routines should be
  // calling vrpn_gettimeofday() directly.

  #ifdef VRPN_EXPORT_GETTIMEOFDAY

    #ifndef _STRUCT_TIMEZONE
      #define _STRUCT_TIMEZONE
      /* from HP-UX */
      struct timezone {
	  int     tz_minuteswest; /* minutes west of Greenwich */
	  int     tz_dsttime;     /* type of dst correction */
      };

      // manually define this too.  _WIN32 sans cygwin doesn't have gettimeofday
      #define gettimeofday  vrpn_gettimeofday
    #endif

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
//
//  * added a function vrpn_TimevalNormalize


// make sure tv_usec is less than 1,000,000
extern struct timeval vrpn_TimevalNormalize( const struct timeval & tv );

extern struct timeval vrpn_TimevalSum( const struct timeval& tv1, 
				  const struct timeval& tv2 );
extern struct timeval vrpn_TimevalDiff( const struct timeval& tv1, 
				   const struct timeval& tv2 );
extern struct timeval vrpn_TimevalScale (const struct timeval & tv,
                                         double scale);
extern int vrpn_TimevalGreater (const struct timeval & tv1,
                                const struct timeval & tv2);
extern int vrpn_TimevalEqual( const struct timeval& tv1,
			      const struct timeval& tv2 );
extern double vrpn_TimevalMsecs( const struct timeval& tv1 );

extern struct timeval vrpn_MsecsTimeval( const double dMsecs );

extern void vrpn_SleepMsecs( double dMsecs );

// Tells whether the maci

//--------------------------------------------------------------
// vrpn_* buffer util functions and endian-ness related
// definitions and functions.

// xform a double to/from network order -- like htonl and htons
extern vrpn_float64 htond( vrpn_float64 d );
extern vrpn_float64 ntohd( vrpn_float64 d );

extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_int8 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_int16 value);
extern int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                         const vrpn_uint16 value);
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

extern int vrpn_unbuffer (const char ** buffer, vrpn_int8 * cval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_int16 * lval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_uint16 * lval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_int32 * lval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_uint32 * lval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_float32 * fval);
extern int vrpn_unbuffer (const char ** buffer, vrpn_float64 * dval);
extern int vrpn_unbuffer (const char ** buffer, timeval * t);
extern int vrpn_unbuffer (const char ** buffer, char * string,
                           vrpn_int32 length);

// From this we get the variable "vrpn_big_endian" set to true if the machine we are
// on is big endian and to false if it is little endian.  This can be used by
// custom packing and unpacking code to bypass the buffer and unbuffer routines
// for cases that have to be particularly fast (like video data).
static	const   int     vrpn_int_data_for_endian_test = 1;
static	const   char    *vrpn_char_data_for_endian_test = (char *)(void *)(&vrpn_int_data_for_endian_test);
static	const   bool    vrpn_big_endian = (vrpn_char_data_for_endian_test[3] == 1);


// XXX should this be done in cygwin?
// No sleep() function, but Sleep(DWORD) defined in winbase.h
#if defined(_WIN32) && (!defined(__CYGWIN__))
#define	sleep(x)	Sleep( DWORD(1000.0 * x) )
#endif

#endif  // VRPN_SHARED_H
