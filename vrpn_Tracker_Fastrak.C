// vrpn_Tracker_Fastrak.C
//	This file contains the code to operate a Polhemus Fastrak Tracker.
// This file is based on the vrpn_3Space.C file, with modifications made
// to allow it to operate a Fastrak instead. The modifications are based
// on the old version of the Fastrak driver, which had been mainly copied
// from the Trackerlib driver and was very difficult to understand.
//	This version was written in the Summer of 1999 by Russ Taylor.

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
#include "vrpn_Tracker_Fastrak.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"

#define	REPORT_LEN		(3 + 3*4 + 4*4)
#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)
#define	INCHES_TO_METERS	(2.54/100.0)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


//   This routine will reset the tracker and set it to generate the types
// of reports we want. It relies on the power-on configuration to set the
// active sensors based on the 'Rcvr Select Switch', as described on page
// 128 of the Fastrak manual printed November 1993.

void vrpn_Tracker_Fastrak::reset()
{
   static int numResets = 0;	// How many resets have we tried?
   int i,resetLen,ret;
   char reset[10];

   //--------------------------------------------------------------------
   // This section deals with resetting the tracker to its default state.
   // Multiple attempts are made to reset, getting more aggressive each
   // time. This section completes when the tracker reports a valid status
   // message after the reset has completed.
   //--------------------------------------------------------------------

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
   fprintf(stderr, "Resetting the Fastrak (attempt #%d)",numResets);
   for (i = 0; i < resetLen; i++) {
	if (vrpn_write_characters(serial_fd, (unsigned char*)&reset[i], 1) == 1) {
		fprintf(stderr,".");
		sleep(2);  // Wait after each character to give it time to respond
   	} else {
		perror("Fastrak: Failed writing to tracker");
		status = TRACKER_FAIL;
		return;
	}
   }
   //XXX Take out the sleep and make it keep spinning quickly
   sleep(10);	// Sleep to let the reset happen
   fprintf(stderr,"\n");

   // Get rid of the characters left over from before the reset
   vrpn_flush_input_buffer(serial_fd);

   // Make sure that the tracker has stopped sending characters
   sleep(2);
   unsigned char scrap[80];
   if ( (ret = vrpn_read_available_characters(serial_fd, scrap, 80)) != 0) {
     fprintf(stderr,"  Fastrak warning: got >=%d characters after reset:\n",ret);
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
   if (vrpn_write_characters(serial_fd, (const unsigned char *) "S", 1) == 1) {
      sleep(1); // Sleep for a second to let it respond
   } else {
	perror("  Fastrak write failed");
	status = TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[56];
   if ( (ret = vrpn_read_available_characters(serial_fd, statusmsg, 55)) != 55){
  	fprintf(stderr, "  Got %d of 55 characters for status\n",ret);
   }
   if ( (statusmsg[0]!='2') || (statusmsg[54]!=(char)(10)) ) {
     int i;
     statusmsg[55] = '\0';	// Null-terminate the string
     fprintf(stderr, "  Fastrak: status is (");
     for (i = 0; i < ret; i++) {
      	if (isprint(statusmsg[i])) {
         	fprintf(stderr,"%c",statusmsg[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",statusmsg[i]);
         }
     }
     fprintf(stderr, ")\n  Bad status report from Fastrak, retrying reset\n");
     return;
   } else {
     fprintf(stderr, "  Fastrak gives status!\n");
     numResets = 0; 	// Success, use simple reset next time
   }

   //--------------------------------------------------------------------
   // Now that the tracker has given a valid status report, set all of
   // the parameters the way we want them. We rely on power-up setting
   // based on the receiver select switches to turn on the receivers that
   // the user wants. We ask for XYZ+QUAT reports, filtering is enabled on
   // position and orientation if it is desired, continuous mode in binary
   // report mode.
   //--------------------------------------------------------------------

   // Set output format for each of the possible stations to be position and
   // quaternion.  This command is a capitol 'o' followed by the number of the
   // station, then comma-separated values (2 for xyz, 11 for quat) that
   // indicate data sets, followed by character 13 (octal 15).

   for (i = 1; i <= num_stations; i++) {
	char	outstring[30];
	sprintf(outstring, "O%d,2,11\015", i);

	if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring,
	    strlen(outstring)) == (int)strlen(outstring)) {
		vrpn_SleepMsecs(50);	// Sleep for a bit to let command run
	} else {
		perror("  Fastrak write failed on format command");
		status = TRACKER_FAIL;
		return;
	}
   }

   // Enable filtering if the constructor parameter said to.
   // Set filtering for both position (x command) and orientation (v command)
   // to the values that are recommended as a "jumping off point" in the
   // Fastrak manual.

   if (do_filter) {
     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"x0.2,0.2,0.8,0.8\015", 17) == 17) {
	sleep(1); // Sleep for a second to let it respond
     } else {
	perror("  Fastrak write position filter failed");
	status = TRACKER_FAIL;
	return;
     }
     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"v0.2,0.2,0.8,0.8\015", 17) == 17) {
	sleep(1); // Sleep for a second to let it respond
     } else {
	perror("  Fastrak write orientation filter failed");
	status = TRACKER_FAIL;
	return;
     }
   }
   
   // Set data format to BINARY mode
   vrpn_write_characters(serial_fd, (const unsigned char *)"f", 1);

   // Set tracker to continuous mode
   if (vrpn_write_characters(serial_fd,(const unsigned char *) "C", 1) != 1)
   	perror("  Fastrak write failed");
   else {
   	fprintf(stderr, "  Fastrak set to continuous mode\n");
   }

   fprintf(stderr, "  (at the end of Fastrak reset routine)\n");
   gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = TRACKER_SYNCING;	// We're trying for a new reading
}

