#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef linux
#include <termios.h>
#endif

#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#ifdef	_WIN32
#include <io.h>
#endif

#ifdef FreeBSD
#include <termios.h>
#endif

#include "vrpn_Shared.h"
#include "vrpn_Serial.h"

//#define VERBOSE

#ifndef VRPN_CLIENT_ONLY

#define time_add(t1,t2, tr)     { (tr).tv_sec = (t1).tv_sec + (t2).tv_sec ; \
                                  (tr).tv_usec = (t1).tv_usec + (t2).tv_usec ; \
                                  if ((tr).tv_usec >= 1000000L) { \
                                        (tr).tv_sec++; \
                                        (tr).tv_usec -= 1000000L; \
                                  } }
#define time_greater(t1,t2)     ( (t1.tv_sec > t2.tv_sec) || \
                                 ((t1.tv_sec == t2.tv_sec) && \
                                  (t1.tv_usec > t2.tv_usec)) )


int vrpn_open_commport(char *portname, long baud)
{
#if defined(_WIN32) || defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix) || defined(FreeBSD)
	fprintf(stderr,
		"vrpn_open_commport: Not implemented in NT, sparc, ultrix, or HP\n");
	return -1;
#else
  int fileDescriptor;
  struct termio   sttyArgs;


  // Open the serial port for r/w
  if ( (fileDescriptor = open(portname, O_RDWR)) == -1) {
    perror("Tracker: cannot open serial port");
    return -1;
  }

  /* get current settings */
  if ( ioctl(fileDescriptor, TCGETA , &sttyArgs) == -1 ) {
    perror("Tracker: ioctl failed");
    return(-1);
  }

  /* override old values 	*/
  // note, newer sgi's don't support regular ("old") stty speed
  // settings -- they require the use of the new ospeed field to 
  // use settings above 38400.  The O2 line can use any baud rate 
  // up 460k (no fixed settings).
#if defined(sgi) && defined(__NEW_MAX_BAUD)
  sttyArgs.c_ospeed = baud;
#else
  sttyArgs.c_cflag &= ~CBAUD;
  switch (baud) {
  case   300: sttyArgs.c_cflag |=   B300; break;
  case  1200: sttyArgs.c_cflag |=  B1200; break;
  case  2400: sttyArgs.c_cflag |=  B2400; break;
  case  4800: sttyArgs.c_cflag |=  B4800; break;
  case  9600: sttyArgs.c_cflag |=  B9600; break;
  case 19200: sttyArgs.c_cflag |=  B19200; break;
  case 38400: sttyArgs.c_cflag |=  B38400; break;
#ifdef B57600
  case 57600: sttyArgs.c_cflag |=  B57600; break;
#endif
#ifdef B115200
  case 115200: sttyArgs.c_cflag |=  B115200; break;
#endif
  default:
    fprintf(stderr,"vrpn: Serial tracker: unknown baud rate %ld\n",baud);
    return -1;
  }
#endif

  sttyArgs.c_iflag = (IGNBRK | IGNPAR);  /* Ignore break & parity errs */
  sttyArgs.c_oflag = 0;                  /* Raw output, leave tabs alone */
  sttyArgs.c_lflag = 0;              /* Raw input (no KILL, etc.), no echo */
  
  sttyArgs.c_cflag |= CS8;	/* 8 bits */
  sttyArgs.c_cflag &= ~CSTOPB;	/* One stop bit */
  sttyArgs.c_cflag &= ~PARENB;	/* No parity */
  sttyArgs.c_cflag |= CREAD;	/* Allow reading */
  sttyArgs.c_cflag |= CLOCAL;	/* No modem between us and device */

  sttyArgs.c_cc[VMIN] = 0;	/* Return read even if no chars */
  sttyArgs.c_cc[VTIME] = 0;	/* Return without waiting */
  
  /* pass the new settings back to the driver */
  if ( ioctl(fileDescriptor, TCSETA, &sttyArgs) == -1 ) {
    perror("Tracker: ioctl failed");
    close(fileDescriptor);
    return(-1);
  }
  
  return(fileDescriptor);
#endif  // _WIN32
}

#endif  // VRPN_CLIENT_ONLY

