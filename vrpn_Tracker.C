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

unsigned long vrpn_duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

int vrpn_open_commport(char *portname, long baud)
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
int vrpn_flushInputBuffer(int comm)
{
#ifdef	linux
   tcflush(comm, TCIFLUSH);
#endif
   return 0;
}


vrpn_Tracker::vrpn_Tracker(char *name, vrpn_Connection *c) {
	// Set our connection to the one passed in
	connection = c;

	// Register this tracker device and the needed message types
	if (connection) {
	  my_id = connection->register_sender(name);
	  message_id = connection->register_message_type("Tracker Pos/Quat");
	}

	// Set the current time to zero, just to have something there
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	// Set the position to the origin and the orientation to identity
	// just to have something there in case nobody fills them in later
	pos[0] = pos[1] = pos[2] = 0.0f;
	quat[0] = quat[1] = quat[2] = 0.0f;
	quat[3] = 1.0f;
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
//#endif // #ifndef _WIN32

vrpn_Tracker_NULL::vrpn_Tracker_NULL(char *name, vrpn_Connection *c,
	int sensors, float Hz) : vrpn_Tracker(name, c), update_rate(Hz),
	num_sensors(sensors)
{
	// Nothing left to do
}

void	vrpn_Tracker_NULL::mainloop(void)
{
	struct timeval current_time;
	char	msgbuf[1000];
	int	i,len;

	// See if its time to generate a new report
	gettimeofday(&current_time, NULL);
	if ( vrpn_duration(current_time,timestamp) >= 1000000.0/update_rate) {

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


vrpn_Tracker_Serial::vrpn_Tracker_Serial(char *name, vrpn_Connection *c,
	char *port, long baud): vrpn_Tracker(name, c)
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
   if ( ( serial_fd = vrpn_open_commport(portname, baudrate) ) == -1) {
	fprintf(stderr,"vrpn_Tracker_Serial: Cannot Open serial port\n");
	status = TRACKER_FAIL;
   }

   // Reset the tracker and find out what time it is
   status = TRACKER_RESETTING;
   gettimeofday(&timestamp, NULL);
}

vrpn_Tracker_Remote::vrpn_Tracker_Remote(char *name) :
	vrpn_Tracker(name, vrpn_get_connection_by_name(name)) ,
	change_list(NULL)
{
	// Make sure that we have a valid connection
	if (connection == NULL) {
		fprintf(stderr,"vrpn_Tracker_Remote: No connection\n");
		return;
	}

	// Register a handler for the change callback from this device.
	if (connection->register_handler(message_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Tracker_Remote: can't register handler\n");
		connection = NULL;
	}

	// Find out what time it is and put this into the timestamp
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

