#include "vrpn_ConnectionOldCOmmonStuff.h"

//-----------------------------------------------------------------------
// GLOBAL UTILITY  FUNCTIONS
//-----------------------------------------------------------------------

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
          vrpn_int32 local_log_mode,
          const char * remote_logfile_name,
          vrpn_int32 remote_log_mode,
	  vrpn_float64 dFreq,
	  vrpn_int32 cSyncWindow)
{
	vrpn_KNOWN_CONNECTION	*curr = known;
	char			*where_at;	// Part of name past last '@'
	vrpn_int32 port;
	vrpn_int32 is_file = 0;

	if (cname == NULL) {
		fprintf(stderr,"vrpn_get_connection_by_name(): NULL name\n");
		return NULL;
	}

	// Find the relevant part of the name (skip past last '@'
	// if there is one)
	if ( (where_at = strrchr(cname, '@')) != NULL) {
		cname = where_at+1;	// Chop off the front of the name
	}

	// See if the connection is one that is already open.
	// Leave curr pointing to it if so, or to NULL if not.
	while ( (curr != NULL) && (strcmp(cname, curr->name) != 0)) {
		curr = curr->next;
	}

	// If its not already open, open it and add it to the list.
	if (curr == NULL) {

		// connections now self-register in the known list --
		// this is kind of odd, but oh well (can probably be done
		// more cleanly later).

		is_file = !strncmp(cname, "file:", 5);

                if (is_file)
                        new vrpn_File_Connection (cname);
                else {
			port = vrpn_get_port_number(cname);
                        new vrpn_Synchronized_Connection
                                (cname, port,
                                 local_logfile_name, local_log_mode,
                                 remote_logfile_name, remote_log_mode,
                                 dFreq, cSyncWindow);
		}

		// and now set curr properly (it will have been added to list
		// by the vrpn_Connection constructor)

		curr = known;
		while ( (curr != NULL) && (strcmp(cname, curr->name) != 0)) {
		  curr = curr->next;
		}
		if (!curr) {
		  // unable to open connection
		  return NULL;
		}
	}

	// See if the connection is okay.  If so, return it.  If not, NULL
	if (curr->c->doing_okay()) {
		return curr->c;
	} else {
		return NULL;
	}
}


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
// efficient methid, i admit. also, haven't figured out how
// to do it in windows yet, so it always returns false.
// - sain 7/99
vrpn_bool vrpn_am_i_mcast_capable(void){

	// #include <fcntl.h>

	vrpn_int32 fd, mcast_capable;
	char buffer[8];
	
#ifdef WIN32
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
