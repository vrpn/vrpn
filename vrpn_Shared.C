#include "vrpn_Shared.h"

#ifdef _WIN32
#include <iomanip.h>
#endif

#include <stdio.h>
#include <math.h>
#include <sys/types.h>

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
int vrpn_TimevalGreater (const timeval & tv1, const timeval & tv2)
{
    if (tv1.tv_sec > tv2.tv_sec) return 1;
    if ((tv1.tv_sec == tv2.tv_sec) &&
        (tv1.tv_usec > tv2.tv_usec)) return 1;
    return 0;
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
#if defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS)
    Sleep(DWORD(dMsecs));
#else
    timeval timeout;

    // Convert milliseconds to seconds
    timeout.tv_sec = (int)(dMsecs / 1000);

    // Subtract of whole number of seconds
    dMsecs -= timeout.tv_sec * 1000;

    // Convert remaining milliseconds to microsec
    timeout.tv_usec = (int)(dMsecs * 1000);
    
    // A select() with NULL file descriptors acts like a microsecond
    // timer.
    select(0, 0, 0, 0, & timeout);  // wait for that long;
#endif
}



static long lTestEndian = 0x1;
static const int fLittleEndian = (*((char *)&lTestEndian) == 0x1);

// convert vrpn_float64 to/from network order
// I have chosen big endian as the network order for vrpn_float64
vrpn_float64 htond( vrpn_float64 d )
{
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
    int length = sizeof(netValue);

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
    int length = sizeof(netValue);

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
    int length = sizeof(netValue);

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
    int length = sizeof(netValue);

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
  vrpn_int32 length = sizeof(char);

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
    vrpn_int32 length = sizeof(netValue);

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

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <iostream.h>
#include <math.h>

#pragma optimize("", on)

///////////////////////////////////////////////////////////////
// Although VC++ doesn't include a gettimeofday
// call, Cygnus Solutions Cygwin32 environment does,
// so this is not used when compiling with gcc under WIN32.
// XXX Note that the cygwin one was wrong in an earlier
// version.  It is claimed that they have fixed it now, but
// better check.
///////////////////////////////////////////////////////////////
int gettimeofday(timeval *tp, struct timezone *tzp)
{
    if (tp != NULL) {
            struct _timeb t;
            _ftime(&t);
            tp->tv_sec = t.time;
            tp->tv_usec = (long)t. millitm * 1000;
    }
    if (tzp != NULL) {
            TIME_ZONE_INFORMATION tz;
            GetTimeZoneInformation(&tz);
            tzp->tz_minuteswest = tz.Bias ;
            tzp->tz_dsttime = (tz.StandardBias != tz.Bias);
    }
    return 0;}

#endif //defined(_WIN32)
