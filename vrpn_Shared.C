#include <math.h>   // for floor, fmod
#include <stddef.h> // for size_t
#include <stdio.h>  // for fprintf() and such
#include <ctime>

#ifdef _MSC_VER
// Don't tell us about strcpy being dangerous.
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable : 4996)
#endif

#include "vrpn_Shared.h"

#if defined(__APPLE__) || defined(__ANDROID__)
#include <unistd.h>
#endif

#if !(defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS))
#include <sys/select.h> // for select
#include <netinet/in.h> // for htonl, htons
#endif

#define CHECK(a) \
    if (a == -1) return -1

#if defined(VRPN_USE_WINSOCK_SOCKETS)
/* from HP-UX */
struct timezone {
	int tz_minuteswest; /* minutes west of Greenwich */
	int tz_dsttime;     /* type of dst correction */
};
#endif

// perform normalization of a timeval
// XXX this still needs to be checked for errors if the timeval
// or the rate is negative
static inline void timevalNormalizeInPlace(timeval &in_tv)
{
    const long div_77777 = (in_tv.tv_usec / 1000000);
    in_tv.tv_sec += div_77777;
    in_tv.tv_usec -= (div_77777 * 1000000);
}
timeval vrpn_TimevalNormalize(const timeval &in_tv)
{
    timeval out_tv = in_tv;
    timevalNormalizeInPlace(out_tv);
    return out_tv;
}

// Calcs the sum of tv1 and tv2.  Returns the sum in a timeval struct.
// Calcs negative times properly, with the appropriate sign on both tv_sec
// and tv_usec (these signs will match unless one of them is 0).
// NOTE: both abs(tv_usec)'s must be < 1000000 (ie, normal timeval format)
timeval vrpn_TimevalSum(const timeval &tv1, const timeval &tv2)
{
    timeval tvSum = tv1;

    tvSum.tv_sec += tv2.tv_sec;
    tvSum.tv_usec += tv2.tv_usec;

    // do borrows, etc to get the time the way i want it: both signs the same,
    // and abs(usec) less than 1e6
    if (tvSum.tv_sec > 0) {
        if (tvSum.tv_usec < 0) {
            tvSum.tv_sec--;
            tvSum.tv_usec += 1000000;
        }
        else if (tvSum.tv_usec >= 1000000) {
            tvSum.tv_sec++;
            tvSum.tv_usec -= 1000000;
        }
    }
    else if (tvSum.tv_sec < 0) {
        if (tvSum.tv_usec > 0) {
            tvSum.tv_sec++;
            tvSum.tv_usec -= 1000000;
        }
        else if (tvSum.tv_usec <= -1000000) {
            tvSum.tv_sec--;
            tvSum.tv_usec += 1000000;
        }
    }
    else {
        // == 0, so just adjust usec
        if (tvSum.tv_usec >= 1000000) {
            tvSum.tv_sec++;
            tvSum.tv_usec -= 1000000;
        }
        else if (tvSum.tv_usec <= -1000000) {
            tvSum.tv_sec--;
            tvSum.tv_usec += 1000000;
        }
    }

    return tvSum;
}

// Calcs the diff between tv1 and tv2.  Returns the diff in a timeval struct.
// Calcs negative times properly, with the appropriate sign on both tv_sec
// and tv_usec (these signs will match unless one of them is 0)
timeval vrpn_TimevalDiff(const timeval &tv1, const timeval &tv2)
{
    timeval tv;

    tv.tv_sec = -tv2.tv_sec;
    tv.tv_usec = -tv2.tv_usec;

    return vrpn_TimevalSum(tv1, tv);
}

timeval vrpn_TimevalScale(const timeval &tv, double scale)
{
    timeval result;
    result.tv_sec = (long)(tv.tv_sec * scale);
    result.tv_usec =
        (long)(tv.tv_usec * scale + fmod(tv.tv_sec * scale, 1.0) * 1000000.0);
    timevalNormalizeInPlace(result);
    return result;
}

// returns 1 if tv1 is greater than tv2;  0 otherwise
bool vrpn_TimevalGreater(const timeval &tv1, const timeval &tv2)
{
    if (tv1.tv_sec > tv2.tv_sec) return 1;
    if ((tv1.tv_sec == tv2.tv_sec) && (tv1.tv_usec > tv2.tv_usec)) return 1;
    return 0;
}

