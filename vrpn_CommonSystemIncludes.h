
#ifndef VRPN_COMMONSYSTEMINCLUDES_H
#define VRPN_COMMONSYSTEMINCLUDES_H


//--------------------------------------
// bunch of includes from Connection.C
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream.h>
#include <sys/stat.h>
#include <fcntl.h>


#ifndef FreeBSD
// malloc.h is deprecated in FreeBSD;  all the functionality *should*
// be in stdlib.h
#include <malloc.h>
#endif

#ifdef _WIN32
#include <winsock.h>
#include <process.h>
// a socket in windows can not be closed like it can in unix-land
#define close closesocket
#else
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef  sgi
#include <bstring.h>
#endif

#ifdef hpux
#include <arpa/nameser.h>
#include <resolv.h>  // for herror() - but it isn't there?
#endif

#ifdef _WIN32
#include <winsock.h>
#include <sys/timeb.h>
#else
#include <sys/wait.h>
#include <sys/resource.h>  // for wait3() on sparc_solaris
#include <netinet/tcp.h>
#endif


// cast fourth argument to setsockopt()
#ifdef _WIN32
#define SOCK_CAST (char *)
#else
 #ifdef sparc
#define SOCK_CAST (const char *)
 #else
#define SOCK_CAST
 #endif
#endif

#ifdef linux
#define GSN_CAST (unsigned int *)
#else
#define GSN_CAST
#endif

//  NOT SUPPORTED ON SPARC_SOLARIS
//  gethostname() doesn't seem to want to link out of stdlib
#ifdef sparc
extern "C" {
int gethostname (char *, int);
}
#endif

#endif // VRPN_COMMONSYSTEMINCLUDES_H
