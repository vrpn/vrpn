#include "vrpn_ConnectionCommonStuff.h"
#include "vrpn_CommonSystemIncludes.h"

//-----------------------------------------------------------------------
// GLOBAL UTILITY  FUNCTIONS
//-----------------------------------------------------------------------


//---------------------------------------------------------------------------
// This routine opens a UDP socket with the requested port number.
// The socket is not bound to any particular outgoing address.
// The routine returns -1 on failure and the file descriptor on success.
// The portno parameter is filled in with the actual port that is opened
// (this is useful when the address INADDR_ANY is used, since it tells
// what port was opened).

vrpn_int32 vrpn_open_udp_socket (vrpn_uint16 * portno)
{
   vrpn_int32 sock;
   struct sockaddr_in server_addr;
   struct sockaddr_in udp_name;
   vrpn_int32	udp_namelen = sizeof(udp_name);

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)  {
	perror("vrpn: vrpn_open_udp_socket: can't open socket");
	return -1;
   }

   // bind to local address
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(*portno);
   server_addr.sin_addr.s_addr = htonl((u_long)0); // don't care which client
   if (bind(sock, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0){
	perror("vrpn: vrpn_open_udp_socket: can't bind address");
	return -1;
   }

   // Find out which port was actually bound
   if (getsockname(sock, (struct sockaddr *) &udp_name,
                   GSN_CAST (int *) &udp_namelen)) {
	fprintf(stderr, "vrpn: open_udp_socket: cannot get socket name.\n");
	return -1;
   }
   *portno = ntohs(udp_name.sin_port);

   return sock;
}

// used for debugging time synch code
void printTime( char *pch, const timeval& tv ) {
    cerr << pch << " " << tv.tv_sec*1000.0 + tv.tv_usec/1000.0
         << " msecs." << endl;
}

// Marshall the message into the buffer if it will fit.  Return the number
// of characters sent.
vrpn_uint32 vrpn_marshall_message(
	char *outbuf,		// Base pointer to the output buffer
	vrpn_uint32 outbuf_size,// Total size of the output buffer
	vrpn_uint32 initial_out,// How many characters are already in outbuf
	vrpn_uint32 len,	// Length of the message payload
	struct timeval time,	// Time the message was generated
	vrpn_int32 type,	// Type of the message
	vrpn_int32 sender,	// Sender of the message
	const char * buffer)	// Message payload
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
   if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
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


//------------------------------------------------------------------------
//	This section holds data structures and functions to open
// connections by name.
//	The intention of this section is that it can open connections for
// objects that are in different libraries (trackers, buttons and sound),
// even if they all refer to the same connection.