// return 1 if tv1 is equal to tv2; 0 otherwise
bool vrpn_TimevalEqual(const timeval &tv1, const timeval &tv2)
{
    if (tv1.tv_sec == tv2.tv_sec && tv1.tv_usec == tv2.tv_usec)
        return true;
    else
        return false;
}

unsigned long vrpn_TimevalDuration(struct timeval endT, struct timeval startT)
{
    return (endT.tv_usec - startT.tv_usec) +
           1000000L * (endT.tv_sec - startT.tv_sec);
}

double vrpn_TimevalDurationSeconds(struct timeval endT, struct timeval startT)
{
    return (endT.tv_usec - startT.tv_usec) / 1000000.0 +
           (endT.tv_sec - startT.tv_sec);
}

double vrpn_TimevalMsecs(const timeval &tv)
{
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

timeval vrpn_MsecsTimeval(const double dMsecs)
{
    timeval tv;
    tv.tv_sec = (long)floor(dMsecs / 1000.0);
    tv.tv_usec = (long)((dMsecs / 1000.0 - tv.tv_sec) * 1e6);
    return tv;
}

// Sleep for dMsecs milliseconds, freeing up the processor while you
// are doing so.

void vrpn_SleepMsecs(double dMilliSecs)
{
#if defined(_WIN32)
    Sleep((DWORD)dMilliSecs);
#else
    timeval timeout;

    // Convert milliseconds to seconds
    timeout.tv_sec = (int)(dMilliSecs / 1000.0);

    // Subtract of whole number of seconds
    dMilliSecs -= timeout.tv_sec * 1000;

    // Convert remaining milliseconds to microsec
    timeout.tv_usec = (int)(dMilliSecs * 1000);

    // A select() with NULL file descriptors acts like a microsecond
    // timer.
    select(0, 0, 0, 0, &timeout); // wait for that long;
#endif
}

// convert vrpn_float64 to/from network order
// I have chosen big endian as the network order for vrpn_float64
// to match the standard for htons() and htonl().
// NOTE: There is an added complexity when we are using an ARM
// processor in mixed-endian mode for the doubles, whereby we need
// to not just swap all of the bytes but also swap the two 4-byte
// words to get things in the right order.
#if defined(__arm__)
#include <endian.h>
#endif

vrpn_float64 vrpn_htond(vrpn_float64 d)
{
    if (!vrpn_big_endian) {
        vrpn_float64 dSwapped;
        char *pchSwapped = (char *)&dSwapped;
        char *pchOrig = (char *)&d;

        // swap to big-endian order.
        unsigned i;
        for (i = 0; i < sizeof(vrpn_float64); i++) {
            pchSwapped[i] = pchOrig[sizeof(vrpn_float64) - i - 1];
        }

#if defined(__arm__) && !defined(__ANDROID__)
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
    }
    else {
        return d;
    }
}

// they are their own inverses, so ...
vrpn_float64 vrpn_ntohd(vrpn_float64 d) { return vrpn_htond(d); }

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

VRPN_API int vrpn_buffer(char **insertPt, vrpn_int32 *buflen, const timeval t)
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

VRPN_API int vrpn_buffer(char **insertPt, vrpn_int32 *buflen,
                         const char *string, vrpn_int32 length)
{
    if (length > *buflen) {
        fprintf(stderr, "vrpn_buffer:  buffer not long enough for string.\n");
        return -1;
    }

    if (length == -1) {
        size_t len =
            strlen(string) + 1; // +1 for the NULL terminating character
        if (len > (unsigned)*buflen) {
            fprintf(stderr,
                    "vrpn_buffer:  buffer not long enough for string.\n");
            return -1;
        }
        strcpy(*insertPt, string);
        *insertPt += len;
        *buflen -= static_cast<vrpn_int32>(len);
    }
    else {
        memcpy(*insertPt, string, length);
        *insertPt += length;
        *buflen -= length;
    }

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

VRPN_API int vrpn_unbuffer(const char **buffer, timeval *t)
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

    If the length is specified as less than zero, then the string will be
   assumed to be NULL-terminated and will be read using the string-copy
   routines with a length that is at most the magnitude of the number
   (-16 means at most 16).
     NEVER use this on a string that was packed with other than the
   NULL-terminating condition, since embedded NULL characters will ruin the
   argument parsing for any later arguments in the message.
*/

VRPN_API int vrpn_unbuffer(const char **buffer, char *string, vrpn_int32 length)
{
    if (!string) return -1;

    if (length < 0) {
        // Read the string up to maximum length, then check to make sure we
        // found the null-terminator in the length we read.
        size_t max_len = static_cast<size_t>(-length);
        strncpy(string, *buffer, max_len);
        size_t i;
        bool found = false;
        for (i = 0; i < max_len; i++) {
            if (string[i] == '\0') {
                found = true;
                break;
            }
        }
        if (!found) {
            return -1;
        }
        *buffer += strlen(*buffer) + 1; // +1 for NULL terminating character
    } else {
        memcpy(string, *buffer, length);
        *buffer += length;
    }

    return 0;
}

//=====================================================================
// This section contains various implementations of vrpn_gettimeofday().
//   Which one is selected depends on various #defines.  There is a second
// section that deals with handling various configurations on Windows.
//   The first section deals with the fact that we may want to use the
// std::chrono classes introduced in C++-11 as a cross-platform (even
// Windows) solution to timing.  If VRPN_USE_STD_CHRONO is defined, then
// we do this -- converting from chrono epoch and interval into the
// gettimeofday() standard tick of microseconds and epoch start of
// midnight, January 1, 1970.

///////////////////////////////////////////////////////////////
// Implementation with std::chrono follows, and overrides any of
// the Windows-specific definitions if it is present.
///////////////////////////////////////////////////////////////

#ifdef VRPN_USE_STD_CHRONO
#include <chrono>

///////////////////////////////////////////////////////////////
// With Visual Studio 2013 64-bit, the hires clock produces a clock that has a
// tick interval of around 15.6 MILLIseconds, repeating the same
// time between them.
///////////////////////////////////////////////////////////////
// With Visual Studio 2015 64-bit, the hires clock produces a good, high-
// resolution clock with no blips.  However, its epoch seems to
// restart when the machine boots, whereas the system clock epoch
// starts at the standard midnight January 1, 1970.
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// Helper function to convert from the high-resolution clock
// time to the equivalent system clock time (assuming no clock
// adjustment on the system clock since program start).
//  To make this thread safe, we semaphore the determination of
// the offset to be applied.  To handle a slow-ticking system
// clock, we repeatedly sample it until we get a change.
//  This assumes that the high-resolution clock on different
// threads has the same epoch.
///////////////////////////////////////////////////////////////

static bool hr_offset_determined = false;
static vrpn_Semaphore hr_offset_semaphore;
static struct timeval hr_offset;

static struct timeval high_resolution_time_to_system_time(
    struct timeval hi_res_time //< Time computed from high-resolution clock
    )
{
    // If we haven't yet determined the offset between the high-resolution
    // clock and the system clock, do so now.  Avoid a race between threads
    // using the semaphore and checking the boolean both before and after
    // grabbing the semaphore (in case someone beat us to it).
    if (!hr_offset_determined) {
        hr_offset_semaphore.p();
        // Someone else who had the semaphore may have beaten us to this.
        if (!hr_offset_determined) {
            // Watch the system clock until it changes; this will put us
            // at a tick boundary.  On many systems, this will change right
            // away, but on Windows 8 it will only tick every 16ms or so.
            std::chrono::system_clock::time_point pre =
                std::chrono::system_clock::now();
            std::chrono::system_clock::time_point post;
            // On Windows 8.1, this took from 1-16 ticks, and seemed to
            // get offsets to the epoch that were consistent to within
            // around 1ms.
            do {
                post = std::chrono::system_clock::now();
            } while (pre == post);

            // Now read the high-resolution timer to find out the time
            // equivalent to the post time on the system clock.
            std::chrono::high_resolution_clock::time_point high =
                std::chrono::high_resolution_clock::now();

            // Now convert both the hi-resolution clock time and the
            // post-tick system clock time into struct timevals and
            // store the difference between them as the offset.
            std::time_t high_secs =
                std::chrono::duration_cast<std::chrono::seconds>(
                    high.time_since_epoch())
                    .count();
            std::chrono::high_resolution_clock::time_point
                fractional_high_secs = high - std::chrono::seconds(high_secs);
            struct timeval high_time;
            high_time.tv_sec = static_cast<unsigned long>(high_secs);
            high_time.tv_usec = static_cast<unsigned long>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    fractional_high_secs.time_since_epoch())
                    .count());

            std::time_t post_secs =
                std::chrono::duration_cast<std::chrono::seconds>(
                    post.time_since_epoch())
                    .count();
            std::chrono::system_clock::time_point fractional_post_secs =
                post - std::chrono::seconds(post_secs);
            struct timeval post_time;
            post_time.tv_sec = static_cast<unsigned long>(post_secs);
            post_time.tv_usec = static_cast<unsigned long>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    fractional_post_secs.time_since_epoch())
                    .count());

            hr_offset = vrpn_TimevalDiff(post_time, high_time);

            // We've found our offset ... re-use it from here on.
            hr_offset_determined = true;
        }
        hr_offset_semaphore.v();
    }

    // The offset has been determined, by us or someone else.  Apply it.
    return vrpn_TimevalSum(hi_res_time, hr_offset);
}

