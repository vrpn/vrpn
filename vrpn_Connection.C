#include "vrpn_Connection.h"

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <string.h>  // for strerror()
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

#ifdef VRPN_USE_WINSOCK_SOCKETS
//XXX #include <winsock.h>
// a socket in windows can not be closed like it can in unix-land
#define close closesocket
#else
#include <unistd.h>
  // gethostname() and getdtablesize() should be here on SGIs,
  // but apparently aren't under g++
#include <strings.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // for inet_addr()?
#ifdef _AIX
#define _USE_IRS
#endif
#include <netdb.h>
#endif

#ifdef	sparc
#include <arpa/inet.h>
#define INADDR_NONE -1
#endif

#ifdef	sgi
#include <bstring.h>
#endif

#ifdef hpux
#include <arpa/nameser.h>
#include <resolv.h>  // for herror() - but it isn't there?
#endif

#ifndef VRPN_USE_WINSOCK_SOCKETS
#include <sys/wait.h>
#include <sys/resource.h>  // for wait3() on sparc_solaris
#ifndef __CYGWIN__
#include <netinet/tcp.h>
#endif /* __CYGWIN__ */
#endif /* VRPN_USE_WINSOCK_SOCKETS */

// cast fourth argument to setsockopt()
#ifdef VRPN_USE_WINSOCK_SOCKETS
  #define SOCK_CAST (char *)
#else
  #ifdef sparc
    #define SOCK_CAST (const char *)
  #else
    #define SOCK_CAST
  #endif
#endif

#ifdef _AIX
 #define GSN_CAST (socklen_t *)
#else
 #ifdef linux
  #define GSN_CAST (unsigned int *)
 #else
  #define GSN_CAST
 #endif
#endif

//  NOT SUPPORTED ON SPARC_SOLARIS
//  gethostname() doesn't seem to want to link out of stdlib
#ifdef sparc
extern "C" {
int gethostname (char *, int);
}
#endif

//XXX #include "vrpn_Connection.h"
#include "vrpn_Clock.h"  // for vrpn_Synchronized_Connection
#include "vrpn_FileConnection.h"  // for vrpn_get_connection_by_name

//#define	VERBOSE
//#define	VERBOSE2
//#define	VERBOSE3
//#define	PRINT_READ_HISTOGRAM

//   Warning:  PRINT_READ_HISTOGRAM is not thread-safe.

// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#ifndef	VRPN_USE_WINSOCK_SOCKETS
#define	INVALID_SOCKET	-1
#endif

// Syntax of MAGIC:
//   The minor version number must follow the last period ('.') and be
// the only significant thing following the last period in the MAGIC
// string.  Since minor versions should interoperate, MAGIC is only
// checked through the last period;  characters after that are ignored.

const char * vrpn_MAGIC = (const char *) "vrpn: ver. 04.12";
const int vrpn_MAGICLEN = 16;  // Must be a multiple of vrpn_ALIGN bytes!

// This is the list of states that a connection can be in
// (possible values for status).  doing_okay() returns VRPN_TRUE
// for connections >= TRYING_TO_CONNECT.
#define LISTEN			(1)
#define CONNECTED		(0)
#define COOKIE_PENDING          (-1)
#define TRYING_TO_CONNECT	(-2)
#define BROKEN			(-3)
#define DROPPED			(-4)



//**********************************************************************
//**  This section has been pulled from the "SDI" library and had its
//**  functions renamed to vrpn_ from sdi_.  This removes our dependence
//**  on libsdi.a for VRPN.

#ifdef sparc

// On capefear and swift, getdtablesize() isn't declared in unistd.h
// even though the man page says it should be.  Similarly, wait3()
// isn't declared in sys/{wait,time,resource}.h.
extern "C" {
    extern int getdtablesize (void);
    pid_t wait3 (int * statusp, int options, struct rusage * rusage);
}
#endif

/* On HP's, this defines how many possible open file descriptors can be
 * open at once.  This is what is returned by the getdtablesize() function
 * on other architectures. */
#ifdef  hpux
#define getdtablesize() MAXFUPLIM
#endif

#ifdef __hpux
#define getdtablesize() MAXFUPLIM
#endif

/* The version of rsh in /usr/local/bin is the AFS version that passes tokens
 * to the remote machine.  This will allow remote execution of anything you
 * can execute locally.  This is the default location from which to get rsh.
 * If the VRPN_RSH environment variable is set, that will be used as the full
 * path instead.  */
#ifdef  linux
#define RSH             (char *) "/usr/local/bin/ssh"
#else
#define RSH             (char *) "/usr/local/bin/rsh"
#endif

/* How long to wait for a UDP packet to cause a callback connection,
 * and how many times to retry. */
#define	UDP_CALL_TIMEOUT	(2)
#define	UDP_CALL_RETRIES	(5)

/* How long to wait for the server to connect, and how many times to wait
 * this long.  The death of the child is checked for between each of the
 * waits, in order to allow faster exit if the child quits before calling
 * back. */
#define SERVCOUNT       (20)
#define SERVWAIT        (120/SERVCOUNT)


//
// vrpn_ConnectionManager
// Singleton class that keeps track of all known VRPN connections
// and makes sure they're deleted on shutdown.
// We make it static to guarantee that the destructor is called
// on program close so that the destructors of all the vrpn_Connections
// that have been allocated are called so that all open logs are flushed
// to disk.
//

//      This section holds data structures and functions to open
// connections by name.
//      The intention of this section is that it can open connections for
// objects that are in different libraries (trackers, buttons and sound),
// even if they all refer to the same connection.


class vrpn_ConnectionManager {

  public:

    ~vrpn_ConnectionManager (void);

    static vrpn_ConnectionManager & instance (void);
      // The only way to get access to an instance of this class.
      // Guarantees that there is only one, global object.
      // Also guarantees that it will be constructed the first time
      // this function is called, and (hopefully?) destructed when
      // the program terminates.

    void addConnection (vrpn_Connection *, const char * name);
    void deleteConnection (vrpn_Connection *);
      // NB implementation is not particularly efficient;  we expect
      // to have O(10) connections, not O(1000).

    vrpn_Connection * getByName (const char * name);
      // Searches through d_kcList but NOT d_anonList
      // (Connections constructed with no name)

  private:

    struct knownConnection {
      char name [1000];
      vrpn_Connection * connection;
      knownConnection * next;
    };

    knownConnection * d_kcList;
      // named connections

    knownConnection * d_anonList;
      // unnamed (server) connections

    vrpn_ConnectionManager (void);

    vrpn_ConnectionManager (const vrpn_ConnectionManager &);
      // copy constructor undefined to prevent instantiations

    static void deleteConnection (vrpn_Connection *, knownConnection **);
};

vrpn_ConnectionManager::~vrpn_ConnectionManager (void) {

  //fprintf(stderr, "In ~vrpn_ConnectionManager:  tearing down the list.\n");

  // Call the destructor of every known connection.
  // That destructor will call vrpn_ConnectionManager::deleteConnection()
  // to remove itself from d_kcList.
  while (d_kcList) {
    delete d_kcList->connection;
  }
  while (d_anonList) {
    delete d_anonList->connection;
  }

}

// static
vrpn_ConnectionManager & vrpn_ConnectionManager::instance (void) {
  static vrpn_ConnectionManager manager;
  return manager;
}

void vrpn_ConnectionManager::addConnection (vrpn_Connection * c,
                                            const char * name) {
  knownConnection * p;

  p = new knownConnection;
  p->connection = c;

  if (name) {
    strncpy(p->name, name, 1000);
    p->next = d_kcList;
    d_kcList = p;
  } else {
    p->name[0] = 0;
    p->next = d_anonList;
    d_anonList = p;
  }
}

void vrpn_ConnectionManager::deleteConnection (vrpn_Connection * c) {
  deleteConnection(c, &d_kcList);
  deleteConnection(c, &d_anonList);
}

void vrpn_ConnectionManager::deleteConnection (vrpn_Connection * c,
                                               knownConnection ** snitch) {
  knownConnection * victim = *snitch;

  while (victim && (victim->connection != c)) {
    snitch = &((*snitch)->next);
    victim = *snitch;
  }

  if (!victim) {
    // No warning, because this connection might be on the *other* list.
  } else {
    *snitch = victim->next;
    delete victim;
  }
}

vrpn_Connection * vrpn_ConnectionManager::getByName (const char * name) {
  knownConnection * p;
  for (p = d_kcList; p && strcmp(p->name, name); p = p->next) {
    // do nothing
  }
  if (!p) {
    return NULL;
  }
  return p->connection;
}

vrpn_ConnectionManager::vrpn_ConnectionManager (void) :
    d_kcList (NULL),
    d_anonList (NULL) {

}




/**********************
 *   This function returns the host IP address in string form.  For example,
 * the machine "ioglab.cs.unc.edu" becomes "152.2.130.90."  This is done
 * so that the remote host can connect back even if it can't resolve the
 * DNS name of this host.  This is especially useful at conferences, where
 * DNS may not even be running.
 **********************/

static int	vrpn_getmyIP (char * myIPchar, int maxlen,
                              char * nameP = NULL,
                              const char * NIC_IP = NULL)
{	
	char	myname[100];		// Host name of this host
        struct	hostent *host;          // Encoded host IP address, etc.
	char	myIPstring[100];	// Hold "152.2.130.90" or whatever

        if (NIC_IP) {
          if (strlen(NIC_IP) > maxlen) {
            fprintf(stderr,"vrpn_getmyIP: Name too long to return\n");
            return -1;
          }
          strcpy(myIPchar, NIC_IP);
          return 0;
        }

	// Find out what my name is
	if (gethostname(myname, sizeof(myname))) {
		fprintf(stderr, "vrpn_getmyIP: Error finding local hostname\n");
		return -1;
	}

	// Find out what my IP address is
	if ( (host = gethostbyname(myname)) == NULL ) {
		fprintf(stderr, "vrpn_getmyIP: error finding host by name\n");
		return -1;
	}

	// Convert this back into a string
#ifndef CRAY
	if (host->h_length != 4) {
		fprintf(stderr, "vrpn_getmyIP: Host length not 4\n");
		return -1;
	}
#endif
	sprintf(myIPstring, "%u.%u.%u.%u",
		(unsigned int)(unsigned char)host->h_addr_list[0][0],
		(unsigned int)(unsigned char)host->h_addr_list[0][1],
		(unsigned int)(unsigned char)host->h_addr_list[0][2],
		(unsigned int)(unsigned char)host->h_addr_list[0][3]);

	// Copy this to the output
	if (myIPchar == NULL) {
		fprintf(stderr,"vrpn_getmyIP: NULL pointer passed in\n");
		return -1;
	}
	if ((int)strlen(myIPstring) > maxlen) {
		fprintf(stderr,"vrpn_getmyIP: Name too long to return\n");
		return -1;
	}

	strcpy(myIPchar, myIPstring);
	return 0;
}


/**********************
 *	This routine will perform like a normal select() call, but it will
 * restart if it quit because of an interrupt.  This makes it more robust
 * in its function, and allows this code to perform properly on pxpl5, which
 * sends USER1 interrupts while rendering an image.
 **********************/
int vrpn_noint_select(int width, fd_set *readfds, fd_set *writefds, 
		     fd_set *exceptfds, struct timeval * timeout)
{
	fd_set	tmpread, tmpwrite, tmpexcept;
	int	ret;
	int	done = 0;
        struct	timeval timeout2;
        struct	timeval *timeout2ptr;
	struct	timeval	start, stop, now;
	struct	timezone zone;

	/* If the timeout parameter is non-NULL and non-zero, then we
	 * may have to adjust it due to an interrupt.  In these cases,
	 * we will copy the timeout to timeout2, which will be used
	 * to keep track.  Also, the current time is found so that we
	 * can track elapsed time. */
	if ( (timeout != NULL) && 
	     ((timeout->tv_sec != 0) || (timeout->tv_usec != 0)) ) {
		timeout2 = *timeout;
		timeout2ptr = &timeout2;
		gettimeofday(&start, &zone);	/* Find start time */
		//time_add(start,*timeout,stop);	/* Find stop time */
		stop = vrpn_TimevalSum(start, *timeout);/* Find stop time */
	} else {
		timeout2ptr = timeout;
	}

	/* Repeat selects until it returns for a reason other than interrupt */
	do {
		/* Set the temp file descriptor sets to match parameters */
		if (readfds != NULL) {
			tmpread = *readfds;
		} else {
			FD_ZERO(&tmpread);
		}
		if (writefds != NULL) {
			tmpwrite = *writefds;
		} else {
			FD_ZERO(&tmpwrite);
		}
		if (exceptfds != NULL) {
			tmpexcept = *exceptfds;
		} else {
			FD_ZERO(&tmpexcept);
		}

		/* Do the select on the temporary sets of descriptors */
		ret = select(width, &tmpread,&tmpwrite,&tmpexcept, timeout2ptr);
		if (ret >= 0) {	/* We are done if timeout or found some */
			done = 1;
		} else if (errno != EINTR) {	/* Done if non-intr error */
			done = 1;
		} else if ( (timeout != NULL) &&
		  ((timeout->tv_sec != 0) || (timeout->tv_usec != 0))) {

		    /* Interrupt happened.  Find new time timeout value */
		    gettimeofday(&now, &zone);
		    if (vrpn_TimevalGreater(now, stop)) {/* Past stop time */
			done = 1;
		    } else {	/* Still time to go. */
			unsigned long	usec_left;
			usec_left = (stop.tv_sec - now.tv_sec) * 1000000L;
			usec_left += stop.tv_usec - now.tv_usec;
			timeout2.tv_sec = usec_left / 1000000L;
			timeout2.tv_usec = usec_left % 1000000L;
		    }
		}
	} while (!done);

	/* Copy the temporary sets back to the parameter sets */
	if (readfds != NULL) {
		*readfds = tmpread;
	}
	if (writefds != NULL) {
		*writefds = tmpwrite;
	}
	if (exceptfds != NULL) {
		*exceptfds = tmpexcept;
	}

	return(ret);
}


/*      This routine will write a block to a file descriptor.  It acts just
 * like the write() system call does on files, but it will keep sending to
 * a socket until an error or all of the data has gone.
 *      This will also take care of problems caused by interrupted system
 * calls, retrying the write when they occur.  It will also work when
 * sending large blocks of data through socket connections, since it will
 * send all of the data before returning.
 *	This routine will either write the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data is sent). */

#ifndef VRPN_USE_WINSOCK_SOCKETS

int vrpn_noint_block_write (int outfile, const char buffer[], int length)
{
        register int    sofar;          /* How many characters sent so far */
        register int    ret;            /* Return value from write() */

        sofar = 0;
        do {
                /* Try to write the remaining data */
                ret = write(outfile, buffer+sofar, length-sofar);
                sofar += ret;

                /* Ignore interrupted system calls - retry */
                if ( (ret == -1) && (errno == EINTR) ) {
                        ret = 1;	/* So we go around the loop again */
                        sofar += 1;	/* Restoring it from above -1 */
                }

        } while ( (ret > 0) && (sofar < length) );

        if (ret == -1) return(-1);	/* Error during write */
	if (ret == 0) return(0);	/* EOF reached */

        return(sofar);			/* All bytes written */
}


/*      This routine will read in a block from the file descriptor.
 * It acts just like the read() routine does on normal files, so that
 * it hides the fact that the descriptor may point to a socket.
 *      This will also take care of problems caused by interrupted system
 * calls, retrying the read when they occur.
 *	This routine will either read the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data arrives). */

int vrpn_noint_block_read(int infile, char buffer[], int length)
{
        register int    sofar;          /* How many we read so far */
        register int    ret;            /* Return value from the read() */

  // TCH 4 Jan 2000 - hackish - Cygwin will block forever on a 0-length
  // read(), and from the man pages this is close enough to in-spec that
  // other OS may do the same thing.

  if (!length) {
    return 0;
  }
        sofar = 0;
        do {
		/* Try to read all remaining data */
                ret = read(infile, buffer+sofar, length-sofar);
                sofar += ret;

                /* Ignore interrupted system calls - retry */
                if ( (ret == -1) && (errno == EINTR) ) {
                        ret = 1;	/* So we go around the loop again */
                        sofar += 1;	/* Restoring it from above -1 */
                }
        } while ((ret > 0) && (sofar < length));

        if (ret == -1) return(-1);	/* Error during read */
	if (ret == 0) return(0);	/* EOF reached */

        return(sofar);			/* All bytes read */
}

#else /* winsock sockets */

int vrpn_noint_block_write(SOCKET outsock, char *buffer, int length)
{
	int nwritten, sofar = 0;
	do {
		    /* Try to write the remaining data */
		nwritten = send(outsock, buffer+sofar, length-sofar, 0);

		if (nwritten == SOCKET_ERROR) {
			return -1;
        }

		sofar += nwritten;		
	} while ( sofar < length );
	
	return(sofar);			/* All bytes written */
}

int vrpn_noint_block_read(SOCKET insock, char *buffer, int length)
{
    int nread, sofar = 0;  

  // TCH 4 Jan 2000 - hackish - Cygwin will block forever on a 0-length
  // read(), and from the man pages this is close enough to in-spec that
  // other OS may do the same thing.

  if (!length) {
    return 0;
  }

    do {
            /* Try to read all remaining data */
        nread = recv(insock, buffer+sofar, length-sofar, 0);

		if (nread == SOCKET_ERROR) {
            return -1;
        }
		if (nread == 0) {   /* socket closed */
			return 0;
		}
        
        sofar += nread;        
    } while (sofar < length);
	    
    return(sofar);			/* All bytes read */
}


#endif /* VRPN_USE_WINSOCK_SOCKETS */


/*	This routine will read in a block from the file descriptor.
 * It acts just like the read() routine on normal files, except that
 * it will time out if the read takes too long.
 *      This will also take care of problems caused by interrupted system
 * calls, retrying the read when they occur.
 *	This routine will either read the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data arrives), or return the number
 * of characters read before timeout (in the case of a timeout). */

#ifdef VRPN_USE_WINSOCK_SOCKETS
int vrpn_noint_block_read_timeout(SOCKET infile, char buffer[], 
				 int length, struct timeval *timeout)
#else
int vrpn_noint_block_read_timeout(int infile, char buffer[], 
				 int length, struct timeval *timeout)
