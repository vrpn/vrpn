#include "vrpn_Shared.h"

// Calcs the sum of tv1 and tv2.  Returns the sum in a timeval struct.
// Calcs negative times properly, with the appropriate sign on both tv_sec
// and tv_usec (these signs will match unless one of them is 0).
// NOTE: both abs(tv_usec)'s must be < 1000000 (ie, normal timeval format)
struct timeval timevalSum( const struct timeval& tv1, 
			   const struct timeval& tv2 ) {
  struct timeval tvSum=tv1;

  tvSum.tv_sec += tv2.tv_sec;
  tvSum.tv_usec += tv2.tv_usec;

  // do borrows, etc to get the time the way i want it: both signs the same,
  // and abs(usec) less than 1e6
  if (tvSum.tv_sec>=0) {
    if (tvSum.tv_usec<0) {
      tvSum.tv_sec--;
      tvSum.tv_usec += 1000000;
    } else if (tvSum.tv_usec >= 1000000) {
      tvSum.tv_sec++;
      tvSum.tv_usec -= 1000000;
    }
  } else {
    if (tvSum.tv_usec>0) {
      tvSum.tv_sec++;
      tvSum.tv_usec -= 1000000;
    } else if (tvSum.tv_usec <= -1000000) {
      tvSum.tv_sec--;
      tvSum.tv_usec += 1000000;
    }
  }

  return tvSum;
}


// Calcs the diff between tv1 and tv2.  Returns the diff in a timeval struct.
// Calcs negative times properly, with the appropriate sign on both tv_sec
// and tv_usec (these signs will match unless one of them is 0)
struct timeval timevalDiff( const struct timeval& tv1, 
			    const struct timeval& tv2 ) {
  struct timeval tv;

  tv.tv_sec = -tv2.tv_sec;
  tv.tv_usec = -tv2.tv_usec;

  return timevalSum( tv1, tv );
}

double timevalMsecs( const struct timeval& tv ) {
  return tv.tv_sec*1000 + tv.tv_usec/1000.0;
}

#ifdef _WIN32
#include <iostream.h>

// The pc has no gettimeofday call, and the closest thing to it is _ftime.
// _ftime, however, has only about 6 ms resolution, so we use the peformance 
// as an offset from a base time which is established by a call to by _ftime.

// The first call to gettimeofday will establish a new time frame
// on which all later calls will be based.  This means that the time returned
// by gettimeofday will not always match _ftime (even at _ftime's resolution),
// but it will be consistent across all gettimeofday calls.

int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
  static int fFirst=1;
  static int fHasPerfCounter=1;
  static struct _timeb tbInit;
  static struct timeval tvInit;
  struct timeval tvNow;
  struct timeval tvDiff;
  static LARGE_INTEGER cLIClockFreq;
  static LARGE_INTEGER cLICounter;
  
  if (!fHasPerfCounter) {
    _ftime(&tbInit);
    tp->tv_sec  = tbInit.time;
    tp->tv_usec = tbInit.millitm*1000;
    return 0;
  } 

  if (fFirst) {
    // establish a time base
    fFirst=0;

    // check that hi-perf clock is available
    if ( !(fHasPerfCounter = QueryPerformanceFrequency( &cLIClockFreq )) ) {
      cerr << "\ngettimeofday: no hi performance clock available. " 
	   << "Defaulting to _ftime (~6 ms resolution) ..." << endl;
      gettimeofday( tp, tzp );
      return 0;
    }

    // get current time
    QueryPerformanceCounter( &cLICounter );
    _ftime(&tbInit);

    // we now consider it to be exactly the time _ftime returned
    // (beyond the resolution of _ftime, down to the perfCounter res)
    
    /* now pack counter into a timeval struct -- int divide is fine here */
    tvInit.tv_sec = (long) (cLICounter.QuadPart / cLIClockFreq.QuadPart);
      
    /* counter/freq = num_of_secs (floor), sub out num_secs*freq to
       get num_ticks and do 1e6 * (double) num_ticks / (double) freq
       to get num usecs */
    tvInit.tv_usec = (long) (1e6 * ((double) (cLICounter.QuadPart - 
				         cLIClockFreq.QuadPart*tvInit.tv_sec)
				    /(double) cLIClockFreq.QuadPart));
  } 

  // now do the regular get time call to get the current time
  QueryPerformanceCounter( &cLICounter );
  tvNow.tv_sec = (long) (cLICounter.QuadPart / cLIClockFreq.QuadPart);
  tvNow.tv_usec = (long) (1e6 * ((double) (cLICounter.QuadPart - 
				     cLIClockFreq.QuadPart*tvNow.tv_sec)
				 /(double) cLIClockFreq.QuadPart));

  // find offset from initial value
  tvDiff = timevalDiff( tvNow, tvInit );
  
  if (tvDiff.tv_sec<0) {
    // need to reset clock, hi-perf timer rolled over
    // this will probably never be experienced, but just in case ...
    cerr << "gettimeofday: hi-perf timer rolled over. resetting clock ..." 
	 << endl;
    fFirst=1;
    gettimeofday(tp, tzp);
    return 0;
  } 

  // pack the value and clean it up
  tp->tv_sec  = tbInit.time + tvDiff.tv_sec;
  tp->tv_usec = tbInit.millitm*1000 + tvDiff.tv_usec;  
  if (tp->tv_usec >= 1000000) {
    tp->tv_sec++;
    tp->tv_usec -= 1000000;
  }
  
  return 0;
}
#endif