int vrpn_gettimeofday(timeval *tp, void *tzp)
{
    // If we have nothing to fill in, don't try.
    if (tp == NULL) {
        return 0;
    }
	struct timezone *timeZone = reinterpret_cast<struct timezone *>(tzp);

    // Find out the time, and how long it has been in seconds since the
    // epoch.
    std::chrono::high_resolution_clock::time_point now =
        std::chrono::high_resolution_clock::now();
    std::time_t secs =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();

    // Subtract the time in seconds from the full time to get a
    // remainder that is a fraction of a second since the epoch.
    std::chrono::high_resolution_clock::time_point fractional_secs =
        now - std::chrono::seconds(secs);

    // Store the seconds and the fractional seconds as microseconds into
    // the timeval structure.  Then convert from the hi-res clock time
    // to system clock time.
    struct timeval hi_res_time;
    hi_res_time.tv_sec = static_cast<unsigned long>(secs);
    hi_res_time.tv_usec = static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            fractional_secs.time_since_epoch())
            .count());
    *tp = high_resolution_time_to_system_time(hi_res_time);

    // @todo Fill in timezone structure with relevant info.
    if (timeZone != NULL) {
		timeZone->tz_minuteswest = 0;
		timeZone->tz_dsttime = 0;
    }

    return 0;
}

