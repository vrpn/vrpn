#ifndef VRPN_SHARED_H
#define VRPN_SHARED_H

#ifdef _WIN32
#include <sys/timeb.h>
#include <winsock.h>   // timeval is defined here

  /* from HP-UX */
struct timezone {
    int     tz_minuteswest; /* minutes west of Greenwich */
    int     tz_dsttime;     /* type of dst correction */
};

extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
#define close closesocket
#else
#include <sys/time.h>
#endif

extern struct timeval timevalSum( const struct timeval& tv1, 
				  const struct timeval& tv2 );
extern struct timeval timevalDiff( const struct timeval& tv1, 
				   const struct timeval& tv2 );
extern double timevalMsecs( const struct timeval& tv1 );

#endif

