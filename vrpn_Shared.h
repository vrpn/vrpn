#ifndef VRPN_SHARED_H
#define VRPN_SHARED_H

//------------------------------------------------------------------
//   This section contains definitions for architecture-dependent
// types.  It is important that the data sent over a vrpn_Connection
// be of the same size on all hosts sending and receiving it.  Since
// C++ does not constrain the size of 'int', 'long', 'double' and
// so forth, we create new types here that are defined correctly for
// each architecture and use them for all data that might be sent
// across a connection.
//   Part of porting VRPN to a new architecture is defining the
// types below on that architecture in such as way that the compiler
// can determine which machine type it is on.
//------------------------------------------------------------------

#ifdef	sgi
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	hpux
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

// For PixelFlow aCC compiler
#ifdef	__hpux
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	sparc
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	linux
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

// _WIN32 is defined for all compilers for Windows (Cygnus G++ included)
// WIN32 is defined only by the Windows VC compiler
#ifdef	_WIN32
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	long
#define	vrpn_uint32	unsigned long
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifndef	vrpn_int8
XXX	Need to define architecture-dependent sizes here
#endif

//------------------------------------------------------------------
//------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
#include <sys/timeb.h>
#include <winsock.h>   // timeval is defined here

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

#else

#include <sys/time.h>

#endif  // not _WIN32

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

// xform a double to/from network order -- like htonl and htons
extern vrpn_float64 htond( vrpn_float64 d );
extern vrpn_float64 ntohd( vrpn_float64 d );


#ifdef	_WIN32	// No sleep() function ?!?!?!?!?
#define	sleep(x)	vrpn_SleepMsecs(1000.0 * x)
#endif

#endif  // VRPN_SHARED_H