#else // VRPN_USE_STD_CHRONO

///////////////////////////////////////////////////////////////
// Implementation without std::chrono follows.
///////////////////////////////////////////////////////////////

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
#include <math.h> // for floor, fmod

#pragma optimize("", on)

#ifdef _WIN32
void get_time_using_GetLocalTime(unsigned long &sec, unsigned long &usec)
{
	SYSTEMTIME stime;   // System time in funky structure
	FILETIME ftime;     // Time in 100-nsec intervals since Jan 1 1601
	ULARGE_INTEGER tics; // ftime stored into a 64-bit quantity

	GetLocalTime(&stime);
	SystemTimeToFileTime(&stime, &ftime);

	// Copy the data into a structure that can be treated as a 64-bit integer
	tics.HighPart = ftime.dwHighDateTime;
	tics.LowPart = ftime.dwLowDateTime;

	// Change units (100 nanoseconds --> microseconds)
	tics.QuadPart /= 10;
	
	// Subtract the offset between the two clock bases (Jan 1, 1601 --> Jan 1, 1970)
	tics.QuadPart -= 11644473600000000ULL;

	// Convert the 64-bit time into seconds and microseconds since Jan 1 1970
	sec = (unsigned long)(tics.QuadPart / 1000000UL);
	usec = (unsigned long)(tics.QuadPart % 1000000UL);
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
int vrpn_gettimeofday(timeval *tp, void *tzp)
{
	struct timezone *timeZone = reinterpret_cast<struct timezone *>(tzp);

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
        tp->tv_usec = (long)t.millitm * 1000;
#endif
    }
    if (tzp != NULL) {
        TIME_ZONE_INFORMATION tz;
        GetTimeZoneInformation(&tz);
		timeZone->tz_minuteswest = tz.Bias;
		timeZone->tz_dsttime = (tz.StandardBias != tz.Bias);
    }
    return 0;
}

