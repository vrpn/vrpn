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

#ifdef	_AIX
#include <sys/termio.h>
#endif

#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

/*#if defined(__CYGWIN__)
#include <sys/unistd.h>
#endif*/

#if defined(_WIN32)
#include <io.h>
#if defined(__CYGWIN__)
#include <netinet/in.h>
#include <sys/socket.h>
#else
//#include <afxcoll.h>
#endif
#endif

#ifdef FreeBSD
#include <termios.h>
#endif

#include "vrpn_Shared.h"
#include "vrpn_Serial.h"

//#define VERBOSE

// HACK - T. Hudson April 00
#if defined(__CYGWIN__)
#define vrpn_closesocket closesocket
#define vrpn_readsocket(a,b,c) recv(a,b,c,0)
#define vrpn_writesocket(a,b,c) send(a, (char *) b,c,0)
#else
#define vrpn_closesocket close
#define vrpn_readsocket read
#define vrpn_writesocket write
#endif

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

#if defined(_WIN32) && !defined(__CYGWIN__)
static void **commConnections;
static int curCom = -1;
static int maxCom = 10;
#endif

int vrpn_open_commport(char *portname, long baud, int charsize, vrpn_SER_PARITY parity)
{
#if defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix) || defined(FreeBSD) || defined(__CYGWIN__)
	fprintf(stderr,
		"vrpn_open_commport: Not implemented in sparc, ultrix, HP, or Cygwin\n");
	return -1;
#else

#if defined(_WIN32)
  DCB dcb;
  COMMTIMEOUTS cto;
  HANDLE hCom;
  BOOL fSuccess;
#else
  int fileDescriptor;
  struct termio   sttyArgs;
#endif


#if defined(_WIN32)
  hCom = CreateFile( portname,    GENERIC_READ | GENERIC_WRITE,
					 0,    // comm devices must be opened w/exclusive-access 
					 NULL, // no security attributes 
					 OPEN_EXISTING, // comm devices must use OPEN_EXISTING 
					 0, // not overlapped I/O 
					 NULL);  // hTemplate must be NULL for comm devices     );

  if (curCom == -1)
	  commConnections = (void**)new char[maxCom];
  else
  {
	  if (curCom >= maxCom)
	  {
		  void *old = commConnections;
		  maxCom *= 2;
		  commConnections = (void**)new char[maxCom];
		  delete [] old;
	  }

  }
  
  if (hCom == INVALID_HANDLE_VALUE) 
  {    
    perror("vrpn_open_commport: cannot open serial port");
    return -1;
  }
 
  if (!(fSuccess = GetCommState(hCom, &dcb)))
  {
	perror("vrpn_open_commport: cannot get serial port configuration settings");
	CloseHandle(hCom);
    return -1;
  }

  switch (baud) {
  case   300: dcb.BaudRate =   CBR_300; break;
  case  1200: dcb.BaudRate =  CBR_1200; break;
  case  2400: dcb.BaudRate =  CBR_2400; break;
  case  4800: dcb.BaudRate =  CBR_4800; break;
  case  9600: dcb.BaudRate =  CBR_9600; break;
  case 19200: dcb.BaudRate =  CBR_19200; break;
  case 38400: dcb.BaudRate =  CBR_38400; break;
  case 57600: dcb.BaudRate =  CBR_57600; break;
  case 115200: dcb.BaudRate =  CBR_115200; break;
  default:
    fprintf(stderr,"vrpn_open_commport: unknown baud rate %ld\n",baud);
	CloseHandle(hCom);
    return -1;
  }   

  switch (parity) {
  case vrpn_SER_PARITY_NONE:
      dcb.fParity = FALSE;
      dcb.Parity = NOPARITY;
      break;
  case vrpn_SER_PARITY_ODD:
      dcb.fParity = TRUE;
      dcb.Parity = 1;
      break;
  case vrpn_SER_PARITY_EVEN:
      dcb.fParity = TRUE;
      dcb.Parity = 2;
      break;
  case vrpn_SER_PARITY_MARK:
      dcb.fParity = TRUE;
      dcb.Parity = 3;
      break;
  case vrpn_SER_PARITY_SPACE:
      dcb.fParity = TRUE;
      dcb.Parity = 3;
      break;
  default:
      fprintf(stderr,"vrpn_open_commport: unknown parity setting\n");
      CloseHandle(hCom);
      return -1;
  }

  dcb.StopBits = ONESTOPBIT;
  if (charsize == 8)
     dcb.ByteSize = 8;
  else if (charsize == 7)
     dcb.ByteSize = 7;
  else {
     fprintf(stderr, "vrpn_open_commport: unknown character size (charsize = %d)\n", charsize); 
     CloseHandle(hCom);
     return -1;
  }

  if (!(fSuccess = SetCommState(hCom, &dcb)))
  {
	perror("vrpn_open_commport: cannot set serial port configuration settings");
	CloseHandle(hCom);
    return -1;
  }

  cto.ReadIntervalTimeout = MAXDWORD;
  cto.ReadTotalTimeoutMultiplier = MAXDWORD;
  cto.ReadTotalTimeoutConstant = 1;
  cto.WriteTotalTimeoutConstant = 0;
  cto.WriteTotalTimeoutMultiplier = 0;

  if (!(fSuccess = SetCommTimeouts(hCom, &cto)))
  {
	perror("vrpn_open_commport: cannot set serial port timeouts");
	CloseHandle(hCom);
    return -1;
  }

  curCom++;
  commConnections[curCom] = hCom;

  return curCom;

  // -- This section is "Win32"
#else
  // -- This section is "Not win32"

  // Open the serial port for r/w
#ifdef sol
  if ( (fileDescriptor = open(portname, O_RDWR|O_NDELAY|O_NOCTTY)) == -1) {
#else
  if ( (fileDescriptor = open(portname, O_RDWR)) == -1) {
#endif
    perror("vrpn_open_commport: cannot open serial port");
    return -1;
  }

  /* get current settings */
  if ( ioctl(fileDescriptor, TCGETA , &sttyArgs) == -1 ) {
    perror("vrpn_open_commport: ioctl failed");
    return(-1);
  }

  /* override old values 	*/
  // note, newer sgi's don't support regular ("old") stty speed
  // settings -- they require the use of the new ospeed field to 
  // use settings above 38400.  The O2 line can use any baud rate 
  // up 460k (no fixed settings).
#if defined(sgi) && defined(__NEW_MAX_BAUD)
  sttyArgs.c_ospeed = baud;
  sttyArgs.c_ispeed = baud;
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
#endif //End B57600
#ifdef B115200
  case 115200: sttyArgs.c_cflag |=  B115200; break;
#endif //End B115200
  default:
    fprintf(stderr,"vrpn_open_commport: unknown baud rate %ld\n",baud);
    return -1;
  }
#endif //SGI NEX MAX BAUD check


  sttyArgs.c_iflag = (IGNBRK | IGNPAR);  /* Ignore break & parity errs */
  sttyArgs.c_oflag = 0;                  /* Raw output, leave tabs alone */
  sttyArgs.c_lflag = 0;              /* Raw input (no KILL, etc.), no echo */
  
  sttyArgs.c_cflag &= ~CSIZE;
  if (charsize == 8)
     sttyArgs.c_cflag |= CSIZE & CS8;  /* 8 bits */
  else if (charsize == 7)
     sttyArgs.c_cflag |= CSIZE & CS7;  /* 7 bits */
  else {
     fprintf(stderr, "vrpn_open_commport: unknown character size (charsize = %d)\n", charsize);
     return -1;
  }
  sttyArgs.c_cflag &= ~CSTOPB;	/* One stop bit */

  switch (parity) {
  case vrpn_SER_PARITY_NONE:
      sttyArgs.c_cflag &= ~PARENB;	/* No parity */
      break;
  case vrpn_SER_PARITY_ODD:
      sttyArgs.c_cflag |= PARENB | PARODD; 
      break;
  case vrpn_SER_PARITY_EVEN:
      sttyArgs.c_cflag |= PARENB;
      sttyArgs.c_cflag &= ~PARODD;
      break;
  case vrpn_SER_PARITY_MARK:
  case vrpn_SER_PARITY_SPACE:
  default:
      fprintf(stderr,"vrpn_open_commport: unsupported parity setting (only none, odd and even)\n");
      return -1;
  }

  sttyArgs.c_cflag |= CREAD;	/* Allow reading */
  sttyArgs.c_cflag |= CLOCAL;	/* No modem between us and device */

  sttyArgs.c_cc[VMIN] = 0;	/* Return read even if no chars */
  sttyArgs.c_cc[VTIME] = 0;	/* Return without waiting */
  
  /* pass the new settings back to the driver */
  if ( ioctl(fileDescriptor, TCSETA, &sttyArgs) == -1 ) {
    perror("vrpn_open_commport: ioctl failed");
    close(fileDescriptor);
    return(-1);
  }
  
  return(fileDescriptor);
#endif  // _WIN32

#endif // !defined(...lots of stuff...)
}