#endif
{
        register int    sofar;          /* How many we read so far */
        register int    ret;            /* Return value from the read() */
        struct	timeval timeout2;
        struct	timeval *timeout2ptr;
	struct	timeval	start, stop, now;
	struct	timezone zone;

  // TCH 4 Jan 2000 - hackish - Cygwin will block forever on a 0-length
  // read(), and from the man pages this is close enough to in-spec that
  // other OS may do the same thing.

  if (!length) {
    return 0;
  }

	/* If the timeout parameter is non-NULL and non-zero, then we
	 * may have to adjust it due to an interrupt.  In these cases,
	 * we will copy the timeout to timeout2, which will be used
	 * to keep track.  Also, the current time is found so that we
	 * can track elapsed time. */
	if ( (timeout != NULL) && 
	     ((timeout->tv_sec != 0) || (timeout->tv_usec != 0)) ) {
		timeout2 = *timeout;
		timeout2ptr = &timeout2;
		gettimeofday(&start, &zone);	/* Find start time */
		stop = vrpn_TimevalSum(start, *timeout);/* Find stop time */
	} else {
		timeout2ptr = timeout;
	}

        sofar = 0;
        do {	
		int	sel_ret;
		fd_set	readfds, exceptfds;

		/* See if there is a character ready for read */
		FD_ZERO(&readfds);
		FD_SET(infile, &readfds);
		FD_ZERO(&exceptfds);
		FD_SET(infile, &exceptfds);
		sel_ret = vrpn_noint_select(32, &readfds, NULL, &exceptfds,
					   timeout2ptr);
		if (sel_ret == -1) {	/* Some sort of error on select() */
			return -1;
		}
		if (FD_ISSET(infile, &exceptfds)) {	/* Exception */
			return -1;
		}
		if (!FD_ISSET(infile, &readfds)) {	/* No characters */
			if ( (timeout != NULL) &&
			     (timeout->tv_sec == 0) &&
			     (timeout->tv_usec == 0) ) {	/* Quick poll */
				return sofar;	/* Timeout! */
			}
		}

		/* See what time it is now and how long we have to go */
		if (timeout2ptr) {
			gettimeofday(&now, &zone);
			if (vrpn_TimevalGreater(now, stop)) {	/* Timeout! */
				return sofar;
			} else {
				timeout2 = vrpn_TimevalDiff(stop, now);
			}
		}

		if (!FD_ISSET(infile, &readfds)) {	/* No chars yet */
			continue;
		}

		/* Try to read one character if there is one */
		/* We read one at a time because that's all that we are
		 * guaranteed to have available.  On a socket, this is
		 * not a problem because it won't block, but on a file it
		 * will. */
#ifndef VRPN_USE_WINSOCK_SOCKETS
                ret = read(infile, buffer+sofar, 1);
                sofar += ret;

                /* Ignore interrupted system calls - retry */
                if ( (ret == -1) && (errno == EINTR) ) {
                        ret = 1;	/* So we go around the loop again */
                        sofar += 1;	/* Restoring it from above -1 */
                }
#else
                {
                    int nread;
                    nread = recv(infile, buffer+sofar, length-sofar, 0);
                    sofar += nread;
                    ret = nread;
                }
#endif

        } while ((ret > 0) && (sofar < length));
#ifndef VRPN_USE_WINSOCK_SOCKETS
        if (ret == -1) return(-1);	/* Error during read */
#endif
        if (ret == 0) return(0);	/* EOF reached */

        return(sofar);			/* All bytes read */
}

//---------------------------------------------------------------------------
// This section deals with implementing a method of connection termed a
// UDP request.  This works by having the client open a TCP socket that
// it listens on. It then lobs datagrams to the server asking to be
// called back at the socket. This allows it to timeout on waiting for
// a connection request, resend datagrams in case some got lost, or give
// up at any time. The whole algorithm is implemented in the
// vrpn_udp_request_call() function; the functions before that are helper
// functions that have been broken out to allow a subset of the algorithm
// to be run by a connection whose server has dropped and they want to
// re-establish it.

// This routine will lob a datagram to the given port on the given
// machine asking it to call back at the port on this machine that
// is also specified. It returns 0 on success and -1 on failure.

int vrpn_udp_request_lob_packet(
		const char * machine,	// Name of the machine to call
		const int remote_port,	// UDP port on remote machine
		const int local_port,	// TCP port on this machine
                const char * NIC_IP = NULL)
{
	SOCKET	udp_sock;		/* We lob datagrams from here */
	struct sockaddr_in udp_name;	/* The UDP socket binding name */
	int	udp_namelen;
        struct	hostent *host;          /* The host to connect to */
	char	msg[150];		/* Message to send */
	vrpn_int32	msglen;		/* How long it is (including \0) */
	char	myIPchar[100];		/* IP decription this host */

	/* Create a UDP socket and connect it to the port on the remote
	 * machine. */

        udp_sock = socket(AF_INET,SOCK_DGRAM,0);
        if (udp_sock < 0) {
	    fprintf(stderr,"vrpn_udp_request_lob_packet: can't open udp socket().\n");
            return(-1);
        }
	udp_name.sin_family = AF_INET;
	udp_namelen = sizeof(udp_name);
	host = gethostbyname(machine);
        if (host) {

#ifdef CRAY

          { int i;
            u_long foo_mark = 0;
            for  (i = 0; i < 4; i++) {
                u_long one_char = host->h_addr_list[0][i];
                foo_mark = (foo_mark << 8) | one_char;
            }
            udp_name.sin_addr.s_addr = foo_mark;
          }

#else


	  memcpy(&(udp_name.sin_addr.s_addr), host->h_addr,  host->h_length);
#endif

        } else {

#ifdef VRPN_USE_WINDOWS_GETHOSTBYNAME_HACK

// gethostbyname() fails on SOME Windows NT boxes, but not all,
// if given an IP octet string rather than a true name.
// Until we figure out WHY, we have this extra clause in here.
// It probably wouldn't hurt to enable it for non-NT systems
// as well.

            int retval;
            retval = sscanf(machine, "%u.%u.%u.%u",
                            ((char *) &udp_name.sin_addr.s_addr)[0],
                            ((char *) &udp_name.sin_addr.s_addr)[1],
                            ((char *) &udp_name.sin_addr.s_addr)[2],
                            ((char *) &udp_name.sin_addr.s_addr)[3]);
            
            if (retval != 4) {
                
#endif  // VRPN_USE_WINDOWS_GETHOSTBYNAME_HACK

                // Note that this is the failure clause of gethostbyname() on
                // non-WIN32 systems, but of the sscanf() on WIN32 systems.
                
		close(udp_sock);
		fprintf(stderr,
			"vrpn_udp_request_lob_packet: error finding host by name\n");
		return(-1);
                
#ifdef VRPN_USE_WINDOWS_GETHOSTBYNAME_HACK
            }
#endif
        }
        
#ifndef VRPN_USE_WINSOCK_SOCKETS
	udp_name.sin_port = htons(remote_port);
#else
	udp_name.sin_port = htons((u_short)remote_port);
#endif
        if ( connect(udp_sock,(struct sockaddr*)&udp_name,udp_namelen) ) {
	    fprintf(stderr,"vrpn_udp_request_lob_packet: can't bind udp socket().\n");
	    close(udp_sock);
	    return(-1);
        }

	/* Fill in the request message, telling the machine and port that
	 * the remote server should connect to.  These are ASCII, separated
	 * by a space. */

	if (vrpn_getmyIP(myIPchar, sizeof(myIPchar), NULL, NIC_IP)) {
		fprintf(stderr,
		   "vrpn_udp_request_lob_packet: Error finding local hostIP\n");
		close(udp_sock);
		return(-1);
	}
	sprintf(msg, "%s %d", myIPchar, local_port);
	msglen = strlen(msg) + 1;	/* Include the terminating 0 char */

	// Lob the message
	if (send(udp_sock, msg, msglen, 0) == -1) {
	  perror("vrpn_udp_request_lob_packet: send() failed");
	  close(udp_sock);
	  return -1;
	}

	close(udp_sock);	// We're done with the port
	return 0;
}

// This routine will get a TCP socket that is ready to accept connections.
// That is, listen() has already been called on it.
// It will get whatever socket is available from the system. It returns
// 0 on success and -1 on failure. On success, it fills in the pointers to
// the socket and the port number of the socket that it obtained.
// To select between multiple network interfaces, we can specify an IPaddress;
// the default value is NULL, which uses the default NIC.


int vrpn_get_a_TCP_socket (SOCKET * listen_sock, int * listen_portnum,
                           const char * IPaddress = NULL)
{
	struct sockaddr_in listen_name;	/* The listen socket binding name */
	int	listen_namelen;

	/* Create a TCP socket to listen for incoming connections from the
	 * remote server. */

  struct hostent *phe;           /* pointer to host information entry   */

  memset((void *) &listen_name, 0, sizeof(listen_name));
  //bzero ((char *) &listen_name, sizeof(listen_name));
  listen_name.sin_family = AF_INET;

  // Map service name to port number
  listen_name.sin_port = htons(0);

  // Map host name to IP address, allowing for dotted decimal
  if (!IPaddress) {
    listen_name.sin_addr.s_addr = INADDR_ANY;
  } else if (phe = gethostbyname(IPaddress)) {
    memcpy((void *)&listen_name.sin_addr, (const void *) phe->h_addr, phe->h_length);
    //bcopy(phe->h_addr, (char *)&listen_name.sin_addr, phe->h_length);
  } else if ((listen_name.sin_addr.s_addr = inet_addr(IPaddress))
              == INADDR_NONE) {
    printf("can't get %s host entry\n", IPaddress);
    return -1;
  }

  listen_namelen = sizeof(listen_name);

  // We're using TCP
  if ((*listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "vrpn_get_a_TCP_socket:  can't create socket.\n");
    fprintf(stderr, "  -- errno %d (%s).\n", errno, strerror(errno));
    return -1;
  }
  // Bind the socket
  if (bind (*listen_sock, (struct sockaddr *) &listen_name,
            listen_namelen) < 0) {
    fprintf(stderr, "vrpn_get_a_TCP_socket:  can't bind socket.\n");
    fprintf(stderr, "  -- errno %d (%s).\n", errno, strerror(errno));
    return -1;
  }


        if (getsockname(*listen_sock,
	                (struct sockaddr *) &listen_name,
                        GSN_CAST &listen_namelen)) {
		fprintf(stderr,
			"vrpn_get_a_TCP_socket: cannot get socket name.\n");
                close(*listen_sock);
                return(-1);
        }
	*listen_portnum = ntohs(listen_name.sin_port);
	if ( listen(*listen_sock,2) ) {
		fprintf(stderr,"vrpn_get_a_TCP_socket: listen() failed.\n");
		close(*listen_sock);
		return(-1);
	}
//fprintf(stderr, "Listening on port %d, address %d %d %d %d.\n",
//*listen_portnum, listen_name.sin_addr.s_addr >> 24,
//(listen_name.sin_addr.s_addr >> 16) & 0xff,
//(listen_name.sin_addr.s_addr >> 8) & 0xff,
//listen_name.sin_addr.s_addr & 0xff);

	return 0;
}

// This routine will check the listen socket to see if there has been a
// connection request. If so, it will accept a connection on the accept
// socket and set TCP_NODELAY on that socket. The attempt will timeout
// in the amount of time specified.  If the accept and set are
// successfull, it returns 1. If there is nothing asking for a connection,
// it returns 0. If there is an error along the way, it returns -1.

int vrpn_poll_for_accept(SOCKET listen_sock, SOCKET *accept_sock, double timeout = 0.0)
{
	fd_set	rfds;
	struct	timeval t;

	// See if we have a connection attempt within the timeout
	FD_ZERO(&rfds);
	FD_SET(listen_sock, &rfds);	/* Check for read (connect) */
	t.tv_sec = (long)(timeout);
	t.tv_usec = (long)( (timeout - t.tv_sec) * 1000000L );
	if (select(32, &rfds, NULL, NULL, &t) == -1) {
	  perror("vrpn_poll_for_accept: select() failed");
	  return -1;
	}
	if (FD_ISSET(listen_sock, &rfds)) {	/* Got one! */
		/* Accept the connection from the remote machine and set TCP_NODELAY
		* on the socket. */
		if ( (*accept_sock = accept(listen_sock,0,0)) == -1 ) {
			perror("vrpn_poll_for_accept: accept() failed");
			return -1;
		}

		{	struct	protoent	*p_entry;
			int	nonzero = 1;

			if ( (p_entry = getprotobyname("TCP")) == NULL ) {
				fprintf(stderr,
				"vrpn_poll_for_accept: getprotobyname() failed.\n");
				close(*accept_sock);
				return(-1);
			}
	
			if (setsockopt(*accept_sock, p_entry->p_proto,
			TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
				perror("vrpn_poll_for_accept: setsockopt() failed");
				close(*accept_sock);
				return(-1);
			}
		}

		return 1;	// Got one!
	}

	return 0;	// Nobody called
}

#if 0
  -- obsolete ??  never called!

SOCKET vrpn_udp_request_call (const char * machine, int port,
                              const char * IPaddress)

#endif  // 0

// This is like sdi_start_server except that the convention for
// passing information on the client machine to the server program is 
// different; everything else has been left the same
/**********************************
 *      This routine will start up a given server on a given machine.  The
 * name of the machine requesting service and the socket number to connect to
 * are supplied after the -client argument. The "args" parameters are
 * passed next.  They should be space-separated arguments, each of which is
 * passed separately.  It is NOT POSSIBLE to send parameters with embedded
 * spaces.
 *      This routine returns a file descriptor that points to the socket
 * to the server on success and -1 on failure.
 **********************************/
int vrpn_start_server(const char * machine, char * server_name, char * args,
                      const char * IPaddress = NULL)
{
#ifdef  VRPN_USE_WINSOCK_SOCKETS
        fprintf(stderr,"VRPN: vrpn_start_server not ported"
                " for windows winsocks!\n");
        return -1;
#else
        int     pid;    /* Child's process ID */
        int     server_sock;    /* Where the accept returns */
        int     child_socket;   /* Where the final socket is */
        int     PortNum;        /* Port number we got */

        /* Open a socket and insure we can bind it */
	if (vrpn_get_a_TCP_socket(&server_sock, &PortNum, IPaddress)) {
		fprintf(stderr,"vrpn_start_server: Cannot get listen socket\n");
		return -1;
	}

        if ( (pid = fork()) == -1) {
                fprintf(stderr,"vrpn_start_server: cannot fork().\n");
                close(server_sock);
                return(-1);
        }
        if (pid == 0) {  /* CHILD */
                int     loop;
                int     ret;
                int     num_descriptors;/* Number of available file descr */
                char    myIPchar[100];    /* Host name of this host */
                char    command[600];   /* Command passed to system() call */
                char    *rsh_to_use;    /* Full path to Rsh command. */

                if (vrpn_getmyIP(myIPchar,sizeof(myIPchar), NULL, IPaddress)) {
                        fprintf(stderr,
                           "vrpn_start_server: Error finding my IP\n");
                        close(server_sock);
                        return(-1);
                }

                /* Close all files except stdout and stderr. */
                /* This prevents a hung child from keeping devices open */
                num_descriptors = getdtablesize();
                for (loop = 0; loop < num_descriptors; loop++) {
                  if ( (loop != 1) && (loop != 2) ) {
                        close(loop);
                  }
                }

                /* Find the RSH command, either from the environment
                 * variable or the default, and use it to start up the
                 * remote server. */

                if ( (rsh_to_use =(char *)getenv("VRPN_RSH")) == NULL) {
                        rsh_to_use = RSH;
                }
                sprintf(command,"%s %s %s %s -client %s %d",rsh_to_use, machine,
                        server_name, args, myIPchar, PortNum);
                ret = system(command);
                if ( (ret == 127) || (ret == -1) ) {
                        fprintf(stderr,
                                "vrpn_start_server: system() failed !!!!!\n");
                        perror("Error");
                        fprintf(stderr, "Attempted command was: '%s'\n",
                                command);
                        close(server_sock);
                        exit(-1);  /* This should never occur */
                }
                exit(0);

        } else {  /* PARENT */
                int     waitloop;

                /* Check to see if the child
                 * is trying to call us back.  Do SERVCOUNT waits, each of
                 * which is SERVWAIT long.
                 * If the child dies while we are waiting, then we can be
                 * sure that they will not be calling us back.  Check for
                 * this while waiting for the callback. */

                for (waitloop = 0; waitloop < (SERVCOUNT); waitloop++) {
		    int ret;
                    pid_t deadkid;
#if defined(sparc) || defined(FreeBSD) || defined(_AIX)
                    int status;  // doesn't exist on sparc_solaris or FreeBSD
#else
                    union wait status;
#endif
		    
                    /* Check to see if they called back yet. */
		    ret = vrpn_poll_for_accept(server_sock, &child_socket, SERVWAIT);
		    if (ret == -1) {
			    fprintf(stderr,"vrpn_start_server: Accept poll failed\n");
			    close(server_sock);
			    return -1;
		    }
		    if (ret == 1) {
			    break;	// Got it!
		    }

                    /* Check to see if the child is dead yet */
#if defined(hpux) || defined(sgi) || defined(__hpux)
                    /* hpux include files have the wrong declaration */
                    deadkid = wait3((int*)&status, WNOHANG, NULL);
#else
                    deadkid = wait3(&status, WNOHANG, NULL);
#endif
                    if (deadkid == pid) {
                        fprintf(stderr,
                                "vrpn_start_server: server process exited\n");
                        close(server_sock);
                        return(-1);
                    }
                }
		if (waitloop == SERVCOUNT) {
			fprintf(stderr,
                        "vrpn_start_server: server failed to connect in time\n");
                    fprintf(stderr,
                        "                  (took more than %d seconds)\n",
                        SERVWAIT*SERVCOUNT);
                    close(server_sock);
                    kill(pid,SIGKILL);
                    wait(0);
                    return(-1);
                }

                close(server_sock);
                return(child_socket);
        }
        return 0;
#endif
}


//**  End of section pulled from SDI library
//*********************************************************************

// COOKIE MANIPULATION

// Writes the magic cookie into buffer with given length.
// On failure (if the buffer is too short), returns -1;
// otherwise returns 0.

// This is exposed in vrpn_Connection.h so that we can write
// add_vrpn_cookie.

int write_vrpn_cookie (char * buffer, int length, long remote_log_mode)
{
  if (length < vrpn_MAGICLEN + vrpn_ALIGN + 1)
    return -1;

  sprintf(buffer, "%s  %c", vrpn_MAGIC, remote_log_mode + '0');
  return 0;
}

// Checks to see if the given buffer has the magic cookie.
// Returns -1 on a mismatch, 0 on an exact match,
// 1 on a minor version difference.

int check_vrpn_cookie (const char * buffer)
{
  char * bp;

  // Comparison changed 9 Feb 98 by TCH
  //   We don't care if the minor version numbers don't match,
  // so only check the characters through the last '.' in our
  // template.  (If there is no last '.' in our template, somebody's
  // modified this code to break the constraints above, and we just
  // use a maximally restrictive check.)
  //   XXX This pointer arithmetic isn't completely safe.

  bp = strrchr(buffer, '.');
  if (strncmp(buffer, vrpn_MAGIC,
      (bp == NULL ? vrpn_MAGICLEN : bp + 1 - buffer))) {
    fprintf(stderr, "check_vrpn_cookie:  "
            "bad cookie (wanted '%s', got '%s'\n", vrpn_MAGIC, buffer);
    return -1;
  }

  if (strncmp(buffer, vrpn_MAGIC, vrpn_MAGICLEN)) {
    fprintf(stderr, "vrpn_Connection: "
        "Warning:  minor version number doesn't match: (prefer '%s', got '%s')\n",
	    vrpn_MAGIC, buffer);
    return 1;
  }

  return 0;
}

int vrpn_cookie_size (void) {
  return vrpn_MAGICLEN + vrpn_ALIGN;
}

// END OF COOKIE (-CUTTER?) CODE

void vrpn_OneConnection::init(void)
{
	vrpn_int32 i;

	tcp_client_listen_sock = INVALID_SOCKET;
	tcp_sock = INVALID_SOCKET;
	udp_outbound = INVALID_SOCKET;

	// Never lobbed a packet yet
	last_UDP_lob.tv_sec = 0;
	last_UDP_lob.tv_usec = 0;

	// Set all of the local IDs to -1, in case the other side
	// sends a message of a type that it has not yet defined.
	// (for example, arriving on the UDP line ahead of its TCP
	// definition).
	num_other_senders = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
		other_senders[i].local_id = -1;
		other_senders[i].name = NULL;
	}

	num_other_types = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		other_types[i].local_id = -1;
		other_types[i].name = NULL;
	}
	
	tvClockOffset.tv_sec = 0;
	tvClockOffset.tv_usec = 0;

	// Set up default value for the cookie received from the server
	// because if we are using a file connection and want to 
	// write a log, we never receive a cookie from the server.
	d_logmagic = new char[vrpn_cookie_size() + 1];
	write_vrpn_cookie(d_logmagic, vrpn_cookie_size() + 1,
			     vrpn_LOG_NONE);

}