// empty the input buffer (discarding the chars)
// Return 0 on success, -1 on failure.
// NOT CALLED!  OBSOLETE? -- no ... used by vrpn_Flock
int vrpn_flush_input_buffer(int comm)
{
#if defined(_WIN32) || defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix)
   fprintf(stderr,
	"vrpn_flush_input_buffer: Not impemented on NT, sparc, ultrix, or HP\n");
   return -1;
#else
   return tcflush(comm, TCIFLUSH);
#endif
}

// empty the output buffer, discarding all of the chars
// Return 0 on success, tc err codes other on failure.
// NOT CALLED!  OBSOLETE? -- no ... used by vrpn_Flock
int vrpn_flush_output_buffer(int comm)
{
#if defined(_WIN32) || defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix)
   fprintf(stderr,
	"vrpn_flush_output_buffer: Not impemented on NT, sparc, ultrix, or HP\n");
   return -1;
#else
   return tcflush(comm, TCOFLUSH);
#endif
}

// empty the output buffer, waiting for all of the chars to be delivered
// Return 0 on success, tc err codes on failure.
// NOT CALLED!  OBSOLETE? -- no ... used by vrpn_Flock
int vrpn_drain_output_buffer(int comm)
{
#if defined(_WIN32) || defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix)
   fprintf(stderr,
	"vrpn_drain_output_buffer: Not impemented on NT, sparc, ultrix, or HP\n");
   return -1;
#else
   return tcdrain(comm);
#endif
}

#ifndef VRPN_CLIENT_ONLY

// This routine will read any available characters from the handle into
// the buffer specified, up to the number of bytes requested.  It returns
// the number of characters read or -1 on failure.  Note that it only
// reads characters that are available at the time of the read, so less
// than the requested number may be returned.
int vrpn_read_available_characters(int comm, unsigned char *buffer, int bytes)
{
#ifdef	_WIN32
	fprintf(stderr,"vrpn_read_available_characters: Not yet on NT\n");
	return -1;
#else
   int bRead;

   // on sgi's (and possibly other architectures) the folks from 
   // ascension have noticed that a read command will not necessarily
   // read everything available in the read buffer (see the following file:
   // /afs/cs.unc.edu/project/hmd/src/libs/tracker/apps/ascension/FLOCK232/C/unix.txt
   // For this reason, we loop and try to fill the buffer, stopping our
   // loop whenever no chars are available.
   int cReadThisTime = 0;
   int cSpaceLeft = bytes;
   unsigned char *pch = buffer;

   do {
     if ( (cReadThisTime = read(comm, (char *)pch, cSpaceLeft)) == -1) {
       perror("vrpn_read_available_characters: cannot read from serial port");
       fprintf(stderr, "buffer = %p, %d\n", pch, bytes);  
       return -1;
     }
     cSpaceLeft -= cReadThisTime;
     pch += cReadThisTime;
   }  while ((cReadThisTime!=0) && (cSpaceLeft>0));
   bRead = pch - buffer;
 
   return bRead;
#endif
}

// Read until either you get the answer, you get an error, or the timeout
// occurs.  If there is a NULL pointer, block indefinitely. Return the number
// of characters read.

int vrpn_read_available_characters(int comm, unsigned char *buffer, int bytes,
		struct timeval *timeout) {
	struct	timeval	start, finish, now;
	int	sofar = 0, ret;	// How many characters we have read so far
	unsigned char *where = buffer;

	// Find out what time it is at the start, and when we should end
	// (unless the timeout is NULL)
	if (timeout != NULL) {
		gettimeofday(&start, NULL);
		time_add(start, *timeout, finish);
	}
	
	// Keep reading until we timeout.  Exit from the middle of this by
	// returning if we complete or find an error so that the loop only has
	// to check for timeout, not other terminating conditions.
	do {
		ret = vrpn_read_available_characters(comm, where, bytes-sofar);
		if (ret == -1) { return -1; }
		sofar += ret;
		where += ret;
		gettimeofday(&now, NULL);
	} while ( (timeout != NULL) && !(time_greater(now,finish)) );

	return sofar;
}


#endif  // _WIN32

