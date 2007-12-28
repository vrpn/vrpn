#include "vrpn_Shared.h"

#include <stdio.h>
#include <math.h>
#ifndef	_WIN32_WCE
#include <sys/types.h>
#endif

#if defined(__APPLE__)
#include <unistd.h>
#endif

#include <string.h>  // for memcpy()

#if !( defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS) )
#include <netinet/in.h>
#endif

//#include "vrpn_cygwin_hack.h"

#define CHECK(a) if (a == -1) return -1

// perform normalization of a timeval
// XXX this still needs to be checked for errors if the timeval
// or the rate is negative
#define TIMEVAL_NORMALIZE(tiva) { \
    const long div_77777 = (tiva.tv_usec / 1000000); \
    tiva.tv_sec += div_77777; \
    tiva.tv_usec -= (div_77777 * 1000000); \
    }

timeval vrpn_TimevalNormalize( const timeval & in_tv )
{
    timeval out_tv = in_tv;
    TIMEVAL_NORMALIZE(out_tv);
    return out_tv;
}

// Calcs the sum of tv1 and tv2.  Returns the sum in a timeval struct.
// Calcs negative times properly, with the appropriate sign on both tv_sec
// and tv_usec (these signs will match unless one of them is 0).
// NOTE: both abs(tv_usec)'s must be < 1000000 (ie, normal timeval format)
timeval vrpn_TimevalSum( const timeval& tv1, const timeval& tv2 )
{
    timeval tvSum=tv1;

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
timeval vrpn_TimevalDiff( const timeval& tv1, const timeval& tv2 )
{
    timeval tv;
    
    tv.tv_sec = -tv2.tv_sec;
    tv.tv_usec = -tv2.tv_usec;
    
    return vrpn_TimevalSum( tv1, tv );
}



timeval vrpn_TimevalScale (const timeval & tv, double scale)
{
    timeval result;
    result.tv_sec = (long)( tv.tv_sec * scale );
    result.tv_usec = (long)( tv.tv_usec * scale
                             + fmod(tv.tv_sec * scale, 1.0) * 1000000.0 );
    TIMEVAL_NORMALIZE(result);
    return result;
}




// returns 1 if tv1 is greater than tv2;  0 otherwise
bool vrpn_TimevalGreater (const timeval & tv1, const timeval & tv2)
{
    if (tv1.tv_sec > tv2.tv_sec) return 1;
    if ((tv1.tv_sec == tv2.tv_sec) &&
        (tv1.tv_usec > tv2.tv_usec)) return 1;
    return 0;
}

// return 1 if tv1 is equal to tv2; 0 otherwise
bool vrpn_TimevalEqual( const timeval& tv1, const timeval& tv2 )
{
  if( tv1.tv_sec == tv2.tv_sec && tv1.tv_usec == tv2.tv_usec )
    return true;
  else return false;
}

double vrpn_TimevalMsecs( const timeval& tv )
{
    return tv.tv_sec*1000.0 + tv.tv_usec/1000.0;
}

timeval vrpn_MsecsTimeval( const double dMsecs )
{
    timeval tv;
    tv.tv_sec = (long) floor(dMsecs/1000.0);
    tv.tv_usec = (long) ((dMsecs/1000.0 - tv.tv_sec)*1e6);
    return tv;
}

// Sleep for dMsecs milliseconds, freeing up the processor while you
// are doing so.

void vrpn_SleepMsecs( double dMsecs )
{
#if defined(_WIN32)
    Sleep((DWORD)dMsecs);
#else
    timeval timeout;

    // Convert milliseconds to seconds
    timeout.tv_sec = (int)(dMsecs / 1000.0);

    // Subtract of whole number of seconds
    dMsecs -= timeout.tv_sec * 1000;

    // Convert remaining milliseconds to microsec
    timeout.tv_usec = (int)(dMsecs * 1000);
    
    // A select() with NULL file descriptors acts like a microsecond
    // timer.
    select(0, 0, 0, 0, & timeout);  // wait for that long;
#endif
}


// convert vrpn_float64 to/from network order
// I have chosen big endian as the network order for vrpn_float64
// to match the standard for htons() and htonl().
// NOTE: There is an added complexity when we are using an ARM
// processor in mixed-endian mode for the doubles, whereby we need
// to not just swap all of the bytes but also swap the two 4-byte
// words to get things in the right order.
#ifdef  __arm__
#include <endian.h>
#endif

vrpn_float64 htond( vrpn_float64 d )
{
    if (!vrpn_big_endian) {
        vrpn_float64 dSwapped;
        char *pchSwapped= (char *)&dSwapped;
        char *pchOrig= (char *)&d;

        // swap to big-endian order.
        for(int i=0;i<sizeof(vrpn_float64);i++) {
            pchSwapped[i]=pchOrig[sizeof(vrpn_float64)-i-1];
        }

        #ifdef  __arm__
          // On ARM processor, see if we're in mixed mode.  If so,
          // we need to swap the two words after doing the total
          // swap of bytes.
          #if __FLOAT_WORD_ORDER != __BYTE_ORDER
            {
              /* Fixup mixed endian floating point machines */
              vrpn_uint32 *pwSwapped = (vrpn_uint32 *)&dSwapped;
              vrpn_uint32 scratch = pwSwapped[0];
              pwSwapped[0] = pwSwapped[1];
              pwSwapped[1] = scratch;
            }
          #endif
        #endif

        return dSwapped;
    } else {
        return d;
    }
}

// they are their own inverses, so ...
vrpn_float64 ntohd (vrpn_float64 d)
{
    return htond(d);
}

/** Utility routine for placing a vrpn_int16 into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen, const vrpn_int16 value)
{
    vrpn_int16 netValue = htons(value);
    const int length = sizeof(netValue);

    if (length > *buflen) {
        fprintf(stderr, "vrpn_buffer: buffer not large enough\n");
        return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

/** Utility routine for placing a vrpn_uint16 into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen, const vrpn_uint16 value)
{
    vrpn_uint16 netValue = htons(value);
    const int length = sizeof(netValue);

    if (length > *buflen) {
        fprintf(stderr, "vrpn_buffer: buffer not large enough\n");
        return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

/** Utility routine for placing a vrpn_int32 into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen, const vrpn_int32 value)
{
    vrpn_int32 netValue = htonl(value);
    const int length = sizeof(netValue);

    if (length > *buflen) {
        fprintf(stderr, "vrpn_buffer: buffer not large enough\n");
        return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

/** Utility routine for placing a vrpn_uint32 into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer(char ** insertPt, vrpn_int32 * buflen, const vrpn_uint32 value)
{
    vrpn_uint32 netValue = htonl(value);
    const int length = sizeof(netValue);

    if (length > *buflen) {
        fprintf(stderr, "vrpn_buffer: buffer not large enough\n");
        return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

/** Utility routine for placing a character into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen, const char value)
{
  const vrpn_int32 length = sizeof(char);

  if (*buflen < length) {
    fprintf(stderr, "vrpn_buffer:  buffer not large enough.\n");
    return -1;
  }

  **insertPt = value;
  *insertPt += length;
  *buflen -= length;

  return 0;
}

/** Utility routine for placing a vrpn_float32 into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                  const vrpn_float32 value)
{
    vrpn_int32 longval = *((vrpn_int32 *)&value);
    return vrpn_buffer(insertPt, buflen, longval);
}

/** Utility routine for placing a vrpn_float64 into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                  const vrpn_float64 value)
{
    vrpn_float64 netValue = htond(value);
    const vrpn_int32 length = sizeof(netValue);

    if (length > *buflen) {
        fprintf(stderr, "vrpn_buffer: buffer not large enough\n");
        return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

/** Utility routine for placing a timeval struct into a buffer that
    is to be sent as a message. Handles packing into an unaligned
    buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen, const timeval t)
{
    vrpn_int32 sec, usec;

    // tv_sec and usec are 64 bits on some architectures, but we
    // define them as 32 bit for network transmission

    sec = t.tv_sec;
    usec = t.tv_usec;

    if (vrpn_buffer(insertPt, buflen, sec)) return -1;
    return vrpn_buffer(insertPt, buflen, usec);
}

/** Utility routine for placing a character string of given length
    into a buffer that is to be sent as a message. Handles packing into
    an unaligned buffer (though this should not be done). Advances the insertPt
    pointer to just after newly-inserted value. Decreases the buflen
    (space remaining) by the length of the value. Returns zero on
    success and -1 on failure.

    Part of a family of routines that buffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to the
    VRPN standard wire protocol.

    If the length is specified as -1, then the string will be assumed to
    be NULL-terminated and will be copied using the string-copy routines.
*/

int vrpn_buffer (char ** insertPt, vrpn_int32 * buflen,
                  const char * string, vrpn_int32 length)
{
    if (length > *buflen) {
	fprintf(stderr, "vrpn_buffer:  buffer not long enough for string.\n");
	return -1;
    }
    
    if (length == -1) {
	size_t len = strlen(string)+1;	// +1 for the NULL terminating character
	if (len > (unsigned)*buflen) {
    	    fprintf(stderr, "vrpn_buffer:  buffer not long enough for string.\n");
	    return -1;
	}
	strcpy(*insertPt, string);
	*insertPt += len;
	*buflen -= len;
    } else {
	memcpy(*insertPt, string, length);
	*insertPt += length;
	*buflen -= length;
    }
    
    return 0;
}


/** Utility routine for taking a character from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, char * cval) {
  *cval = **buffer;
  *buffer += sizeof(char);
  return 0;
}

/** Utility routine for taking a vrpn_int16 from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, vrpn_int16 * lval)
{
    vrpn_int16	aligned;

    memcpy(&aligned, *buffer, sizeof(aligned));
    *lval = ntohs(aligned);
    *buffer += sizeof(vrpn_int16);
    return 0;
}

/** Utility routine for taking a vrpn_uint16 from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, vrpn_uint16 * lval)
{
    vrpn_uint16	aligned;

    memcpy(&aligned, *buffer, sizeof(aligned));
    *lval = ntohs(aligned);
    *buffer += sizeof(vrpn_uint16);
    return 0;
}

/** Utility routine for taking a vrpn_int32 from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, vrpn_int32 * lval)
{
    vrpn_int32	aligned;

    memcpy(&aligned, *buffer, sizeof(aligned));
    *lval = ntohl(aligned);
    *buffer += sizeof(vrpn_int32);
    return 0;
}

/** Utility routine for taking a vrpn_uint32 from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, vrpn_uint32 * lval)
{
    vrpn_uint32	aligned;

    memcpy(&aligned, *buffer, sizeof(aligned));
    *lval = ntohl(aligned);
    *buffer += sizeof(vrpn_uint32);
    return 0;
}

/** Utility routine for taking a vrpn_float32 from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, vrpn_float32 * fval)
{
    vrpn_int32 lval;
    CHECK(vrpn_unbuffer(buffer, &lval));
    *fval = *((vrpn_float32 *) &lval);
    return 0;
}

/** Utility routine for taking a vrpn_float64 from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, vrpn_float64 * dval)
{
    vrpn_float64 aligned;

    memcpy( &aligned, *buffer, sizeof( aligned ) );
    *dval = ntohd( aligned );
    *buffer += sizeof( aligned );
    return 0;
}

/** Utility routine for taking a struct timeval from a buffer that
    was sent as a message. Handles unpacking from an
    unaligned buffer, because people did this anyway. Advances the reading
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.
*/

int vrpn_unbuffer (const char ** buffer, timeval * t)
{
    vrpn_int32 sec, usec;

    CHECK(vrpn_unbuffer(buffer, &sec));
    CHECK(vrpn_unbuffer(buffer, &usec));

    t->tv_sec = sec;
    t->tv_usec = usec;

    return 0;
}

/** Utility routine for taking a string of specified length from a buffer that
    was sent as a message. Does NOT handle unpacking from an
    unaligned buffer, because the semantics of VRPN require
    message buffers and the values in them to be aligned, in order to
    reduce the amount of copying that goes on. Advances the read
    pointer to just after newly-read value. Assumes that the
    buffer holds a complete value. Returns zero on success and -1 on failure.

    Part of a family of routines that unbuffer different VRPN types
    based on their type (vrpn_buffer is overloaded based on the third
    parameter type). These routines handle byte-swapping to and from
    the VRPN defined wire protocol.

    If the length is specified as -1, then the string will be assumed to
    be NULL-terminated and will be read using the string-copy routines. NEVER
    use this on a string that was packed with other than the NULL-terminating
    condition, since embedded NULL characters will ruin the argument parsing
    for any later arguments in the message.
*/

int vrpn_unbuffer (const char ** buffer, char * string,
                    vrpn_int32 length)
{
    if (!string) return -1;

    if (length == -1) {
	strcpy(string, *buffer);
	*buffer += strlen(*buffer)+1;	// +1 for NULL terminating character
    } else {
	memcpy(string, *buffer, length);
	*buffer += length;
    }

    return 0;
}

///////////////////////////////////////////////////////////////
// More accurate gettimeofday() on some Windows operating systems
// and machines can be gotten by using the Performance Counter
// on the CPU.  This doesn't seem to work in NT/2000 for some
// computers, so the code to do it is #defined out by default.
// To put it back back, #define VRPN_UNSAFE_WINDOWS_CLOCK and
// the following code will use the performance counter (which it
// takes a second to calibrate at program start-up).
///////////////////////////////////////////////////////////////

#ifndef VRPN_UNSAFE_WINDOWS_CLOCK

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <math.h>

#pragma optimize("", on)

#ifdef _WIN32
void	get_time_using_GetLocalTime(unsigned long &sec, unsigned long &usec)
{
    SYSTEMTIME	stime;	    // System time in funky structure
    FILETIME	ftime;	    // Time in 100-nsec intervals since Jan 1 1601
    LARGE_INTEGER   tics;   // ftime stored into a 64-bit quantity

    GetLocalTime(&stime);
    SystemTimeToFileTime(&stime, &ftime);

    // Copy the data into a structure that can be treated as a 64-bit integer
    tics.HighPart = ftime.dwHighDateTime;
    tics.LowPart = ftime.dwLowDateTime;

    // Convert the 64-bit time into seconds and microseconds since July 1 1601
    sec = (long)( tics.QuadPart / 10000000L );
    usec = (long)( ( tics.QuadPart - ( ((LONGLONG)(sec)) * 10000000L ) ) / 10 );

    // Translate the time to be based on January 1, 1970 (_ftime base)
    // The offset here is gotten by using the "time_test" program to report the
    // difference in seconds between the two clocks.
    sec -= 3054524608;
}
#endif

///////////////////////////////////////////////////////////////
// Although VC++ doesn't include a gettimeofday
// call, Cygnus Solutions Cygwin32 environment does,
// so this is not used when compiling with gcc under WIN32.
// XXX Note that the cygwin one was wrong in an earlier
// version.  It is claimed that they have fixed it now, but
// better check.
///////////////////////////////////////////////////////////////
int vrpn_gettimeofday(timeval *tp, void *voidp)
{
#ifndef _STRUCT_TIMEZONE
  #define _STRUCT_TIMEZONE
  /* from HP-UX */
  struct timezone {
      int     tz_minuteswest; /* minutes west of Greenwich */
      int     tz_dsttime;     /* type of dst correction */
  };
#endif
  struct timezone *tzp = (struct timezone *)voidp;
    if (tp != NULL) {
#ifdef _WIN32_WCE
	unsigned long sec, usec;
	get_time_using_GetLocalTime(sec, usec);
	tp->tv_sec = sec;
	tp->tv_usec = usec;
#else
            struct _timeb t;
            _ftime(&t);
            tp->tv_sec = t.time;
            tp->tv_usec = (long)t. millitm * 1000;
#endif
    }
    if (tzp != NULL) {
            TIME_ZONE_INFORMATION tz;
            GetTimeZoneInformation(&tz);
            tzp->tz_minuteswest = tz.Bias ;
            tzp->tz_dsttime = (tz.StandardBias != tz.Bias);
    }
    return 0;
}

#endif //defined(_WIN32)

#else	// In the case that VRPN_UNSAFE_WINDOWS_CLOCK is defined

// Although VC++ doesn't include a gettimeofday
// call, Cygnus Solutions Cygwin32 environment does.
// XXX AND ITS WRONG in the current release 10/11/99, version b20.1
// They claim it will be fixed in the next release, version b21
// so until then, we will make it right using our solution. 
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <math.h>

// utility routines to read the pentium time stamp counter
// QueryPerfCounter drifts too much -- others have documented this
// problem on the net

// This is all based on code extracted from the UNC hiball tracker cib lib

// 200 mhz pentium default -- we change this based on our calibration
static __int64 VRPN_CLOCK_FREQ = 200000000;

// Helium to histidine
// __int64 FREQUENCY = 199434500;

// tori -- but queryperfcounter returns this for us
// __int64 FREQUENCY = 198670000;

// Read Time Stamp Counter
#define rdtsc(li) { _asm _emit 0x0f \
  _asm _emit 0x31 \
  _asm mov li.LowPart, eax \
  _asm mov li.HighPart, edx \
}