#endif // defined(_WIN32)

#else // In the case that VRPN_UNSAFE_WINDOWS_CLOCK is defined

// Although VC++ doesn't include a gettimeofday
// call, Cygnus Solutions Cygwin32 environment does.
// XXX AND ITS WRONG in the current release 10/11/99, version b20.1
// They claim it will be fixed in the next release, version b21
// so until then, we will make it right using our solution.
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <math.h> // for floor, fmod

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
#define rdtsc(li)                                                              \
    {                                                                          \
        _asm _emit 0x0f _asm _emit 0x31 _asm mov li.LowPart,                   \
            eax _asm mov li.HighPart, edx                                      \
    }

/*
 * calculate the time stamp counter register frequency (clock freq)
 */
#ifndef VRPN_WINDOWS_CLOCK_V2
#pragma optimize("", off)
static int vrpn_AdjustFrequency(void)
{
    const int loops = 2;
    const int tPerLoop = 500; // milliseconds for Sleep()
    fprintf(stderr, "vrpn vrpn_gettimeofday: determining clock frequency...");

    LARGE_INTEGER startperf, endperf;
    LARGE_INTEGER perffreq;

    // See if the hardware supports the high-resolution performance counter.
    // If so, get the frequency of it.  If not, we can't use it and so return
    // -1.
    if (QueryPerformanceFrequency(&perffreq) == 0) {
        return -1;
    }

    // don't optimize away these variables
    double sum = 0;
    volatile LARGE_INTEGER liStart, liEnd;

    DWORD dwPriorityClass = GetPriorityClass(GetCurrentProcess());
    int iThreadPriority = GetThreadPriority(GetCurrentThread());
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // pull all into cache and do rough test to see if tsc and perf counter
    // are one and the same
    rdtsc(liStart);
    QueryPerformanceCounter(&startperf);
    Sleep(100);
    rdtsc(liEnd);
    QueryPerformanceCounter(&endperf);

    double freq = perffreq.QuadPart * (liEnd.QuadPart - liStart.QuadPart) /
                  ((double)(endperf.QuadPart - startperf.QuadPart));

    if (fabs(perffreq.QuadPart - freq) < 0.05 * freq) {
        VRPN_CLOCK_FREQ = (__int64)perffreq.QuadPart;
        fprintf(stderr, "\nvrpn vrpn_gettimeofday: perf clock is tsc -- using "
                        "perf clock freq ( %lf MHz)\n",
                perffreq.QuadPart / 1e6);
        SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
        SetThreadPriority(GetCurrentThread(), iThreadPriority);
        return 0;
    }

    // either tcs and perf clock are not the same, or we could not
    // tell accurately enough with the short test. either way we now
    // need an accurate frequency measure, so ...

    fprintf(stderr, " (this will take %lf seconds)...\n",
            loops * tPerLoop / 1000.0);

    for (int j = 0; j < loops; j++) {
        rdtsc(liStart);
        QueryPerformanceCounter(&startperf);
        Sleep(tPerLoop);
        rdtsc(liEnd);
        QueryPerformanceCounter(&endperf);

        // perf counter timer ran for one call to Query and one call to
        // tcs read in addition to the time between the tsc readings
        // tcs read did the same

        // endperf - startperf / perf freq = time between perf queries
        // endtsc - starttsc = clock ticks between perf queries
        //    sum += (endtsc - starttsc) / ((double)(endperf -
        //    startperf)/perffreq);
        sum += perffreq.QuadPart * (liEnd.QuadPart - liStart.QuadPart) /
               ((double)(endperf.QuadPart - startperf.QuadPart));
    }

    SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
    SetThreadPriority(GetCurrentThread(), iThreadPriority);

    // might want last, not sum -- just get into cache and run
    freq = (sum / loops);

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
    if (fabs(perffreq.QuadPart - freq) < 0.05 * freq) {
        VRPN_CLOCK_FREQ = perffreq.QuadPart;
        fprintf(stderr, "vrpn vrpn_gettimeofday: perf clock is tsc -- using "
                        "perf clock freq ( %lf MHz)\n",
                perffreq.QuadPart / 1e6);
    }
    else {
        fprintf(stderr, "vrpn vrpn_gettimeofday: adjusted clock freq to "
                        "measured freq ( %lf MHz )\n",
                freq / 1e6);
    }
    VRPN_CLOCK_FREQ = (__int64)freq;
    return 0;
}
#pragma optimize("", on)
#endif

