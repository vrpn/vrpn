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
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Shared.h"		 // defines gettimeofday for WIN32

//#define VERBOSE
#define READ_HISTOGRAM

#define MAX_LENGTH		(512)

#define MAX_TIME_INTERVAL	(2000000) // max time between reports (usec)

#define TRACKER_SYNCING		(2)
#define TRACKER_REPORT_READY 	(1)
#define TRACKER_PARTIAL 	(0)
#define TRACKER_RESETTING	(-1)
#define TRACKER_FAIL 	 	(-2)

// This constant turns the tracker binary values in the range -32768 to
// 32768 to meters.
#define	T_3_DATA_MAX		(32768.0)
#define	T_3_INCH_RANGE		(65.48)
#define	T_3_CM_RANGE		(T_3_INCH_RANGE * 2.54)
#define	T_3_METER_RANGE		(T_3_CM_RANGE / 100.0)
#define	T_3_BINARY_TO_METERS	(T_3_METER_RANGE / T_3_DATA_MAX)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

static int open_commport(char *portname, long baud)
{
#ifdef linux
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
    sttyArgs.c_cflag &= ~CBAUD;
    switch (baud) {
      case   300: sttyArgs.c_cflag |=   B300; break;
      case  1200: sttyArgs.c_cflag |=  B1200; break;
      case  2400: sttyArgs.c_cflag |=  B2400; break;
      case  4800: sttyArgs.c_cflag |=  B4800; break;
      case  9600: sttyArgs.c_cflag |=  B9600; break;
      case 19200: sttyArgs.c_cflag |=  B19200; break;
      case 38400: sttyArgs.c_cflag |=  B38400; break;
      default:
	fprintf(stderr,"vrpn: Serial tracker: unknown baud rate %ld\n",baud);
	return -1;
    }

    sttyArgs.c_iflag = (IGNBRK | IGNPAR);  /* Ignore break & parity errs */
    sttyArgs.c_oflag = 0;                  /* Raw output, leave tabs alone */
    sttyArgs.c_lflag = 0;              /* Raw input (no KILL, etc.), no echo */

    sttyArgs.c_cflag |= CS8;		/* 8 bits */
    sttyArgs.c_cflag &= ~CSTOPB;	/* One stop bit */
    sttyArgs.c_cflag &= ~PARENB;	/* No parity */
    sttyArgs.c_cflag |= CREAD;		/* Allow reading */
    sttyArgs.c_cflag &= ~CLOCAL;	/* No modem between us and device */
                                        /* WRM: Is this inverted?? */

    sttyArgs.c_cc[VMIN] = 0;		/* Return read even if no chars */
    sttyArgs.c_cc[VTIME] = 0;		/* Return without waiting */

    /* pass the new settings back to the driver */
    if ( ioctl(fileDescriptor, TCSETA, &sttyArgs) == -1 ) {
	perror("Tracker: ioctl failed");
	close(fileDescriptor);
	return(-1);
    }

    return(fileDescriptor);
#else
    return -1;
#endif
}

// Read from the input buffer on the specified handle until all of the
//  characters are read.  Return 0 on success, -1 on failure.
static int	flushInputBuffer(int comm)
{
#ifdef	linux
   tcflush(comm, TCIFLUSH);
#endif
   return 0;
}

void vrpn_Tracker::print_latest_report(void)
{
   printf("----------------------------------------------------\n");
   printf("Sensor   :%d\n", sensor);
   printf("Timestamp:%ld:%ld\n", timestamp.tv_sec, timestamp.tv_usec);
   printf("Pos      :%lf, %lf, %lf\n", pos[0],pos[1],pos[2]);
   printf("Quat     :%lf, %lf, %lf, %lf\n", quat[0],quat[1],quat[2],quat[3]);
}

int	vrpn_Tracker::encode_to(char *buf)
{
   // Message includes: long unitNum, float pos[3], float quat[4]
   // Byte order of each needs to be reversed to match network standard
   // This moving is done by horrible typecast hacks.  Please forgive.

   int i;
   unsigned long *longbuf = (unsigned long*)(void*)(buf);
   int	index = 0;

   // Move the sensor, position, quaternion there
   longbuf[index++] = sensor;
   for (i = 0; i < 3; i++) {
   	longbuf[index++] = *(unsigned long*)(void*)(&pos[i]);
   }
   for (i = 0; i < 4; i++) {
   	longbuf[index++] = *(unsigned long*)(void*)(&quat[i]);
   }

   for (i = 0; i < index; i++) {
   	longbuf[i] = htonl(longbuf[i]);
   }

   return index*sizeof(unsigned long);
}

