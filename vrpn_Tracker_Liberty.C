// vrpn_Tracker_Liberty.C
//	This file contains the code to operate a Polhemus Liberty Tracker.
// This file is based on the vrpn_Tracker_Fastrak.C file.

// Modified to work with the Polhemus Patriot as well.

#include <ctype.h>                      // for isprint
#include <stdio.h>                      // for fprintf, stderr, sprintf, etc
#include <stdlib.h>                     // for atoi
#include <string.h>                     // for strlen, strtok

#include "quat.h"                       // for Q_W, Q_X, Q_Y, Q_Z
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Button.h"                // for vrpn_Button_Server
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc
#include "vrpn_Tracker_Liberty.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#define	INCHES_TO_METERS	(2.54/100.0)
static const bool VRPN_LIBERTY_DEBUG = false;  // General Debug Messages
static const bool VRPN_LIBERTY_DEBUGA = false; // Only errors

vrpn_Tracker_Liberty::vrpn_Tracker_Liberty(const char *name, vrpn_Connection *c, 
		      const char *port, long baud, int enable_filtering, int numstations,
		      const char *additional_reset_commands, int whoamilen) :
    vrpn_Tracker_Serial(name,c,port,baud),
    do_filter(enable_filtering),
    num_stations(numstations>vrpn_LIBERTY_MAX_STATIONS ? vrpn_LIBERTY_MAX_STATIONS : numstations),
    num_resets(0),
    whoami_len(whoamilen>vrpn_LIBERTY_MAX_WHOAMI_LEN ? vrpn_LIBERTY_MAX_WHOAMI_LEN : whoamilen),
    got_single_sync_char(0)
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
		stylus_buttons[i] = NULL;
	}

	if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG] Constructed Liberty Object\n");
}

vrpn_Tracker_Liberty::~vrpn_Tracker_Liberty()
{

}

/** This routine augments the basic sensor-output setting function of the Liberty
 .  It sets the device for position + quaternion + any of
    the extended fields.  It puts a space at the end so that we can check to make
    sure we have complete good records for each report.

    Returns 0 on success and -1 on failure.
*/