// Swap the endian-ness of the 4-byte entry in the buffer.
// This is used to make the little-endian IEEE float values
// returned by the Fastrak into the big-endian format that is
// expected by the VRPN unbuffer routines.

void vrpn_Tracker_Fastrak::swap_endian4(char *buffer)
{
	char c;

	c = buffer[0]; buffer[0] = buffer[3]; buffer[3] = c;
	c = buffer[1]; buffer[1] = buffer[2]; buffer[2] = c;
}



// This function will read characters until it has a full report, then
// put that report into the time, sensor, pos and quat fields so that it can
// be sent the next time through the loop. The time stored is that of
// the first character received as part of the report. Reports start with
// the header "0xy", where x is the station number and y is either the
// space character or else one of the characters "A-F". Characters "A-F"
// indicate weak signals and so forth, but in practice it is much harder
// to deal with them than to ignore them (they don't indicate hard error
// conditions). The report follows, 4 bytes per word in little-endian byte
// order; each word is an IEEE floating-point binary value. The first three
// are position in X,Y and Z. The next four are the unit quaternion in the
// order W, X,Y,Z.
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize with the Fastrak by waiting
// until the start-of-report character ('0') comes around again.
// The routine that calls this one
// makes sure we get a full reading often enough (ie, it is responsible
// for doing the watchdog timing to make sure the tracker hasn't simply
// stopped sending characters).
   