//When finished close the commport.
int vrpn_close_commport(int comm)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
	return CloseHandle(commConnections[comm]);

	for(int i = comm; i < curCom - 1; i++)
		commConnections[i] = commConnections[i+1];

	commConnections[curCom--] = NULL;

	if (curCom == -1)
		delete [] commConnections;

#else
	return vrpn_closesocket(comm);
#endif
}

#endif  // VRPN_CLIENT_ONLY

// empty the input buffer (discarding the chars)
// Return 0 on success, -1 on failure.
// NOT CALLED!  OBSOLETE? -- no ... used by vrpn_Flock
int vrpn_flush_input_buffer(int comm)
{
#if defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix) || defined(__CYGWIN__)
   fprintf(stderr,
	"vrpn_flush_input_buffer: Not impemented on cygwin, sparc, ultrix, or HP\n");
   return -1;
#else
#if defined(_WIN32)
   return PurgeComm(commConnections[comm], PURGE_RXCLEAR);
#else
   return tcflush(comm, TCIFLUSH);
#endif
#endif
}

// empty the output buffer, discarding all of the chars
// Return 0 on success, tc err codes other on failure.
// NOT CALLED!  OBSOLETE? -- no ... used by vrpn_Flock
int vrpn_flush_output_buffer(int comm)
{
#if defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix) || defined(__CYGWIN__)
   fprintf(stderr,
	"vrpn_flush_output_buffer: Not impemented on NT, sparc, ultrix, HP, or cygwin\n");
   return -1;
#else

#if defined(_WIN32)
   return FlushFileBuffers(commConnections[comm]);
#else
   return tcflush(comm, TCOFLUSH);
#endif

#endif
}

