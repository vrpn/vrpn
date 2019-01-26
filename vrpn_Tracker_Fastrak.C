// vrpn_Tracker_Fastrak.C
//	This file contains the code to operate a Polhemus Fastrak Tracker.
// This file is based on the vrpn_3Space.C file, with modifications made
// to allow it to operate a Fastrak instead. The modifications are based
// on the old version of the Fastrak driver, which had been mainly copied
// from the Trackerlib driver and was very difficult to understand.
//	This version was written in the Summer of 1999 by Russ Taylor.
//	Modifications were made to this version to allow it to run the
// Intersense IS600 tracker; basically, this involved allowing the user
// to set extra things in the reset routine, and to pause to let the tracker
// handle the parameter-setting commands.
//	Modifications were later made to support the IS-900 trackers,
// including wands and styli.

#include <ctype.h>                      // for isprint, isalpha
#include <stdio.h>                      // for fprintf, sprintf, stderr, etc
#include <stdlib.h>                     // for atoi
#include <string.h>                     // for strlen, strtok

#include "quat.h"                       // for Q_W, Q_X, Q_Y, Q_Z
#include "vrpn_Analog.h"                // for vrpn_Clipping_Analog_Server
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Button.h"                // for vrpn_Button_Server
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR
#include "vrpn_Tracker_Fastrak.h"

vrpn_Tracker_Fastrak::vrpn_Tracker_Fastrak(const char *name, vrpn_Connection *c, 
		      const char *port, long baud, int enable_filtering, int numstations,
		      const char *additional_reset_commands, int is900_timestamps) :
    vrpn_Tracker_Serial(name,c,port,baud),
    do_filter(enable_filtering),
    num_stations(numstations>vrpn_FASTRAK_MAX_STATIONS ? vrpn_FASTRAK_MAX_STATIONS : numstations),
    num_resets(0),
    do_is900_timestamps(is900_timestamps)
{
	int i;

	reset_time.tv_sec = reset_time.tv_usec = 0;
	if (additional_reset_commands == NULL) {
		add_reset_cmd[0] = '\0';
	} else {
		vrpn_strcpy(add_reset_cmd, additional_reset_commands);
	}

	// Initially, set to no buttons or analogs on the stations.  The
	// methods to add buttons and analogs must be called to add them.
	for (i = 0; i < num_stations; i++) {
	    is900_buttons[i] = NULL;
	    is900_analogs[i] = NULL;
	}

	// Let me assume I am an Intersense till proven false,
	// by an instance of a fastrak-only command like "FTStylus"
	// in the initialization strings.
	really_fastrak = false;
}

vrpn_Tracker_Fastrak::~vrpn_Tracker_Fastrak()
{
    int i;

    // Delete any button and analog devices that were created
    for (i = 0; i < num_stations; i++) {
      try {
        if (is900_buttons[i]) {
          delete is900_buttons[i];
        }
        if (is900_analogs[i]) {
          delete is900_analogs[i];
        }
      } catch (...) {
        fprintf(stderr, "vrpn_Tracker_Fastrak::~vrpn_Tracker_Fastrak(): delete failed\n");
        return;
      }
    }
}

/** This routine augments the basic sensor-output setting function of the Fastrak
    to allow the possibility of requesting timestamp, button data, and/or analog
    data from the device.  It sets the device for position + quaternion + any of
    the extended fields.  It puts a space at the end so that we can check to make
    sure we have complete good records for each report.

    Returns 0 on success and -1 on failure.
*/