void vrpn_Tracker_Fastrak::get_report(void)
{
   int ret;		// Return value from function call to be checked
   int i;		// Loop counter
   char *bufptr;	// Points into buffer at the current value to read

   //--------------------------------------------------------------------
   // The reports are each REPORT_LEN characters long, and each start with an
   // ASCII '0' character. If we're synching, read a byte at a time until we
   // find a '0' character.
   //--------------------------------------------------------------------

   if (status == TRACKER_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) {
      	return;
      }

      // If it is not an '0', we don't want it but we
      // need to look at the next one, so just return and stay
      // in Syncing mode so that we will try again next time through.
      if ( buffer[0] != '0') {
      	fprintf(stderr,"Tracker Fastrak: Syncing (looking for '0', "
		"got '%c')\n", buffer[0]);
      	return;
      }

      // Got the first character of a report -- go into PARTIAL mode
      // and record that we got one character at this time. The next
      // bit of code will attempt to read the rest of the report.
      // The time stored here is as close as possible to when the
      // report was generated.
      bufcount = 1;
      gettimeofday(&timestamp, NULL);
      status = TRACKER_PARTIAL;
   }

   //--------------------------------------------------------------------
   // Read as many bytes of this report as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.  The routine that calls this one
   // makes sure we get a full reading often enough (ie, it is responsible
   // for doing the watchdog timing to make sure the tracker hasn't simply
   // stopped sending characters).
   //--------------------------------------------------------------------

   ret = vrpn_read_available_characters(serial_fd, &buffer[bufcount],
		REPORT_LEN-bufcount);
   if (ret == -1) {
	fprintf(stderr,"Fastrak: Error reading\n");
	//XXX Put out a VRPN text message here, and at other error locations
	status = TRACKER_FAIL;
	return;
   }
   bufcount += ret;
   if (bufcount < REPORT_LEN) {	// Not done -- go back for more
	return;
   }

   //--------------------------------------------------------------------
   // We now have enough characters to make a full report. Check to make
   // sure that its format matches what we expect. If it does, the next
   // section will parse it. If it does not, we need to go back into
   // synch mode and ignore this report. A well-formed report has the
   // first character '0', the next character is the ASCII station
   // number, and the third character is either a space or a letter.
   // The station number is converted into a VRPN sensor number, where
   // the first station is '1' and the first sensor is 0.
   //--------------------------------------------------------------------

   if (buffer[0] != '0') {
	   status = TRACKER_SYNCING;
      	   fprintf(stderr,"Tracker Fastrak: Not '0' in record\n");
	   return;
   }
   sensor = buffer[1] - '1';	// Convert ASCII 1 to sensor 0 and so on.
   if ( (sensor < 0) || (sensor >= num_stations) ) {
	   status = TRACKER_SYNCING;
      	   fprintf(stderr,"Tracker Fastrak: Bad sensor # (%d) in record\n",
		sensor);
	   return;
   }
   if ( (buffer[2] != ' ') && !isalpha(buffer[2]) ) {
	   status = TRACKER_SYNCING;
      	   fprintf(stderr,"Tracker Fastrak: Bad 3rd char in record\n");
	   return;
   }

   //--------------------------------------------------------------------
   // Decode the X,Y,Z of the position and the W,X,Y,Z of the quaternion
   // (keeping in mind that we store quaternions as X,Y,Z, W).
   //--------------------------------------------------------------------
   // The reports coming from the Fastrak are in little-endian order,
   // which is the opposite of the network-standard byte order that is
   // used by VRPN. Here we swap the order to big-endian so that the
   // routines below can pull out the values in the correct order.
   // This is slightly inefficient on machines that have little-endian
   // order to start with, since it means swapping the values twice, but
   // that is more than outweighed by the cleanliness gained by keeping
   // all architecture-dependent code in the vrpn_Shared.C file.
   //--------------------------------------------------------------------

   // Point at the first value in the buffer (position X)
   bufptr = (char *)&buffer[3];

   // Copy the values into local float32 arrays, then copy these into the
   // tracker's fields (which are float64s)
   vrpn_float32	read_pos[3], read_quat[4];

   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_pos[0]);
   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_pos[1]);
   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_pos[2]);

   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_quat[3]);
   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_quat[0]);
   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_quat[1]);
   swap_endian4(bufptr); vrpn_unbuffer( (const char **)&bufptr, &read_quat[2]);

   // When copying the positions, convert from inches to meters, since the
   // Fastrak reports in inches and VRPN reports in meters.
   for (i = 0; i < 3; i++) {
	pos[i] = read_pos[i] * INCHES_TO_METERS;
   }
   for (i = 0; i < 4; i++) {
	quat[i] = read_quat[i];
   }

   //--------------------------------------------------------------------
   // Done with the decoding, set the report to ready
   //--------------------------------------------------------------------

   status = TRACKER_REPORT_READY;
   bufcount = 0;

#ifdef VERBOSE
      print_latest_report();
#endif
}


// This function should be called each time through the main loop
// of the server code. It polls for a report from the tracker and
// sends it if there is one. It will reset the tracker if there is
// no data from it for a few seconds.

void vrpn_Tracker_Fastrak::mainloop(const struct timeval * /*timeout*/ )
{
  switch (status) {
    case TRACKER_REPORT_READY:
      {
#ifdef	VERBOSE
	static int count = 0;
	if (count++ == 120) {
		printf("  vrpn_Tracker_Fastrak: Got report\n"); print_latest_report();
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
		  fprintf(stderr,"Fastrak: cannot write message: tossing\n");
		}
	} else {
		fprintf(stderr,"Tracker Fastrak: No valid connection\n");
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
		fprintf(stderr,"Fastrak failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",current_time.tv_sec, current_time.tv_usec, timestamp.tv_sec, timestamp.tv_usec);
		status = TRACKER_FAIL;
	}
      }
      break;

    case TRACKER_RESETTING:
	reset();
	break;

    case TRACKER_FAIL:
	fprintf(stderr, "Fastrak failed, trying to reset (try power cycle if more than 4 attempts made).\n");
	vrpn_close_commport(serial_fd);
	serial_fd = vrpn_open_commport(portname, baudrate);
	status = TRACKER_RESETTING;
	break;
   }
}