vrpn_OneConnection::vrpn_OneConnection (void) :
    tcp_client_listen_sock (INVALID_SOCKET),
    tcp_sock (INVALID_SOCKET),
    udp_outbound (INVALID_SOCKET),
    udp_inbound (INVALID_SOCKET),
    num_other_senders (0),
    num_other_types (0),
    d_logbuffer (NULL),
    d_first_log_entry (NULL),
    d_logname (NULL),
    d_logmode (vrpn_LOG_NONE),
    d_logfile_handle (-1),
    d_logfile (NULL),
    d_log_filters (NULL)
{
	init();
}

vrpn_OneConnection::vrpn_OneConnection
                      (const SOCKET _tcp, const SOCKET _udp_out,
			const SOCKET _udp_in, int _num_send,
			int _num_type, vrpn_LOGLIST *_d_logbuffer,
			vrpn_LOGLIST *_d_firstlogentry, char *_d_logname,
			long _d_logmode, int _d_logfilehandle,
			FILE *_d_logfile, vrpnLogFilterEntry *_d_logfilters) :
    tcp_client_listen_sock (INVALID_SOCKET),
    tcp_sock (_tcp),
    udp_outbound (_udp_out),
    udp_inbound (_udp_in),
    num_other_senders (_num_send),
    num_other_types (_num_type),
    d_logbuffer (_d_logbuffer),
    d_first_log_entry (_d_firstlogentry),
    d_logname (_d_logname),
    d_logmode (_d_logmode),
    d_logfile_handle (_d_logfilehandle),
    d_logfile (_d_logfile),
    d_log_filters (_d_logfilters)
{
	init();
}

vrpn_OneConnection::~vrpn_OneConnection(void)
{
	vrpn_int32 i;
  	for (i=0;i<num_other_types;i++) {
		delete other_types[i].name;
	}
	for (i=0;i<num_other_senders;i++) {
		delete other_senders[i].name;
	}

	if (d_logname) {
		close_log();
	}

	if (d_log_filters) {
		vrpnLogFilterEntry * next;
		while (d_log_filters) {
			next = d_log_filters->next;
			delete d_log_filters;
			d_log_filters = next;
		}
	}
	if (d_logmagic) {
	    delete [] d_logmagic;
	}
}

// Clear out the remote mapping list. This is done when a
// connection is dropped and we want to try and re-establish
// it.
void	vrpn_OneConnection::clear_other_senders_and_types(void)
{	int	i;

	// Free up space reserved for the names
  	for (i=0;i<num_other_types;i++) {
		delete other_types[i].name;
	}
	for (i=0;i<num_other_senders;i++) {
		delete other_senders[i].name;
	}

	// Set all of the local IDs to -1, in case the other side
	// sends a message of a type that it has not yet defined.
	// (for example, arriving on the UDP line ahead of its TCP
	// definition).
	num_other_senders = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
		other_senders[i].local_id = -1;
		other_senders[i].name = NULL;
	}

	num_other_types = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		other_types[i].local_id = -1;
		other_types[i].name = NULL;
	}
}


// Make the local mapping for the otherside sender with the same
// name, if there is one.  Return 1 if there was a mapping; this
// lets the higher-ups know that there is someone that cares
// on the other side.
int	vrpn_OneConnection::newLocalSender(const char *name, vrpn_int32 which)
{
	vrpn_int32	i;
#ifdef	VERBOSE
		printf("  ...looking for other-side sender for '%s'\n", name);
#endif
	for (i = 0; i < num_other_senders; i++) {
#ifdef	VERBOSE
		printf("    ...checking against '%s'\n", other_senders[i].name);
#endif
		if (strcmp(other_senders[i].name, name) == 0) {
#ifdef	VERBOSE
			printf("  ...mapping from other-side sender %d\n", i);
#endif
			other_senders[i].local_id = which;
			return 1;
		}
	}
	return 0;
}


// If the other side has declared this type, establish the
// mapping for it.  Return 1 if there was a mapping; this
// lets the higher-ups know that there is someone that cares
// on the other side.
int	vrpn_OneConnection::newLocalType(const char *name, vrpn_int32 which)
{	vrpn_int32 i;

	for (i = 0; i < num_other_types; i++) {
		if (strcmp(other_types[i].name, name) == 0) {
			other_types[i].local_id = which;
#ifdef	VERBOSE
	printf("  ...mapping from other-side type (who cares!) %d\n", i);
#endif
			return 1;
		}
	}
	return 0;
}

int	vrpn_OneConnection::local_type_id(vrpn_int32 remote_type)
{
	return other_types[remote_type].local_id;
}

int	vrpn_OneConnection::local_sender_id(vrpn_int32 remote_sender)
{
	return other_senders[remote_sender].local_id;
}

// Adds a new remote type and returns its index.  Returns -1 on error.
int	vrpn_OneConnection::newRemoteType(cName type_name, vrpn_int32 local_id)
{
	// The assumption is that the type has not been defined before.
	// Add this as a new type to the list of those defined
	if (num_other_types >= vrpn_CONNECTION_MAX_TYPES) {
		fprintf(stderr,"vrpn: Too many types from other side (%d)\n",
                        num_other_types);
		return -1;
	}

        if (!other_types[num_other_types].name) {
	  other_types[num_other_types].name = new cName;
          if (!other_types[num_other_types].name) {
            fprintf(stderr, "vrpn_OneConnection::newRemoteType:  "
                            "Can't allocate memory for new record\n");
            return -1;
          }
        }

	memcpy(other_types[num_other_types].name, type_name, sizeof(cName));
	other_types[num_other_types].local_id = local_id;

	num_other_types++;		// Another type
	return num_other_types - 1;	// Return the index of this type
}

// Adds a new remote sender and returns its index.  Returns -1 on error.
int	vrpn_OneConnection::newRemoteSender(cName sender_name, vrpn_int32 local_id)
{
	// The assumption is that the sender has not been defined before.
	// Add this as a new sender to the list of those defined
	if (num_other_senders >= vrpn_CONNECTION_MAX_SENDERS) {
		fprintf(stderr,"vrpn: Too many senders from other side (%d)\n",
                        num_other_senders);
		return -1;
	}

        if (!other_senders[num_other_senders].name) {
	  other_senders[num_other_senders].name = new cName;
          if (!other_senders[num_other_senders].name) {
            fprintf(stderr, "vrpn_OneConnection::newRemoteSender:  "
                            "Can't allocate memory for new record\n");
            return -1;
          }
        }

	memcpy(other_senders[num_other_senders].name, sender_name, sizeof(cName));
	other_senders[num_other_senders].local_id = local_id;

	num_other_senders++;		// Another sender
	return num_other_senders - 1;	// Return the index of this sender
}

//---------------------------------------------------------------------------
//  This routine opens a TCP socket and connects it to the machine and port
// that are passed in the msg parameter.  This is a string that contains
// the machine name, a space, then the port number.
//  The routine returns -1 on failure and the file descriptor on success.

int vrpn_OneConnection::connect_tcp_to (const char * msg)
{	char	machine [5000];
	int	port;
#ifndef VRPN_USE_WINSOCK_SOCKETS
	int	server_sock;		/* Socket on this host */
#else
	SOCKET server_sock;
#endif
    struct	sockaddr_in client;     /* The name of the client */
    struct	hostent *host;          /* The host to connect to */

	// Find the machine name and port number
	if (sscanf(msg, "%s %d", machine, &port) != 2) {
		return -1;
	}

	/* set up the socket */

	server_sock = socket(AF_INET,SOCK_STREAM,0);
	if ( server_sock < 0 ) {
		fprintf(stderr,
		      "vrpn_OneConnection::connect_tcp_to: can't open socket\n");
		return(-1);
	}
	client.sin_family = AF_INET;
	host = gethostbyname(machine);
    if (host) {

#ifdef CRAY
		{ 
			int i;
			u_long foo_mark = 0;
			for  (i = 0; i < 4; i++) {
				u_long one_char = host->h_addr_list[0][i];
				foo_mark = (foo_mark << 8) | one_char;
			}
			client.sin_addr.s_addr = foo_mark;
		}
#else

		memcpy(&(client.sin_addr.s_addr), host->h_addr,  host->h_length);
#endif

	} else {

		perror("gethostbyname error:");
#if !defined(hpux) && !defined(__hpux) && !defined(_WIN32)  && !defined(sparc)
		herror("gethostbyname error:");
#endif

#ifdef VRPN_USE_WINDOWS_GETHOSTBYNAME_HACK

// gethostbyname() fails on SOME Windows NT boxes, but not all,
// if given an IP octet string rather than a true name.
// Until we figure out WHY, we have this extra clause in here.
// It probably wouldn't hurt to enable it for non-NT systems
// as well.

		int retval;
		unsigned int a, b, c, d;
		retval = sscanf(machine, "%u.%u.%u.%u", &a, &b, &c, &d);
		if (retval != 4) {
			fprintf(stderr,
				"vrpn_OneConnection::connect_tcp_to:  "
					"error: bad address string\n");
			return -1;
		}
#else	// not gethostbyname hack

// Note that this is the failure clause of gethostbyname() on
// non-WIN32 systems
		
		fprintf(stderr,
			"vrpn_OneConnection::connect_tcp_to:  "
							"error finding host by name\n");
		return -1;
	}
#endif
#ifdef VRPN_USE_WINDOWS_GETHOSTBYNAME_HACK
// XXX OK, so this doesn't work.  What's wrong???

		client.sin_addr.s_addr = (a << 24) + (b << 16) + (c << 8) + d;
		//client.sin_addr.s_addr = (d << 24) + (c << 16) + (b << 8) + a;
		fprintf(stderr, "vrpn_OneConnection::connect_tcp_to:  "
					  "gethostname() failed;  we think we're\n"
					  "looking for %d.%d.%d.%d.\n", a, b, c, d);

		// here we can try an alternative strategy:
		unsigned long addr = inet_addr(machine);
		if (addr == INADDR_NONE) {	// that didn't work either
			fprintf(stderr, "vrpn_OneConnection::connect_tcp_to:  "
					"error reading address format\n");

			return -1;
		} else {
			host = gethostbyaddr((char *)&addr,sizeof(addr), AF_INET);
			if (host){
				printf("gethostbyaddr() was successful\n");
				memcpy(&(client.sin_addr.s_addr), host->h_addr,  host->h_length);
			}
			else{
				fprintf(stderr, "vrpn_OneConnection::connect_tcp_to:  "
							" gethostbyaddr() failed\n");
				return -1;
			}
		}
	}

		
#endif


#ifndef VRPN_USE_WINSOCK_SOCKETS
	client.sin_port = htons(port);
#else
	client.sin_port = htons((u_short)port);
#endif

	if (connect(server_sock,(struct sockaddr*)&client,sizeof(client)) < 0 ){
		fprintf(stderr,
		     "vrpn_OneConnection::connect_tcp_to: Could not connect\n");
#ifdef VRPN_USE_WINSOCK_SOCKETS
		int error = WSAGetLastError();
		fprintf(stderr, "Winsock error: %d\n", error);
#endif
		close(server_sock);
		return(-1);
	}

	/* Set the socket for TCP_NODELAY */

	{	struct	protoent	*p_entry;
		int	nonzero = 1;

		if ( (p_entry = getprotobyname("TCP")) == NULL ) {
			fprintf(stderr,
			  "vrpn_OneConnection::connect_tcp_to: getprotobyname() failed.\n");
			close(server_sock);
			return(-1);
		}

//#ifndef VRPN_USE_WINSOCK_SOCKETS
		if (setsockopt(server_sock, p_entry->p_proto,
			TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
//#else
//		if (setsockopt(server_sock, p_entry->p_proto,
//			TCP_NODELAY, (const char *)&nonzero, sizeof(nonzero))==SOCKET_ERROR) {
//#endif

			perror("vrpn_OneConnection::connect_tcp_to: setsockopt() failed");
			close(server_sock);
			return(-1);
		}
	}

	return(server_sock);
}

int vrpn_OneConnection::log_message (vrpn_int32 len, struct timeval time,
                vrpn_int32 type, vrpn_int32 sender, const char * buffer,
                int isRemote)
{
  vrpn_LOGLIST * lp;
  vrpn_HANDLERPARAM p;

  lp = new vrpn_LOGLIST;
  if (!lp) {
    fprintf(stderr, "vrpn_Connection::log_message:  "
                    "Out of memory!\n");
    return -1;
  }
  lp->data.type = htonl(type);
  lp->data.sender = htonl(sender);

  // adjust the time stamp by the clock offset (as in do_callbacks_for)
  struct timeval tvTemp=vrpn_TimevalSum(time, tvClockOffset);
  
  lp->data.msg_time.tv_sec = htonl(tvTemp.tv_sec);
  lp->data.msg_time.tv_usec = htonl(tvTemp.tv_usec);

  lp->data.payload_len = htonl(len);
  lp->data.buffer = new char [len];
  if (!lp->data.buffer) {
    fprintf(stderr, "vrpn_Connection::log_message:  "
                    "Out of memory!\n");
    delete lp;
    return -1;
  }
    // need to explicitly override the const
  // NOTE: then this should probably not be a const char * (weberh 9/14/98)
  memcpy((char *) lp->data.buffer, buffer, len);

  // filter (user) messages

  if (type >= 0) {  // do not filter system messages

    if (isRemote) {

      // UGLY!
      // The user's filtering routines can't know anything about
      // remote IDs, only local IDs, so we translate the IDs into
      // something they can understand

      p.type = local_type_id(type);
      p.sender = local_sender_id(sender);
    } else {
      p.type = type;
      p.sender = sender;
    }

    p.msg_time.tv_sec = time.tv_sec;
    p.msg_time.tv_usec = time.tv_usec;
    p.payload_len = len;
    p.buffer = lp->data.buffer;

    if (check_log_filters(p)) {  // abort logging
      delete [] (char *) lp->data.buffer;
      delete lp;
      return 0;  // this is not a failure - do not return nonzero!
    }
  }

//if (type < 0)
//fprintf(stderr, "LOGGED MESSAGE:  type %ld, sender %ld (%s)\n",
//type, sender,
//type == -1 ? sender_name(isRemote ? other_senders[sender].local_id : sender) :
//(type == -2 ? message_type_name(isRemote ? other_types[sender].local_id :
//                                           sender) : "n/a"));

  lp->next = d_logbuffer;
  lp->prev = NULL;
  if (d_logbuffer)
    d_logbuffer->prev = lp;
  d_logbuffer = lp;
  if (!d_first_log_entry)
    d_first_log_entry = lp;

  return 0;
}

int vrpn_OneConnection::check_log_filters (vrpn_HANDLERPARAM message) {
  vrpnLogFilterEntry * nextFilter;

  for (nextFilter = d_log_filters; nextFilter;
       nextFilter = nextFilter->next)
    if ((*nextFilter->filter)(nextFilter->userdata, message))
      return 1;  // don't log

  return 0;
}

int vrpn_OneConnection::open_log (void) {

  if (!d_logname) {
    fprintf(stderr, "vrpn_Connection::open_log:  "
                    "Log file has no name.\n");
    return -1;
  }
  if (d_logfile) {
    fprintf(stderr, "vrpn_Connection::open_log:  "
                    "Log file is already open.\n");
    return 0;  // not a catastrophic failure
  }

  // Can't use this because MICROSOFT doesn't support Unix standards!

  // Create the file in write-only mode.
  // Permissions are 744 (user read/write, group & others read)
  // Return an error if it already exists (on some filesystems;
  // O_EXCL doesn't work on linux)

  //d_logfile_handle = open(d_logname, O_WRONLY | O_CREAT | O_EXCL,
  //                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  
  // check to see if it exists
  d_logfile = fopen(d_logname, "r");
  if (d_logfile) {
    fprintf(stderr, "vrpn_Connection::open_log:  "
                    "Log file \"%s\" already exists.\n", d_logname);
    d_logfile = NULL;
  } else
    d_logfile = fopen(d_logname, "wb");

  if (!d_logfile) {
    fprintf(stderr, "vrpn_Connection::open_log:  "
                    "Couldn't open log file \"%s\".\n", d_logname);

    // Try to write to "/tmp/vrpn_emergency_log"
    d_logfile = fopen("/tmp/vrpn_emergency_log", "r");
    if (d_logfile)
      d_logfile = NULL;
    else
      d_logfile = fopen("/tmp/vrpn_emergency_log", "wb");

    if (!d_logfile) {
      fprintf(stderr, "vrpn_Connection::open_log:\n  "
                      "Couldn't open emergency log file "
                      "\"/tmp/vrpn_emergency_log\".\n");

      return -1;
    } else
      fprintf(stderr, "Writing to /tmp/vrpn_emergency_log instead.\n");

  }

  return 0;
}