// The pc has no gettimeofday call, and the closest thing to it is _ftime.
// _ftime, however, has only about 6 ms resolution, so we use the peformance
// as an offset from a base time which is established by a call to by _ftime.

// The first call to vrpn_gettimeofday will establish a new time frame
// on which all later calls will be based.  This means that the time returned
// by vrpn_gettimeofday will not always match _ftime (even at _ftime's
// resolution),
// but it will be consistent across all vrpn_gettimeofday calls.

///////////////////////////////////////////////////////////////
// Although VC++ doesn't include a gettimeofday
// call, Cygnus Solutions Cygwin32 environment does,
// so this is not used when compiling with gcc under WIN32

// XXX AND ITS WRONG in the current release 10/11/99
// They claim it will be fixed in the next release,
// so until then, we will make it right using our solution.
///////////////////////////////////////////////////////////////
#ifndef VRPN_WINDOWS_CLOCK_V2
int vrpn_gettimeofday(timeval *tp, void *tzp)
{
	struct timezone *timeZone = reinterpret_cast<struct timezone *>(tzp);
	static int fFirst = 1;
    static int fHasPerfCounter = 1;
    static struct _timeb tbInit;
    static LARGE_INTEGER liInit;
    static LARGE_INTEGER liNow;
    static LARGE_INTEGER liDiff;
    timeval tvDiff;

    if (!fHasPerfCounter) {
        _ftime(&tbInit);
        tp->tv_sec = tbInit.time;
        tp->tv_usec = tbInit.millitm * 1000;
        return 0;
    }

    if (fFirst) {
        LARGE_INTEGER liTemp;
        // establish a time base
        fFirst = 0;

        // Check to see if we are on a Windows NT machine (as opposed to a
        // Windows 95/98 machine).  If we are not, then use the _ftime code
        // because the hi-perf clock does not work under Windows 98SE on my
        // laptop, although the query for one seems to indicate that it is
        // available.

        {
            OSVERSIONINFO osvi;

            memset(&osvi, 0, sizeof(OSVERSIONINFO));
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&osvi);

            if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) {
                fprintf(stderr,
                        "\nvrpn_gettimeofday: disabling hi performance clock "
                        "on non-NT system. "
                        "Defaulting to _ftime (~6 ms resolution) ...\n");
                fHasPerfCounter = 0;
                vrpn_gettimeofday(tp, tzp);
                return 0;
            }
        }

        // check that hi-perf clock is available
        if (!(fHasPerfCounter = QueryPerformanceFrequency(&liTemp))) {
            fprintf(stderr,
                    "\nvrpn_gettimeofday: no hi performance clock available. "
                    "Defaulting to _ftime (~6 ms resolution) ...\n");
            fHasPerfCounter = 0;
            vrpn_gettimeofday(tp, tzp);
            return 0;
        }

        if (vrpn_AdjustFrequency() < 0) {
            fprintf(stderr,
                    "\nvrpn_gettimeofday: can't verify clock frequency. "
                    "Defaulting to _ftime (~6 ms resolution) ...\n");
            fHasPerfCounter = 0;
            vrpn_gettimeofday(tp, tzp);
            return 0;
        }
        // get current time
        // We assume this machine has a time stamp counter register --
        // I don't know of an easy way to check for this
        rdtsc(liInit);
        _ftime(&tbInit);

        // we now consider it to be exactly the time _ftime returned
        // (beyond the resolution of _ftime, down to the perfCounter res)
    }

    // now do the regular get time call to get the current time
    rdtsc(liNow);

    // find offset from initial value
    liDiff.QuadPart = liNow.QuadPart - liInit.QuadPart;

    tvDiff.tv_sec = (long)(liDiff.QuadPart / VRPN_CLOCK_FREQ);
    tvDiff.tv_usec =
        (long)(1e6 * ((liDiff.QuadPart - VRPN_CLOCK_FREQ * tvDiff.tv_sec) /
                      (double)VRPN_CLOCK_FREQ));

    // pack the value and clean it up
    tp->tv_sec = tbInit.time + tvDiff.tv_sec;
    tp->tv_usec = tbInit.millitm * 1000 + tvDiff.tv_usec;
    while (tp->tv_usec >= 1000000) {
        tp->tv_sec++;
        tp->tv_usec -= 1000000;
    }

    return 0;
}
#else // VRPN_WINDOWS_CLOCK_V2 is defined