/*
 * calculate the time stamp counter register frequency (clock freq)
 */
#ifndef	VRPN_WINDOWS_CLOCK_V2
#pragma optimize("",off)
static int vrpn_AdjustFrequency(void)
{
    const int loops = 2;
    const int tPerLoop = 500; // milliseconds for Sleep()
    fprintf(stderr,"vrpn vrpn_gettimeofday: determining clock frequency...");

    LARGE_INTEGER startperf, endperf;
    LARGE_INTEGER perffreq;

    // See if the hardware supports the high-resolution performance counter.
    // If so, get the frequency of it.  If not, we can't use it and so return
    // -1.
    if (QueryPerformanceFrequency( &perffreq ) == 0) {
	return -1;
    }

    // don't optimize away these variables
    double sum = 0;
    volatile LARGE_INTEGER liStart, liEnd;

    DWORD dwPriorityClass=GetPriorityClass(GetCurrentProcess());
    int iThreadPriority=GetThreadPriority(GetCurrentThread());
    SetPriorityClass( GetCurrentProcess() , REALTIME_PRIORITY_CLASS );
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

    // pull all into cache and do rough test to see if tsc and perf counter
    // are one and the same
    rdtsc( liStart );
    QueryPerformanceCounter( &startperf );
    Sleep(100);
    rdtsc( liEnd );
    QueryPerformanceCounter( &endperf );

    double freq = perffreq.QuadPart * (liEnd.QuadPart - liStart.QuadPart) / 
        ((double)(endperf.QuadPart - startperf.QuadPart));

    if (fabs(perffreq.QuadPart - freq) < 0.05*freq) {
        VRPN_CLOCK_FREQ = (__int64) perffreq.QuadPart;
        fprintf(stderr,"\nvrpn vrpn_gettimeofday: perf clock is tsc -- using perf clock freq ( %lf MHz)\n", perffreq.QuadPart/1e6);
        SetPriorityClass( GetCurrentProcess() , dwPriorityClass );
        SetThreadPriority( GetCurrentThread(), iThreadPriority );
        return 0;
    } 

    // either tcs and perf clock are not the same, or we could not
    // tell accurately enough with the short test. either way we now
    // need an accurate frequency measure, so ...

    fprintf(stderr," (this will take %lf seconds)...\n", loops*tPerLoop/1000.0);

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
  
    // if we are on a uniprocessor system, then use the freq estimate
    // This used to check against a 200 mhz assumed clock, but now 
    // we assume the routine works and trust the result.
    //  if (fabs(freq - VRPN_CLOCK_FREQ) > 0.05 * VRPN_CLOCK_FREQ) {
    //    cerr << "vrpn vrpn_gettimeofday: measured freq is " << freq/1e6 
    //	 << " MHz - DOES NOT MATCH" << endl;
    //    return -1;
    //  }

    // if we are in a system where the perf clock is the tsc, then use the
    // rate the perf clock returns (or rather, if the freq we measure is
    // approx the perf clock freq).
    if (fabs(perffreq.QuadPart - freq) < 0.05*freq) {
        VRPN_CLOCK_FREQ = perffreq.QuadPart;
	fprintf(stderr, "vrpn vrpn_gettimeofday: perf clock is tsc -- using perf clock freq ( %lf MHz)\n", perffreq.QuadPart/1e6);
    } else {
        fprintf(stderr, "vrpn vrpn_gettimeofday: adjusted clock freq to measured freq ( %lf MHz )\n", freq/1e6);
    }
    VRPN_CLOCK_FREQ = (__int64) freq;
    return 0;
}
#pragma optimize("", on)
#endif