int vrpn_Tracker_Liberty::set_sensor_output_format(int sensor)
{
    char    outstring[64];
    const char    *timestring;
    const char    *buttonstring;
    const char    *analogstring;

    // Set output format for the station to be position and quaternion,
    // and any of the extended Liberty (stylus with button) or
    // IS900 states (timestamp, button, analog).
    // This command is a capitol 'o' followed by the number of the
    // station, then comma-separated values (2 for xyz, 7 for quat, 8 for
    // timestamp, 10 for buttons, 0 for space) that
    // indicate data sets, followed by character 13 (octal 15).
    // Note that the sensor number has to be bumped to map to station number.

    timestring = ",8";
    buttonstring = stylus_buttons[sensor] ? ",10" : "";
    analogstring="";

     sprintf(outstring, "O%d,2,7%s%s%s,0\015", sensor+1, timestring,
	buttonstring, analogstring);
 
     if (VRPN_LIBERTY_DEBUG)     fprintf(stderr,"[DEBUG]: %s \n",outstring);
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

/** This routine augments the standard Liberty report (3 initial characters +
    3*4 for position + 4*4 for quaternions) to include the timestamp for the given sensor.

    It returns the number of characters total to expect for a report for the
    given sensor.
*/

int vrpn_Tracker_Liberty::report_length(int sensor)
{
    int	len;

    len = 9;	// Basic report: Header information (8) + space at the end (1)
    len += 3*4;	// Four bytes/float, 3 floats for position
    len += 4*4;	// Four bytes/float, 4 floats for quaternion
    len += 4;   // Timestamp
	len += stylus_buttons[sensor] ? 4 : 0;	// If applicable, 4 bytes for button info

    return len;
}

//   This routine will reset the tracker and set it to generate the types
// of reports we want.

void vrpn_Tracker_Liberty::reset()
{
   int i,resetLen,ret;
   char reset[10];
   char errmsg[512];
   char outstring1[64],outstring3[64];

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
   fprintf(stderr,"[DEBUG] Beginning Reset");
   resetLen = 0;
   num_resets++;		  	// We're trying another reset
   if (num_resets > 0) {	// Try to get it out of a query loop if its in one
   	reset[resetLen++] = (char) (13); // Return key -> get ready
	reset[resetLen++] = 'F';
	reset[resetLen++] = '0';
 	reset[resetLen++] = (char) (13); // Return key -> get ready	
	//	reset[resetLen++] = (char) (13);
	//	reset[resetLen++] = (char) (13);
   }
   /* XXX These commands are probably never needed, and can cause real
      headaches for people who are keeping state in their trackers (especially
      the InterSense trackers).  Taking them out in version 05.01; you can put
      them back in if your tracker isn't resetting as well.
   if (num_resets > 3) {	// Get a little more aggressive
	reset[resetLen++] = 'W'; // Reset to factory defaults
	reset[resetLen++] = (char) (11); // Ctrl + k --> Burn settings into EPROM
   }
   */
   if (num_resets > 2) {
       reset[resetLen++] = (char) (25); // Ctrl + Y -> reset the tracker
       reset[resetLen++] = (char) (13); // Return Key
   }
   reset[resetLen++] = 'P'; // Put it into polled (not continuous) mode

   sprintf(errmsg, "Resetting the tracker (attempt %d)", num_resets);
   VRPN_MSG_WARNING(errmsg);
   for (i = 0; i < resetLen; i++) {
	if (vrpn_write_characters(serial_fd, (unsigned char*)&reset[i], 1) == 1) {
		fprintf(stderr,".");
		vrpn_SleepMsecs(1000.0*2);  // Wait after each character to give it time to respond
   	} else {
		perror("Liberty: Failed writing to tracker");
		status = vrpn_TRACKER_FAIL;
		return;
	}
   }
   //XXX Take out the sleep and make it keep spinning quickly
   // You only need to sleep 10 seconds for an actual Liberty.
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

   // Asking for tracker status. S not implemented in Liberty and hence
   // ^V (WhoAmI) is used. It retruns 196 bytes

   char statusCommand[2];
   statusCommand[0]=(char)(22); // ^V
   statusCommand[1]=(char)(13); // Return Key

   if (vrpn_write_characters(serial_fd, (const unsigned char *) &statusCommand[0], 2) == 2) {
      vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
   } else {
	perror("  Liberty write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[vrpn_LIBERTY_MAX_WHOAMI_LEN+1];

   // Attempt to read whoami_len characters. 
   ret = vrpn_read_available_characters(serial_fd, statusmsg, whoami_len);
   if (ret != whoami_len) {
  	fprintf(stderr,"  Got %d of %d characters for status\n",ret, whoami_len);
   }
   if (ret != -1) {
      statusmsg[ret] = '\0';	// Null-terminate the string
   }
   // It seems like some versions of the tracker report longer
   // messages; so we reduced this check so that it does not check for the
   // appropriate length of message or for the last character being a 10,
   // so that it works more generally.  The removed tests are:
   // || (ret!=whoami_len) || (statusmsg[ret-1]!=(char)(10))
   if ( (statusmsg[0]!='0') ) {
     int i;
     fprintf(stderr, "  Liberty: status is (");
     for (i = 0; i < ret; i++) {
      	if (isprint(statusmsg[i])) {
         	fprintf(stderr,"%c",statusmsg[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",statusmsg[i]);
         }
     }
     fprintf(stderr,"\n)\n");
     VRPN_MSG_ERROR("Bad status report from Liberty, retrying reset");
     return;
   } else {
     VRPN_MSG_WARNING("Liberty/Isense gives status (this is good)");
printf("LIBERTY LATUS STATUS (whoami):\n%s\n\n",statusmsg);
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

   // Enable filtering if the constructor parameter said to.
   // Set filtering for both position (X command) and orientation (Y command)
   // to the values that are recommended as a "jumping off point" in the
   // Liberty manual.

   if (do_filter) {
     if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG]: Enabling filtering\n");

     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"X0.2,0.2,0.8,0.8\015", 17) == 17) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  Liberty write position filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"Y0.2,0.2,0.8,0.8\015", 17) == 17) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  Liberty write orientation filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
   } else {
     if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG]: Disabling filtering\n");

     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"X0,1,0,0\015", 9) == 9) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  Liberty write position filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
     if (vrpn_write_characters(serial_fd,
	     (const unsigned char *)"Y0,1,0,0\015", 9) == 9) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  Liberty write orientation filter failed");
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
	char	add_cmd_copy[sizeof(add_reset_cmd)];
	char	string_to_send[sizeof(add_reset_cmd)];
	int	seconds_to_wait;

	printf("  Liberty writing extended reset commands...\n");

	// Make a copy of the additional reset string, since it is consumed
        vrpn_strcpy(add_cmd_copy, add_reset_cmd);

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
   sprintf(outstring1, "F1\r");
   if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring1,
			    strlen(outstring1)) == (int)strlen(outstring1)) {
      fprintf(stderr, "  Liberty set to binary mode\n");

   }

   // Set tracker to continuous mode
   sprintf(outstring3, "C\r");
   if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring3,
                                strlen(outstring3)) != (int)strlen(outstring3)) {
 	perror("  Liberty write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   } else {
   	fprintf(stderr, "  Liberty set to continuous mode\n");
   }

   // If we are using the Liberty timestamps, clear the timer on the device and
   // store the time when we cleared it.  First, drain any characters in the output
   // buffer to ensure we're sending right away.  Then, send the reset command and
   // store the time that we sent it, plus the estimated time for the characters to
   // get across the serial line to the device at the current baud rate.
   // Set time units to milliseconds (MT) and reset the time (MZ).

	char	clear_timestamp_cmd[] = "Q0\r";

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
	vrpn_gettimeofday(&liberty_zerotime, NULL);

   // Done with reset.
   vrpn_gettimeofday(&watchdog_timestamp, NULL);	// Set watchdog now
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
// character or something and re-synchronize with the Liberty by waiting
// until the start-of-report character ('0') comes around again.
// The routine that calls this one makes sure we get a full reading often
// enough (ie, it is responsible for doing the watchdog timing to make sure
// the tracker hasn't simply stopped sending characters).
   
int vrpn_Tracker_Liberty::get_report(void)
{
   char errmsg[512];	// Error message to send to VRPN
   int ret;		// Return value from function call to be checked
   unsigned char *bufptr;	// Points into buffer at the current value to read

   //--------------------------------------------------------------------
   // Each report starts with the ASCII 'LY' characters. If we're synching,
   // read a byte at a time until we find a 'LY' characters.
   //--------------------------------------------------------------------
   // For the Patriot this is 'PA'.
   // For the (high speed) Liberty Latus this is 'LU'.

   if (status == vrpn_TRACKER_SYNCING) {

     // Try to get the first sync character if don't already have it. 
     // If none, just return.
     if (got_single_sync_char != 1) {
       ret = vrpn_read_available_characters(serial_fd, buffer, 1);
       if (ret != 1) {
	 //if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG]: Missed First Sync Char, ret= %i\n",ret);
	 return 0;
       }
     }

     // Try to get the second sync character. If none, just return
     ret = vrpn_read_available_characters(serial_fd, &buffer[1], 1);
     if (ret == 1) {
       //Got second sync Char
       got_single_sync_char = 0;
     }
     else if (ret != -1) {
       if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG]: Missed Second Sync Char\n");
       got_single_sync_char = 1;
       return 0;
     }

      // If it is not 'LY' or 'PA' or 'LU' , we don't want it but we
      // need to look at the next one, so just return and stay
      // in Syncing mode so that we will try again next time through.
      // Also, flush the buffer so that it won't take as long to catch up.
      if (
      ((( buffer[0] == 'L') && (buffer[1] == 'Y')) != 1) 
      && 
      ((( buffer[0] == 'P') && (buffer[1] == 'A')) != 1)
      && 
      ((( buffer[0] == 'L') && (buffer[1] == 'U')) != 1)
      ) 
      {
      	sprintf(errmsg,"While syncing (looking for 'LY' or 'PA' or 'LU', "
		"got '%c%c')", buffer[0], buffer[1]);
	VRPN_MSG_INFO(errmsg);
	vrpn_flush_input_buffer(serial_fd);
	if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUGA]: Getting Report - Not LY or PA or LU, Got Character %c %c \n",buffer[0],buffer[1]);
      	return 0;
      }

        if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG]: Getting Report - Got LY or PA or LU\n");

      // Got the first character of a report -- go into AWAITING_STATION mode
      // and record that we got one character at this time. The next
      // bit of code will attempt to read the station.
      // The time stored here is as close as possible to when the
      // report was generated.  For the InterSense 900 in timestamp
      // mode, this value will be overwritten later.
      bufcount = 2;
      //      vrpn_gettimeofday(&timestamp, NULL);
      status = vrpn_TRACKER_AWAITING_STATION;
   }

   //--------------------------------------------------------------------
   // The third character of each report is the station number.  Once
   // we know this, we can compute how long the report should be for the
   // given station, based on what values are in its report.
   // The station number is converted into a VRPN sensor number, where
   // the first Liberty station is '1' and the first VRPN sensor is 0.
   //--------------------------------------------------------------------

   if (status == vrpn_TRACKER_AWAITING_STATION) {

      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, &buffer[bufcount], 1) != 1) {
      	return 0;
      }
            if (VRPN_LIBERTY_DEBUG) fprintf(stderr,"[DEBUG]: Awaiting Station - Got Station (%i) \n",buffer[2]);

      d_sensor = buffer[2] - 1;	// Convert ASCII 1 to sensor 0 and so on.
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
	if (VRPN_LIBERTY_DEBUGA) fprintf(stderr,"[DEBUG]: Error Reading Report\n");
	VRPN_MSG_ERROR("Error reading report");
	status = vrpn_TRACKER_FAIL;
	return 0;
   }
   bufcount += ret;
   if (bufcount < REPORT_LEN) {	// Not done -- go back for more
     if (VRPN_LIBERTY_DEBUG)	fprintf(stderr,"[DEBUG]: Don't have full report (%i of %i)\n",bufcount,REPORT_LEN);
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
   //	fprintf(stderr,"[DEBUG]: Got full report\n");

   if (
   ((buffer[0] != 'L') || (buffer[1] != 'Y'))
   && 
   ((buffer[0] != 'P') || (buffer[1] != 'A'))
   && 
   ((buffer[0] != 'L') || (buffer[1] != 'U'))
   ) {
     if (VRPN_LIBERTY_DEBUGA)	fprintf(stderr,"[DEBUG]: Don't have LY or PA or 'LU' at beginning");
	   status = vrpn_TRACKER_SYNCING;
	   VRPN_MSG_INFO("Not 'LY' or 'PA' or 'LU' in record, re-syncing");
	   vrpn_flush_input_buffer(serial_fd);
	   return 0;
   }

   if (buffer[bufcount-1] != ' ') {
	   status = vrpn_TRACKER_SYNCING;
	   VRPN_MSG_INFO("No space character at end of report, re-syncing\n");
	   vrpn_flush_input_buffer(serial_fd);
	   if (VRPN_LIBERTY_DEBUGA) fprintf(stderr,"[DEBUG]: Don't have space at end of report, got (%c) sensor %i\n",buffer[bufcount-1], d_sensor);

	   return 0;
   }

   //Decode the error status and output a debug message
   if (buffer[4] != ' ') {
     // An error has been flagged
     if (VRPN_LIBERTY_DEBUGA) fprintf(stderr,"[DEBUG]:Error Flag %i\n",buffer[4]);
   }

   //--------------------------------------------------------------------
   // Decode the X,Y,Z of the position and the W,X,Y,Z of the quaternion
   // (keeping in mind that we store quaternions as X,Y,Z, W).
   //--------------------------------------------------------------------
   // The reports coming from the Liberty are in little-endian order,
   // which is the opposite of the network-standard byte order that is
   // used by VRPN. Here we swap the order to big-endian so that the
   // routines below can pull out the values in the correct order.
   // This is slightly inefficient on machines that have little-endian
   // order to start with, since it means swapping the values twice, but
   // that is more than outweighed by the cleanliness gained by keeping
   // all architecture-dependent code in the vrpn_Shared.C file.
   //--------------------------------------------------------------------

   // Point at the first value in the buffer (position of the X value)
   bufptr = &buffer[8];

   // When copying the positions, convert from inches to meters, since the
   // Liberty reports in inches and VRPN reports in meters.
   pos[0] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * INCHES_TO_METERS;
   pos[1] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * INCHES_TO_METERS;
   pos[2] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * INCHES_TO_METERS;

   // Change the order of the quaternion fields to match quatlib order
   d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
   d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
   d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
   d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);

   //--------------------------------------------------------------------
   // Decode the time from the Liberty system (unsigned 32bit int), add it to the
   // time we zeroed the tracker, and update the report time.  Remember
   // to convert the MILLIseconds from the report into MICROseconds and
   // seconds.
   //--------------------------------------------------------------------

       struct timeval delta_time;   // Time since the clock was reset

       // Read the integer value of the time from the record.
       vrpn_uint32 read_time = vrpn_unbuffer_from_little_endian<vrpn_uint32>(bufptr);

       // Convert from the float in MILLIseconds to the struct timeval
       delta_time.tv_sec = (long)(read_time / 1000);	// Integer trunction to seconds
       vrpn_uint32 read_time_milliseconds = read_time - delta_time.tv_sec * 1000;	// Subtract out what we just counted
       delta_time.tv_usec = (long)(read_time_milliseconds * 1000);	// Convert remainder to MICROseconds

       // The time that the report was generated
       timestamp = vrpn_TimevalSum(liberty_zerotime, delta_time);
       vrpn_gettimeofday(&watchdog_timestamp, NULL);	// Set watchdog now       
 
   //--------------------------------------------------------------------
   // If this sensor has button on it, decode the button values
   // into the button device and mainloop the button device so that
   // it will report any changes.
   //--------------------------------------------------------------------

   if (stylus_buttons[d_sensor]) {
	   // Read the integer value of the bytton status from the record.
	   vrpn_uint32 button_status = vrpn_unbuffer_from_little_endian<vrpn_uint32>(bufptr);
	   
	   stylus_buttons[d_sensor]->set_button(0, button_status);
	   stylus_buttons[d_sensor]->mainloop();
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

// this routine is called when an "Stylus" button is encountered
// by the tracker init string parser it sets up the VRPN button
// device
int vrpn_Tracker_Liberty::add_stylus_button(const char *button_device_name, int sensor, int numbuttons)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= num_stations) ) {
	return -1;
    }

    // Add a new button device and set the pointer to point at it.
    try { stylus_buttons[sensor] = new vrpn_Button_Server(button_device_name, d_connection, numbuttons); }
    catch (...) {
	VRPN_MSG_ERROR("Cannot open button device");
	return -1;
    }

    // Send a new station-format command to the tracker so it will report the button states.
    return set_sensor_output_format(sensor);
}