// utility routines to parse names (<service>@<URL>)
char * vrpn_copy_service_name (const char * fullname)
{
  vrpn_int32 len = 1 + strcspn(fullname, "@");
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
  vrpn_int32 offset = strcspn(fullname, "@");
  vrpn_int32 len = strlen(fullname) - offset;
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
  vrpn_int32 len;

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
  vrpn_int32 nearoffset = 0;
    // if it begins with "x-vrpn://" or "x-vrsh://" skip that
  vrpn_int32 faroffset;
    // if it contains a ':', copy only the prefix before the last ':'
    // otherwise copy all of it
  vrpn_int32 len;
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


vrpn_int32 vrpn_get_port_number (const char * hostspecifier)
{
  const char * pn;
  vrpn_int32 port = vrpn_DEFAULT_LISTEN_PORT_NO;

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
  vrpn_int32 nearoffset = 0; // location of first char after machine name
  vrpn_int32 faroffset; // location of last character of program name
  vrpn_int32 len;
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
  vrpn_int32 nearoffset = 0; // location of first char after server name
  vrpn_int32 faroffset; // location of last character
  vrpn_int32 len;
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

// check if the network interface supports multicast.
// uses system calls and unix utilites to determine
// whether interface supports multicast. not the most
// efficient method, i admit. also, haven't figured out how
// to do it in windows yet, so it always returns false.
// - sain 7/99
vrpn_bool vrpn_am_i_mcast_capable(){

	// #include <fcntl.h>

	vrpn_int32 fd, mcast_capable;
	char buffer[8];
	
#ifdef _WIN32
	return vrpn_false;
#else
	system("ifconfig -a | grep MULTICAST | wc -l > mcast.tmp");
	fd = open("mcast.tmp",O_RDONLY);
	read(fd,buffer,sizeof(buffer));
	mcast_capable = atoi(buffer);
	close(fd);
	system("rm -f mcast.tmp");
	if ( mcast_capable > 0 ){
		return vrpn_true;
	}
	else{
		return vrpn_false;
	}
#endif
}


/**********************
 *   This function returns the host IP address in string form.  For example,
 * the machine "ioglab.cs.unc.edu" becomes "152.2.130.90."  This is done
 * so that the remote host can connect back even if it can't resolve the
 * DNS name of this host.  This is especially useful at conferences, where
 * DNS may not even be running.
 **********************/

int	vrpn_getmyIP(char *myIPchar, vrpn_int32 maxlen)
{	
	char	myname[100];		// Host name of this host
    struct	hostent *host;          // Encoded host IP address, etc.
	char	myIPstring[100];	// Hold "152.2.130.90" or whatever

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
	if ((vrpn_int32)strlen(myIPstring) > maxlen) {
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
vrpn_int32 vrpn_noint_select(
    vrpn_int32 width, 
    fd_set *readfds, 
    fd_set *writefds, 
    fd_set *exceptfds, 
    struct timeval * timeout)
{
	fd_set	tmpread, tmpwrite, tmpexcept;
	vrpn_int32	ret;
	vrpn_int32	done = 0;
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
                vrpn_uint32	usec_left;
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

#if !defined(_WIN32)

vrpn_int32 vrpn_noint_block_write (
    int outfile, 
    const char buffer[], 
    vrpn_int32 length)
{
        register vrpn_int32    sofar;          /* How many characters sent so far */
        register vrpn_int32    ret;            /* Return value from write() */

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

vrpn_int32 vrpn_noint_block_read(
    int infile, 
    char buffer[], 
    vrpn_int32 length)
{
        register vrpn_int32    sofar;          /* How many we read so far */
        register vrpn_int32    ret;            /* Return value from the read() */

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

#else /* _WIN32 */

vrpn_int32 vrpn_noint_block_write(
    SOCKET outsock, 
    char *buffer, 
    vrpn_int32 length)
{
	vrpn_int32 nwritten, sofar = 0;
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

vrpn_int32 vrpn_noint_block_read(
    SOCKET insock, 
    char *buffer, 
    vrpn_int32 length)
{
    vrpn_int32 nread, sofar = 0;  
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


#endif /* _WIN32 */


/*	This routine will read in a block from the file descriptor.
 * It acts just like the read() routine on normal files, except that
 * it will time out if the read takes too long.
 *      This will also take care of problems caused by interrupted system
 * calls, retrying the read when they occur.
 *	This routine will either read the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data arrives), or return the number
 * of characters read before timeout (in the case of a timeout). */

#ifdef _WIN32
vrpn_int32 vrpn_noint_block_read_timeout(
    SOCKET infile, 
    char buffer[], 
    vrpn_int32 length, 
    struct timeval *timeout)
#else
vrpn_int32 vrpn_noint_block_read_timeout(
    vrpn_int32 infile, 
    char buffer[], 
    vrpn_int32 length, 
    struct timeval *timeout)
#endif
{
    register vrpn_int32    sofar;          /* How many we read so far */
    register vrpn_int32    ret;            /* Return value from the read() */
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
		stop = vrpn_TimevalSum(start, *timeout);/* Find stop time */
	} else {
		timeout2ptr = timeout;
	}

    sofar = 0;
    do {	
		vrpn_int32	sel_ret;
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
#ifndef _WIN32
        ret = read(infile, buffer+sofar, 1);
        sofar += ret;
        
        /* Ignore interrupted system calls - retry */
        if ( (ret == -1) && (errno == EINTR) ) {
            ret = 1;	/* So we go around the loop again */
            sofar += 1;	/* Restoring it from above -1 */
        }
#else
        {
            vrpn_int32 nread;
            nread = recv(infile, buffer+sofar, length-sofar, 0);
            sofar += nread;
            ret = nread;
        }
#endif
        
    } while ((ret > 0) && (sofar < length));

#ifndef _WIN32
    if (ret == -1) return(-1);	/* Error during read */
#endif
    if (ret == 0) return(0);	/* EOF reached */
    
    return(sofar);			/* All bytes read */
}