int vrpn_OneConnection::close_log (void)
{
  vrpn_LOGLIST * lp;
  int host_len;
  int final_retval = 0;
  int retval;

  // Make sure the file is open. If not, then error.
  if (d_logfile == NULL) {
	fprintf(stderr,
		"vrpn_Connection::close_log: File not open (can't write!)\n");
	return -1;
  }

  // Write out the log header (magic cookie)
  // TCH 20 May 1999

  // There's at least one hack here:
  //   What logging mode should a client that plays back the log at a
  // later time be forced into?  I believe NONE, but there might be
  // arguments the other way? So, you may want to adjust the cookie
  // to make the log mode 0.

  retval = fwrite(d_logmagic, 1, vrpn_cookie_size(), d_logfile);
  if (retval != vrpn_cookie_size()) {
    fprintf(stderr, "vrpn_Connection::close_log:  "
                    "Couldn't write magic cookie to log file "
                    "(got %d, expected %d).\n",
            retval, vrpn_cookie_size());
    lp = d_logbuffer;
    final_retval = -1;
  }


  // Write out the messages in the log,
  // starting at d_first_log_entry and working backwards

  if (!d_logfile) {
    fprintf(stderr, "vrpn_Connection::close_log:  "
                    "Log file is not open!\n");

    // Abort writing out log without destroying data needed to
    // clean up memory.

    d_first_log_entry = NULL;
    final_retval = -1;
  }

  for (lp = d_first_log_entry; lp && !final_retval; lp = lp->prev) {

    retval = fwrite(&lp->data, 1, sizeof(lp->data), d_logfile);
        //vrpn_noint_block_write(d_logfile_handle, (char *) &lp->data,
        //                            sizeof(lp->data));
    
    if (retval != sizeof(lp->data)) {
      fprintf(stderr, "vrpn_Connection::close_log:  "
                      "Couldn't write log file (got %d, expected %d).\n",
              retval, sizeof(lp->data));
      lp = d_logbuffer;
      final_retval = -1;
      continue;
    }

//fprintf(stderr, "type %d, sender %d, payload length %d\n",
//htonl(lp->data.type), htonl(lp->data.sender), host_len);

    host_len = ntohl(lp->data.payload_len);

    retval = fwrite(lp->data.buffer, 1, host_len, d_logfile);
        //vrpn_noint_block_write(d_logfile_handle, lp->data.buffer, host_len);

    if (retval != host_len) {
      fprintf(stderr, "vrpn_Connection::close_log:  "
                      "Couldn't write log file.\n");
      lp = d_logbuffer;
      final_retval = -1;
      continue;
    }
  }

  retval = fclose(d_logfile);
  if (retval) {
    fprintf(stderr, "vrpn_Connection::close_log:  "
                    "close of log file failed!\n");
    final_retval = -1;
  }

  if (d_logname)
    delete [] d_logname;

  // clean up the linked list
  while (d_logbuffer) {
    lp = d_logbuffer->next;
    delete [] (char *) d_logbuffer->data.buffer;  // ugly cast
    delete d_logbuffer;
    d_logbuffer = lp;
  }

  d_logname = NULL;
  d_first_log_entry = NULL;

  d_logfile_handle = -1;
  d_logfile = NULL;

  return final_retval;
}


void setClockOffset( void *userdata, const vrpn_CLOCKCB& info )
{
#if 0
  cerr << "clock offset is " << vrpn_TimevalMsecs(info.tvClockOffset) 
       << " msecs (used round trip which took " 
       << 2*vrpn_TimevalMsecs(info.tvHalfRoundTrip) << " msecs)." << endl;
#endif
  (*(struct timeval *) userdata) = info.tvClockOffset;
}

vrpn_Synchronized_Connection::vrpn_Synchronized_Connection
    (unsigned short listen_port_no,
     const char * local_logfile, long local_logmode,
     const char * NIC_IPaddress) :
  vrpn_Connection (listen_port_no, local_logfile, local_logmode, NIC_IPaddress)
{
   pClockServer = new vrpn_Clock_Server(this);

   pClockRemote = (vrpn_Clock_Remote *) NULL;
}

// register the base connection name so that vrpn_Clock_Remote can use it
vrpn_Synchronized_Connection::vrpn_Synchronized_Connection
         (const char * server_name,
	  int port,
          const char * local_logfile,
          long local_logmode,
          const char * remote_logfile,
          long remote_logmode,
          double dFreq, 
	  int cSyncWindow,
          const char * NIC_IPaddress) :
  vrpn_Connection (server_name, port, local_logfile, local_logmode,
                   remote_logfile, remote_logmode, NIC_IPaddress)
{

   pClockServer = (vrpn_Clock_Server *) NULL;

   if ( (status != CONNECTED) && (status != TRYING_TO_CONNECT)  &&
        (status != COOKIE_PENDING) ) { 
     return;
   }

   // DO WE NEED TO ALLOW COOKIE_PENDING HERE?

   pClockRemote = new vrpn_Clock_Remote (server_name, dFreq, cSyncWindow);
   pClockRemote->register_clock_sync_handler( &endpoint.tvClockOffset, 
					      setClockOffset );
   // -2 as freq tells connection to immediately perform
   // a full sync to calc clock offset accurately.
   if (dFreq==-2) {
#ifdef	VERBOSE
     printf("vrpn_Synchronized_Connection: starting full sync\n");
#endif
     // register messages
     mainloop();
     mainloop();

     // do full sync
#ifdef	VERBOSE
     printf("vrpn_Synchronized_Connection: calling full sync\n");
#endif
     pClockRemote->fullSync();
     mainloop();
   }
}

vrpn_Synchronized_Connection::~vrpn_Synchronized_Connection() {
	  if (pClockServer) {
		  delete pClockServer;
	  }
	  if (pClockRemote) {
		delete pClockRemote;
	  }
}

struct timeval vrpn_Synchronized_Connection::fullSync(void)
{
  if (pClockRemote) {
    // set the fullsync flag
    pClockRemote->fullSync();
    // call the mainloop to do the sync
    mainloop();
  } else {
    perror("vrpn_Synchronized_Connection::fullSync: only valid for clients");
  }
  return endpoint.tvClockOffset;
}

int vrpn_Synchronized_Connection::mainloop (const struct timeval * timeout)
{
  // If we are not in a connected state, don't do synchronization, just
  // call the parent class mainloop()
  if (status != CONNECTED) {
	return vrpn_Connection::mainloop(timeout);
  }
  if (pClockServer) {
    pClockServer->mainloop();
    // call the base class mainloop
    return vrpn_Connection::mainloop(timeout);
  } 
  else if (pClockRemote) {
    // the remote device always calls the base class connection mainloop already
    pClockRemote->mainloop(timeout);
  } 
  else {
    fprintf(stderr,
	"vrpn_Synchronized_Connection::mainloop: no clock client or server\n");
    return -1;
  }
  return 0;
}

//---------------------------------------------------------------------------
// This routine opens a UDP socket with the requested port number.
// The socket is not bound to any particular outgoing address.
// The routine returns -1 on failure and the file descriptor on success.
// The portno parameter is filled in with the actual port that is opened
// (this is useful when the address INADDR_ANY is used, since it tells
// what port was opened).
// To select between multiple NICs, we can specify the IP address of the
// socket to open;  the default value of NULL selects the default NIC.

static	int open_udp_socket (unsigned short * portno,
                             const char * IPaddress = NULL)
{
   int sock;
   struct sockaddr_in server_addr;
   struct sockaddr_in udp_name;
   int	udp_namelen = sizeof(udp_name);

  struct hostent *phe;           /* pointer to host information entry   */

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)  {
    fprintf(stderr, "open_udp_socket: can't open socket.\n");
    fprintf(stderr, "  -- errno %d (%s).\n", errno, strerror(errno));
	return -1;
   }

//fprintf(stderr, "IPaddress is %d (%s).\n", IPaddress, IPaddress);

   memset((void *) &server_addr, 0, sizeof(server_addr));
   //bzero((char *) &server_addr, sizeof(server_addr));

   // bind to local address
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(*portno);

  // Map host name to IP address, allowing for dotted decimal
  if (!IPaddress) {
    server_addr.sin_addr.s_addr = INADDR_ANY;
  } else if (phe = gethostbyname(IPaddress)) {
    memcpy((void *)&server_addr.sin_addr, (const void *)phe->h_addr, phe->h_length);
    //bcopy(phe->h_addr, (char *)&server_addr.sin_addr, phe->h_length);
  } else if ((server_addr.sin_addr.s_addr = inet_addr(IPaddress))
              == INADDR_NONE) {
    printf("can't get %s host entry\n", IPaddress);
    return -1;
  }

//fprintf(stderr, "open_udp_socket:  port %d, using NIC %d %d %d %d.\n",
//*portno, server_addr.sin_addr.s_addr >> 24,
//(server_addr.sin_addr.s_addr >> 16) & 0xff,
//(server_addr.sin_addr.s_addr >> 8) & 0xff,
//server_addr.sin_addr.s_addr & 0xff);

   if (bind(sock, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0){
	fprintf(stderr, "open_udp_socket:  can't bind address");
        fprintf(stderr, "  --  %d  --  %s\n", errno, strerror(errno));
	return -1;
   }

   // Find out which port was actually bound
   if (getsockname(sock, (struct sockaddr *) &udp_name,
                   GSN_CAST &udp_namelen)) {
	fprintf(stderr, "vrpn: open_udp_socket: cannot get socket name.\n");
	return -1;
   }
   *portno = ntohs(udp_name.sin_port);

   return sock;
}

//---------------------------------------------------------------------------
//  This routine opens a UDP socket and then connects it to a the specified
// port on the specified machine.
//  The routine returns -1 on failure and the file descriptor on success.

#ifdef VRPN_USE_WINSOCK_SOCKETS
static	SOCKET connect_udp_to (char * machine, int portno)
{
   SOCKET sock; 
#else
static	int connect_udp_to (char * machine, int portno)
{
   int sock;
#endif
   struct	sockaddr_in client;     /* The name of the client */
   struct	hostent *host;          /* The host to connect to */

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)  {
	fprintf(stderr, "connect_udp_to: can't open socket.\n");
    fprintf(stderr, "  -- errno %d (%s).\n", errno, strerror(errno));
	return INVALID_SOCKET;
   }

   // Connect it to the specified port on the specified machine
	client.sin_family = AF_INET;
	if ( (host=gethostbyname(machine)) == NULL ) {
#ifndef VRPN_USE_WINSOCK_SOCKETS
		close(sock);
		fprintf(stderr,
			 "vrpn: connect_udp_to: error finding host by name\n");
		return(INVALID_SOCKET);
#else
		unsigned long addr = inet_addr(machine);
		if (addr == INADDR_NONE){
			close(sock);
			fprintf(stderr,
				 "vrpn: connect_udp_to: error finding host by name\n");
			return(INVALID_SOCKET);
		} else{
			host = gethostbyaddr((char *)&addr,sizeof(addr), AF_INET);
			if (!host){
				close(sock);
				fprintf(stderr,
					 "vrpn: connect_udp_to: error finding host by name\n");
				return(INVALID_SOCKET);
			}
		}
#endif
	}
#ifdef CRAY
        { int i;
          u_long foo_mark = 0;
          for  (i = 0; i < 4; i++) {
              u_long one_char = host->h_addr_list[0][i];
              foo_mark = (foo_mark << 8) | one_char;
          }
          client.sin_addr.s_addr = foo_mark;
        }
#elif defined(_WIN32)
    // XXX does cygwin have bcopy? cygwin-1.0 appears to have bcopy!
	memcpy((char*)&(client.sin_addr.s_addr), host->h_addr, host->h_length);
#else
	bcopy(host->h_addr, (char*)&(client.sin_addr.s_addr), host->h_length);
#endif
	client.sin_port = htons(portno);
	if (connect(sock,(struct sockaddr*)&client,sizeof(client)) < 0 ){
		perror("vrpn: connect_udp_to: Could not connect to client");
#ifdef VRPN_USE_WINSOCK_SOCKETS
		int error = WSAGetLastError();
		fprintf(stderr, "Winsock error: %d\n", error);
#endif
		close(sock);
		return(INVALID_SOCKET);
	}

   return sock;
}


//---------------------------------------------------------------------------
//  This routine checks for a request for attention from a remote machine.
//  The requests come as datagrams to the well-known port number on this
// machine.
//  The datagram includes the machine name and port number on the remote
// machine to which this one should establish a connection.
//  This routine establishes the TCP connection and then sends a magic
// cookie describing this as a VRPN connection and telling which port to
// use for incoming UDP packets of data associated  with the current TCP
// session.  It reads the associated information from the other side and
// sets up the local UDP sender.
//  It then sends descriptions for all of the known packet types.

void vrpn_Connection::check_connection (const struct timeval * pTimeout)
{  int	request;
   char	msg[200];	// Message received on the request channel
   timeval timeout;
   if (pTimeout) {
     timeout = *pTimeout;
   } else {
     timeout.tv_sec = 0;
     timeout.tv_usec = 0;
   }

   // Do a zero-time select() to see if there is an incoming packet on
   // the UDP socket.

   fd_set f;
   FD_ZERO (&f);
   FD_SET ( listen_udp_sock, &f);
   request = select(32, &f, NULL, NULL, &timeout);
   if (request == -1 ) {	// Error in the select()
	perror("vrpn: vrpn_Connection: select failed");
	status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::check_connection.\n");
	return;
   } else if (request != 0) {	// Some data to read!  Go get it.
	struct sockaddr from;
	int fromlen = sizeof(from);
	if (recvfrom(listen_udp_sock, msg, sizeof(msg), 0,
                     &from, GSN_CAST &fromlen) == -1) {
		fprintf(stderr,
			"vrpn: Error on recvfrom: Bad connection attempt\n");
		return;
	}

	printf("vrpn: Connection request received: %s\n", msg);
	endpoint.tcp_sock = endpoint.connect_tcp_to(msg);
	if (endpoint.tcp_sock != -1) {
		status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection::check_connection.\n");
	}
   }
   if (status != COOKIE_PENDING) {   // Something broke
	return;
   } else {
   	handle_connection();
   }
   return;
}

int vrpn_Connection::connect_to_client (const char *machine, int port)
{
    char msg[100];
    if (status != LISTEN) return -1;
    sprintf(msg, "%s %d", machine, port);
    printf("vrpn: Connection request received: %s\n", msg);
    endpoint.tcp_sock = endpoint.connect_tcp_to(msg);
    if (endpoint.tcp_sock != -1) {
	status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection::connect_to_client.\n");
    }
    if (status != COOKIE_PENDING) {   // Something broke
	return -1;
    } else {
	handle_connection();
    }
    return 0;
}

void vrpn_Connection::handle_connection(void)
{
   // Set TCP_NODELAY on the socket
/*XXX It looks like this means something different to Linux than what I
	expect.  I expect it means to send acknowlegements without waiting
	to piggyback on reply.  This causes the serial port to fail.  As
	long as the other end has this set, things seem to work fine.  From
	this end, if we set it it fails (unless we're talking to another
	PC).  Russ Taylor

   {    struct  protoent        *p_entry;
	int     nonzero = 1;

	endpoint.init();

	if ( (p_entry = getprotobyname("TCP")) == NULL ) {
		fprintf(stderr,
		  "vrpn: vrpn_Connection: getprotobyname() failed.\n");
		close(endpoint.tcp_sock); endpoint.tcp_sock = -1;
		status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::handle_connection.\n");
		return;
	}

	if (setsockopt(endpoint.tcp_sock, p_entry->p_proto,
		TCP_NODELAY, &nonzero, sizeof(nonzero))==-1) {
		perror("vrpn: vrpn_Connection: setsockopt() failed");
		close(endpoint.tcp_sock); endpoint.tcp_sock = -1;
		status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::handle_connection.\n");
		return;
	}
   }
*/

   // Ignore SIGPIPE, so that the program does not crash when a
   // remote client shuts down its TCP connection.  We'll find out
   // about it as an exception on the socket when we select() for
   // read.
   // Mips/Ultrix header file signal.h appears to be broken and
   // require the following cast
#ifndef _WIN32  // XXX what about cygwin?
 #ifdef ultrix
   signal( SIGPIPE, (void (*) (int)) SIG_IGN );
 #else
   signal( SIGPIPE, SIG_IGN );
 #endif
#endif

   // Set up the things that need to happen when a new connection is
   // started.
   if (setup_new_connection()) {
	fprintf(stderr,"vrpn_Connection::handle_connection():  "
                       "Can't set up new connection!\n");
	drop_connection();
	return;
   }

}

void vrpn_Connection::poll_for_cookie (const timeval * pTimeout) {

  timeval timeout;
  if (pTimeout) {
    timeout = *pTimeout;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
  }

  fd_set  readfds, exceptfds;

  // most of this code copied from mainloop() case CONNECTED

  // check for pending incoming tcp or udp reports
  // we do this so that we can trigger out of the timeout
  // on either type of message without waiting on the other
    
  FD_ZERO(&readfds);              /* Clear the descriptor sets */
  FD_ZERO(&exceptfds);

  // Read incoming COOKIE from TCP channel

  FD_SET(endpoint.tcp_sock, &readfds);
  FD_SET(endpoint.tcp_sock, &exceptfds);

  // Select to see if ready to hear from other side, or exception
    
  if (select(32, &readfds, NULL ,&exceptfds, (timeval *)&timeout) == -1) {
    if (errno == EINTR) { /* Ignore interrupt */
      //break;
      return;
    } else {
      perror("vrpn: vrpn_Connection::poll_for_cookie(): select failed.");
      drop_connection();
      return;
    }
  }

  // See if exceptional condition on either socket
  if (FD_ISSET(endpoint.tcp_sock, &exceptfds)) {
    fprintf(stderr, "vrpn_Connection::poll_for_cookie(): Exception on socket\n");
    return;
  }

  // Read incoming COOKIE from the TCP channel
  if (FD_ISSET(endpoint.tcp_sock,&readfds)) {
    finish_new_connection_setup();
    if (!doing_okay()) {
      printf("vrpn: cookie handling failed\n");
      return;
    }
#ifdef VERBOSE3
    else {
      if
        printf("vrpn_Connection::poll_for_cookie() got cookie\n",tcp_messages_read);
    }
#endif
  }

}