// The pc has no gettimeofday call, and the closest thing to it is _ftime.
// _ftime, however, has only about 6 ms resolution, so we use the peformance 
// as an offset from a base time which is established by a call to by _ftime.

// The first call to vrpn_gettimeofday will establish a new time frame
// on which all later calls will be based.  This means that the time returned
// by vrpn_gettimeofday will not always match _ftime (even at _ftime's resolution),
// but it will be consistent across all vrpn_gettimeofday calls.

///////////////////////////////////////////////////////////////
// Although VC++ doesn't include a gettimeofday
// call, Cygnus Solutions Cygwin32 environment does,
// so this is not used when compiling with gcc under WIN32

// XXX AND ITS WRONG in the current release 10/11/99
// They claim it will be fixed in the next release, 
// so until then, we will make it right using our solution. 
///////////////////////////////////////////////////////////////
#ifndef	VRPN_WINDOWS_CLOCK_V2
int vrpn_gettimeofday(timeval *tp, void *voidp)
{
    static int fFirst=1;
    static int fHasPerfCounter=1;
    static struct _timeb tbInit;
    static LARGE_INTEGER liInit;
    static LARGE_INTEGER liNow;
    static LARGE_INTEGER liDiff;
    timeval tvDiff;

#ifndef _STRUCT_TIMEZONE
  #define _STRUCT_TIMEZONE
  /* from HP-UX */
  struct timezone {
      int     tz_minuteswest; /* minutes west of Greenwich */
      int     tz_dsttime;     /* type of dst correction */
  };
#endif
  struct timezone *tzp = (struct timezone *)voidp;

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

	// Check to see if we are on a Windows NT machine (as opposed to a
	// Windows 95/98 machine).  If we are not, then use the _ftime code
	// because the hi-perf clock does not work under Windows 98SE on my
	// laptop, although the query for one seems to indicate that it is
	// available.

	{   OSVERSIONINFO osvi;

	    memset(&osvi, 0, sizeof(OSVERSIONINFO));
	    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	    GetVersionEx(&osvi);

	    if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) {
                fprintf(stderr, "\nvrpn_gettimeofday: disabling hi performance clock on non-NT system. " 
	             "Defaulting to _ftime (~6 ms resolution) ...\n");
		fHasPerfCounter=0;
	        vrpn_gettimeofday( tp, tzp );
		return 0;
	    }
	}

        // check that hi-perf clock is available
        if ( !(fHasPerfCounter = QueryPerformanceFrequency( &liTemp )) ) {
            fprintf(stderr, "\nvrpn_gettimeofday: no hi performance clock available. " 
                 "Defaulting to _ftime (~6 ms resolution) ...\n");
            fHasPerfCounter=0;
            vrpn_gettimeofday( tp, tzp );
            return 0;
        }

        if (vrpn_AdjustFrequency()<0) {
            fprintf(stderr, "\nvrpn_gettimeofday: can't verify clock frequency. " 
                 "Defaulting to _ftime (~6 ms resolution) ...\n");
            fHasPerfCounter=0;
            vrpn_gettimeofday( tp, tzp );
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
    tvDiff.tv_usec = (long)(1e6*((liDiff.QuadPart-VRPN_CLOCK_FREQ
                                  *tvDiff.tv_sec)
                                 / (double) VRPN_CLOCK_FREQ) );
    
    // pack the value and clean it up
    tp->tv_sec  = tbInit.time + tvDiff.tv_sec;
    tp->tv_usec = tbInit.millitm*1000 + tvDiff.tv_usec;  
    while (tp->tv_usec >= 1000000) {
        tp->tv_sec++;
        tp->tv_usec -= 1000000;
    }
  
    return 0;
}
#else //defined(VRPN_WINDOWS_CLOCK_V2)