int vrpn_Tracker_Fastrak::set_sensor_output_format(int sensor)
{
    char    outstring[64];
    const char    *timestring;
    const char    *buttonstring;
    const char    *analogstring;

    // Set output format for the station to be position and quaternion,
    // and any of the extended Fastrak (stylus with button) or
    // IS900 states (timestamp, button, analog).
    // This command is a capitol 'o' followed by the number of the
    // station, then comma-separated values (2 for xyz, 11 for quat, 21 for
    // timestamp, 22 for buttons, 16 for Fastrak stylus button,
    // 23 for joystick, 0 for space) that
    // indicate data sets, followed by character 13 (octal 15).
    // Note that the sensor number has to be bumped to map to station number.

    timestring = do_is900_timestamps ? ",21" : "";
    if (really_fastrak) {
      buttonstring = is900_buttons[sensor] ? ",16" : ""; 
    } else {
      buttonstring = is900_buttons[sensor] ? ",22" : "";
    }
    analogstring = is900_analogs[sensor] ? ",23" : "";
    sprintf(outstring, "O%d,2,11%s%s%s,0\015", sensor+1, timestring,
	buttonstring, analogstring);
    if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring,
	    strlen(outstring)) == (int)strlen(outstring)) {
		vrpn_SleepMsecs(50);	// Sleep for a bit to let command run
    } else {
		VRPN_MSG_ERROR("Write failed on format command");
		status = vrpn_TRACKER_FAIL;
		return -1;
    }

    return 0;
}

/** This routine augments the standard Fastrak report (3 initial characters +
    3*4 for position + 4*4 for quaternions) to include the timestamp, button
    or analog response, if any, for the given sensor.

    It returns the number of characters total to expect for a report for the
    given sensor.
*/

int vrpn_Tracker_Fastrak::report_length(int sensor)
{
    int	len;

    len = 4;	// Basic report, "0" plus Fastrak station char + status char + space at the end
    len += 3*4;	// Four bytes/float, 3 floats for position
    len += 4*4;	// Four bytes/float, 4 floats for quaternion

    // Add in the timestamp (4 bytes of float) if it is present
    if (do_is900_timestamps) {
	len += 4;
    }

    // Add in the buttons (1 byte for IS900, 2 for Fastrak) if present
    if (is900_buttons[sensor]) {
       if (really_fastrak) {
         len += 2;
       } else {
         len += 1;
       }
    }

    // Add in the joystick (one byte each for two values) if present
    if (is900_analogs[sensor]) {
	len += 2*1;
    }

    return len;
}

//   This routine will reset the tracker and set it to generate the types
// of reports we want. It relies on the power-on configuration to set the
// active sensors based on the 'Rcvr Select Switch', as described on page
// 128 of the Fastrak manual printed November 1993.