void get_time_using_GetLocalTime(unsigned long &sec, unsigned long &usec)
{
    static LARGE_INTEGER first_count = {0, 0};
    static unsigned long first_sec, first_usec;
    static LARGE_INTEGER perf_freq; //< Frequency of the performance counter.
    SYSTEMTIME stime;               // System time in funky structure
    FILETIME ftime;     // Time in 100-nsec intervals since Jan 1 1601
    LARGE_INTEGER tics; // ftime stored into a 64-bit quantity
    LARGE_INTEGER perf_counter;

    // The first_count value will be zero only the first time through; we
    // rely on this to set up the structures needed to interpret the data
    // that we get from querying the performance counter.
    if (first_count.QuadPart == 0) {
        QueryPerformanceCounter(&first_count);

        // Find out how fast the performance counter runs.  Store this for later
        // runs.
        QueryPerformanceFrequency(&perf_freq);

        // Find out what time it is in a Windows structure.
        GetLocalTime(&stime);
        SystemTimeToFileTime(&stime, &ftime);

        // Copy the data into a structure that can be treated as a 64-bit
        // integer
        tics.HighPart = ftime.dwHighDateTime;
        tics.LowPart = ftime.dwLowDateTime;

        // Convert the 64-bit time into seconds and microseconds since July 1
        // 1601
        sec = (long)(tics.QuadPart / 10000000L);
        usec = (long)((tics.QuadPart - (((LONGLONG)(sec)) * 10000000L)) / 10);
        first_sec = sec;
        first_usec = usec;
    }
    else {
        QueryPerformanceCounter(&perf_counter);
        if (perf_counter.QuadPart >= first_count.QuadPart) {
            perf_counter.QuadPart =
                perf_counter.QuadPart - first_count.QuadPart;
        }
        else {
            // Take care of the case when the counter rolls over.
            perf_counter.QuadPart = 0x7fffffffffffffffLL -
                                    first_count.QuadPart +
                                    perf_counter.QuadPart;
        }

        // Reinterpret the performance counter into seconds and microseconds
        // by dividing by the performance counter.  Microseconds is placed
        // into perf_counter by subtracting the seconds value out, then
        // multiplying by 1 million and re-dividing by the performance counter.
        sec = (long)(perf_counter.QuadPart / perf_freq.QuadPart);
        perf_counter.QuadPart -= perf_freq.QuadPart * sec;
        perf_counter.QuadPart *= 1000000L; //< Turn microseconds into seconds
        usec = first_usec + (long)(perf_counter.QuadPart / perf_freq.QuadPart);
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

int vrpn_gettimeofday(timeval *tp, void *tzp)
{
	struct timezone *timeZone = reinterpret_cast<struct timezone *>(tzp);
    unsigned long sec, usec;
    get_time_using_GetLocalTime(sec, usec);
    tp->tv_sec = sec;
    tp->tv_usec = usec;
    if (tzp != NULL) {
        TIME_ZONE_INFORMATION tz;
        GetTimeZoneInformation(&tz);
        timeZone->tz_minuteswest = tz.Bias;
        timeZone->tz_dsttime = (tz.StandardBias != tz.Bias);
    }
    return 0;
}

#endif // defined(VRPN_WINDOWS_CLOCK_V2)

#endif // defined(_WIN32)

// do the calibration before the program ever starts up
static timeval __tv;
static int __iTrash = vrpn_gettimeofday(&__tv, (struct timezone *)NULL);

#endif // VRPN_UNSAFE_WINDOWS_CLOCK

#endif // VRPN_USE_STD_CHRONO

// End of the section dealing with vrpn_gettimeofday()
//=====================================================================

bool vrpn_test_pack_unpack(void)
{
    // Get a buffer to use that is large enough to test all of the routines.
    vrpn_float64 dbuffer[256];
    vrpn_int32 buflen;

    vrpn_float64 in_float64 = 42.1;
    vrpn_int32 in_int32 = 17;
    vrpn_uint16 in_uint16 = 397;
    vrpn_uint8 in_uint8 = 1;

    vrpn_float64 out_float64;
    vrpn_int32 out_int32;
    vrpn_uint16 out_uint16;
    vrpn_uint8 out_uint8;

    // Test packing using little-endian routines.
    // IMPORTANT: Do these from large to small to get good alignment.
    char *bufptr = (char *)dbuffer;
    buflen = sizeof(dbuffer);
    if (vrpn_buffer_to_little_endian(&bufptr, &buflen, in_float64) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer little endian\n");
        return false;
    }
    if (vrpn_buffer_to_little_endian(&bufptr, &buflen, in_int32) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer little endian\n");
        return false;
    }
    if (vrpn_buffer_to_little_endian(&bufptr, &buflen, in_uint16) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer little endian\n");
        return false;
    }
    if (vrpn_buffer_to_little_endian(&bufptr, &buflen, in_uint8) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer little endian\n");
        return false;
    }

    // Test unpacking using little-endian routines.
    bufptr = (char *)dbuffer;
    if (in_float64 !=
        (out_float64 =
             vrpn_unbuffer_from_little_endian<vrpn_float64>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer little endian\n");
        return false;
    }
    if (in_int32 !=
        (out_int32 = vrpn_unbuffer_from_little_endian<vrpn_int32>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer little endian\n");
        return false;
    }
    if (in_uint16 !=
        (out_uint16 = vrpn_unbuffer_from_little_endian<vrpn_uint16>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer little endian\n");
        return false;
    }
    if (in_uint8 !=
        (out_uint8 = vrpn_unbuffer_from_little_endian<vrpn_uint8>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer little endian\n");
        return false;
    }

    // Test packing using big-endian routines.
    bufptr = (char *)dbuffer;
    buflen = sizeof(dbuffer);
    if (vrpn_buffer(&bufptr, &buflen, in_float64) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer big endian\n");
        return false;
    }
    if (vrpn_buffer(&bufptr, &buflen, in_int32) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer big endian\n");
        return false;
    }
    if (vrpn_buffer(&bufptr, &buflen, in_uint16) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer big endian\n");
        return false;
    }
    if (vrpn_buffer(&bufptr, &buflen, in_uint8) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer big endian\n");
        return false;
    }

    // Test unpacking using big-endian routines.
    bufptr = (char *)dbuffer;
    if (in_float64 != (out_float64 = vrpn_unbuffer<vrpn_float64>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer big endian\n");
        return false;
    }
    if (in_int32 != (out_int32 = vrpn_unbuffer<vrpn_int32>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer big endian\n");
        return false;
    }
    if (in_uint16 != (out_uint16 = vrpn_unbuffer<vrpn_uint16>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer big endian\n");
        return false;
    }
    if (in_uint8 != (out_uint8 = vrpn_unbuffer<vrpn_uint8>(bufptr))) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not unbuffer big endian\n");
        return false;
    }

    // XXX Test pack/unpack of all other types.

    // Test packing little-endian and unpacking big-endian; they should
    // be different.
    bufptr = (char *)dbuffer;
    buflen = sizeof(dbuffer);
    if (vrpn_buffer_to_little_endian(&bufptr, &buflen, in_float64) != 0) {
        fprintf(stderr,
                "vrpn_test_pack_unpack(): Could not buffer little endian\n");
        return false;
    }
    bufptr = (char *)dbuffer;
    if (in_float64 == (out_float64 = vrpn_unbuffer<vrpn_float64>(bufptr))) {
        fprintf(
            stderr,
            "vrpn_test_pack_unpack(): Cross-packing produced same result\n");
        return false;
    }

    return true;
}
