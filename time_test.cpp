#include <Windows.h>
#include <Winbase.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>

/*
void	get_time_using_GetLocalTime(unsigned long &sec, unsigned long &usec)
{
    SYSTEMTIME	stime;	    // System time in funky structure
    FILETIME	ftime;	    // Time in 100-nsec intervals since Jan 1 1601
    LARGE_INTEGER   tics;   // ftime stored into a 64-bit quantity

    GetLocalTime(&stime);
    SystemTimeToFileTime(&stime, &ftime);
    tics.HighPart = ftime.dwHighDateTime;
    tics.LowPart = ftime.dwLowDateTime;
    sec = (long)( tics.QuadPart / 10000000L );
    usec = (long)( ( tics.QuadPart - ( ((LONGLONG)(sec)) * 10000000L ) ) / 10 );
}
*/

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
    sec -= 3054524608L;
}

void	get_time_using_ftime(unsigned long &sec, unsigned long &usec)
{
    struct _timeb t;
    _ftime(&t);
    sec = t.time;
    usec = (long)t. millitm * 1000;
}

int main(int argc, char *argv[]) {
    unsigned long    lsec, lusec;
    unsigned long    fsec, fusec;
    long    dsec, dusec;
    int	    i;

    for (i = 0; i < 10; i++) {
	get_time_using_GetLocalTime(lsec, lusec);
	get_time_using_ftime(fsec, fusec);
	dsec = lsec - fsec;
	dusec = lusec - fusec;

	printf("L: %u:%u, F: %u:%u, Difference: %u:%ld\n",
	    lsec, lusec, fsec, fusec, dsec, dusec);

	Sleep(1000);
    }

    return 0;
}