void vrpn_Tracker_Fastrak::reset()
{
   int i,resetLen,ret;
   unsigned char reset[10];
   char errmsg[512];

   //--------------------------------------------------------------------
   // This section deals with resetting the tracker to its default state.
   // Multiple attempts are made to reset, getting more aggressive each
   // time. This section completes when the tracker reports a valid status
   // message after the reset has completed.
   //--------------------------------------------------------------------

   // Send the tracker a string that should reset it.  The first time we
   // try this, just do the normal 'c' command to put it into polled mode.
   // after a few tries with this, use the ^Y reset.  Later, try to reset
   // to the factory defaults.  Then toggle the extended mode.
   // Then put in a carriage return to try and break it out of
   // a query mode if it is in one.  These additions are cumulative: by the
   // end, we're doing them all.
   resetLen = 0;
   num_resets++;		  	// We're trying another reset
   if (num_resets > 1) {	// Try to get it out of a query loop if its in one
   	reset[resetLen++] = (unsigned char) (13); // Return key -> get ready
   }
   if (num_resets > 5) {
	reset[resetLen++] = 'Y'; // Put tracker into tracking (not point) mode
   }
   if (num_resets > 4) { // Even more aggressive
       reset[resetLen++] = 't'; // Toggle extended mode (in case it is on)
   }
   /* XXX These commands are probably never needed, and can cause real
      headaches for people who are keeping state in their trackers (especially
      the InterSense trackers).  Taking them out in version 05.01; you can put
      them back in if your tracker isn't resetting as well.
   if (num_resets > 3) {	// Get a little more aggressive
	reset[resetLen++] = 'W'; // Reset to factory defaults
	reset[resetLen++] = (unsigned char) (11); // Ctrl + k --> Burn settings into EPROM
   }
   */
   if (num_resets > 2) {
       reset[resetLen++] = (unsigned char) (25); // Ctrl + Y -> reset the tracker
   }
   reset[resetLen++] = 'c'; // Put it into polled (not continuous) mode

   sprintf(errmsg, "Resetting the tracker (attempt %d)", num_resets);
   VRPN_MSG_WARNING(errmsg);
   for (i = 0; i < resetLen; i++) {
	if (vrpn_write_characters(serial_fd, &reset[i], 1) == 1) {
		fprintf(stderr,".");
		vrpn_SleepMsecs(1000.0*2);  // Wait after each character to give it time to respond
   	} else {
		perror("Fastrak: Failed writing to tracker");
		status = vrpn_TRACKER_FAIL;
		return;
	}
   }
   //XXX Take out the sleep and make it keep spinning quickly
   // You only need to sleep 10 seconds for an actual Fastrak.
   // For the Intersense trackers, you need to sleep 20. So,
   // sleeping 20 is the more general solution...
   if (num_resets > 2) {
       vrpn_SleepMsecs(1000.0*20);	// Sleep to let the reset happen, if we're doing ^Y
   }

   fprintf(stderr,"\n");

   // Get rid of the characters left over from before the reset
   vrpn_flush_input_buffer(serial_fd);

   // Make sure that the tracker has stopped sending characters
   vrpn_SleepMsecs(1000.0*2);
   unsigned char scrap[80];
   if ( (ret = vrpn_read_available_characters(serial_fd, scrap, 80)) != 0) {
     sprintf(errmsg,"Got >=%d characters after reset",ret);
     VRPN_MSG_WARNING(errmsg);
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
      vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
   } else {
	perror("  Fastrak write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[56];

   // Attempt to read 55 characters.  For some reason, later versions of the
   // InterSense IS900 only report a 54-character status message.  If this
   // happens, handle it.
   ret = vrpn_read_available_characters(serial_fd, statusmsg, 55);
   if ( (ret != 55) && (ret != 54) ) {
  	fprintf(stderr,
	 "  Got %d of 55 characters for status (54 expected for IS900)\n",ret);
   }
   if ( (statusmsg[0]!='2') || (statusmsg[ret-1]!=(char)(10)) ) {
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
     fprintf(stderr,"\n)\n");
     VRPN_MSG_ERROR("Bad status report from Fastrak, retrying reset");
     return;
   } else {
     VRPN_MSG_WARNING("Fastrak/Isense gives status (this is good)");
     num_resets = 0; 	// Success, use simple reset next time
   }

   //--------------------------------------------------------------------
   // Now that the tracker has given a valid status report, set all of
   // the parameters the way we want them. We rely on power-up setting
   // based on the receiver select switches to turn on the receivers that
   // the user wants.
   //--------------------------------------------------------------------

   // Set output format for each of the possible stations.

   for (i = 0; i < num_stations; i++) {
       if (set_sensor_output_format(i)) {
	   return;
       }
   }

   if (really_fastrak) {
      char outstring[64];
      sprintf(outstring, "e1,0\r");
      if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring,
                                strlen(outstring)) == (int)strlen(outstring)) {
        vrpn_SleepMsecs(50);   // Sleep for a bit to let command run
      } else {
        VRPN_MSG_ERROR("Write failed on mouse format command");
        status = vrpn_TRACKER_FAIL;
      }
   }

   // Enable filtering if the constructor parameter said to.
   // Set filtering for both position (x command) and orientation (v command)
   // to the values that are recommended as a "jumping off point" in the
   // Fastrak manual.

   if (do_filter) {
     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"x0.2,0.2,0.8,0.8\015", 17) == 17) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  Fastrak write position filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"v0.2,0.2,0.8,0.8\015", 17) == 17) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  Fastrak write orientation filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
   }

   // Send the additional reset commands, if any, to the tracker.
   // These commands come in lines, with character \015 ending each
   // line. If a line start with an asterisk (*), treat it as a pause
   // command, with the number of seconds to wait coming right after
   // the asterisk. Otherwise, the line is sent directly to the tracker.
   // Wait a while for them to take effect, then clear the input
   // buffer.
   if (strlen(add_reset_cmd) > 0) {
	char	*next_line;
	char	add_cmd_copy[sizeof(add_reset_cmd)+1];
	char	string_to_send[sizeof(add_reset_cmd)+1];
	int	seconds_to_wait;

	printf("  Fastrak writing extended reset commands...\n");

	// Make a copy of the additional reset string, since it is consumed
        vrpn_strcpy(add_cmd_copy, add_reset_cmd);
	add_cmd_copy[sizeof(add_cmd_copy)-1] = '\0';

	// Pass through the string, testing each line to see if it is
	// a sleep command or a line to send to the tracker. Continue until
	// there are no more line delimiters ('\015'). Be sure to write the
	// \015 to the end of the string sent to the tracker.
	// Note that strok() puts a NULL character in place of the delimiter.

	next_line = strtok(add_cmd_copy, "\015");
	while (next_line != NULL) {
		if (next_line[0] == '*') {	// This is a "sleep" line, see how long
			seconds_to_wait = atoi(&next_line[1]);
			fprintf(stderr,"   ...sleeping %d seconds\n",seconds_to_wait);
			vrpn_SleepMsecs(1000.0*seconds_to_wait);
		} else {	// This is a command line, send it
			sprintf(string_to_send, "%s\015", next_line);
			fprintf(stderr, "   ...sending command: %s\n", string_to_send);
			vrpn_write_characters(serial_fd,
				(const unsigned char *)string_to_send,strlen(string_to_send));
		}
		next_line = strtok(next_line+strlen(next_line)+1, "\015");
	}

	// Sleep a little while to let this finish, then clear the input buffer
	vrpn_SleepMsecs(1000.0*2);
	vrpn_flush_input_buffer(serial_fd);
   }
   
   // Set data format to BINARY mode
   vrpn_write_characters(serial_fd, (const unsigned char *)"f", 1);

   // Set tracker to continuous mode
   if (vrpn_write_characters(serial_fd,(const unsigned char *) "C", 1) != 1) {
   	perror("  Fastrak write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   } else {
   	fprintf(stderr, "  Fastrak set to continuous mode\n");
   }

   // If we are using the IS-900 timestamps, clear the timer on the device and
   // store the time when we cleared it.  First, drain any characters in the output
   // buffer to ensure we're sending right away.  Then, send the reset command and
   // store the time that we sent it, plus the estimated time for the characters to
   // get across the serial line to the device at the current baud rate.
   // Set time units to milliseconds (MT) and reset the time (MZ).
   if (do_is900_timestamps) {
	char	clear_timestamp_cmd[] = "MT\015MZ\015";

	vrpn_drain_output_buffer(serial_fd);

	if (vrpn_write_characters(serial_fd, (const unsigned char *)clear_timestamp_cmd,
	        strlen(clear_timestamp_cmd)) != (int)strlen(clear_timestamp_cmd)) {
	    VRPN_MSG_ERROR("Cannot send command to clear timestamp");
	    status = vrpn_TRACKER_FAIL;
	    return;
	}

	// Drain the output buffer again, then record the time as the base time from
	// the tracker.
	vrpn_drain_output_buffer(serial_fd);
	vrpn_gettimeofday(&is900_zerotime, NULL);
   }

   // Done with reset.
   vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
   VRPN_MSG_WARNING("Reset Completed (this is good)");
   status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
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
// order W, X,Y,Z.  There are some optional fields for the Intersense 900
// tracker, then there is an ASCII space character at the end.
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize with the Fastrak by waiting
// until the start-of-report character ('0') comes around again.
// The routine that calls this one makes sure we get a full reading often
// enough (ie, it is responsible for doing the watchdog timing to make sure
// the tracker hasn't simply stopped sending characters).
   
int vrpn_Tracker_Fastrak::get_report(void)
{
   char errmsg[512];	// Error message to send to VRPN
   int ret;		// Return value from function call to be checked
   int i;		// Loop counter
   unsigned char *bufptr;	// Points into buffer at the current value to read

   //--------------------------------------------------------------------
   // Each report starts with an ASCII '0' character. If we're synching,
   // read a byte at a time until we find a '0' character.
   //--------------------------------------------------------------------

   if (status == vrpn_TRACKER_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) {
      	return 0;
      }

      // If it is not an '0', we don't want it but we
      // need to look at the next one, so just return and stay
      // in Syncing mode so that we will try again next time through.
      // Also, flush the buffer so that it won't take as long to catch up.
      if ( buffer[0] != '0') {
      	sprintf(errmsg,"While syncing (looking for '0', "
		"got '%c')", buffer[0]);
	VRPN_MSG_INFO(errmsg);
	vrpn_flush_input_buffer(serial_fd);
      	return 0;
      }

      // Got the first character of a report -- go into AWAITING_STATION mode
      // and record that we got one character at this time. The next
      // bit of code will attempt to read the station.
      // The time stored here is as close as possible to when the
      // report was generated.  For the InterSense 900 in timestamp
      // mode, this value will be overwritten later.
      bufcount = 1;
      vrpn_gettimeofday(&timestamp, NULL);
      status = vrpn_TRACKER_AWAITING_STATION;
   }

   //--------------------------------------------------------------------
   // The second character of each report is the station number.  Once
   // we know this, we can compute how long the report should be for the
   // given station, based on what values are in its report.
   // The station number is converted into a VRPN sensor number, where
   // the first Fastrak station is '1' and the first VRPN sensor is 0.
   //--------------------------------------------------------------------

   if (status == vrpn_TRACKER_AWAITING_STATION) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, &buffer[bufcount], 1) != 1) {
      	return 0;
      }

      d_sensor = buffer[1] - '1';	// Convert ASCII 1 to sensor 0 and so on.
      if ( (d_sensor < 0) || (d_sensor >= num_stations) ) {
	   status = vrpn_TRACKER_SYNCING;
      	   sprintf(errmsg,"Bad sensor # (%d) in record, re-syncing", d_sensor);
	   VRPN_MSG_INFO(errmsg);
	   vrpn_flush_input_buffer(serial_fd);
	   return 0;
      }

      // Figure out how long the current report should be based on the
      // settings for this sensor.
      REPORT_LEN = report_length(d_sensor);

      // Got the station report -- to into PARTIAL mode and record
      // that we got one character at this time.  The next bit of code
      // will attempt to read the rest of the report.
      bufcount++;
      status = vrpn_TRACKER_PARTIAL;
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
	VRPN_MSG_ERROR("Error reading report");
	status = vrpn_TRACKER_FAIL;
	return 0;
   }
   bufcount += ret;
   if (bufcount < REPORT_LEN) {	// Not done -- go back for more
	return 0;
   }

   //--------------------------------------------------------------------
   // We now have enough characters to make a full report. Check to make
   // sure that its format matches what we expect. If it does, the next
   // section will parse it. If it does not, we need to go back into
   // synch mode and ignore this report. A well-formed report has the
   // first character '0', the next character is the ASCII station
   // number, and the third character is either a space or a letter.
   //--------------------------------------------------------------------

   if (buffer[0] != '0') {
	   status = vrpn_TRACKER_SYNCING;
	   VRPN_MSG_INFO("Not '0' in record, re-syncing");
	   vrpn_flush_input_buffer(serial_fd);
	   return 0;
   }
   // Sensor checking was handled when we received the character for it
   if ( (buffer[2] != ' ') && !isalpha(buffer[2]) ) {
	   status = vrpn_TRACKER_SYNCING;
	   VRPN_MSG_INFO("Bad 3rd char in record, re-syncing");
	   vrpn_flush_input_buffer(serial_fd);
	   return 0;
   }
   if (buffer[bufcount-1] != ' ') {
	   status = vrpn_TRACKER_SYNCING;
	   VRPN_MSG_INFO("No space character at end of report, re-syncing");
	   vrpn_flush_input_buffer(serial_fd);
	   return 0;
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

   // Point at the first value in the buffer (position of the X value)
   bufptr = &buffer[3];

   // When copying the positions, convert from inches to meters, since the
   // Fastrak reports in inches and VRPN reports in meters.
   pos[0] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * VRPN_INCHES_TO_METERS;
   pos[1] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * VRPN_INCHES_TO_METERS;
   pos[2] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * VRPN_INCHES_TO_METERS;

   // Change the order of the quaternion fields to match quatlib order
   d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
   d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
   d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
   d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);

   //--------------------------------------------------------------------
   // If we are doing IS900 timestamps, decode the time, add it to the
   // time we zeroed the tracker, and update the report time.  Remember
   // to convert the MILLIseconds from the report into MICROseconds and
   // seconds.
   //--------------------------------------------------------------------

   if (do_is900_timestamps) {
       struct timeval delta_time;   // Time since the clock was reset

       // Read the floating-point value of the time from the record.
       vrpn_float32 read_time = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);

       // Convert from the float in MILLIseconds to the struct timeval
       delta_time.tv_sec = (long)(read_time / 1000);	// Integer trunction to seconds
       read_time -= delta_time.tv_sec * 1000;	// Subtract out what we just counted
       delta_time.tv_usec = (long)(read_time * 1000);	// Convert remainder to MICROseconds

       // Store the current time
       timestamp = vrpn_TimevalSum(is900_zerotime, delta_time);
   }

   //--------------------------------------------------------------------
   // If this sensor has an IS900 button or fastrak button on it, decode
   // the button values into the button device and mainloop the button
   // device so that it will report any changes.  Each button is stored
   // in one bit of the byte, with the lowest-numbered button in the
   // lowest bit.
   //--------------------------------------------------------------------

   if (is900_buttons[d_sensor]) {
       // the following section was modified by Debug to add support for Fastrak stylus button
       int value;
       if (really_fastrak) {
	  value=*(++bufptr)-'0'; // only one button, status in ASCII, skip over space
	  is900_buttons[d_sensor]->set_button(0, value);
       } else {// potentially multiple buttons, values encoded in binary as a bit field
	  for (i = 0; i < is900_buttons[d_sensor]->number_of_buttons(); i++) {
	    value = ( (*bufptr) >> i) & 1;
	    is900_buttons[d_sensor]->set_button(i, value);
	  }
       }
       is900_buttons[d_sensor]->mainloop();

       bufptr++;
   }

   //--------------------------------------------------------------------
   // If this sensor has an IS900 analog on it, decode the analog values
   // into the analog device and mainloop the analog device so that it
   // will report any changes.  The first byte holds the unsigned char
   // representation of left/right.  The second holds up/down.  For each,
   // 0 means min (left or rear), 127 means center and 255 means max.
   //--------------------------------------------------------------------

   if (is900_analogs[d_sensor]) {

       // Read the raw values for the left/right and top/bottom channels
       unsigned char raw_lr = *bufptr;
       bufptr++;
       unsigned char raw_tb = *bufptr;
       bufptr++;

       // Normalize the values to the range -1 to 1
       is900_analogs[d_sensor]->setChannelValue(0, (raw_lr - 127) / 128.0);
       is900_analogs[d_sensor]->setChannelValue(1, (raw_tb - 127) / 128.0);

       // Report the new values
       is900_analogs[d_sensor]->report_changes();
       is900_analogs[d_sensor]->mainloop();
   }

   //--------------------------------------------------------------------
   // Done with the decoding, set the report to ready
   //--------------------------------------------------------------------

   status = vrpn_TRACKER_SYNCING;
   bufcount = 0;

