#ifndef VRPN_SHARED_H
#define VRPN_SHARED_H

#ifdef _WIN32
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
#define close closesocket
#else
#include <sys/time.h>
#endif

extern struct timeval vrpn_TimevalSum( const struct timeval& tv1, 
				  const struct timeval& tv2 );
extern struct timeval vrpn_TimevalDiff( const struct timeval& tv1, 
				   const struct timeval& tv2 );
extern double vrpn_TimevalMsecs( const struct timeval& tv1 );

extern void vrpn_SleepMsecs( double dMsecs );

// xform a double to/from network order -- like htonl and htons
extern double htond( double d );
extern double ntohd( double d );


#endif

