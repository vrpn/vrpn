#ifdef _WIN32
#include <sys/timeb.h>
#include <winsock.h>   // timeval is defined here
#endif

#include "vrpn_Shared.h"


#ifdef _WIN32
int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	struct _timeb timeb;
	_ftime(&timeb);
	tp->tv_sec  = timeb.time;
	tp->tv_usec = timeb.millitm*1000;  // milli to micro secs
	return 0;
}
#endif