#ifdef VERBOSE2
      print_latest_report();
#endif

   return 1;
}


/** This function indicates to the driver that there is some sort of InterSense IS-900-
    compatible button device attached to the port (either a Wand or a Stylus).  The driver
    will configure the device to send reports when buttons are pressed and released.

    This routine returns 0 on success and -1 on failure (due to the sensor number being too
    large or errors writing to the device or can't create the button).
*/

int vrpn_Tracker_Fastrak::add_is900_button(const char *button_device_name, int sensor, int numbuttons)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= num_stations) ) {
	return -1;
    }

    // Add a new button device and set the pointer to point at it.
    try { is900_buttons[sensor] = new vrpn_Button_Server(button_device_name, d_connection, numbuttons); }
    catch (...) {
	VRPN_MSG_ERROR("Cannot open button device");
	return -1;
    }

    // Send a new station-format command to the tracker so it will report the button states.
    return set_sensor_output_format(sensor);
}

// this routine is called when an "FTStylus" button is encountered by the tracker init string parser
// it sets up the VRPN button device & enables the switch "really_fastrak" that affects subsequent interpretation
// of button readings - Debug
int vrpn_Tracker_Fastrak::add_fastrak_stylus_button(const char *button_device_name, int sensor, int numbuttons)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= num_stations) ) {
	return -1;
    }

    // Add a new button device and set the pointer to point at it.
    try { is900_buttons[sensor] = new vrpn_Button_Server(button_device_name, d_connection, numbuttons); }
    catch (...) {
	VRPN_MSG_ERROR("Cannot open button device");
	return -1;
    }

	// Debug's HACK here.  Make sure it knows that it is a plain vanilla fastrak, so it will
	// get the button output parameters right.
	really_fastrak=true;

    // Send a new station-format command to the tracker so it will report the button states.
    return set_sensor_output_format(sensor);
}