void get_time_using_GetLocalTime(unsigned long &sec, unsigned long &usec)
{
    static LARGE_INTEGER first_count = { 0, 0 };
    static unsigned long first_sec, first_usec;
    static LARGE_INTEGER perf_freq;   //< Frequency of the performance counter.
    SYSTEMTIME	stime;	    // System time in funky structure
    FILETIME	ftime;	    // Time in 100-nsec intervals since Jan 1 1601
    LARGE_INTEGER   tics;   // ftime stored into a 64-bit quantity
    LARGE_INTEGER perf_counter;

    // The first_count value will be zero only the first time through; we 
    // rely on this to set up the structures needed to interpret the data
    // that we get from querying the performance counter.
    if (first_count.QuadPart == 0) {
	QueryPerformanceCounter( &first_count );

        // Find out how fast the performance counter runs.  Store this for later runs.
	QueryPerformanceFrequency( &perf_freq );

	// Find out what time it is in a Windows structure.
	GetLocalTime(&stime);
	SystemTimeToFileTime(&stime, &ftime);

	// Copy the data into a structure that can be treated as a 64-bit integer
	tics.HighPart = ftime.dwHighDateTime;
	tics.LowPart = ftime.dwLowDateTime;

	// Convert the 64-bit time into seconds and microseconds since July 1 1601
	sec = (long)( tics.QuadPart / 10000000L );
	usec = (long)( ( tics.QuadPart - ( ((LONGLONG)(sec)) * 10000000L ) ) / 10 );
	first_sec = sec;
	first_usec = usec;

    } else {
	QueryPerformanceCounter( &perf_counter );
	if ( perf_counter.QuadPart >= first_count.QuadPart ) {
	  perf_counter.QuadPart = perf_counter.QuadPart - first_count.QuadPart;
	} else {
	  // Take care of the case when the counter rolls over.
	  perf_counter.QuadPart = 0x7fffffffffffffff - first_count.QuadPart + perf_counter.QuadPart;
	}

	// Reinterpret the performance counter into seconds and microseconds
	// by dividing by the performance counter.  Microseconds is placed
	// into perf_counter by subtracting the seconds value out, then
	// multiplying by 1 million and re-dividing by the performance counter.
	sec = (long)( perf_counter.QuadPart / perf_freq.QuadPart );
	perf_counter.QuadPart -= perf_freq.QuadPart * sec;
	perf_counter.QuadPart *= 1000000L;  //< Turn microseconds into seconds
	usec = first_usec + (long)( perf_counter.QuadPart / perf_freq.QuadPart );
	sec += first_sec;

	// Make sure that we've not got more than a million microseconds.
	// If so, then shift it into seconds.  We don't expect it to be above
	// more than 1 million because we added two things, each of which were
	// less than 1 million.
	if (usec > 1000000L) {
	  usec -= 1000000L;
	  sec++;
	}
    }

    // Translate the time to be based on January 1, 1970 (_ftime base)
    // The offset here is gotten by using the "time_test" program to report the
    // difference in seconds between the two clocks.
    sec -= 3054524608L;
}

