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

#include "vrpn_3Space.h"

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

// This constant turns the tracker binary values in the range -32768 to
// 32768 to meters.
#define T_3_DATA_MAX            (32768.0)
#define T_3_INCH_RANGE          (65.48)
#define T_3_CM_RANGE            (T_3_INCH_RANGE * 2.54)
#define T_3_METER_RANGE         (T_3_CM_RANGE / 100.0)
#define T_3_BINARY_TO_METERS    (T_3_METER_RANGE / T_3_DATA_MAX)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


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
   vrpn_flush_input_buffer(serial_fd);

   // Make sure that the tracker has stopped sending characters
   sleep(2);
   unsigned char scrap[80];
   if ( (ret = read_available_characters(scrap, 80)) != 0) {
     fprintf(stderr,"  3Space warning: got >=%d characters after reset:\n",ret);
     for (i = 0; i < ret; i++) {
      	if (isprint(scrap[i])) {
         	fprintf(stderr,"%c",scrap[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",scrap[i]);
         }
     }
     fprintf(stderr, "\n");
     vrpn_flush_input_buffer(serial_fd);		// Flush what's left
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
   if ( (ret = read_available_characters(statusmsg, 55)) != 55) {
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
      if (read_available_characters(buffer, 1) != 1) {
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
   ret = read_available_characters(&buffer[bufcount], 20-bufcount);
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
		printf("  vrpn_Tracker_3Space: Got report\n"); print();
		count = 0;
	}
#endif            

	// Send the message on the connection
	if (connection) {
		char	msgbuf[1000];
		int	len = encode_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			position_m_id, my_id, msgbuf,
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
	serial_fd = vrpn_open_commport(portname, baudrate);
	status = TRACKER_RESETTING;
	break;
   }
}