/** This function indicates to the driver that there is an InterSense IS-900-
    compatible joystick device attached to the port (a Wand).  The driver
    will configure the device to send reports indicating the current status of the
    analogs when they change.  Note that a separate call to add_is900_button must
    be made in order to enable the buttons on the wand: this routine only handles
    the analog channels.

    The c0 and c1 parameters specify the clipping and scaling to take the reports
    from the two joystick axes into the range [-1..1].  The default is unscaled.

    This routine returns 0 on success and -1 on failure (due to the sensor number being too
    large or errors writing to the device or can't create the analog).
*/

int vrpn_Tracker_Fastrak::add_is900_analog(const char *analog_device_name, int sensor,
	    double c0Min, double c0Low, double c0Hi, double c0Max,
	    double c1Min, double c1Low, double c1Hi, double c1Max)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= num_stations) ) {
	return -1;
    }

    // Add a new analog device and set the pointer to point at it.
    try { is900_analogs[sensor] = new vrpn_Clipping_Analog_Server(analog_device_name, d_connection); }
    catch (...) {
	VRPN_MSG_ERROR("Cannot open analog device");
	return -1;
    }

    // Set the analog to have two channels, and set its channels to 0 to start with
    is900_analogs[sensor]->setNumChannels(2);
    is900_analogs[sensor]->setChannelValue(0, 0.0);
    is900_analogs[sensor]->setChannelValue(1, 0.0);

    // Set the scaling on the two channels.
    is900_analogs[sensor]->setClipValues(0, c0Min, c0Low, c0Hi, c0Max);
    is900_analogs[sensor]->setClipValues(1, c1Min, c1Low, c1Hi, c1Max);

    // Send a new station-format command to the tracker so it will report the analog status
    return set_sensor_output_format(sensor);
}