// network initialization
int vrpn_Connection::setup_new_connection
         (long remote_log_mode, const char * remote_logfile_name)
{
	char sendbuf [501];  // HACK
	vrpn_int32 sendlen;
        int retval;

	num_live_connections++;

	retval = write_vrpn_cookie(sendbuf, vrpn_cookie_size() + 1,
			     remote_log_mode);
	if (retval < 0) {
		perror("vrpn_Connection::setup_new_connection:  "
		   "Internal error - array too small.  The code's broken.");
		return -1;
	}
	sendlen = vrpn_cookie_size();

	// Write the magic cookie header to the server
	if (vrpn_noint_block_write(endpoint.tcp_sock, sendbuf, sendlen)
            != sendlen) {
	  perror(
	    "vrpn_Connection::setup_new_connection: Can't write cookie");
          status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::setup_new_connection.\n");
	  return -1;
	}

        d_pendingLogmode = remote_log_mode;
        if (remote_logfile_name) {
          d_pendingLogname = new char [1 + strlen(remote_logfile_name)];
          if (!d_pendingLogname) {
            fprintf(stderr, "vrpn_Connection::setup_new_connection:  "
                            "Out of memory.\n");
            status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::setup_new_connection.\n");
            return -1;
          }
          strcpy(d_pendingLogname, remote_logfile_name);
        }

        status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection::setup_new_connection.\n");

//fprintf(stderr, "Leaving setup_new_connection().\n");

        poll_for_cookie();

        return 0;
}

int vrpn_Connection::finish_new_connection_setup (void) {

	char recvbuf [501];  // HACK
	vrpn_int32 sendlen;
	long received_logmode;
	unsigned short udp_portnum;
        int i;

//fprintf(stderr, "Entering finish_new_connection_setup().\n");

	sendlen = vrpn_cookie_size();

	// Read the magic cookie from the server
	if (vrpn_noint_block_read(endpoint.tcp_sock, recvbuf, sendlen)
            != sendlen) {
	  perror(
	    "vrpn_Connection::finish_new_connection_setup: Can't read cookie");
          status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
	  return -1;
	}

	if (check_vrpn_cookie(recvbuf) < 0) {
          status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
		return -1;
	}

	// Store the magic cookie from the other side into a buffer so
	// that it can be put into an incoming log file.
	memcpy(endpoint.d_logmagic, recvbuf, vrpn_cookie_size());

	// Find out what log mode they want us to be in BEFORE we pack
	// type, sender, and udp descriptions!  If it's nonzero, the
	// filename to use should come in a log_description message later.

	received_logmode = recvbuf[vrpn_MAGICLEN + 2] - '0';
	if ((received_logmode < 0) ||
            (received_logmode > (vrpn_LOG_INCOMING | vrpn_LOG_OUTGOING))) {
	  fprintf(stderr, "vrpn_Connection::finish_new_connection_setup:  "
                          "Got invalid log mode %d\n", received_logmode);
          status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
          return -1;
        }
	endpoint.d_logmode |= received_logmode;

	if (d_pendingLogmode)
	  if (pack_log_description(d_pendingLogmode,
	                           d_pendingLogname) == -1) {
	    fprintf(stderr, "vrpn_Connection::finish_new_connection_setup:  "
	                    "Can't pack remote logging instructions.\n");
            status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
	    return -1;
	  }

	if (endpoint.udp_inbound == INVALID_SOCKET) {
	  // Open the UDP port to accept time-critical messages on.
	  udp_portnum = (unsigned short)INADDR_ANY;
	  if ((endpoint.udp_inbound = open_udp_socket(&udp_portnum,
                                                      d_NIC_IP)) == -1)  {
	    fprintf(stderr, "vrpn_Connection::finish_new_connection_setup:  "
                            "can't open UDP socket\n");
            status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
	    return -1;
	  }
	}

        // status must be sent to CONNECTED *before* any messages are
        // packed;  otherwise they're silently discarded in pack_message.

        status = CONNECTED;
//fprintf(stderr, "CONNECTED - vrpn_Connection::finish_new_connection_setup.\n");

	// Tell the other side what port number to send its UDP messages to.
	if (pack_udp_description(udp_portnum) == -1) {
	  fprintf(stderr,
	    "vrpn_Connection::finish_new_connection_setup: Can't pack UDP msg\n");
          status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
	  return -1;
	}

	// Pack messages that describe the types of messages and sender
	// ID mappings that have been described to this connection.  These
	// messages use special IDs (negative ones).
	for (i = 0; i < num_my_senders; i++) {
		pack_sender_description(i);
	}
	for (i = 0; i < num_my_types; i++) {
		pack_type_description(i);
	}

	// Send the messages
	if (send_pending_reports() == -1) {
	  fprintf(stderr,
	    "vrpn_Connection::finish_new_connection_setup: Can't send UDP msg\n");
          status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::finish_new_connection_setup.\n");
	  return -1;
	}

	// Message needs to be dispatched *locally only*, so we do_callbacks_for
	// and never pack_message()
	struct timeval now;
	gettimeofday(&now, NULL);

	if (!num_live_connections) {
	  do_callbacks_for(register_message_type(vrpn_got_first_connection),
		     register_sender(vrpn_CONTROL),
		     now, 0, NULL);
	}

	do_callbacks_for(register_message_type(vrpn_got_connection),
		   register_sender(vrpn_CONTROL),
		   now, 0, NULL);

//fprintf(stderr, "Leaving finish_new_connection_setup().\n");

	return 0;
}


int vrpn_Connection::pack_type_description(vrpn_int32 which)
{
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32	len = strlen(my_types[which].name) + 1;
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_TYPE_DESCRIPTION
   // whose sender ID is the ID of the type that is being
   // described and whose body contains the length of the name
   // and then the name of the type.

#ifdef	VERBOSE
   printf("  vrpn_Connection: Packing type '%s'\n",my_types[which].name);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   memcpy(&buffer[sizeof(len)], my_types[which].name, (vrpn_int32) len);
   gettimeofday(&now,NULL);

   return pack_message((vrpn_uint32) (len + sizeof(len)), now,
   	vrpn_CONNECTION_TYPE_DESCRIPTION, which, buffer,
	vrpn_CONNECTION_RELIABLE);
}

int vrpn_Connection::pack_sender_description(vrpn_int32 which)
{
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32	len = strlen(my_senders[which]) + 1;
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_SENDER_DESCRIPTION
   // whose sender ID is the ID of the sender that is being
   // described and whose body contains the length of the name
   // and then the name of the sender.

#ifdef	VERBOSE
	printf("  vrpn_Connection: Packing sender '%s'\n", my_senders[which]);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   memcpy(&buffer[sizeof(len)], my_senders[which], (vrpn_int32) len);
   gettimeofday(&now,NULL);

   return pack_message((vrpn_uint32)(len + sizeof(len)), now,
   	vrpn_CONNECTION_SENDER_DESCRIPTION, which, buffer,
	vrpn_CONNECTION_RELIABLE);
}

int vrpn_Connection::pack_udp_description(int portno)
{
   struct timeval now;
   vrpn_uint32	portparam = portno;
   char myIPchar[1000];

   // Find the local host name
   if (vrpn_getmyIP(myIPchar, sizeof(myIPchar), NULL, d_NIC_IP)) {
	perror("vrpn_Connection::pack_udp_description: can't get host name");
	return -1;
   }

   // Pack a message with type vrpn_CONNECTION_UDP_DESCRIPTION
   // whose sender ID is the ID of the port that is to be
   // used and whose body holds the zero-terminated string
   // name of the host to contact.

#ifdef	VERBOSE
	printf("  vrpn_Connection: Packing UDP %s:%d\n", myIPchar,
		portno);
#endif
   gettimeofday(&now, NULL);

   return pack_message(strlen(myIPchar) + 1, now,
        vrpn_CONNECTION_UDP_DESCRIPTION,
	portparam, myIPchar, vrpn_CONNECTION_RELIABLE);
}

int vrpn_Connection::pack_log_description (long logmode,
                                           const char * logfile) {
  struct timeval now;

  // Pack a message with type vrpn_CONNECTION_LOG_DESCRIPTION whose
  // sender ID is the logging mode to be used by the remote connection
  // and whose body holds the zero-terminated string name of the file
  // to write to.

  gettimeofday(&now, NULL);
  return pack_message(strlen(logfile) + 1, now,
                      vrpn_CONNECTION_LOG_DESCRIPTION, logmode, logfile,
                      vrpn_CONNECTION_RELIABLE);
}

//static
int vrpn_Connection::handle_log_message (void * userdata,
                                         vrpn_HANDLERPARAM p) {
  vrpn_Connection * me = (vrpn_Connection *) userdata;
  int retval = 0;

  // if we're already logging, ignore the name (?)
  if (!me->endpoint.d_logname) {
    me->endpoint.d_logname = new char [p.payload_len];
    if (!me->endpoint.d_logname) {
      fprintf(stderr, "vrpn_Connection::handle_log_message:  "
                      "Out of memory!\n");
      return -1;
    }
    strncpy(me->endpoint.d_logname, p.buffer, p.payload_len);
    me->endpoint.d_logname[p.payload_len - 1] = '\0';
    retval = me->endpoint.open_log();

    // Safety check:
    // If we can't log when the client asks us to, close the connection.
    // Something more talkative would be useful.
    // The problem with implementing this is that it's over-strict:  clients
    // that assume logging succeeded unless the connection was dropped
    // will be running on the wrong assumption if we later change this to
    // be a notification message.
    if (retval == -1) {
      me->drop_connection();
      me->status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::handle_log_message.\n");
    }
  } else {
    fprintf(stderr, "vrpn_Connection::handle_log_message:  "
                    "Remote connection requested logging while logging.\n");
  }

  // OR the remotely-requested logging mode with whatever we've
  // been told to do locally
  me->endpoint.d_logmode |= p.sender;

  return retval;
}

// Get the UDP port description from the other side and connect the
// outgoing UDP socket to that port so we can send lossy but time-
// critical (tracker) messages that way.

int vrpn_Connection::handle_UDP_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	char	rhostname[1000];	// name of remote host
	vrpn_Connection * me = (vrpn_Connection *)userdata;

#ifdef	VERBOSE
	printf("  Received request for UDP channel to %s\n", p.buffer);
#endif

	// Get the name of the remote host from the buffer (ensure terminated)
	strncpy(rhostname, p.buffer, sizeof(rhostname));
	rhostname[sizeof(rhostname)-1] = '\0';

	// Open the UDP outbound port and connect it to the port on the
	// remote machine.
	// (remember that the sender field holds the UDP port number)
	me->endpoint.udp_outbound = connect_udp_to(rhostname, (int)p.sender);
	if (me->endpoint.udp_outbound == -1) {
	    perror("vrpn: vrpn_Connection: Couldn't open outbound UDP link");
	    return -1;
	}

	// Put this here because currently every connection opens a UDP
	// port and every host will get this data.  Previous implementation
	// did it in connect_tcp_to, which only gets called by servers.

	strncpy(me->endpoint.rhostname, rhostname,
		sizeof(me->endpoint.rhostname));

#ifdef	VERBOSE
	printf("  Opened UDP channel to %s:%d\n", rhostname, p.sender);
#endif
	return 0;
}

int vrpn_Connection::pack_message(vrpn_uint32 len, struct timeval time,
		vrpn_int32 type, vrpn_int32 sender, const char * buffer,
		vrpn_uint32 class_of_service)
{
	int	ret;

//fprintf(stderr, "In vrpn_Connection::pack_message()\n");

	// Make sure type is either a system type (-) or a legal user type
	if ( type >= num_my_types ) {
	    printf("vrpn_Connection::pack_message: bad type (%ld)\n",
		type);
	    return -1;
	}

	// If this is not a system message, make sure the sender is legal.
	if (type >= 0) {
	  if ( (sender < 0) || (sender >= num_my_senders) ) {
	    printf("vrpn_Connection::pack_message: bad sender (%ld)\n",
		sender);
	    return -1;
	  }
	}

        // Logging must come before filtering and should probably come before
        // any other failure-prone action (such as do_callbacks_for()).  Only
        // semantic checking should precede it.

        if (endpoint.d_logmode & vrpn_LOG_OUTGOING) {
          if (endpoint.log_message(len, time, type, sender, buffer)) {
            return -1;
          }
        }

	// See if there are any local handlers for this message type from
	// this sender.  If so, yank the callbacks.
	if (do_callbacks_for(type, sender, time, len, buffer)) {
		return -1;
	}

	// If we're not connected, nowhere to send the message so just
	// toss it out.
	if (!connected()) { return 0; }

	// TCH 28 Oct 97
	// If the other side doesn't care about the message, toss it out.
#ifdef vrpn_FILTER_MESSAGES
	if ((type >= 0) && !my_types[type].cCares) {
#ifdef VERBOSE
		printf("  vrpn_Connection:  discarding a message of type %ld "
                       "because the receiver doesn't care.\n", type);
#endif // VERBOSE
		return 0;
	}
#endif  // vrpn_FILTER_MESSAGES

	// If we don't have a UDP outbound channel, send everything TCP
	if (endpoint.udp_outbound == -1) {
	    ret = marshall_message(d_tcp_outbuf, d_tcp_buflen, d_tcp_num_out,
				   len, time, type, sender, buffer,
                                   d_sequenceNumberTCP++);
	    d_tcp_num_out += ret;
	    //	    return -(ret==0);

	    // If the marshalling failed, try clearing the outgoing buffers
	    // by sending the stuff in them to see if this makes enough
	    // room.  If not, we'll have to give up.
	    if (ret == 0) {
		if (send_pending_reports() != 0) { return -1; }
		ret = marshall_message(d_tcp_outbuf, d_tcp_buflen,
				d_tcp_num_out, len, time, type, sender,
                                buffer, d_sequenceNumberTCP++);
		d_tcp_num_out += ret;
	    }
	    return (ret==0) ? -1 : 0;
	}

	// Determine the class of service and pass it off to the
	// appropriate service (TCP for reliable, UDP for everything else).
	if (class_of_service & vrpn_CONNECTION_RELIABLE) {
	    ret = marshall_message(d_tcp_outbuf, d_tcp_buflen, d_tcp_num_out,
				   len, time, type, sender, buffer,
                                   d_sequenceNumberTCP++);
	    d_tcp_num_out += ret;
	    //	    return -(ret==0);

	    // If the marshalling failed, try clearing the outgoing buffers
	    // by sending the stuff in them to see if this makes enough
	    // room.  If not, we'll have to give up.
	    if (ret == 0) {
		if (send_pending_reports() != 0) { return -1; }
		ret = marshall_message(d_tcp_outbuf, d_tcp_buflen,
				d_tcp_num_out, len, time, type, sender,
                                buffer, d_sequenceNumberTCP++);
		d_tcp_num_out += ret;
	    }
	    return (ret==0) ? -1 : 0;
	} else {
	    ret = marshall_message(d_udp_outbuf, d_udp_buflen, d_udp_num_out,
				   len, time, type, sender, buffer,
                                   d_sequenceNumberUDP++);
	    d_udp_num_out += ret;
	    //	    return -(ret==0);

	    // If the marshalling failed, try clearing the outgoing buffers
	    // by sending the stuff in them to see if this makes enough
	    // room.  If not, we'll have to give up.
	    if (ret == 0) {
		if (send_pending_reports() != 0) { return -1; }
		ret = marshall_message(d_udp_outbuf, d_udp_buflen,
				d_udp_num_out, len, time, type, sender,
                                buffer, d_sequenceNumberUDP++);
		d_udp_num_out += ret;
	    }
	    return (ret==0) ? -1 : 0;
	}
}

// Returns the time since the connection opened.
// Some subclasses may redefine time.

// virtual
int vrpn_Connection::time_since_connection_open
                                (struct timeval * elapsed_time) {
  struct timeval now;
  gettimeofday(&now, NULL);
  *elapsed_time = vrpn_TimevalDiff(now, start_time);

  return 0;
}

// Returns the name of the specified sender/type, or NULL
// if the parameter is invalid.
// virtual
const char * vrpn_Connection::sender_name (vrpn_int32 sender) {
  if ((sender < 0) || (sender >= num_my_senders)) return NULL;
  return (const char *) my_senders[sender];
}

// virtual
const char * vrpn_Connection::message_type_name (vrpn_int32 type) {
  if ((type < 0) || (type >= num_my_types)) return NULL;
  return (const char *) my_types[type].name;
}

// virtual
int vrpn_Connection::register_log_filter (vrpn_LOGFILTER filter,
                                          void * userdata) {
  vrpnLogFilterEntry * newEntry;

  newEntry = new vrpnLogFilterEntry;
  if (!newEntry) {
    fprintf(stderr, "vrpn_Connection::register_log_filter:  "
                    "Out of memory.\n");
    return -1;
  }

  newEntry->filter = filter;
  newEntry->userdata = userdata;
  newEntry->next = endpoint.d_log_filters;
  endpoint.d_log_filters = newEntry;

  return 0;
}

// virtual
vrpn_File_Connection * vrpn_Connection::get_File_Connection (void) {
  return NULL;
}


vrpn_int32 vrpn_Connection::set_tcp_outbuf_size (vrpn_int32 bytecount) {
  char * new_outbuf;

  if (bytecount < 0)
    return d_tcp_buflen;

  new_outbuf = new char [bytecount];

  if (!new_outbuf)
    return -1;

  if (d_tcp_outbuf)
    delete [] d_tcp_outbuf;

  d_tcp_outbuf = new_outbuf;
  d_tcp_buflen = bytecount;

  return d_tcp_buflen;
}



// Marshal the message into the buffer if it will fit.  Return the number
// of characters sent.

// TCH 22 Feb 99
// Marshall the sequence number, but never unmarshall it - it's currently
// only provided for the benefit of sniffers.

int vrpn_Connection::marshall_message(
	char *outbuf,		// Base pointer to the output buffer
	vrpn_uint32 outbuf_size,// Total size of the output buffer
	vrpn_uint32 initial_out,// How many characters are already in outbuf
	vrpn_uint32 len,	// Length of the message payload
	struct timeval time,	// Time the message was generated
	vrpn_int32 type,	// Type of the message
	vrpn_int32 sender,	// Sender of the message
	const char * buffer,	// Message payload
        vrpn_uint32 seqNo)      // Sequence number
{
   vrpn_uint32	ceil_len, header_len, total_len;
   vrpn_uint32	curr_out = initial_out;	// How many out total so far

   // Compute the length of the message plus its padding to make it
   // an even multiple of vrpn_ALIGN bytes.

   // Compute the total message length and put the message
   // into the message buffer (if we have room for the whole message)
   ceil_len = len; 
   if (len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - len%vrpn_ALIGN;}
   header_len = 5*sizeof(vrpn_int32);
   if (header_len%vrpn_ALIGN) {
	header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;
   }
   total_len = header_len + ceil_len;
   if ((curr_out + total_len) > (vrpn_uint32)outbuf_size) {
   	return 0;
   }
   
   // The packet header len field does not include the padding bytes,
   // these are inferred on the other side.
   // Later, to make things clearer, we should probably infer the header
   // len on the other side (in the same way the padding is done)
   // The reason we don't include the padding in the len is that we
   // would not be able to figure out the size of the padding on the
   // far side)
   *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(header_len+len);
   curr_out+= sizeof(vrpn_uint32);

   // Pack the time (using gettimeofday() format) into the buffer
   // and do network byte ordering.
   *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(time.tv_sec);
   curr_out += sizeof(vrpn_uint32);
   *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(time.tv_usec);
   curr_out += sizeof(vrpn_uint32);

   // Pack the sender and type and do network byte-ordering
   *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(sender);
   curr_out += sizeof(vrpn_uint32);
   *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(type);
   curr_out += sizeof(vrpn_uint32);

   // Pack the sequence number.  If something's really screwy with
   // our sizes/types and there isn't room for the sequence number,
   // skipping for alignment below will overwrite it!
   *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(seqNo);
   curr_out += sizeof(vrpn_uint32);

   // skip chars if needed for alignment
   curr_out = initial_out + header_len;

   // Pack the message from the buffer.  Then skip as many characters
   // as needed to make the end of the buffer fall on an even alignment
   // of vrpn_ALIGN bytes (the size of largest element sent via vrpn.
   if (buffer != NULL) {
	memcpy(&outbuf[curr_out], buffer, len);
   }
   curr_out += ceil_len;
#ifdef	VERBOSE
   printf("Marshalled: len %d, ceil_len %d: '",len,ceil_len);
   printf("'\n");
#endif
   return curr_out - initial_out;	// How many extra bytes we sent
}

