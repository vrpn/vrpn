#include "vrpn_Shared.h"

#ifdef _WIN32
#include <iomanip.h>
#endif

#include <math.h>

// Calcs the sum of tv1 and tv2.  Returns the sum in a timeval struct.
// Calcs negative times properly, with the appropriate sign on both tv_sec
// and tv_usec (these signs will match unless one of them is 0).
// NOTE: both abs(tv_usec)'s must be < 1000000 (ie, normal timeval format)
struct timeval vrpn_TimevalSum( const struct timeval& tv1, 
			   const struct timeval& tv2 ) {
  struct timeval tvSum=tv1;

  tvSum.tv_sec += tv2.tv_sec;
  tvSum.tv_usec += tv2.tv_usec;

  // do borrows, etc to get the time the way i want it: both signs the same,
  // and abs(usec) less than 1e6
  if (tvSum.tv_sec>0) {
    if (tvSum.tv_usec<0) {
      tvSum.tv_sec--;
      tvSum.tv_usec += 1000000;
    } else if (tvSum.tv_usec >= 1000000) {
      tvSum.tv_sec++;
      tvSum.tv_usec -= 1000000;
    }
  } else if (tvSum.tv_sec<0) {
    if (tvSum.tv_usec>0) {
      tvSum.tv_sec++;
      tvSum.tv_usec -= 1000000;
    } else if (tvSum.tv_usec <= -1000000) {
      tvSum.tv_sec--;
      tvSum.tv_usec += 1000000;
    }
  } else {
    // == 0, so just adjust usec
    if (tvSum.tv_usec >= 1000000) {
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
struct timeval vrpn_TimevalDiff( const struct timeval& tv1, 
			    const struct timeval& tv2 ) {
  struct timeval tv;

  tv.tv_sec = -tv2.tv_sec;
  tv.tv_usec = -tv2.tv_usec;

  return vrpn_TimevalSum( tv1, tv );
}



struct timeval vrpn_TimevalScale (const struct timeval & tv,
                                  double scale) {
  struct timeval result;

  result.tv_sec = (long)( tv.tv_sec * scale );
  result.tv_usec = (long)( tv.tv_usec * scale
                 + fmod(tv.tv_sec * scale, 1.0) * 1000000.0 );
  return result;
}




// returns 1 if tv1 is greater than tv2;  0 otherwise

int vrpn_TimevalGreater (const struct timeval & tv1,
                         const struct timeval & tv2) {
  if (tv1.tv_sec > tv2.tv_sec) return 1;
  if ((tv1.tv_sec == tv2.tv_sec) &&
      (tv1.tv_usec > tv2.tv_usec)) return 1;
  return 0;
}




double vrpn_TimevalMsecs( const struct timeval& tv ) {
  return tv.tv_sec*1000.0 + tv.tv_usec/1000.0;
}

struct timeval vrpn_MsecsTimeval( const double dMsecs ) {
  struct timeval tv;
  tv.tv_sec = (long) floor(dMsecs/1000.0);
  tv.tv_usec = (long) ((dMsecs/1000.0 - tv.tv_sec)*1e6);
  return tv;
}

void vrpn_SleepMsecs( double dMsecs ) {
  struct timeval tvStart, tvNow;
  gettimeofday(&tvStart, NULL);
  do {
    gettimeofday(&tvNow, NULL);
  } while (vrpn_TimevalMsecs(vrpn_TimevalDiff( tvNow, tvStart ))<dMsecs);
}



static long lTestEndian = 0x1;
static const int fLittleEndian = (*((char *)&lTestEndian) == 0x1);

// convert vrpn_float64 to/from network order
// I have chosen big endian as the network order for vrpn_float64
vrpn_float64 htond( vrpn_float64 d ) {
  if (fLittleEndian) {
    vrpn_float64 dSwapped;
    char *pchSwapped= (char *)&dSwapped;
    char *pchOrig= (char *)&d;

    // swap to big
    for(int i=0;i<sizeof(vrpn_float64);i++) {
      pchSwapped[i]=pchOrig[sizeof(vrpn_float64)-i-1];
    }
    return dSwapped;
  } else {
    return d;
  }
}

// they are their own inverses, so ...
vrpn_float64 ntohd( vrpn_float64 d ) {
  return htond(d);
}

#ifdef _WIN32
#include <iostream.h>
#include <math.h>

// utility routines to read the pentium time stamp counter
// QueryPerfCounter drifts too much -- others have documented this
// problem on the net

// This is all based on code extracted from the hiball tracker cib lib

// 200 mhz pentium default -- we change this based on our calibration
static __int64 VRPN_CLOCK_FREQ = 200000000;

// Helium to histidine
// __int64 FREQUENCY = 199434500;

// tori -- but queryperfcounter returns this for us
// __int64 FREQUENCY = 198670000;

// ReaD Time Stamp Counter
#define rdtsc(li) { _asm _emit 0x0f \
  _asm _emit 0x31 \
  _asm mov li.LowPart, eax \
  _asm mov li.HighPart, edx \
}

 /*
  * calculate the time stamp counter register frequency (clock freq)
  */
#pragma optimize("",off)
static int vrpn_AdjustFrequency(void)
{
  const int loops = 2;
  const int tPerLoop = 500; // milliseconds for Sleep()
  cerr.precision(4);
  cerr.setf(ios::fixed);
  cerr << "vrpn gettimeofday: determining clock frequency ...";

  LARGE_INTEGER startperf, endperf;
  LARGE_INTEGER perffreq;

  QueryPerformanceFrequency( &perffreq );
  
  // don't optimize away these variables
  double sum = 0;
  volatile LARGE_INTEGER liStart, liEnd;

  DWORD dwPriorityClass=GetPriorityClass(GetCurrentProcess());
  int iThreadPriority=GetThreadPriority(GetCurrentThread());
  SetPriorityClass( GetCurrentProcess() , REALTIME_PRIORITY_CLASS );
  SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

  // pull all into cache and do rough test to see if tsc and perf counter
  // are one in the same
  rdtsc( liStart );
  QueryPerformanceCounter( &startperf );
  Sleep(100);
  rdtsc( liEnd );
  QueryPerformanceCounter( &endperf );

  double freq = perffreq.QuadPart * (liEnd.QuadPart - liStart.QuadPart) / 
      ((double)(endperf.QuadPart - startperf.QuadPart));

  if (fabs(perffreq.QuadPart - freq) < 0.05*freq) {
    VRPN_CLOCK_FREQ = (__int64) perffreq.QuadPart;
    cerr << "\nvrpn gettimeofday: perf clock is tsc -- using perf clock freq (" 
	 << perffreq.QuadPart/1e6 << " MHz)" << endl;
    SetPriorityClass( GetCurrentProcess() , dwPriorityClass );
    SetThreadPriority( GetCurrentThread(), iThreadPriority );
    return 0;
  } 

  // either tcs and perf clock are not the same, or we could not
  // tell accurately enough with the short test. either way we now
  // need an accurate frequency measure, so ...

  cerr << " (this will take " << setprecision(0) << loops*tPerLoop/1000.0 
       << " seconds) ... " << endl;
  cerr.precision(4);

  for (int j = 0; j < loops; j++) {
    rdtsc( liStart );
    QueryPerformanceCounter( &startperf );
    Sleep(tPerLoop);
    rdtsc( liEnd );
    QueryPerformanceCounter( &endperf );

    // perf counter timer ran for one call to Query and one call to
    // tcs read in addition to the time between the tsc readings
    // tcs read did the same

    // endperf - startperf / perf freq = time between perf queries
    // endtsc - starttsc = clock ticks between perf queries
    //    sum += (endtsc - starttsc) / ((double)(endperf - startperf)/perffreq);
    sum += perffreq.QuadPart * (liEnd.QuadPart - liStart.QuadPart) / 
      ((double)(endperf.QuadPart - startperf.QuadPart));
  }
  
  SetPriorityClass( GetCurrentProcess() , dwPriorityClass );
  SetThreadPriority( GetCurrentThread(), iThreadPriority );

  // might want last, not sum -- just get into cache and run
  freq = (sum/loops);
  
  cerr.precision(5);

  // if we are on a uniprocessor system, then use the freq estimate
  // This used to check against a 200 mhz assumed clock, but now 
  // we assume the routine works and trust the result.
  //  if (fabs(freq - VRPN_CLOCK_FREQ) > 0.05 * VRPN_CLOCK_FREQ) {
  //    cerr << "vrpn gettimeofday: measured freq is " << freq/1e6 
  //	 << " MHz - DOES NOT MATCH" << endl;
  //    return -1;
  //  }

  // if we are in a system where the perf clock is the tsc, then use the
  // rate the perf clock returns (or rather, if the freq we measure is
  // approx the perf clock freq)
  if (fabs(perffreq.QuadPart - freq) < 0.05*freq) {
    VRPN_CLOCK_FREQ = perffreq.QuadPart;
    cerr << "vrpn gettimeofday: perf clock is tsc -- using perf clock freq (" 
	 << perffreq.QuadPart/1e6 << " MHz)" << endl;
  } else {
    cerr << "vrpn gettimeofday: adjusted clock freq to measured freq (" 
	 << freq/1e6 << " MHz)" << endl;
  }
  VRPN_CLOCK_FREQ = (__int64) freq;
  return 0;
}
#pragma optimize("", on)

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
  static LARGE_INTEGER liInit;
  static LARGE_INTEGER liNow;
  static LARGE_INTEGER liDiff;
  struct timeval tvDiff;

  
  if (!fHasPerfCounter) {
    _ftime(&tbInit);
    tp->tv_sec  = tbInit.time;
    tp->tv_usec = tbInit.millitm*1000;
    return 0;
  } 

  if (fFirst) {
    LARGE_INTEGER liTemp;
    // establish a time base
    fFirst=0;

    // check that hi-perf clock is available
    if ( !(fHasPerfCounter = QueryPerformanceFrequency( &liTemp )) ) {
      cerr << "\nvrpn gettimeofday: no hi performance clock available. " 
	   << "Defaulting to _ftime (~6 ms resolution) ..." << endl;
      gettimeofday( tp, tzp );
      return 0;
    }
    
    if (vrpn_AdjustFrequency()<0) {
      cerr << "\nvrpn gettimeofday: can't verify clock frequency. " 
	   << "Defaulting to _ftime (~6 ms resolution) ..." << endl;
      fHasPerfCounter=0;
      gettimeofday( tp, tzp );
      return 0;
    }

    // get current time
    // We assume this machine has a time stamp counter register --
    // I don't know of an easy way to check for this
    rdtsc( liInit );
    _ftime(&tbInit);

    // we now consider it to be exactly the time _ftime returned
    // (beyond the resolution of _ftime, down to the perfCounter res)
  } 

  // now do the regular get time call to get the current time
  rdtsc( liNow );

  // find offset from initial value
  liDiff.QuadPart = liNow.QuadPart - liInit.QuadPart;

  tvDiff.tv_sec = (long) ( liDiff.QuadPart / VRPN_CLOCK_FREQ );
  tvDiff.tv_usec = (long)(1e6*((liDiff.QuadPart-VRPN_CLOCK_FREQ*tvDiff.tv_sec)
			   / (double) VRPN_CLOCK_FREQ) );

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

// do the calibration before the program ever starts up
static struct timeval __tv;
static int __iTrash = gettimeofday(&__tv, (struct timezone *)NULL);