int vrpn_gettimeofday(timeval *tp, void *voidp)
{
#ifndef _STRUCT_TIMEZONE
  #define _STRUCT_TIMEZONE
  /* from HP-UX */
  struct timezone {
      int     tz_minuteswest; /* minutes west of Greenwich */
      int     tz_dsttime;     /* type of dst correction */
  };
#endif
  struct timezone *tzp = (struct timezone *)voidp;

  unsigned  long sec,usec;
  get_time_using_GetLocalTime(sec,usec);
  tp->tv_sec = sec;
  tp->tv_usec = usec;
  if (tzp != NULL) {
    TIME_ZONE_INFORMATION tz;
    GetTimeZoneInformation(&tz);
    tzp->tz_minuteswest = tz.Bias ;
    tzp->tz_dsttime = (tz.StandardBias != tz.Bias);
  }
  return 0;
}

#endif //defined(VRPN_WINDOWS_CLOCK_V2)

#endif //defined(_WIN32)

// do the calibration before the program ever starts up
static timeval __tv;
static int __iTrash = vrpn_gettimeofday(&__tv, (struct timezone *)NULL);

#endif // VRPN_UNSAFE_WINDOWS_CLOCK


#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <errno.h>
#include <signal.h>
#endif

#define ALL_ASSERT(exp, msg) if(!(exp)){ fprintf(stderr, "\nAssertion failed! \n %s (%s, %s)\n", msg, __FILE__, __LINE__); exit(-1);}