void vrpn_Connection::drop_connection(void)
{

	//XXX Store name when opening, say which dropped here
	printf("vrpn: Connection dropped\n");
	if (endpoint.tcp_sock != INVALID_SOCKET) {
		close(endpoint.tcp_sock);
		endpoint.tcp_sock = INVALID_SOCKET;
		d_tcp_num_out = 0;	// Ignore characters waiting to go

	}
	if (endpoint.udp_outbound != INVALID_SOCKET) {
		close(endpoint.udp_outbound);
		endpoint.udp_outbound = INVALID_SOCKET;
		d_udp_num_out = 0;	// Ignore characters waiting to go
	}
	if (endpoint.udp_inbound != INVALID_SOCKET) {
		close(endpoint.udp_inbound);
		endpoint.udp_inbound = INVALID_SOCKET;
	}
	if (listen_udp_sock != INVALID_SOCKET) {
		// We're a server, so we go back to listening on our
		// UDP socket.
		status = LISTEN;
//fprintf(stderr, "LISTEN - vrpn_Connection::drop_connection.\n");
	} else {
		// We're a client, so we try to reconnect to the server
		// that just dropped its connection.
		status = TRYING_TO_CONNECT;
//fprintf(stderr, "TRYING_TO_CONNECT - vrpn_Connection::drop_connection.\n");
	}

	// Remove the remote mappings for senders and types. If we
	// reconnect, we will want to fill them in again. First,
	// free the space allocated for the list of names, then
	// set all of the local IDs to -1, in case the other side
	// sends a message of a type that it has not yet defined.
	// (for example, arriving on the UDP line ahead of its TCP
	// definition).

	endpoint.clear_other_senders_and_types();

  num_live_connections--;

  // Message needs to be dispatched *locally only*, so we do_callbacks_for()
  // and never pack_message()
  struct timeval now;
  gettimeofday(&now, NULL);

  do_callbacks_for(register_message_type(vrpn_dropped_connection),
                   register_sender(vrpn_CONTROL),
                   now, 0, NULL);

  if (!num_live_connections) {
    do_callbacks_for(register_message_type(vrpn_dropped_last_connection),
                     register_sender(vrpn_CONTROL),
                     now, 0, NULL);
  }

  // If we are logging, put a message in the log telling that we
  // have had a disconnection. We don't close the logfile here unless
  // there is an error logging the message. This is because we'll want
  // to keep logging if there is a reconnection. We close the file when
  // the endpoint is destroyed.
  if (endpoint.d_logname) {
	vrpn_int32	scrap_payload = 0;

	if (endpoint.log_message(sizeof(scrap_payload),	// Size of payload
		now,					// Message is now
		vrpn_CONNECTION_DISCONNECT_MESSAGE,	// Message type
		scrap_payload,				// Sender is zero
		(char*)(void*)&scrap_payload,		// Not looked at
		0)					// Not a remote type
	    == -1) {
		fprintf(stderr,"vrpn_Connection::drop_connection: Can't log\n");
		endpoint.close_log();	// Hope for the best...
	}
  }
}

int vrpn_Connection::send_pending_reports(void)
{
   vrpn_int32	ret, sent = 0;

   // Do nothing if not connected.
   if (!connected()) {
   	return 0;
   }

   // Check for an exception on the socket.  If there is one, shut it
   // down and go back to listening.
   timeval timeout;
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   fd_set f;
   FD_ZERO (&f);
   FD_SET ( endpoint.tcp_sock, &f);

   int connection = select(32, NULL, NULL, &f, &timeout);
   if (connection != 0) {
	drop_connection();
	return -1;
   }

   // Send all of the messages that have built
   // up in the TCP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
#ifdef	VERBOSE
   if (d_tcp_num_out) printf("TCP Need to send %d bytes\n",d_tcp_num_out);
#endif
   while (sent < d_tcp_num_out) {
	ret = send(endpoint.tcp_sock, &d_tcp_outbuf[sent],
                   d_tcp_num_out - sent, 0);
#ifdef	VERBOSE
   printf("TCP Sent %d bytes\n",ret);
#endif
      if (ret == -1) {
	printf("vrpn: TCP send failed\n");
	drop_connection();
	return -1;
      }
      sent += ret;
   }

   // Send all of the messages that have built
   // up in the UDP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
   if (d_udp_num_out > 0) {
	ret = send(endpoint.udp_outbound, d_udp_outbuf, d_udp_num_out, 0);
#ifdef	VERBOSE
   printf("UDP Sent %d bytes\n",ret);
#endif 
      if (ret == -1) {
	perror("vrpn_Connection: UDP send failed");
	drop_connection();
	return -1;
      }
   }

   d_tcp_num_out = 0;	// Clear the buffer for the next time
   d_udp_num_out = 0;	// Clear the buffer for the next time
   return 0;
}

int vrpn_Connection::mainloop (const struct timeval * pTimeout)
{
  int	tcp_messages_read;
  int	udp_messages_read;

  timeval timeout;
  if (pTimeout) {
    timeout = *pTimeout;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
  }

#ifdef	VERBOSE2
  printf("vrpn_Connection::mainloop() called (status %d)\n",status);
#endif
  
  // struct timeval perSocketTimeout;
  // const int numSockets = 2;
  // divide timeout over all selects()
  //  if (timeout) {
  //    perSocketTimeout.tv_sec = timeout->tv_sec / numSockets;
  //    perSocketTimeout.tv_usec = timeout->tv_usec / numSockets
  //      + (timeout->tv_sec % numSockets) *
  //      (1000000L / numSockets);
  //  } else {
  //    perSocketTimeout.tv_sec = 0L;
  //    perSocketTimeout.tv_usec = 0L;
  //  }

  // BETTER IDEA: do one select rather than multiple -- otherwise you are
  // potentially holding up one or the other.  This allows clients which
  // should not do anything until they get messages to request infinite
  // waits (servers can't usually do infinite waits this because they need
  // to service other devices to generate info which they then send to
  // clients) .  weberh 3/20/99

    fd_set  readfds, exceptfds;
  
  switch (status) {
  case LISTEN:
    check_connection(pTimeout);
    break;
    
  case CONNECTED:
    
    // Send all pending reports on the way out
    send_pending_reports();
    
    // check for pending incoming tcp or udp reports
    // we do this so that we can trigger out of the timeout
    // on either type of message without waiting on the other
    
    FD_ZERO(&readfds);              /* Clear the descriptor sets */
    FD_ZERO(&exceptfds);

    // Read incoming messages from both the UDP and TCP channels

    if (endpoint.udp_inbound != -1) {
      FD_SET(endpoint.udp_inbound, &readfds);     /* Check for read */
      FD_SET(endpoint.udp_inbound, &exceptfds);   /* Check for exceptions */
    }
    FD_SET(endpoint.tcp_sock, &readfds);
    FD_SET(endpoint.tcp_sock, &exceptfds);

    // Select to see if ready to hear from other side, or exception
    
    if (select(32, &readfds, NULL ,&exceptfds, (timeval *)&timeout) == -1) {
      if (errno == EINTR) { /* Ignore interrupt */
        break;
      } else {
        perror("vrpn: vrpn_Connection::mainloop: select failed.");
        drop_connection();
	return -1;
      }
    }

    // See if exceptional condition on either socket
    if (FD_ISSET(endpoint.tcp_sock, &exceptfds) ||
        ((endpoint.udp_inbound!=-1) && 
         FD_ISSET(endpoint.udp_inbound, &exceptfds))) {
      fprintf(stderr, "vrpn_Connection::mainloop: Exception on socket\n");
      drop_connection();
      return -1;
    }

    // Read incoming messages from the UDP channel
    if ((endpoint.udp_inbound != -1) && 
        FD_ISSET(endpoint.udp_inbound,&readfds)) {
      if ( (udp_messages_read = 
            handle_udp_messages(endpoint.udp_inbound,
                                NULL)) == -1) {
   //                               &perSocketTimeout)) == -1) {
        fprintf(stderr,
                "vrpn: UDP handling failed, dropping connection\n");
        drop_connection();
        break;
      }
#ifdef VERBOSE3
      if(udp_messages_read != 0 )
        printf("udp message read = %d\n",udp_messages_read);
#else
	udp_messages_read = udp_messages_read;	// Avoid compiler warning
#endif
    }

    // Read incoming messages from the TCP channel
    if (FD_ISSET(endpoint.tcp_sock,&readfds)) {
      if ((tcp_messages_read =
           handle_tcp_messages(endpoint.tcp_sock,
                               NULL)) == -1) {
        //                             &perSocketTimeout)) == -1) {
        printf("vrpn: TCP handling failed, dropping connection\n");
        drop_connection();
        break;
      }
#ifdef VERBOSE3
      else {
        if(tcp_messages_read!=0)
          printf("tcp_message_read %d bytes\n",tcp_messages_read);
      }
#else
	tcp_messages_read = tcp_messages_read; // Avoid compiler warning
#endif
    }
#ifdef	PRINT_READ_HISTOGRAM
#define      HISTSIZE 25
   {
        static vrpn_uint32 count = 0;
        static int tcp_histogram[HISTSIZE+1];
        static int udp_histogram[HISTSIZE+1];
        count++;

        if (tcp_messages_read > HISTSIZE) {tcp_histogram[HISTSIZE]++;}
        else {tcp_histogram[tcp_messages_read]++;};

        if (udp_messages_read > HISTSIZE) {udp_histogram[HISTSIZE]++;}
        else {udp_histogram[udp_messages_read]++;};

        if (count == 3000L) {
		int i;
                count = 0;
		printf("\nHisto (tcp): ");
                for (i = 0; i < HISTSIZE+1; i++) {
                        printf("%d ",tcp_histogram[i]);
                        tcp_histogram[i] = 0;
                }
                printf("\n");
		printf("      (udp): ");
                for (i = 0; i < HISTSIZE+1; i++) {
                        printf("%d ",udp_histogram[i]);
                        udp_histogram[i] = 0;
                }
                printf("\n");
        }
   }
#endif
      	break;

      case COOKIE_PENDING:

        poll_for_cookie(pTimeout);

        break;

      case TRYING_TO_CONNECT:
	{	struct timeval	now;
		int ret;

#ifdef	VERBOSE
		printf("TRYING_TO_CONNECT\n");
#endif
		// See if we have a connection yet (nonblocking select).
		ret = vrpn_poll_for_accept(endpoint.tcp_client_listen_sock,
					 &endpoint.tcp_sock);
		if (ret  == -1) {
			fprintf(stderr,
			  "vrpn_Connection: mainloop: Can't poll for accept\n");
			status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::mainloop.\n");
			break;
		}
		if (ret == 1) {	// Got one!
		  status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection::mainloop.\n");
		  printf("vrpn: Connection established\n");

		  // Set up the things that need to happen when a new connection
		  // is established.
		  if (setup_new_connection(endpoint.remote_log_mode,
				endpoint.remote_log_name)){
		      fprintf(stderr, "vrpn_Connection: mainloop: "
				      "Can't set up new connection!\n");
		      drop_connection();
		      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::mainloop.\n");
		      break;
		  }
		  break;
		}

		// Lob a request-to-connect packet every couple of seconds
		gettimeofday(&now, NULL);
		if (now.tv_sec - endpoint.last_UDP_lob.tv_sec >= 2) {

		  if (vrpn_udp_request_lob_packet(endpoint.remote_machine_name,
					endpoint.remote_UDP_port,
					endpoint.tcp_client_listen_port,
                                        d_NIC_IP) == -1){
			fprintf(stderr,
			  "vrpn_Connection: mainloop: Can't lob UDP request\n");
			status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::mainloop.\n");
			break;
		  }
		}
	}
	break;

      case BROKEN:
      	fprintf(stderr, "vrpn: Fatal connection failure.  Giving up!\n");
	status = DROPPED;
//fprintf(stderr, "DROPPED - vrpn_Connection::mainloop.\n");
      	return -1;

      case DROPPED:
      	break;
   }
   return 0;
}

void	vrpn_Connection::init (void)
{
	vrpn_int32	i;

	// Lots of constants used to be set up here.  They were moved
	// into the constructors in 02.10;  this will create a slight
        // increase in maintenance burden keeping the constructors consistient.

	// offset of clocks on connected machines -- local - remote
	// (this should really not be here, it should be in adjusted time
	// connection, but this makes it a lot easier
	endpoint.tvClockOffset.tv_sec=0;
	endpoint.tvClockOffset.tv_usec=0;

	// Set up the default handlers for the different types of user
	// incoming messages.  Initially, they are all set to the
	// NULL handler, which tosses them out.
	// Note that system messages always have negative indices.
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		my_types[i].who_cares = NULL;
		my_types[i].cCares = 0;
		my_types[i].name = NULL;
	}

	// Set up the handlers for the system messages.  Skip any ones
	// that we don't know how to handle.
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		system_messages[i] = NULL;
	}
	system_messages[-vrpn_CONNECTION_SENDER_DESCRIPTION] =
		handle_sender_message;
	system_messages[-vrpn_CONNECTION_TYPE_DESCRIPTION] =
		handle_type_message;
	system_messages[-vrpn_CONNECTION_UDP_DESCRIPTION] =
		handle_UDP_message;
	system_messages[-vrpn_CONNECTION_LOG_DESCRIPTION] =
	        handle_log_message;
	system_messages[-vrpn_CONNECTION_DISCONNECT_MESSAGE] =
	        handle_disconnect_message;

	for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
		my_senders[i] = NULL;
	}

	gettimeofday(&start_time, NULL);

#ifdef VRPN_USE_WINSOCK_SOCKETS

  // Make sure sockets are set up
  // TCH 2 Nov 98 after Phil Winston

  WSADATA wsaData;
  int winStatus;

  winStatus = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (winStatus) {
    fprintf(stderr, "vrpn_Connection::init():  "
                    "Failed to set up sockets.\n");
    fprintf(stderr, "WSAStartup failed with error code %d\n", winStatus);
    exit(0);
  }

#endif  // windows sockets

}

vrpn_Connection::vrpn_Connection (unsigned short listen_port_no,
       const char * local_logfile_name, long local_log_mode,
       const char * NIC_IPaddress) :
    //my_name (NULL),
    endpoint (INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, 0, 0,
		NULL, NULL, NULL, vrpn_LOG_NONE, -1, NULL, NULL ),
    num_live_connections (0),
    listen_udp_sock (INVALID_SOCKET),
    num_my_senders (0),
    num_my_types (0),
    generic_callbacks (NULL),
    d_tcp_outbuf (new char [vrpn_CONNECTION_TCP_BUFLEN]),
    d_udp_outbuf (new char [vrpn_CONNECTION_UDP_BUFLEN]),
    d_tcp_buflen (d_tcp_outbuf ? vrpn_CONNECTION_TCP_BUFLEN : 0),
    d_udp_buflen (d_udp_outbuf ? vrpn_CONNECTION_UDP_BUFLEN : 0),
    d_tcp_num_out (0),
    d_udp_num_out (0),
    d_pendingLogmode (0),
    d_pendingLogname (NULL),
    d_TCPbuf ((char*)(&d_TCPinbufToAlignRight[0])),
    d_UDPinbuf ((char*)(&d_UDPinbufToAlignRight[0])),
    d_sequenceNumberUDP (0),
    d_sequenceNumberTCP (0),
    d_NIC_IP (NIC_IPaddress)
{
  int retval;

   // Initialize the things that must be for any constructor
   init();

   if (!d_tcp_buflen || !d_udp_buflen) {
     status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
     fprintf(stderr, "vrpn_Connection couldn't allocate buffers.\n");
     return;
   }

   if ( (listen_udp_sock = open_udp_socket(&listen_port_no, NIC_IPaddress))
           == INVALID_SOCKET) {
      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
   } else {
      status = LISTEN;
//fprintf(stderr, "LISTEN - vrpn_Connection::vrpn_Connection.\n");
   printf("vrpn: Listening for requests on port %d\n",listen_port_no);
   }
  
  vrpn_ConnectionManager::instance().addConnection(this, NULL);

  if (local_logfile_name) {
    endpoint.d_logname = new char [1 + strlen(local_logfile_name)];
    if (!endpoint.d_logname) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
                      "Out of memory!\n");
      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
      return;
    }

    strcpy(endpoint.d_logname, local_logfile_name);
    endpoint.d_logmode = local_log_mode;
    retval = endpoint.open_log();
    if (retval == -1) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
                      "Couldn't open log file.\n");
      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
      return;
    }
  }

}