// This routine will read any available characters from the handle into
// the buffer specified, up to the number of bytes requested.  It returns
// the number of characters read or -1 on failure.  Note that it only
// reads characters that are available at the time of the read, so less
// than the requested number may be returned.
#ifndef _WIN32
int vrpn_Tracker_Serial::readAvailableCharacters(unsigned char *buffer,
	int bytes)
{
   int bRead;
   if ( (bRead = read(serial_fd, (char *)buffer, bytes)) == -1) {
      perror("Tracker: cannot read from serial port");
      return -1;
   }

#ifdef	VERBOSE
   if (bRead > 0) printf("  XXX Read %d bytes\n",(int)(bRead));
#endif

#ifdef	READ_HISTOGRAM
   // When we are using a 16550A UART with buffers enabled, the histogram is:
   // 49734 97 0 0 34 0 0 37 1 0 0 1 37 0 0 35 0 0 0 24 0 0 0 0 (0's)
   // When we use 'setserial /dev/ttyS1 uart 16550 to disable buffers, it is:
   // 49739 99 9 10 9 9 9 10 9 8 8 9 10 9 9 9 8 9 9 9 0 0 0 0 (0's)
   #define	HISTSIZE 30
   if (bRead >= 0) {
	static long count = 0;
	static int histogram[HISTSIZE+1];
	count++;

	if (bRead > HISTSIZE) {histogram[HISTSIZE]++;}
	else {histogram[bRead]++;};

	if (count == 100000L) {
		count = 0;
		for (int i = 0; i < HISTSIZE+1; i++) {
			printf("%d ",histogram[i]);
			histogram[i] = 0;
		}
		printf("\n");
	}
   }
#endif

   return bRead;
}
#endif // #ifndef _WIN32

vrpn_Tracker_NULL::vrpn_Tracker_NULL(char *name, vrpn_Connection *c,
	int sensors, float Hz) : vrpn_Tracker(c), update_rate(Hz),
	num_sensors(sensors)
{
	// Set the current time to zero, so we'll do an initial report
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	// Register the tracker device and the needed message types
      if (connection) {
	my_id = connection->register_sender(name);
	message_id = connection->register_message_type("Tracker Pos/Quat");
      }

	// Set the position to the origin and the orientation to identity
	pos[0] = pos[1] = pos[2] = 0.0f;
	quat[0] = quat[1] = quat[2] = 0.0f;
	quat[3] = 1.0f;
}

void	vrpn_Tracker_NULL::mainloop(void)
{
	struct timeval current_time;
	char	msgbuf[1000];
	int	i,len;

	// See if its time to generate a new report
	gettimeofday(&current_time, NULL);
	if ( duration(current_time,timestamp) >= 1000000.0/update_rate) {

	  // Update the time
	  timestamp.tv_sec = current_time.tv_sec;
	  timestamp.tv_usec = current_time.tv_usec;

	  // Send messages for all sensors if we have a connection
	  if (connection) {
	    for (i = 0; i < num_sensors; i++) {
		sensor = i;
		len = encode_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			message_id, my_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}
	    }
	  }
	}
}