// init all fields in init()
vrpn_Semaphore::vrpn_Semaphore( int cNumResources ) : 
  cResources(cNumResources)
{
  init();
}

// create a new internal structure for the semaphore
// (binary copy is not ok)
// This does not copy the state of the semaphore
vrpn_Semaphore::vrpn_Semaphore( const vrpn_Semaphore& s ) : 
  cResources(s.cResources)
{
  init();
}

bool vrpn_Semaphore::init() {
#ifdef sgi
  if (vrpn_Semaphore::ppaArena==NULL) {
    vrpn_Semaphore::allocArena();
  }
  if (cResources==1) {
    fUsingLock=true;
    ps=NULL;
    // use lock instead of semaphore
    if ((l = usnewlock(Semaphore::ppaArena)) == NULL) {
      fprintf(stderr,"vrpn_Semaphore::vrpn_Semaphore: error allocating lock from arena.\n");
      return false;
    }
  } else {    
    fUsingLock=false;
    l=NULL;
    if ((ps = usnewsema(vrpn_Semaphore::ppaArena, cResources)) == NULL) {
      fprintf(stderr,"vrpn_Semaphore::vrpn_Semaphore: error allocating semaphore from arena.\n");
      return false;
    }
  }
#elif defined(_WIN32)
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
    
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		   FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,    GetLastError(),
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		     // Default language
		   (LPTSTR) &lpMsgBuf,    0,    NULL );
    fprintf(stderr,"vrpn_Semaphore::vrpn_Semaphore: error creating semaphore, "
      "WIN32 CreateSemaphore call caused the following error: %s\n", (LPTSTR) lpMsgBuf);
    // Free the buffer.
    LocalFree( lpMsgBuf );
    return false;
  }
#else
  // Posix threads are the default.  We're still not out of the woods yet, though,
  // because there seem to be two posix standards: sem_open (mac) and sem_init (linux)
  // and so we need to figure out which to use.  To make things worse, the mac
  // declares sem_init() but it fails with a "not implemented" errno.

    int numMax = cResources;
    if (numMax < 1) {
      numMax = 1;
    }
  #ifdef MACOSX
    char template_name[] = "/tmp/semaphore.XXXXXXXX";
    char *tempname = mktemp(template_name);
    semaphore = sem_open(tempname, O_CREAT | O_EXCL, 0xffff, numMax);
   #if __APPLE_CC__ >= 5465
     if (semaphore == SEM_FAILED) {
   #else
     // Strange cast due to incompatibility in semaphore.h definition.
     if ((int)(semaphore) == SEM_FAILED) {
   #endif
  #else
    semaphore = new sem_t;
    if (sem_init(semaphore, 0, numMax) != 0) {
  #endif
        perror("vrpn_Semaphore::vrpn_Semaphore: error initializing semaphore");
        return false;
    }
#endif

    return true;
}

bool vrpn_Semaphore::destroy() {
#ifdef sgi
  if (fUsingLock) {
    usfreelock( l, vrpn_Semaphore::ppaArena );
  } else {
    usfreesema( ps, vrpn_Semaphore::ppaArena );
  }
#elif defined(_WIN32)
  if (!CloseHandle(hSemaphore)) {
    // get error info from windows (from FormatMessage help page)
    LPVOID lpMsgBuf;
    
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		   FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,    GetLastError(),
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		     // Default language
		   (LPTSTR) &lpMsgBuf,    0,    NULL );
    fprintf(stderr, "vrpn_Semaphore::destroy: error destroying semaphore, "
      "WIN32 CloseHandle call caused the following error: %s\n", (LPTSTR) lpMsgBuf);
    // Free the buffer.
    LocalFree( lpMsgBuf );
    return false;
  }
#else
  // Posix threads are the default.
#ifdef MACOSX
  if (sem_close(semaphore) != 0) {
      perror("vrpn_Semaphore::destroy: error destroying semaphore.");
      return false;
  }
#else
  if (sem_destroy(semaphore) != 0) {
      fprintf(stderr, "vrpn_Semaphore::destroy: error destroying semaphore.\n");
      return false;
  }
#endif
  semaphore = NULL;
#endif
  return true;
}

vrpn_Semaphore::~vrpn_Semaphore() {
  if (!destroy()) {
      fprintf(stderr, "vrpn_Semaphore::~vrpn_Semaphore: error destroying semaphore.\n");
  }
}

// routine to reset it
bool vrpn_Semaphore::reset( int cNumResources ) {
  cResources = cNumResources;

  // Destroy the old semaphore and then create a new one with the correct
  // value.
  if (!destroy()) {
      fprintf(stderr, "vrpn_Semaphore::reset: error destroying semaphore.\n");
      return false;
  }
  if (!init()) {
      fprintf(stderr, "vrpn_Semaphore::reset: error initializing semaphore.\n");
      return false;
  }  
  return true;
}