vrpn_Connection::vrpn_Connection
      (const char * station_name, int port,
       const char * local_logfile_name, long local_log_mode,
       const char * remote_logfile_name, long remote_log_mode,
       const char * NIC_IPaddress) :
    //my_name (NULL),
    endpoint (INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, 0, 0,
		NULL, NULL, NULL, vrpn_LOG_NONE, -1, NULL, NULL),
    num_live_connections (0),
    listen_udp_sock (INVALID_SOCKET),
    num_my_senders (0),
    num_my_types (0),
    generic_callbacks (NULL),
    d_tcp_outbuf (new char [vrpn_CONNECTION_TCP_BUFLEN]),
    d_udp_outbuf (new char [vrpn_CONNECTION_UDP_BUFLEN]),
    d_tcp_buflen (d_tcp_outbuf ? vrpn_CONNECTION_TCP_BUFLEN : 0),
    d_udp_buflen (d_udp_outbuf ? vrpn_CONNECTION_UDP_BUFLEN : 0),
    d_tcp_num_out (0),
    d_udp_num_out (0),
    d_pendingLogmode (0),
    d_pendingLogname (NULL),
    d_TCPbuf ((char*) (&d_TCPinbufToAlignRight[0])),
    d_UDPinbuf ((char *) (&d_UDPinbufToAlignRight[0])),
    d_sequenceNumberUDP (0),
    d_sequenceNumberTCP (0),
    d_NIC_IP (NIC_IPaddress)
{
	int retval;
	int isfile;
	int isrsh;

	isfile = (strstr(station_name, "file:") ? 1 : 0);
	isrsh = (strstr(station_name, "x-vrsh:") ? 1 : 0);

	// Initialize the things that must be for any constructor
	init();
	endpoint.init();

	// If we are neither a file nor a remote-server-starting
	// type of connection, then set up to lob UDP packets
	// to the other side and put us in the mode that will
	// wait for the responses. Go ahead and set up the TCP
	// socket that we will listen on and lob a packet.

	if (!isfile && !isrsh) {
	  // Open a connection to the station using a UDP request
	  // that asks to machine to call us back here.
	  endpoint.remote_machine_name = vrpn_copy_machine_name(station_name);
	  if (!endpoint.remote_machine_name) {
	    fprintf(stderr, "vrpn_Connection: Out of memory!\n");
	    status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
            return;
          }
	  if (port < 0) {
		endpoint.remote_UDP_port = vrpn_DEFAULT_LISTEN_PORT_NO;
	  } else {
		endpoint.remote_UDP_port = port;
	  }

	  // Store the remote log file name and the remote log mode
	  endpoint.remote_log_mode = remote_log_mode;
	  if (remote_logfile_name == NULL) {
		endpoint.remote_log_name = new char[10];
		strcpy(endpoint.remote_log_name, "");
	  } else {
		endpoint.remote_log_name =
			new char[strlen(remote_logfile_name)+1];
		strcpy(endpoint.remote_log_name, remote_logfile_name);
	  }
	 
	  status = TRYING_TO_CONNECT;
//fprintf(stderr, "TRYING_TO_CONNECT - vrpn_Connection::vrpn_Connection.\n");

#ifdef	VERBOSE
	  printf("vrpn_Connection: Getting the TCP port to listen on\n");
#endif
	  // Set up the connection that we will listen on.
	  if (vrpn_get_a_TCP_socket(&endpoint.tcp_client_listen_sock,
				    &endpoint.tcp_client_listen_port,
                                    NIC_IPaddress) == -1) {
		fprintf(stderr,"vrpn_Connection: Can't create listen socket\n");
		status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
		return;
	  }

	  // Lob a packet asking for a connection on that port.
	  gettimeofday(&endpoint.last_UDP_lob, NULL);
	  if (vrpn_udp_request_lob_packet(endpoint.remote_machine_name,
				endpoint.remote_UDP_port,
				endpoint.tcp_client_listen_port,
                                NIC_IPaddress) == -1) {
		fprintf(stderr,"vrpn_Connection: Can't lob UDP request\n");
		status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
		return;
	  }

	  // See if we have a connection yet (wait for 1 sec at most).
	  // This will allow the connection to come up fast if things are
	  // all set. Otherwise, we'll drop into re-sends when we get
	  // to mainloop().
	  retval = vrpn_poll_for_accept(endpoint.tcp_client_listen_sock,
				 &endpoint.tcp_sock, 1.0);
	  if (retval  == -1) {
		fprintf(stderr,
		  "vrpn_Connection: Can't poll for accept\n");
		status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
		return;
	  }
	  if (retval == 1) {	// Got one!
	    status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection::vrpn_Connection.\n");
	    printf("vrpn: Connection established on initial try\n");

	    // Set up the things that need to happen when a new connection
	    // is established.
	    if (setup_new_connection(endpoint.remote_log_mode,
			endpoint.remote_log_name)) {
	      fprintf(stderr, "vrpn_Connection: "
			      "Can't set up new connection!\n");
	      drop_connection();
	      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
	      return;
	    }
	  }
	}

	// If we are a remote-server-starting type of connection,
	// Try to start the remote server and connect to it.  If
	// we fail, then the connection is broken. Otherwise, we
	// are connected.

	if (isrsh) {
	  // Start up the server and wait for it to connect back
	  char * machinename;
	  char *server_program;
          char *server_args;   // server program plus its arguments
	  char *token;

          machinename = vrpn_copy_machine_name(station_name);
	  server_program = vrpn_copy_rsh_program(station_name);
          server_args = vrpn_copy_rsh_arguments(station_name);
	  token = server_args;
          // replace all argument separators (',') with spaces (' ')
          while (token = strchr(token, ','))
                *token = ' ';
	  endpoint.tcp_sock = vrpn_start_server(machinename, server_program, 
							     server_args,
                                                NIC_IPaddress);
	  if (machinename) delete [] (char *) machinename;
	  if (server_program) delete [] (char *) server_program;
	  if (server_args) delete [] (char *) server_args;

	  if (endpoint.tcp_sock < 0) {
	      fprintf(stderr, "vrpn_Connection:  "
			      "Can't open %s\n", station_name);
	      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
	      return;
	  } else {
		status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection::vrpn_Connection.\n");

		// Set up the things that need to happen when a new connection
		// is established.
		if (!isfile) {
		 if (setup_new_connection(remote_log_mode,remote_logfile_name)){
		      fprintf(stderr, "vrpn_Connection:  "
				      "Can't set up new connection!\n");
		      drop_connection();
		      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
		      return;
		 }
		}

	  }
	}

        vrpn_ConnectionManager::instance().addConnection(this, station_name);

	// If we are doing local logging, turn it on here. If we
	// can't open the file, then the connection is broken.

	if (local_logfile_name) {
		endpoint.d_logname = new char [1 + strlen(local_logfile_name)];
		if (!endpoint.d_logname) {
		  fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
		      "Out of memory!\n");
		  status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
		  return;
		}

		strcpy(endpoint.d_logname, local_logfile_name);
		endpoint.d_logmode = local_log_mode;
		retval = endpoint.open_log();
		if (retval == -1) {
			fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
				      "Couldn't open log file.\n");
			status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
			return;
		}

	}
}

vrpn_Connection::~vrpn_Connection (void) {

  //fprintf(stderr, "In vrpn_Connection destructor.\n");

#ifdef VRPN_USE_WINSOCK_SOCKETS

	if (WSACleanup() == SOCKET_ERROR) {
    fprintf(stderr, "~vrpn_Connection():  "
                    "WSACleanup() failed with error code %d\n",
            WSAGetLastError());
	}

#endif  // VRPN_USE_WINSOCK_SOCKETS
	vrpn_int32 i;
	vrpnMsgCallbackEntry *pVMCB, *pVMCB_Del;
	for (i=0;i<num_my_types;i++) {
		delete my_types[i].name;
		pVMCB=my_types[i].who_cares;
		while (pVMCB) {
			pVMCB_Del=pVMCB;
			pVMCB=pVMCB_Del->next;
			delete pVMCB_Del;
		}
	}
	for (i=0;i<num_my_senders;i++) {
		delete my_senders[i];
	}
	
	// free generic message callbacks
	pVMCB=generic_callbacks;
	
	while (pVMCB) {
		pVMCB_Del=pVMCB;
		pVMCB=pVMCB_Del->next;
		delete pVMCB_Del;
	}

  // Remove myself from the "known connections" list
  //   (or the "anonymous connections" list).
  vrpn_ConnectionManager::instance().deleteConnection(this);

  if (d_tcp_outbuf)
    delete [] d_tcp_outbuf;
  if (d_udp_outbuf)
    delete [] d_udp_outbuf;
}

vrpn_int32 vrpn_Connection::register_sender (const char * name)
{
   vrpn_int32	i;

   /*fprintf(stderr, "vrpn_Connection::register_sender:  "
     "%d senders;  new name \"%s\"\n", num_my_senders, name);
     */

   // See if the name is already in the list.  If so, return it.
   for (i = 0; i < num_my_senders; i++) {
      if (strcmp(my_senders[i], name) == 0) {
	// fprintf(stderr, "It's already there, #%d\n", i);
      	return i;
      }
   }

   // See if there are too many on the list.  If so, return -1.
   if (num_my_senders >= vrpn_CONNECTION_MAX_SENDERS) {
   	fprintf(stderr, "vrpn: vrpn_Connection::register_sender:  "
                        "Too many! (%d)\n", num_my_senders);
   	return -1;
   }

   if (!my_senders[num_my_senders]) {

     //  fprintf(stderr, "Allocating a new name entry\n");

     my_senders[num_my_senders] = new cName;
     if (!my_senders[num_my_senders]) {
       fprintf(stderr, "vrpn_Connection::register_sender:  "
                       "Can't allocate memory for new record\n");
       return -1;
     }
   }

   // Add this one into the list
   strncpy(my_senders[num_my_senders], name, sizeof(cName) - 1);
   num_my_senders++;

   // If we're connected, pack the sender description
   // TCH 24 Jan 00 - Need to do this even if not connected so
   // that it goes into the logs (if we're keeping any).
   //if (connected()) {
   	pack_sender_description(num_my_senders - 1);
   //}

   // If the other side has declared this sender, establish the
   // mapping for it.
   endpoint.newLocalSender(name, num_my_senders - 1);

   // One more in place -- return its index
   return num_my_senders - 1;
}

vrpn_int32 vrpn_Connection::register_message_type (const char * name)
{
	vrpn_int32	i;

	// See if the name is already in the list.  If so, return it.
	i = message_type_is_registered(name);
	if (i != -1)
	  return i;

	// See if there are too many on the list.  If so, return -1.
	if (num_my_types == vrpn_CONNECTION_MAX_TYPES) {
		fprintf(stderr,
		  "vrpn: vrpn_Connection::register_message_type:  "
                  "Too many! (%d)\n", num_my_types);
		return -1;
	}

	if (!my_types[num_my_types].name) {
	  my_types[num_my_types].name = new cName;
          if (!my_types[num_my_types].name) {
            fprintf(stderr, "vrpn_Connection::register_message_type:  "
                            "Can't allocate memory for new record\n");
            return -1;
          }
        }

	// Add this one into the list and return its index
	strncpy(my_types[num_my_types].name, name, sizeof(cName) - 1);
	my_types[num_my_types].who_cares = NULL;
	my_types[num_my_types].cCares = 0;  // TCH 28 Oct 97 - redundant?
	num_my_types++;

	// If we're connected, pack the type description
   // TCH 24 Jan 00 - Need to do this even if not connected so
   // that it goes into the logs (if we're keeping any).
	//if (connected()) {
		pack_type_description(num_my_types-1);
	//}

	if (endpoint.newLocalType(name, num_my_types - 1)) {
		my_types[num_my_types - 1].cCares = 1;  // TCH 28 Oct 97
	}

	// One more in place -- return its index
	return num_my_types - 1;
}


// Yank the callback chain for a message type.  Call all handlers that
// are interested in messages from this sender.  Return 0 if they all
// return 0, -1 otherwise.

int	vrpn_Connection::do_callbacks_for(vrpn_int32 type, vrpn_int32 sender,
		struct timeval time, vrpn_uint32 payload_len, const char * buf)
{
	// Make sure we have a non-negative type.  System messages are
	// handled differently.
    if (type < 0) {
	return 0;
    }
	vrpnMsgCallbackEntry * who;
	vrpn_HANDLERPARAM p;

	// Fill in the parameter to be passed to the routines
	p.type = type;
	p.sender = sender;
	p.msg_time = vrpn_TimevalSum(time, endpoint.tvClockOffset);
#if 0
	cerr << " remote time is " << time.tv_sec 
	     << " " << time.tv_usec << endl;
	cerr << " offset is " << endpoint.tvClockOffset.tv_sec 
	     << " " << endpoint.tvClockOffset.tv_usec << endl;
	cerr << " local time is " << p.msg_time.tv_sec 
	     << " " << p.msg_time.tv_usec << endl;
#endif
	p.payload_len = payload_len;
	p.buffer = buf;

        // Do generic callbacks (vrpn_ANY_TYPE)
	who = generic_callbacks;

	while (who != NULL) {	// For each callback entry

		// Verify that the sender is ANY or matches
		if ( (who->sender == vrpn_ANY_SENDER) ||
		     (who->sender == sender) ) {
			if (who->handler(who->userdata, p)) {
				fprintf(stderr,
                                  "vrpn: vrpn_Connection::do_callbacks_for:  "
                                  "Nonzero user generic handler return\n");
				return -1;
			}
		}

		// Next callback in list
		who = who->next;
	}

	// Find the head for the list of callbacks to call
	who = my_types[type].who_cares;

	while (who != NULL) {	// For each callback entry

		// Verify that the sender is ANY or matches
		if ( (who->sender == vrpn_ANY_SENDER) ||
		     (who->sender == sender) ) {
			if (who->handler(who->userdata, p)) {
				fprintf(stderr,
                                  "vrpn: vrpn_Connection::do_callbacks_for:  "
                                  "Nonzero user handler return\n");
				return -1;
			}
		}

		// Next callback in list
		who = who->next;
	}

	return 0;
}

// Read all messages available on the given file descriptor (a TCP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.

int	vrpn_Connection::handle_tcp_messages (int fd,
                         const struct timeval * timeout)
{	int	sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval localTimeout;
	int	num_messages_read = 0;

#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_tcp_messages() called\n");
#endif

        if (timeout) {
	  localTimeout.tv_sec = timeout->tv_sec;
	  localTimeout.tv_usec = timeout->tv_usec;
        } else {
	  localTimeout.tv_sec = 0L;
	  localTimeout.tv_usec = 0L;
        }
        
	// Read incoming messages until there are no more characters to
	// read from the other side.  For each message, determine what
	// type it is and then pass it off to the appropriate handler
	// routine.

	do {
	  // Select to see if ready to hear from other side, or exception
	  FD_ZERO(&readfds);              /* Clear the descriptor sets */
	  FD_ZERO(&exceptfds);
	  FD_SET(fd, &readfds);     /* Check for read */
	  FD_SET(fd, &exceptfds);   /* Check for exceptions */
	  sel_ret = select(32,&readfds,NULL,&exceptfds, &localTimeout);
	  if (sel_ret == -1) {
	    if (errno == EINTR) { /* Ignore interrupt */
		continue;
	    } else {
		perror(
		  "vrpn: vrpn_Connection::handle_tcp_messages: select failed");
		return(-1);
	    }
	  }

	  // See if exceptional condition on socket
	  if (FD_ISSET(fd, &exceptfds)) {
		fprintf(stderr, "vrpn: vrpn_Connection::handle_tcp_messages: Exception on socket\n");
		return(-1);
	  }

	  // If there is anything to read, get the next message
	  if (FD_ISSET(fd, &readfds)) {
		vrpn_int32	header[5];
		struct timeval time;
		vrpn_int32	sender, type;
		vrpn_int32	len, payload_len, ceil_len;
#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_tcp_messages() something to read\n");
#endif

		// Read and parse the header
		if (vrpn_noint_block_read(fd,(char*)header,sizeof(header)) !=
			sizeof(header)) {
		  printf("vrpn_connection::handle_tcp_messages:  "
                         "Can't read header\n");
			return -1;
		}
		len = ntohl(header[0]);
		time.tv_sec = ntohl(header[1]);
		time.tv_usec = ntohl(header[2]);
		sender = ntohl(header[3]);
		type = ntohl(header[4]);
#ifdef	VERBOSE2
	printf("  header: Len %d, Sender %d, Type %d\n",
		(int)len, (int)sender, (int)type);
#endif

		// skip up to alignment
		vrpn_int32 header_len = sizeof(header);
		if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
		if (header_len > sizeof(header)) {
		  // the difference can be no larger than this
		  char rgch[vrpn_ALIGN];
		  if (vrpn_noint_block_read(fd,(char*)rgch,header_len-sizeof(header)) !=
		      (int)(header_len-sizeof(header))) {
		    printf("vrpn_connection::handle_tcp_messages:  "
                           "Can't read header + alignment\n");
		    return -1;
		  }
		}
		
		// Figure out how long the message body is, and how long it
		// is including any padding to make sure that it is a
		// multiple of four bytes long.
		payload_len = len - header_len;
		ceil_len = payload_len;
		if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}

		// Make sure the buffer is long enough to hold the whole
		// message body.
		if (sizeof(d_TCPinbufToAlignRight) < ceil_len) {
		     fprintf(stderr, "vrpn: vrpn_Connection::handle_tcp_messages: Message too long\n");
		     return -1;
		}

		// Read the body of the message 
		if (vrpn_noint_block_read(fd,d_TCPbuf,ceil_len) != ceil_len) {
		 perror("vrpn: vrpn_Connection::handle_tcp_messages: Can't read body");
		 return -1;
		}

		// Call the handler(s) for this message type
		// If one returns nonzero, return an error.
		if (type >= 0) {	// User handler, map to local id

                  // log regardless of whether local id is set,
                  // but only process if it has been (ie, log ALL
                  // incoming data -- use a filter on the log
                  // if you don't want some of it).
                  if (endpoint.d_logmode & vrpn_LOG_INCOMING) {
                    if (endpoint.log_message(payload_len, time,
                                             type,
                                             sender,
                                             d_TCPbuf, 1)) {
                      return -1;
                    }
                  }

		  if (endpoint.local_type_id(type) >= 0) {
		    if (do_callbacks_for(endpoint.local_type_id(type),
				         endpoint.local_sender_id(sender),
				         time, payload_len, d_TCPbuf)) {
		      return -1;
                    }
		  }

		} else {	// Call system handler if there is one

		 if (system_messages[-type] != NULL) {

		  if (endpoint.d_logmode & vrpn_LOG_INCOMING)
		    if (endpoint.log_message(payload_len, time, type, sender,
                                    d_TCPbuf, 1))
		      return -1;

		  // Fill in the parameter to be passed to the routines
		  vrpn_HANDLERPARAM p;
		  p.type = type;
		  p.sender = sender;
		  p.msg_time = time;
		  p.payload_len = payload_len;
		  p.buffer = d_TCPbuf;

		  if (system_messages[-type](this, p)) {
			    fprintf(stderr, "vrpn: vrpn_Connection::handle_tcp_messages: Nonzero system handler return\n");
			    return -1;
		  }
		 }

		}

		// Got one more message
		num_messages_read++;
	  }

	} while (sel_ret);

	return num_messages_read;
}

// Read all messages available on the given file descriptor (a UDP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.
// XXX At some point, the common parts of this routine and the TCP read
// routine should be merged.  Using read() on the UDP socket fails, so
// we need the UDP version.  If we use the UDP version for the TCP code,
// it hangs when we the client drops its connection, so we need the
// TCP code as well.