#ifndef _WIN32
void vrpn_Tracker_3Space::reset()
{
   static int numResets = 0;	// How many resets have we tried?
   int i,resetLen,ret;
   char reset[10];

   // Send the tracker a string that should reset it.  The first time we
   // try this, just do the normal ^Y reset.  Later, try to reset
   // to the factory defaults.  Then toggle the extended mode.
   // Then put in a carriage return to try and break it out of
   // a query mode if it is in one.  These additions are cumulative: by the
   // end, we're doing them all.
   resetLen = 0;
   numResets++;		  	// We're trying another reset
   if (numResets > 1) {	// Try to get it out of a query loop if its in one
   	reset[resetLen++] = (char) (13); // Return key -> get ready
   }
   if (numResets > 7) {
	reset[resetLen++] = 'Y'; // Put tracker into tracking (not point) mode
   }
   if (numResets > 3) {	// Get a little more aggressive
   	if (numResets > 4) { // Even more aggressive
      	reset[resetLen++] = 't'; // Toggle extended mode (in case it is on)
   }
   reset[resetLen++] = 'W'; // Reset to factory defaults
   reset[resetLen++] = (char) (11); // Ctrl + k --> Burn settings into EPROM
   }
   reset[resetLen++] = (char) (25); // Ctrl + Y -> reset the tracker
   fprintf(stderr, "Resetting the 3Space (attempt #%d)",numResets);
   for (i = 0; i < resetLen; i++) {
	if (write(serial_fd, &reset[i], 1) == 1) {
		fprintf(stderr,".");
		sleep(2);  // Wait 2 seconds each character
   	} else {
		perror("3Space: Failed writing to tracker");
		status = TRACKER_FAIL;
		return;
	}
   }
   sleep(10);	// Sleep to let the reset happen
   fprintf(stderr,"\n");

   // Get rid of the characters left over from before the reset
   flushInputBuffer(serial_fd);

   // Make sure that the tracker has stopped sending characters
   sleep(2);
   unsigned char scrap[80];
   if ( (ret = readAvailableCharacters(scrap, 80)) != 0) {
     fprintf(stderr,"  3Space warning: got >=%d characters after reset:\n",ret);
     for (i = 0; i < ret; i++) {
      	if (isprint(scrap[i])) {
         	fprintf(stderr,"%c",scrap[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",scrap[i]);
         }
     }
     fprintf(stderr, "\n");
     flushInputBuffer(serial_fd);		// Flush what's left
   }

   // Asking for tracker status
   if (write(serial_fd, "S", 1) == 1) {
      sleep(1); // Sleep for a second to let it respond
   } else {
	perror("  3Space write failed");
	status = TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[56];
   if ( (ret = readAvailableCharacters(statusmsg, 55)) != 55) {
  	fprintf(stderr, "  Got %d of 55 characters for status\n",ret);
   }
   if ( (statusmsg[0]!='2') || (statusmsg[54]!=(char)(10)) ) {
     int i;
     statusmsg[55] = '\0';	// Null-terminate the string
     fprintf(stderr, "  Tracker: status is (");
     for (i = 0; i < 55; i++) {
      	if (isprint(statusmsg[i])) {
         	fprintf(stderr,"%c",statusmsg[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",statusmsg[i]);
         }
     }
     fprintf(stderr, ")\n  Bad status report from tracker, retrying reset\n");
     return;
   } else {
     fprintf(stderr, "  3Space gives status!\n");
     numResets = 0; 	// Success, use simple reset next time
   }

   // Set output format to be position,quaternion
   // These are a capitol 'o' followed by comma-separated values that
   // indicate data sets according to appendix F of the 3Space manual,
   // then followed by character 13 (octal 15).
   if (write(serial_fd, "O2,11\015", 6) == 6) {
	sleep(1); // Sleep for a second to let it respond
   } else {
	perror("  3Space write failed");
	status = TRACKER_FAIL;
	return;
   }

   // Set data format to BINARY mode
   write(serial_fd, "f", 1);

   // Set tracker to continuous mode
   if (write(serial_fd, "C", 1) != 1)
   	perror("  3Space write failed");
   else {
   	fprintf(stderr, "  3Space set to continuous mode\n");
   }

   fprintf(stderr, "  (at the end of 3Space reset routine)\n");
   gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = TRACKER_SYNCING;	// We're trying for a new reading
}


void vrpn_Tracker_3Space::get_report(void)
{
   int ret;

   // The reports are each 20 characters long, and each start with a
   // byte that has the high bit set and no other bytes have the high
   // bit set.  If we're synching, read a byte at a time until we find
   // one with the high bit set.
   if (status == TRACKER_SYNCING) {
      // Try to get a character.  If none, just return.
      if (readAvailableCharacters(buffer, 1) != 1) {
      	return;
      }

      // If the high bit isn't set, we don't want it we
      // need to look at the next one, so just return
      if ( (buffer[0] & 0x80) == 0) {
      	fprintf(stderr,"Tracker 3Space: Syncing (high bit not set)\n");
      	return;
      }

      // Got the first character of a report -- go into PARTIAL mode
      // and say that we got one character at this time.
      bufcount = 1;
      gettimeofday(&timestamp, NULL);
      status = TRACKER_PARTIAL;
   }

   // Read as many bytes of this 20 as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.  The routine that calls this one
   // makes sure we get a full reading often enough (ie, it is responsible
   // for doing the watchdog timing to make sure the tracker hasn't simply
   // stopped sending characters).
   ret = readAvailableCharacters(&buffer[bufcount], 20-bufcount);
   if (ret == -1) {
	fprintf(stderr,"3Space: Error reading\n");
	status = TRACKER_FAIL;
	return;
   }
   bufcount += ret;
   if (bufcount < 20) {	// Not done -- go back for more
	return;
   }

   { // Decode the report
	unsigned char decode[17];
	int i;

	static unsigned char mask[8] = {0x01, 0x02, 0x04, 0x08,
					0x10, 0x20, 0x40, 0x80 };
	// Clear the MSB in the first byte
	buffer[0] &= 0x7F;

	// Decode the 3Space binary representation into standard
	// 8-bit bytes.  This is done according to page 4-4 of the
	// 3Space user's manual, which says that the high-order bits
	// of each group of 7 bytes is packed into the 8th byte of the
	// group.  Decoding involves setting those bits in the bytes
	// iff their encoded counterpart is set and then skipping the
	// byte that holds the encoded bits.
	// We decode from buffer[] into decode[] (which is 3 bytes
	// shorter due to the removal of the bit-encoding bytes).

	// decoding from buffer[0-6] into decode[0-6]
	for (i=0; i<7; i++) {
		decode[i] = buffer[i];
		if ( (buffer[7] & mask[i]) != 0) {
			decode[i] |= (unsigned char)(0x80);
		}
	}

	// decoding from buffer[8-14] into decode[7-13]
	for (i=7; i<14; i++) {
		decode[i] = buffer[i+1];
		if ( (buffer[15] & mask[i-7]) != 0) {
			decode[i] |= (unsigned char)(0x80);
		}
	}

	// decoding from buffer[16-18] into decode[14-16]
	for (i=14; i<17; i++) {
		decode[i] = buffer[i+2];
		if ( (buffer[19] & mask[i-14]) != 0) {
			buffer[i+2] |= (unsigned char)(0x80);
		}
	}

	// Parse out sensor number, which is the second byte and is
	// stored as the ASCII number of the sensor, with numbers
	// starting from '1'.  We turn it into a zero-based unit number.
	sensor = decode[1] - '1';

	// Position
	for (i=0; i<3; i++) {
		pos[i] = (* (short*)(&decode[3+2*i])) * T_3_BINARY_TO_METERS;
	}

	// Quarternion orientation.  The 3Space gives quaternions
	// as w,x,y,z while the VR code handles them as x,y,z,w,
	// so we need to switch the order when decoding.  Also the
	// tracker does not normalize the quaternions.
	quat[3] = (* (short*)(&decode[9]));
	for (i=0; i<3; i++) {
		quat[i] = (* (short*)(&decode[11+2*i]));
	}

	//Normalize quaternion
	double norm = sqrt (  quat[0]*quat[0] + quat[1]*quat[1]
			    + quat[2]*quat[2] + quat[3]*quat[3]);
	for (i=0; i<4; i++) {
		quat[i] /= norm;
	}

	// Done with the decoding, set the report to ready
      	status = TRACKER_REPORT_READY;
      	bufcount = 0;
   }
#ifdef VERBOSE
      print();
#endif
}


void vrpn_Tracker_3Space::mainloop()
{
  switch (status) {
    case TRACKER_REPORT_READY:
      {
#ifdef	VERBOSE
	static int count = 0;
	if (count++ == 120) {
		printf("  XXX Got report\n"); print();
		count = 0;
	}
#endif            

	// Send the message on the connection
	if (connection) {
		char	msgbuf[1000];
		int	len = encode_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			message_id, my_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		  fprintf(stderr,"Tracker: cannot write message: tossing\n");
		}
	} else {
		fprintf(stderr,"Tracker 3Space: No valid connection\n");
	}

	// Ready for another report
	status = TRACKER_SYNCING;
      }
      break;

    case TRACKER_SYNCING:
    case TRACKER_PARTIAL:
      {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	if ( duration(current_time,timestamp) < MAX_TIME_INTERVAL) {
		get_report();
	} else {
		fprintf(stderr,"Tracker failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",current_time.tv_sec, current_time.tv_usec, timestamp.tv_sec, timestamp.tv_usec);
		status = TRACKER_FAIL;
	}
      }
      break;

    case TRACKER_RESETTING:
	reset();
	break;

    case TRACKER_FAIL:
	fprintf(stderr, "Tracker failed, trying to reset (Try power cycle if more than 4 attempts made)\n");
	close(serial_fd);
	serial_fd = open_commport(portname, baudrate);
	status = TRACKER_RESETTING;
	break;
   }
}


vrpn_Tracker_Serial::vrpn_Tracker_Serial(char *name, vrpn_Connection *c,
	char *port, long baud)
{
   // Find out the port name and baud rate
   if (port == NULL) {
	fprintf(stderr,"vrpn_Tracker_Serial: NULL port name\n");
	status = TRACKER_FAIL;
	return;
   } else {
	strncpy(portname, port, sizeof(portname));
	portname[sizeof(portname)-1] = '\0';
   }
   baudrate = baud;

   // Open the serial port we're going to use
   if ( (serial_fd=open_commport(portname, baudrate)) == -1) {
	fprintf(stderr,"vrpn_Tracker_Serial: Cannot Open serial port\n");
	status = TRACKER_FAIL;
   }

   // If the connection is valid, use it to register this tracker by
   // name and the tracker message report by name.
   connection = c;
   if (connection != NULL) {
	my_id = connection->register_sender(name);
	message_id = connection->register_message_type("Tracker Pos/Quat");
	if ( (my_id == -1) || (message_id == -1) ) {
		fprintf(stderr,"Tracker_Report: Cannot register IDs\n");
		status = TRACKER_FAIL;
	}
   }

   // Reset the tracker and find out what time it is
   status = TRACKER_RESETTING;
   gettimeofday(&timestamp, NULL);
}

vrpn_Tracker_Remote::vrpn_Tracker_Remote(char *name) : change_list(NULL)
{
	char	*tail;
	char	con_name[1024];

	// Look up the connection to use based on the name
	// XXX Eventually, we want to read the connectivity information
	// from a configuration file.  For now, strip off everything after
	// the last underscore '_' character and prepend 'PC_station_' to
	// it to determine the name of the connection.
	if (name == NULL) { return; }
	if ( (tail = strrchr(name, '_')) == NULL) {
		fprintf(stderr,"vrpn_Tracker_Remote: Can't parse name\n");
		return;
	}
	strcpy(con_name,"PC_station");
	strncat(con_name, tail, sizeof(con_name) - strlen("PC_station")-2);
	con_name[sizeof(con_name)-1] = '\0';

	// Establish the connection
	if ( (connection = vrpn_get_connection_by_name(con_name)) == NULL) {
		return;
	}

	// Register the tracker device and the needed message types
	my_id = connection->register_sender(name);
	message_id = connection->register_message_type("Tracker Pos/Quat");

	// Register a handler for the change callback from this device.
	if (connection->register_handler(message_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Tracker_Remote: can't register handler\n");
		connection = NULL;
	}

	// Find out what time it is
	gettimeofday(&timestamp, NULL);
}

void	vrpn_Tracker_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); }
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
			vrpn_TRACKERCHANGEHANDLER handler)
{
	vrpn_TRACKERCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
		    "vrpn_Tracker_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_TRACKERCHANGELIST) == NULL) {
		fprintf(stderr,
		    "vrpn_Tracker_Remote::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = change_list;
	change_list = new_entry;

	return 0;
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
			vrpn_TRACKERCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERCHANGELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &change_list;
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Tracker_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	long *params = (long*)(p.buffer);
	vrpn_TRACKERCB	tp;
	vrpn_TRACKERCHANGELIST *handler = me->change_list;
	long	temp;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (sizeof(long) + 7*sizeof(float)) ) {
		fprintf(stderr,"vrpn_Tracker: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, sizeof(long) + 7*sizeof(float) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	tp.sensor = ntohl(params[0]);

	// Typecasting used to get the byte order correct on the floats
	// that are coming from the other side.
	for (i = 0; i < 3; i++) {
	 	temp = ntohl(params[1+i]);
		tp.pos[i] = *(float*)(&temp);
	}
	for (i = 0; i < 4; i++) {
		temp = ntohl(params[4+i]);
		tp.quat[i] = *(float*)(&temp);
	}

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

	return 0;
}

#endif // #ifndef _WIN32