// routines to use it (p blocks, cond p does not)
// 1 on success, -1 fail
int vrpn_Semaphore::p() {
#ifdef sgi
  if (fUsingLock) {
    if (ussetlock(l)!=1) {
      perror("vrpn_Semaphore::p: ussetlock:");
      return -1;
    }
  } else {
    if (uspsema(ps)!=1) {
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
    ALL_ASSERT(0,"vrpn_Semaphore::p: infinite wait time timed out!");
    return -1;
    break;
  case WAIT_ABANDONED:
    ALL_ASSERT(0,"vrpn_Semaphore::p: thread holding resource died");
    return -1;
    break;
  case WAIT_FAILED:
    // get error info from windows (from FormatMessage help page)
    LPVOID lpMsgBuf;
    
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		   FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,    GetLastError(),
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		   // Default language
		   (LPTSTR) &lpMsgBuf,    0,    NULL );
    fprintf(stderr, "vrpn_Semaphore::p: error waiting for resource, "
      "WIN32 WaitForSingleObject call caused the following error: %s", (LPTSTR) lpMsgBuf);
    // Free the buffer.
    LocalFree( lpMsgBuf );
    return -1;
    break;
  default:
    ALL_ASSERT(0,"vrpn_Semaphore::p: unknown return code");
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
int vrpn_Semaphore::v() {
#ifdef sgi
  if (fUsingLock) {
    if (usunsetlock(l)) {
      perror("vrpn_Semaphore::v: usunsetlock:");
      return -1;
    }
  } else {
    if (usvsema(ps)) {
      perror("vrpn_Semaphore::v: uspsema:");
      return -1;
    }
  }
#elif defined(_WIN32)
  if (!ReleaseSemaphore(hSemaphore,1,NULL)) {
    // get error info from windows (from FormatMessage help page)
    LPVOID lpMsgBuf;
    
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		   FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,    GetLastError(),
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		     // Default language
		   (LPTSTR) &lpMsgBuf,    0,    NULL );
    fprintf(stderr, "vrpn_Semaphore::v: error v'ing semaphore, "
      "WIN32 ReleaseSemaphore call caused the following error: %s", (LPTSTR) lpMsgBuf);
    // Free the buffer.
    LocalFree( lpMsgBuf );
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
int vrpn_Semaphore::condP() {
  int iRetVal=1;
#ifdef sgi
  if (fUsingLock) {
    // don't spin at all
    iRetVal = uscsetlock(l, 0);
    if (iRetVal<=0) {
      perror("vrpn_Semaphore::condP: uscsetlock:");
      return -1;
    }
  } else {
    iRetVal = uscpsema(ps);
    if (iRetVal<=0) {
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
    iRetVal=0;
    break;
  case WAIT_ABANDONED:
    ALL_ASSERT(0,"vrpn_Semaphore::condP: thread holding resource died");
    return -1;
    break;
  case WAIT_FAILED:
    // get error info from windows (from FormatMessage help page)
    LPVOID lpMsgBuf;
    
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		   FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,    GetLastError(),
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		   // Default language
		   (LPTSTR) &lpMsgBuf,    0,    NULL );
    fprintf(stderr, "Semaphore::condP: error waiting for resource, "
      "WIN32 WaitForSingleObject call caused the following error: %s", (LPTSTR) lpMsgBuf);
    // Free the buffer.
    LocalFree( lpMsgBuf );
    return -1;
    break;
  default:
    ALL_ASSERT(0,"vrpn_Semaphore::p: unknown return code");
    return -1;
  }
#else
  // Posix by default
  iRetVal = sem_trywait(semaphore);
  if (iRetVal == 0) {  iRetVal = 1;
  } else if (errno == EAGAIN) { iRetVal = 0;
  } else {
    perror("vrpn_Semaphore::condP: ");
    iRetVal = -1;
  }
#endif
  return iRetVal;
}

int vrpn_Semaphore::numResources() {
  return cResources;
}

// static var definition
#ifdef sgi
usptr_t *vrpn_Semaphore::ppaArena = NULL;

// for umask stuff
#include <sys/types.h>
#include <sys/stat.h>

void vrpn_Semaphore::allocArena() {
  // /dev/zero is a special file which can only be shared between
  // processes/threads which share file descriptors.
  // It never shows up in the file system.
  if ((ppaArena = usinit("/dev/zero"))==NULL) {
    perror("vrpn_Thread::allocArena: usinit:");
  }
}
#endif

vrpn_Thread::vrpn_Thread(void (*pfThread)(vrpn_ThreadData &ThreadData), 
	       vrpn_ThreadData td) :
  pfThread(pfThread), td(td),
  threadID(0)
{}

bool vrpn_Thread::go() {
  if (threadID != 0) {
    fprintf(stderr, "vrpn_Thread::go: already running\n");
    return false;
  }

#ifdef sgi
  if ((threadID=sproc( &threadFuncShell, PR_SALL, this ))==
      ((unsigned long)-1)) {
    perror("vrpn_Thread::go: sproc");
    return false;
  }
// Threads not defined for the CYGWIN environment yet...
#elif defined(_WIN32) && !defined(__CYGWIN__)
  // pass in func, let it pick stack size, and arg to pass to thread
  if ((threadID=_beginthread( &threadFuncShell, 0, this )) ==
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

bool vrpn_Thread::kill() {
  // kill the os thread
#if defined(sgi) || defined(_WIN32)
  if (threadID>0) {
  #ifdef sgi
      if (::kill( (long) threadID, SIGKILL)<0) {
        perror("vrpn_Thread::kill: kill:");
        return false;
      }
  #elif defined(_WIN32)
      // Return value of -1 passed to TerminateThread causes a warning.
      if (!TerminateThread( (HANDLE) threadID, 1 )) {
        fprintf(stderr, "vrpn_Thread::kill: problem with terminateThread call.\n");
	return false;
      }
  #endif
#else
  if (threadID) {
    // Posix by default
    if (pthread_kill(threadID, SIGKILL) != 0) {
      perror("vrpn_Thread::kill:pthread_kill: ");
      return false;
    }
#endif
  } else {
    fprintf(stderr, "vrpn_Thread::kill: thread is not currently alive.\n");
    return false;
  }
  threadID = 0;
  return true;
}

bool vrpn_Thread::running() {
  return threadID!=0;
}

#if defined(sgi) || defined(_WIN32)
unsigned long vrpn_Thread::pid() {
#else
pthread_t vrpn_Thread::pid() {
#endif
  return threadID;
}

bool vrpn_Thread::available() {
#ifdef vrpn_THREADS_AVAILABLE
  return true;
#else
  return false;
#endif
}

void vrpn_Thread::userData( void *pvNewUserData ) {
  td.pvUD = pvNewUserData;
}

void *vrpn_Thread::userData() {
  return td.pvUD;
}

void vrpn_Thread::threadFuncShell( void *pvThread ) {
  vrpn_Thread *pth = (vrpn_Thread *)pvThread;
  pth->pfThread( pth->td );
  // thread has stopped running
  pth->threadID = 0;
}

// This is a Posix-compatible function prototype that
// just calls the other function.
void *vrpn_Thread::threadFuncShellPosix( void *pvThread ) {
  threadFuncShell(pvThread);
  return NULL;
}


vrpn_Thread::~vrpn_Thread() {
  if (running()) {
    kill();
  }
}

// For the code to get the number of processor cores.
#ifdef MACOSX
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

unsigned vrpn_Thread::number_of_processors() {
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
    fprintf(stderr, "vrpn_Thread::number_of_processors: Found zero, returning 1\n");
    count = 1;
  }
  return count;

#elif MACOSX
  int count;
  size_t size = sizeof(count);
  if (sysctlbyname("hw.ncpu",&count,&size,NULL,0)) {
	return 1;
 } else {
	return static_cast<unsigned>(count);
 }

#else
  fprintf(stderr, "vrpn_Thread::number_of_processors: Not yet implemented on this architecture.\n");
  return 1;
#endif
}

// Thread function to call from within vrpn_test_threads_and_semaphores().
// In this case, the userdata pointer is a pointer to a semaphore that
// the thread should call v() on so that it will free up the main program
// thread.
static void vrpn_test_thread_body(vrpn_ThreadData &threadData)
{
  // We need to p() the semaphore that protects userdata at the beginning of this
  // function and then v() it at the end, to make sure we're not racing with
  // another thread.
  threadData.udSemaphore.p();

    if (threadData.pvUD == NULL) {
      fprintf(stderr, "vrpn_test_thread_body(): pvUD is NULL\n");
      return;
    }
    vrpn_Semaphore *s = static_cast<vrpn_Semaphore *>(threadData.pvUD);
    s->v();

  // We need to p() the semaphore that protects userdata at the beginning of this
  // function and then v() it at the end, to make sure we're not racing with
  // another thread.
  threadData.udSemaphore.v();

  return;
}

bool vrpn_test_threads_and_semaphores(void)
{
  //------------------------------------------------------------
  // Make a semaphore to test in single-threaded mode.  First run its count all the way
  // down to zero, then bring it back to the full complement and then bring it down
  // again.  Check that all of the semaphores are available and also that there are no
  // more than expected available.
  const unsigned sem_count = 5;
  vrpn_Semaphore s(sem_count);
  unsigned i;
  for (i = 0; i < sem_count; i++) {
    if (s.condP() != 1) {
      fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore ran out of counts\n");
      return false;
    }
  }
  if (s.condP() != 0) {
    fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore had too many counts\n");
    return false;
  }
  for (i = 0; i < sem_count; i++) {
    if (s.v() != 0) {
      fprintf(stderr, "vrpn_test_threads_and_semaphores(): Could not release Semaphore\n");
      return false;
    }
  }
  for (i = 0; i < sem_count; i++) {
    if (s.condP() != 1) {
      fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore ran out of counts, round 2\n");
      return false;
    }
  }
  if (s.condP() != 0) {
    fprintf(stderr, "vrpn_test_threads_and_semaphores(): Semaphore had too many counts, round 2\n");
    return false;
  }

  //------------------------------------------------------------
  // Get a semaphore and use it to construct a thread data structure and then
  // a thread.  Use that thread to test whether threading is enabled (if not, then
  // this completes our testing) and to find out how many processors there are.
  vrpn_ThreadData	td;
  td.pvUD = NULL;
  vrpn_Thread	t(vrpn_test_thread_body, td);

  // If threading is not enabled, then we're done.
  if (!t.available()) {
    return true;
  }

  // Find out how many processors we have.
  unsigned num_procs = t.number_of_processors();
  if (num_procs == 0) {
    fprintf(stderr, "vrpn_test_threads_and_semaphores(): vrpn_Thread::number_of_processors() returned zero\n");
    return false;
  }

  //------------------------------------------------------------
  // Now make sure that we can actually run a thread.  Do this by
  // creating a semaphore with one entry and calling p() on it.
  // Then make sure we can't p() it again and then run a thread
  // that will call v() on it when it runs.
  vrpn_Semaphore	sem;
  if (sem.p() != 1) {
    fprintf(stderr, "vrpn_test_threads_and_semaphores(): thread-test Semaphore had no count\n");
    return false;
  }
  if (sem.condP() != 0) {
    fprintf(stderr, "vrpn_test_threads_and_semaphores(): thread-test Semaphore had too many counts\n");
    return false;
  }
  t.userData(&sem);
  if (!t.go()) {
    fprintf(stderr, "vrpn_test_threads_and_semaphores(): Could not start thread\n");
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

    // Time out after three seconds if we haven't had the thread run to reset
    // the semaphore.
    vrpn_gettimeofday(&now, NULL);
    struct timeval diff = vrpn_TimevalDiff( now, start );
    if (diff.tv_sec >= 3) {
      fprintf(stderr, "vrpn_test_threads_and_semaphores(): Thread didn't run\n");
      return false;
    }

    vrpn_SleepMsecs(1);
  }

  return true;
}