// empty the output buffer, waiting for all of the chars to be delivered
// Return 0 on success, tc err codes on failure.
// NOT CALLED!  OBSOLETE? -- no ... used by vrpn_Flock
int vrpn_drain_output_buffer(int comm)
{
#if defined(sparc) || defined(hpux) || defined(__hpux) || defined(ultrix) || defined(__CYGWIN__)
   fprintf(stderr,
	"vrpn_drain_output_buffer: Not impemented on NT, sparc, ultrix, or HP\n");
   return -1;
#else

#if defined(_WIN32)
   return PurgeComm(commConnections[comm], PURGE_TXCLEAR);
#else
   return tcflush(comm, TCIFLUSH);
#endif

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
#if defined(_WIN32) && !defined(__CYGWIN__)
   BOOL fSuccess;
   DWORD numRead;
   COMSTAT cstat;
   DWORD errors;
   OVERLAPPED Overlapped;


   Overlapped.Offset = 0;
   Overlapped.OffsetHigh = 0;
   Overlapped.hEvent = NULL;

   if (!(fSuccess = ClearCommError(commConnections[comm], &errors, &cstat)))
   {
	   perror("vrpn_read_available_characters: can't get current status");
	   return(-1);
   }

   if (cstat.cbInQue > 0)
   {
	   if(!(fSuccess = ReadFile(commConnections[comm], buffer, bytes, &numRead, &Overlapped)))
	   {
		   perror("vrpn_read_available_characters: can't read from serial port");
		   return(-1);
	   }

	   return numRead;
   }
   
   return 0;
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
     if ((cReadThisTime = vrpn_readsocket(comm, (char *) pch,
                                          cSpaceLeft)) == -1) {
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
		struct timeval *timeout) 
{
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
		if (sofar == bytes) { break; }
		where += ret;
		gettimeofday(&now, NULL);
	} while ( (timeout != NULL) && !(time_greater(now,finish)) );

	return sofar;
}


#endif  // VRPN_CLIENT_ONLY


#if 0
// T. Hudson April 00
#if defined(_WIN32) && defined(__CYGWIN__)
// [juliano 9/17/99]
//   added this decl because otherwise I cannot compile vrpn_Serial.C
//   on my PC in cygwin.  I have this problem with both egcs-2.91.57
//   and gcc-2.95.
int write( int fildes, const void *buf, size_t nbyte );
#endif
#endif

// Write the buffer to the serial port

int vrpn_write_characters(int comm, const unsigned char *buffer, int bytes)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
   BOOL fSuccess;
   DWORD numWritten;
   OVERLAPPED Overlapped;

   Overlapped.Offset = 0;
   Overlapped.OffsetHigh = 0;
   Overlapped.hEvent = NULL;

	if(!(fSuccess = WriteFile(commConnections[comm], buffer, bytes, &numWritten, &Overlapped)))
    {
	   perror("vrpn_write_characters: can't write to serial port");
	   return(-1);
    }

	return numWritten;
#else
	return vrpn_writesocket(comm, buffer, bytes);
#endif
}