int	vrpn_Connection::handle_udp_messages (int fd,
                         const struct timeval * timeout)
{	int	sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval localTimeout;
	int	num_messages_read = 0;
	vrpn_int32	inbuf_len;
	char	*inbuf_ptr;

#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_udp_messages() called\n");
#endif

        if (timeout) {
	  localTimeout.tv_sec = timeout->tv_sec;
	  localTimeout.tv_usec = timeout->tv_usec;
        } else {
	  localTimeout.tv_sec = 0L;
	  localTimeout.tv_usec = 0L;
        }

	// Read incoming messages until there are no more packets to
	// read from the other side.  Each packet may have more than one
	// message in it.  For each message, determine what
	// type it is and then pass it off to the appropriate handler
	// routine.

	do {
	  // Select to see if ready to hear from server, or exception
	  FD_ZERO(&readfds);              /* Clear the descriptor sets */
	  FD_ZERO(&exceptfds);
	  FD_SET(fd, &readfds);     /* Check for read */
	  FD_SET(fd, &exceptfds);   /* Check for exceptions */
	  sel_ret = select(32, &readfds, NULL, &exceptfds, &localTimeout);
	  if (sel_ret == -1) {
	    if (errno == EINTR) { /* Ignore interrupt */
		continue;
	    } else {
		perror("vrpn: vrpn_Connection::handle_udp_messages: select failed()");
		return(-1);
	    }
	  }

	  // See if exceptional condition on socket
	  if (FD_ISSET(fd, &exceptfds)) {
		fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Exception on socket\n");
		return(-1);
	  }

	  // If there is anything to read, get the next message
	  if (FD_ISSET(fd, &readfds)) {
		vrpn_int32	header[5];
		struct timeval	time;
		vrpn_int32	sender, type;
		vrpn_uint32	len, payload_len, ceil_len;

#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_udp_messages() something to read\n");
#endif
		// Read the buffer and reset the buffer pointer
		inbuf_ptr = d_UDPinbuf;
		if ( (inbuf_len = recv(fd, d_UDPinbuf, sizeof(d_UDPinbufToAlignRight), 0)) == -1) {
		   perror("vrpn: vrpn_Connection::handle_udp_messages: recv() failed");
		   return -1;
		}

	      // Parse each message in the buffer
	      while ( (inbuf_ptr - d_UDPinbuf) != (vrpn_int32)inbuf_len) {

		// Read and parse the header
		// skip up to alignment
		vrpn_uint32 header_len = sizeof(header);
		if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
		
		if ( ((inbuf_ptr - d_UDPinbuf) + header_len) > (vrpn_uint32)inbuf_len) {
		   fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Can't read header");
		   return -1;
		}
		memcpy(header, inbuf_ptr, sizeof(header));
		inbuf_ptr += header_len;
		len = ntohl(header[0]);
		time.tv_sec = ntohl(header[1]);
		time.tv_usec = ntohl(header[2]);
		sender = ntohl(header[3]);
		type = ntohl(header[4]);

#ifdef VERBOSE
		printf("Message type %ld (local type %ld), sender %ld received\n",
			type,endpoint.local_type_id(type),sender);
#endif
		
		// Figure out how long the message body is, and how long it
		// is including any padding to make sure that it is a
		// multiple of vrpn_ALIGN bytes long.
		payload_len = len - header_len;
		ceil_len = payload_len;
		if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}

		// Make sure we received enough to cover the entire payload
		if ( ((inbuf_ptr - d_UDPinbuf) + ceil_len) > (vrpn_uint32)inbuf_len) {
		   fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Can't read payload");
		   return -1;
		}

		// Call the handler for this message type
		// If it returns nonzero, return an error.
		if (type >= 0) {	// User handler, map to local id


                  // log regardless of whether local id is set,
                  // but only process if it has been (ie, log ALL
                  // incoming data -- use a filter on the log
                  // if you don't want some of it).

                  if (endpoint.d_logmode & vrpn_LOG_INCOMING) {
		      if (endpoint.log_message(payload_len, time,
                                      type,
                                      sender,
                                               inbuf_ptr, 1)) {
                        return -1;
                      }
                  }
		  if (endpoint.local_type_id(type) >= 0) {
		    if (do_callbacks_for(endpoint.local_type_id(type),
				         endpoint.local_sender_id(sender),
				         time, payload_len, inbuf_ptr)) {
		      return -1;
                    }
		  }

		} else {	// System handler

		  if (endpoint.d_logmode & vrpn_LOG_INCOMING)
		    if (endpoint.log_message(payload_len, time, type, sender,
                                    inbuf_ptr, 1))
		      return -1;

		  // Fill in the parameter to be passed to the routines
		  vrpn_HANDLERPARAM p;
		  p.type = type;
		  p.sender = sender;
		  p.msg_time = time;
		  p.payload_len = payload_len;
		  p.buffer = inbuf_ptr;

		  if (system_messages[-type](this, p)) {
		    fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Nonzero system return\n");
		    return -1;
		  }
		}
		inbuf_ptr += ceil_len;

		// Got one more message
		num_messages_read++;
	    }
	  }

	} while (sel_ret);

	return num_messages_read;
}

int	vrpn_Connection::handle_type_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	vrpn_Connection * me = (vrpn_Connection*)userdata;
	cName	type_name;
	vrpn_int32	i;
	vrpn_int32	local_id;

	if (p.payload_len > sizeof(cName)) {
		fprintf(stderr,"vrpn: vrpn_Connection::Type name too long\n");
		return -1;
	}

	// Find out the name of the type (skip the length)
	strncpy(type_name, p.buffer + sizeof(vrpn_int32),
                p.payload_len - sizeof(vrpn_int32));

	// Use the exact length packed into the start of the buffer
	// to figure out where to put the trailing '\0'
	i = ntohl(*((vrpn_int32 *) p.buffer));
	type_name[i] = '\0';

#ifdef	VERBOSE
	printf("Registering other-side type: '%s'\n", type_name);
#endif
	// If there is a corresponding local type defined, find the mapping.
	// If not, clear the mapping.
	// Tell that the other side cares about it if found.
	local_id = -1; // None yet
	for (i = 0; i < me->num_my_types; i++) {
		if (strcmp(type_name, me->my_types[i].name) == 0) {
			local_id = i;
			me->my_types[i].cCares = 1;  // TCH 28 Oct 97
			break;
		}
	}
	if (me->endpoint.newRemoteType(type_name, local_id) == -1) {
		fprintf(stderr, "vrpn: Failed to add remote type %s\n", type_name);
		return -1;
	}

	return 0;
}

int	vrpn_Connection::handle_sender_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	vrpn_Connection * me = (vrpn_Connection*)userdata;
	cName	sender_name;
	vrpn_int32	i;
	vrpn_int32	local_id;

	if (p.payload_len > sizeof(cName)) {
	        fprintf(stderr,"vrpn: vrpn_Connection::Sender name too long\n");
		return -1;
	}

	// Find out the name of the sender (skip the length)
	strncpy(sender_name, p.buffer + sizeof(vrpn_int32),
		p.payload_len - sizeof(vrpn_int32));

	// Use the exact length packed into the start of the buffer
	// to figure out where to put the trailing '\0'
	i = ntohl(*((vrpn_int32 *) p.buffer));
	sender_name[i] = '\0';

#ifdef	VERBOSE
	printf("Registering other-side sender: '%s'\n", sender_name);
#endif
	// If there is a corresponding local sender defined, find the mapping.
	// If not, clear the mapping.
	local_id = -1; // None yet
	for (i = 0; i < me->num_my_senders; i++) {
		if (strcmp(sender_name, me->my_senders[i]) == 0) {
			local_id = i;
			break;
		}
	}
	if (me->endpoint.newRemoteSender(sender_name, local_id) == -1) {
		fprintf(stderr, "vrpn: Failed to add remote sender %s\n", sender_name);
		return -1;
	}

	return 0;
}

// This is called when a disconnect message is found in the logfile.
// It causes the other-side sender and type messages to be cleared,
// in anticipation of a possible new set of messages caused by a
// reconnected server.

int	vrpn_Connection::handle_disconnect_message(void *userdata,
		vrpn_HANDLERPARAM /*p*/)
{
	vrpn_Connection * me = (vrpn_Connection*)userdata;

#ifdef	VERBOSE
	printf("Just read disconnect message from logfile\n");
#endif
	me->endpoint.clear_other_senders_and_types();

	return 0;
}

int vrpn_Connection::register_handler(vrpn_int32 type,
			vrpn_MESSAGEHANDLER handler,
                        void *userdata, vrpn_int32 sender)
{
	vrpnMsgCallbackEntry	*new_entry;

	// Ensure that the type is a valid one (one that has been defined)
        //   OR that it is "any"
	if ( ( (type < 0) || (type >= num_my_types) ) &&
             (type != vrpn_ANY_TYPE) ) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: No such type\n");
		return -1;
	}

	// Ensure that the sender is a valid one (or "any")
	if ( (sender != vrpn_ANY_SENDER) &&
	     ( (sender < 0) || (sender >= num_my_senders) ) ) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: No such sender\n");
		return -1;
	}

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpnMsgCallbackEntry) == NULL) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;
	new_entry->sender = sender;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
#ifdef	VERBOSE
	printf("Adding user handler for type %ld, sender %ld\n",type,sender);
#endif

	if (type == vrpn_ANY_TYPE) {
	  new_entry->next = generic_callbacks;
	  generic_callbacks = new_entry;
	} else {
	  new_entry->next = my_types[type].who_cares;
	  my_types[type].who_cares = new_entry;
	};
	return 0;
}

int vrpn_Connection::unregister_handler(vrpn_int32 type,
			vrpn_MESSAGEHANDLER handler,
                        void *userdata, vrpn_int32 sender)
{
	// The pointer at *snitch points to victim
	vrpnMsgCallbackEntry	*victim, **snitch;

	// Ensure that the type is a valid one (one that has been defined)
        //   OR that it is "any"
	if ( ( (type < 0) || (type >= num_my_types) ) &&
	     (type != vrpn_ANY_TYPE) ) {
		fprintf(stderr,
			"vrpn_Connection::unregister_handler: No such type\n");
		return -1;
	}

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	if (type == vrpn_ANY_TYPE)
	  snitch = &generic_callbacks;
	else
	  snitch = &(my_types[type].who_cares);
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) ||
		  (victim->sender != sender) )){
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		    "vrpn_Connection::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Connection::message_type_is_registered (const char * name) const
{
	vrpn_int32	i;

	// See if the name is already in the list.  If so, return it.
	for (i = 0; i < num_my_types; i++)
		if (strcmp(my_types[i].name, name) == 0)
			return i;

	return -1;
}

// Changed 8 November 1999 by TCH
// With multiple connections allowed, TRYING_TO_CONNECT is an
// "ok" status, so we need to admit it.  (Used to check >= 0)
vrpn_bool vrpn_Connection::doing_okay (void) const {
  return (status >= TRYING_TO_CONNECT);
}

vrpn_bool vrpn_Connection::connected (void) const
{
  return (status == CONNECTED);
}

//------------------------------------------------------------------------
//	This section holds data structures and functions to open
// connections by name.
//	The intention of this section is that it can open connections for
// objects that are in different libraries (trackers, buttons and sound),
// even if they all refer to the same connection.

//	This routine will return a pointer to the connection whose name
// is passed in.  If the routine is called multiple times with the same
// name, it will return the same pointer, rather than trying to make
// multiple of the same connection.  If the connection is in a bad way,
// it returns NULL.
//	This routine will strip off any part of the string before and
// including the '@' character, considering this to be the local part
// of a device name, rather than part of the connection name.  This allows
// the opening of a connection to "Tracker0@ioglab" for example, which will
// open a connection to ioglab.

// this now always creates synchronized connections, but that is ok
// because they are derived from connections, and the default 
// args of freq=-1 makes it behave like a regular connection

vrpn_Connection * vrpn_get_connection_by_name
	 (const char * cname,
          const char * local_logfile_name,
          long local_log_mode,
          const char * remote_logfile_name,
          long remote_log_mode,
	  double dFreq,
	  int cSyncWindow,
          const char * NIC_IPaddress)
{
        vrpn_Connection * c;
	char			*where_at;	// Part of name past last '@'
	int port;
	int is_file = 0;

	if (cname == NULL) {
		fprintf(stderr,"vrpn_get_connection_by_name(): NULL name\n");
		return NULL;
	}

	// Find the relevant part of the name (skip past last '@'
	// if there is one)
	if ( (where_at = strrchr(cname, '@')) != NULL) {
		cname = where_at+1;	// Chop off the front of the name
	}

        c = vrpn_ConnectionManager::instance().getByName(cname);

	// If its not already open, open it.
        // Its constructor will add it to the list (?).
	if (!c) {

		// connections now self-register in the known list --
		// this is kind of odd, but oh well (can probably be done
		// more cleanly later).

		is_file = !strncmp(cname, "file:", 5);

                if (is_file)
                        c = new vrpn_File_Connection (cname, 
				  local_logfile_name, local_log_mode);
                else {
			port = vrpn_get_port_number(cname);
                        c = new vrpn_Synchronized_Connection
                                (cname, port,
                                 local_logfile_name, local_log_mode,
                                 remote_logfile_name, remote_log_mode,
                                 dFreq, cSyncWindow, NIC_IPaddress);
		}

	}

	// Return a pointer to the connection, even if it is not doing
	// okay. This will allow a connection to retry over and over
	// again before connecting to the server.
	return c;
}


// utility routines to parse names (<service>@<URL>)
char * vrpn_copy_service_name (const char * fullname)
{
  int len = 1 + strcspn(fullname, "@");
  char * tbuf = new char [len];
  if (!tbuf)
    fprintf(stderr, "vrpn_copy_service_name:  Out of memory!\n");
  else {
    strncpy(tbuf, fullname, len - 1);
    tbuf[len - 1] = 0;
//fprintf(stderr, "Service Name:  %s.\n", tbuf);
  }
  return tbuf;
}

char * vrpn_copy_service_location (const char * fullname)
{
  int offset = strcspn(fullname, "@");
  int len = strlen(fullname) - offset;
  char * tbuf = new char [len];
  if (!tbuf)
    fprintf(stderr, "vrpn_copy_service_name:  Out of memory!\n");
  else {
    strncpy(tbuf, fullname + offset + 1, len - 1);
    tbuf[len - 1] = 0;
//fprintf(stderr, "Service Location:  %s.\n", tbuf);
  }
  return tbuf;
}

char * vrpn_copy_file_name (const char * filespecifier)
{
  char * filename;
  const char * fp;
  int len;

  fp = filespecifier;
  if (!fp) return NULL;

  if (!strncmp(fp, "file://", 7)) {
    fp += 7;
    fp = strchr(fp, '/') + 1;
  } else if (!strncmp(fp, "file:", 5)) {
    fp += 5;
  }

  len = 1 + strlen(fp);
  filename = new char [len];
  if (!filename) 
    fprintf(stderr, "vrpn_copy_file_name:  Out of memory!\n");
  else {
    strncpy(filename, fp, len - 1);
    filename[len - 1] = 0;
//fprintf(stderr, "Filename:  %s.\n", filename);
  }
  return filename;
}

char * vrpn_copy_machine_name (const char * hostspecifier)
{
  int nearoffset = 0;
    // if it begins with "x-vrpn://" or "x-vrsh://" skip that
  int faroffset;
    // if it contains a ':', copy only the prefix before the last ':'
    // otherwise copy all of it
  int len;
  char * tbuf;

  if (!strncmp(hostspecifier, "x-vrpn://", 9) || 
      !strncmp(hostspecifier, "x-vrsh://", 9))
    nearoffset = 9;
  // stop at first occurrence of :<port #> or /<rsh arguments>
  faroffset = strcspn(hostspecifier + nearoffset, ":/");
  len = 1 + (faroffset ? faroffset : strlen(hostspecifier) - nearoffset);
  tbuf = new char [len];

  if (!tbuf)
    fprintf(stderr, "vrpn_copy_machine_name:  Out of memory!\n");
  else {
    strncpy(tbuf, hostspecifier + nearoffset, len - 1);
    tbuf[len - 1] = 0;
//fprintf(stderr, "Host Name:  %s.\n", tbuf);
  }
  return tbuf;
}


int vrpn_get_port_number (const char * hostspecifier)
{
  const char * pn;
  int port = vrpn_DEFAULT_LISTEN_PORT_NO;

  pn = hostspecifier;
  if (!pn) return -1;

  // skip over ':' in header
  if (!strncmp(pn, "x-vrpn://", 9) ||
      !strncmp(pn, "x-vrsh://", 9))
    pn += 9;

  pn = strrchr(pn, ':');
  if (pn) {
    pn++;
    port = atoi(pn);
  }

//fprintf(stderr, "Port Number:  %d.\n", port);

  return port;
}

char * vrpn_copy_rsh_program (const char * hostspecifier)
{
  int nearoffset = 0; // location of first char after machine name
  int faroffset; // location of last character of program name
  int len;
  char * tbuf;

  if (!strncmp(hostspecifier, "x-vrpn://", 9) ||
      !strncmp(hostspecifier, "x-vrsh://", 9))
    nearoffset += 9;
  nearoffset += strcspn(hostspecifier + nearoffset, "/");
  nearoffset++; // step past the '/'
  faroffset = strcspn(hostspecifier + nearoffset, ",");
  len = 1 + (faroffset ? faroffset : strlen(hostspecifier) - nearoffset);
  tbuf = new char [len];

  if (!tbuf)
    fprintf(stderr, "vrpn_copy_rsh_program: Out of memory!\n");
  else {
    strncpy(tbuf, hostspecifier + nearoffset, len - 1);
    tbuf[len-1] = 0;
    //fprintf(stderr, "server program: '%s'.\n", tbuf);
  }
  return tbuf;
}

char * vrpn_copy_rsh_arguments (const char * hostspecifier)
{
  int nearoffset = 0; // location of first char after server name
  int faroffset; // location of last character
  int len;
  char * tbuf;

  if (!strncmp(hostspecifier, "x-vrpn://", 9) ||
      !strncmp(hostspecifier, "x-vrsh://", 9))
    nearoffset += 9;

  nearoffset += strcspn(hostspecifier + nearoffset, "/");
  nearoffset += strcspn(hostspecifier + nearoffset, ",");
  faroffset = strlen(hostspecifier);
  len = 1 + faroffset - nearoffset;
  tbuf = new char [len];

  if (!tbuf)
    fprintf(stderr, "vrpn_copy_rsh_arguments: Out of memory!\n");
  else {
    strncpy(tbuf, hostspecifier + nearoffset, len - 1);
    tbuf[len-1] = 0;
    //fprintf(stderr, "server args: '%s'.\n", tbuf);
  }
  return tbuf;
}
