// vrpn_Tracker_LibertyHS.C
//	This file contains the class header for a High Speed Polhemus Liberty
// Latus Tracker.
// This file is based on the vrpn_Tracker_Liberty.C file, with modifications made
// to allow it to operate a Liberty Latus instead. It has been tested on Linux.

#include <ctype.h>                      // for isprint
#include <stdio.h>                      // for fprintf, stderr, sprintf, etc
#include <stdlib.h>                     // for atoi
#include <string.h>                     // for strlen, strtok

#include "quat.h"                       // for Q_W, Q_X, Q_Y, Q_Z
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_WARNING, etc
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc
#include "vrpn_Tracker_LibertyHS.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_LIBUSB_1_0)

#include <libusb.h>                     // for libusb_bulk_transfer, etc

static const bool VRPN_LIBERTYHS_METRIC_UNITS = true;
static const bool VRPN_LIBERTYHS_DEBUG = false;  // General Debug Messages
static const bool VRPN_LIBERTYHS_DEBUGA = false; // Only errors

vrpn_Tracker_LibertyHS::vrpn_Tracker_LibertyHS(const char *name, vrpn_Connection *c, 
		      long baud, int enable_filtering, int numstations,
                      int receptoridx, const char *additional_reset_commands, int whoamilen) :
    vrpn_Tracker_USB(name,c,LIBERTYHS_VENDOR_ID,LIBERTYHS_PRODUCT_ID,baud),
    do_filter(enable_filtering),
    num_stations(numstations>vrpn_LIBERTYHS_MAX_STATIONS ? vrpn_LIBERTYHS_MAX_STATIONS : numstations),
    receptor_index(receptoridx),
    num_resets(0),
    whoami_len(whoamilen>vrpn_LIBERTYHS_MAX_WHOAMI_LEN ? vrpn_LIBERTYHS_MAX_WHOAMI_LEN : whoamilen),
    read_len(0), sync_index(-1)
{
	if (additional_reset_commands == NULL) {
		add_reset_cmd[0] = '\0';
	} else {
		vrpn_strcpy(add_reset_cmd, additional_reset_commands);
	}

	if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG] Constructed LibertyHS Object\n");
}

vrpn_Tracker_LibertyHS::~vrpn_Tracker_LibertyHS()
{

   fprintf(stderr," interrupting continuous print output mode\n");
   char pollCommand = 'P';
   write_usb_data(&pollCommand,1);
   vrpn_SleepMsecs(1000.0); // Sleep for a second to let it respond
   flush_usb_data();

}

/** This routine augments the basic sensor-output setting function of the LibertyHS
 .  It sets the device for position + quaternion + any of
    the extended fields.  It puts a space at the end so that we can check to make
    sure we have complete good records for each report.

    Returns 0 on success and -1 on failure.
*/

int vrpn_Tracker_LibertyHS::set_sensor_output_format(int sensor)
{
    // Set output format for the station to be position and quaternion.
    // This command is a capitol 'o' followed by '*' to format all sensors
    // if sensor equals -1 or by the number of one specific station, 
    // then comma-separated values (2 for xyz, 7 for quat, 8 for timestamp, 
    // 0 for space) that indicate data sets, followed by character 13 (octal 15).
    // Note that the sensor number has to be bumped to map to station number.

    char    outstring[64];
    if (sensor == -1)
          sprintf(outstring, "O*,2,7,8,9,0\015");
    else
          sprintf(outstring, "O%d,2,7,8,9,0\015", sensor+1);
    int len = strlen(outstring);
    int ret;
 
    if (VRPN_LIBERTYHS_DEBUG)     fprintf(stderr,"[DEBUG]: %s \n",outstring);
    if ( (ret = write_usb_data(outstring,len)) != len) {
#ifdef libusb_strerror
          fprintf(stderr,"vrpn_Tracker_LibertyHS::libusb_bulk_transfer(): Could not send: %s\n",
                  libusb_strerror(static_cast<libusb_error>(ret)));
#else
          fprintf(stderr,"vrpn_Tracker_LibertyHS::libusb_bulk_transfer(): Could not send: code %d\n",
                  ret);
#endif
          status = vrpn_TRACKER_FAIL;
          return -1;
    }
    vrpn_SleepMsecs(50);	// Sleep for a bit to let command run

    return 0;
 }

/** This routine augments the standard LibertyHS report (3 initial characters +
    3*4 for position + 4*4 for quaternions) to include the timestamp for the given sensor.

    It returns the number of characters total to expect for a report for the
    given sensor.
*/

int vrpn_Tracker_LibertyHS::report_length(int sensor)
{
    int	len;

    len = 9;	// Basic report: Header information (8) + space at the end (1)
    len += 3*4;	// Four bytes/float, 3 floats for position
    len += 4*4;	// Four bytes/float, 4 floats for quaternion
    len += 4;   // Timestamp
    len += 4;   // Framecount

    return len;
}


int vrpn_Tracker_LibertyHS::write_usb_data(void* data, int len)
{
  int sent_len = 0;
  int ret = libusb_bulk_transfer(_device_handle, LIBERTYHS_WRITE_EP | LIBUSB_ENDPOINT_OUT,
                                 (vrpn_uint8*)data, len, &sent_len, 50);

  if (ret != 0)
    fprintf(stderr,"vrpn_Tracker_LibertyHS::write_usb_data(): LIBUSB ERROR '%i'\n",ret);

  return sent_len;
}


int vrpn_Tracker_LibertyHS::read_usb_data(void* data, int maxlen, unsigned int timeout)
{
  int read_len = 0;

  int ret = libusb_bulk_transfer(_device_handle, LIBERTYHS_READ_EP | LIBUSB_ENDPOINT_IN, 
                                 (vrpn_uint8*)data, maxlen, &read_len, timeout);

  /*
  fprintf(stderr,"vrpn_Tracker_LibertyHS::read_usb_data() READ %i chars: ___",read_len);
  for (int i = 0; i < read_len; i++) {
    if (isprint(((unsigned char*)data)[i])) {
      fprintf(stderr,"%c",((unsigned char*)data)[i]);
    } else {
      fprintf(stderr,"[0x%02X]",((unsigned char*)data)[i]);
    }
  }
  fprintf(stderr,"___  ");
  if (ret != 0) fprintf(stderr," LIBUSB ERROR: code #'%i'\n",ret); else fprintf(stderr,"\n");
  */

  // Success: return number of read bytes
  return read_len;
}

void vrpn_Tracker_LibertyHS::flush_usb_data()
{

  int len;
  vrpn_uint8 buf[VRPN_TRACKER_USB_BUF_SIZE];

  // Flush usb data as long as they are available on usb port
  do {
    len = read_usb_data(buf, VRPN_TRACKER_USB_BUF_SIZE); 
  } while(len);

}


// This routine will reset the tracker and set it to generate the types
// of reports we want.

void vrpn_Tracker_LibertyHS::reset()
{
   int i,resetLen,ret;
   char reset[10];
   char errmsg[512];
   char outstring1[64],outstring2[64],outstring3[64],outstring4[64];

   //--------------------------------------------------------------------
   // This section deals with resetting the tracker to its default state.
   // Multiple attempts are made to reset, getting more aggressive each
   // time. This section completes when the tracker reports a valid status
   // message after the reset has completed.
   //--------------------------------------------------------------------

   // Send the tracker a string that should reset it.  The first time we
   // try this, just do the normal 'c' command to put it into polled mode.
   // after a few tries with this, use the '^Y' reset.  Later, try to reset
   // to the factory defaults.  Then toggle the extended mode.
   // Then put in a carriage return to try and break it out of
   // a query mode if it is in one. These additions are cumulative: by the
   // end, we're doing them all.
   if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG] Beginning Reset");
   resetLen = 0;
   num_resets++;		  	// We're trying another reset

   if (num_resets > 0) {	// Try to get it out of a query loop if its in one
	reset[resetLen++] = 'F';
	reset[resetLen++] = '0';
 	reset[resetLen++] = (char) (13); // Return key -> get ready
   }

   if (num_resets > 2) {
       reset[resetLen++] = (char) (25); // Ctrl + Y -> reset the tracker
       reset[resetLen++] = (char) (13); // Return Key
   }

   reset[resetLen++] = 'P'; // Put it into polled (not continuous) mode

   sprintf(errmsg, "Resetting the tracker (attempt %d)", num_resets);
   VRPN_MSG_WARNING(errmsg);
   for (i = 0; i < resetLen; i++) {
         if (write_usb_data(&reset[i],1) == 1) {
           fprintf(stderr,".");
           vrpn_SleepMsecs(1000.0*2);  // Wait after each character to give it time to respond
         } else {
           perror("Liberty: Failed writing to tracker");
           status = vrpn_TRACKER_FAIL;
           return;
         }
   }

   if (num_resets > 2) {
       vrpn_SleepMsecs(1000.0*20);	// Sleep to let the reset happen, if we're doing ^Y
   }

   fprintf(stderr,"\n");

   // Get rid of the characters left over from before the reset
   flush_usb_data();

   // Make sure that the tracker has stopped sending characters
   vrpn_SleepMsecs(1000.0*2);
   unsigned char scrap[80];
   if ( (ret = read_usb_data((void*)scrap, 80)) != 0) {
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
     flush_usb_data();		// Flush what's left
   }

   // Asking for tracker status. ^V (WhoAmI) is used. It retruns 288 bytes
   char statusCommand[2];
   statusCommand[0]=(char)(22); // ^V
   statusCommand[1]=(char)(13); // Return Key

   if (write_usb_data(&statusCommand[0],2) == 2) {
      vrpn_SleepMsecs(1000.0); // Sleep for a second to let it respond
   } else {
	perror("  LibertyHS write failed (WhoAmI command)");
	status = vrpn_TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[vrpn_LIBERTYHS_MAX_WHOAMI_LEN+1];

   // Attempt to read whoami_len characters.
   if ( (ret = read_usb_data((void*)statusmsg, whoami_len)) != whoami_len) {
  	fprintf(stderr,"  Got %d of %d characters for status\n",ret, whoami_len);
   }

   if ( (statusmsg[0]!='0') ) {
     int i;
     if (ret != -1) {
        statusmsg[ret] = '\0';	// Null-terminate the string
     }
     fprintf(stderr, "  LibertyHS: status is (");
     for (i = 0; i < ret; i++) {
      	if (isprint(statusmsg[i])) {
         	fprintf(stderr,"%c",statusmsg[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",statusmsg[i]);
         }
     }
     fprintf(stderr,"\n)\n");
     VRPN_MSG_ERROR("Bad status report from LibertyHS, retrying reset");
     return;
   } else {
     VRPN_MSG_WARNING("LibertyHS gives status (this is good)");
     num_resets = 0; 	// Success, use simple reset next time
   }

   //--------------------------------------------------------------------
   // Now that the tracker has given a valid status report, set all of
   // the parameters the way we want them.
   //--------------------------------------------------------------------

   // Set output format for each of the possible stations.

   if (set_sensor_output_format()) {
	   return;
   }

   // Enable filtering if the constructor parameter said to.
   // Set filtering for both position (X command) and orientation (Y command)
   // to the values that are recommended as a "jumping off point" in the
   // LibertyHS manual.

   if (do_filter) {
     if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG]: Enabling filtering\n");

     if (write_usb_data(const_cast<char*>("X0.2,0.2,0.8,0.8\015"), 17) == 17) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  LibertyHS write position filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
     if (write_usb_data(const_cast<char*>("Y0.2,0.2,0.8,0.8\015"), 17) == 17) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  LibertyHS write orientation filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
   } else {
     if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG]: Disabling filtering\n");

     if (write_usb_data(const_cast<char*>("X0,1,0,0\015"), 9) == 9) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  LibertyHS write position filter failed");
	status = vrpn_TRACKER_FAIL;
	return;
     }
     if (write_usb_data(const_cast<char*>("Y0,1,0,0\015"), 9) == 9) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
     } else {
	perror("  LibertyHS write orientation filter failed");
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
	int	seconds_to_wait, count;

	printf("  LibertyHS writing extended reset commands...\n");

	// Make a copy of the additional reset string, since it is consumed
        vrpn_strcpy(add_cmd_copy, add_reset_cmd);

	// Pass through the string, testing each line to see if it is
	// a sleep command or a line to send to the tracker. Continue until
	// there are no more line delimiters ('\015'). Be sure to write the
	// \015 to the end of the string sent to the tracker.
	// Note that strok() puts a NULL character in place of the delimiter.

	next_line = strtok(add_cmd_copy, "\015");
        count = 0;
	while (next_line != NULL) {
		if (next_line[0] == '*') {	// This is a "sleep" line, see how long
			seconds_to_wait = atoi(&next_line[1]);
			fprintf(stderr,"   ...sleeping %d seconds\n",seconds_to_wait);
			vrpn_SleepMsecs(1000.0*seconds_to_wait);
		} else {	// This is a command line, send it
			sprintf(string_to_send, "%s\015", next_line);
                        fprintf(stderr,"     ...sending command: %s\n", string_to_send);
                        write_usb_data(string_to_send,strlen(string_to_send));
                        vrpn_SleepMsecs(1000.0*2);
		}
		next_line = strtok(next_line+strlen(next_line)+1, "\015");
	}

	// Sleep a little while to let this finish, then clear the input buffer
	vrpn_SleepMsecs(1000.0);
        flush_usb_data();
   }
   
   // Disable USB BUFFERING mode 
   // (disbale output buffering before USB transmission to the host)
   sprintf(outstring1, "@B0\r");
   if (write_usb_data(outstring1, strlen(outstring1)) == (int)strlen(outstring1)) {
      fprintf(stderr, "\n  Tracker USB buffering mode disabled\n");
   }
   vrpn_SleepMsecs(1000.0);

   // Set METRIC units
   if (VRPN_LIBERTYHS_METRIC_UNITS) {
        sprintf(outstring2, "U1\r");
        if (write_usb_data(outstring2, strlen(outstring2)) == (int)strlen(outstring2)) {
              fprintf(stderr, "  LibertyHS set to metric units\n");
        }
   }
   else
        fprintf(stderr, "  LibertyHS set to English units\n");
   vrpn_SleepMsecs(1000.0);

   // Set data format to BINARY mode
   sprintf(outstring3, "F1\r");
   if (write_usb_data(outstring3, strlen(outstring3)) == (int)strlen(outstring3)) {
      fprintf(stderr, "  LibertyHS set to binary mode\n\n");
   }
   vrpn_SleepMsecs(1000.0);

   // Launch detected markers
   if (launch_markers() != num_stations) {
        fprintf(stderr, "\nCould not launch the %i requested markers\n", num_stations);
	status = vrpn_TRACKER_FAIL;
        return;
   } else {
         fprintf(stderr, "\nAll %i markers are ready!\n", num_stations);
   }

   // Set tracker to CONTINUOUS mode
   sprintf(outstring4, "C\r");
   if (write_usb_data(outstring4, strlen(outstring4)) != (int)strlen(outstring4)) {
 	perror("  LibertyHS write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   } else {
   	fprintf(stderr, "\n  LibertyHS set to continuous mode\n\n");
   }

   // If we are using the LibertyHS timestamps, clear the timer on the device and
   // store the time when we cleared it.  Send the reset command and
   // store the time that we sent it.

   char	clear_timestamp_cmd[] = "Q0\r";

   if (write_usb_data(clear_timestamp_cmd, 
                      strlen(clear_timestamp_cmd)) != (int)strlen(clear_timestamp_cmd)) {
         VRPN_MSG_ERROR("Cannot send command to clear timestamp");
         status = vrpn_TRACKER_FAIL;
         return;
   }

   // Record the time as the base time from the tracker.
   vrpn_gettimeofday(&liberty_zerotime, NULL);

   // Done with reset.
   vrpn_gettimeofday(&watchdog_timestamp, NULL);	// Set watchdog now

   VRPN_MSG_WARNING("Reset Completed (this is good)");
   status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
}


// This function will try to read and send as many reports as there are 
// sensors (i.e. num_stations).
// It reads characters until it has a full report, then put that report 
// into the time, sensor, pos and quat fields so that it can
// be sent the next time through the loop. The time stored is that of
// the first character received as part of the report. Reports start with
// the header "0xy", where x is the station number and y is either the
// space character or else one of the characters "A-F". Characters "A-F"
// indicate weak signals and so forth, but in practice it is much harder
// to deal with them than to ignore them (they don't indicate hard error
// conditions). The report follows, 4 bytes per word in little-endian byte
// order; each word is an IEEE floating-point binary value. The first three
// are position in X,Y and Z. The next four are the unit quaternion in the
// order W, X,Y,Z. Then there is an ASCII space character at the end.
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize with the LibertyHS by waiting
// until the start-of-report character ('0') comes around again.
// The routine that calls this one makes sure we get a full reading often
// enough (ie, it is responsible for doing the watchdog timing to make sure
// the tracker hasn't simply stopped sending characters).
  
int vrpn_Tracker_LibertyHS::get_report(void)
{
   char errmsg[512];	// Error message to send to VRPN
   unsigned char *bufptr;	// Points into buffer at the current value to read

   //--------------------------------------------------------------------
   // Each report starts with the ASCII 'LU' characters. If we're synching,
   // read available bytes and check if we find the 'LU' characters.
   //--------------------------------------------------------------------

   for (int i = 0; i < num_stations; i++)
   {
     // Read new data from USB buffer if we are in Syncing mode
     if (status == vrpn_TRACKER_SYNCING) {
       read_len = read_usb_data(buffer, VRPN_TRACKER_USB_BUF_SIZE);

       if (read_len < 1) {
         if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG]: Missed First Sync Char, read_len = %i\n",read_len);
         return 0;
       }

       status = vrpn_TRACKER_PARTIAL;
       sync_index = 0;
     }
     else {   //  Partial mode (status = vrpn_TRACKER_PARTIAL)
       bufcount = 0; 
     }

     // Try to get the two first sync characters. If none, just return.
     while (sync_index < read_len - 1) {

       // If it is not 'LU', we don't want it but we
       // need to look at the next one, so just keep reading the buffer
       // and stay in Syncing mode until we find them.
       if ((buffer[sync_index] == 'L') && (buffer[sync_index + 1] == 'U')) {
         if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG]: Getting Report - Got LU\n");
         bufcount = 2;
         break;
       }

       if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUGA] While syncing: Getting Report - Not LU, Got Character %c \n",buffer[sync_index]);
       sync_index++;
     
     }

     // Try to get next character.  If none, just return and go back to Syncing mode.
     if ( static_cast<vrpn_int32>(sync_index + bufcount) >= (read_len - 1) ) {
       return 0;
     }

     // Get sensor number
     if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG]: Awaiting Station - Got Station (%i) \n",buffer[sync_index + bufcount]);
     d_sensor = buffer[sync_index + bufcount] - 1;	// Convert ASCII 1 to sensor 0 and so on.
     if ( (d_sensor < 0) || (d_sensor >= vrpn_LIBERTYHS_MAX_STATIONS) ) {
       sprintf(errmsg,"Bad sensor # (%d) in record, re-syncing", d_sensor + 1);
       VRPN_MSG_INFO(errmsg);
       status = vrpn_TRACKER_PARTIAL;
       sync_index += bufcount;
       continue;
     }
   
     bufcount++;
   
     // Figure out how long the current report should be based on the
     // settings for this sensor.
     REPORT_LEN = report_length(d_sensor);

     //--------------------------------------------------------------------
     // Read as many bytes of this report as we can, storing them
     // in the buffer.  We keep track of how many have been read so far
     // and only try to read the rest.  The routine that calls this one
     // makes sure we get a full reading often enough (ie, it is responsible
     // for doing the watchdog timing to make sure the tracker hasn't simply
     // stopped sending characters).
     //--------------------------------------------------------------------

     if (read_len - sync_index < static_cast<vrpn_int32>(REPORT_LEN)) {
       if (VRPN_LIBERTYHS_DEBUG) fprintf(stderr,"[DEBUG]: Don't have full report (%i of %i)\n",
                          read_len - sync_index,REPORT_LEN);
       return 0;
     }

     // Full report available
     bufcount = REPORT_LEN;

     //--------------------------------------------------------------------
     // We now have enough characters to make a full report. Check to make
     // sure that its format matches what we expect. If it does, the next
     // section will parse it. If it does not, we need to go to Partial mode
     // and ignore this report. A well-formed report has the
     // first characters 'LU', the next character is the ASCII station
     // number, and the third character is either a space or a letter.
     //--------------------------------------------------------------------
     if (VRPN_LIBERTYHS_DEBUG)	fprintf(stderr,"[DEBUG]: Got full report\n");

     if ((buffer[sync_index] != 'L') || (buffer[sync_index + 1] != 'U')) {
       if (VRPN_LIBERTYHS_DEBUGA)	fprintf(stderr,"[DEBUG]: Don't have 'LU' at beginning");
       VRPN_MSG_INFO("Not 'LU' in record, re-syncing");
       status = vrpn_TRACKER_PARTIAL;
       sync_index++;
       continue;
     }

     if (buffer[sync_index + bufcount - 1] != ' ') {
       VRPN_MSG_INFO("No space character at end of report, re-syncing\n");
       if (VRPN_LIBERTYHS_DEBUGA) fprintf(stderr,"[DEBUG]: Don't have space at end of report, got (%c) sensor %i\n",
                           buffer[sync_index + bufcount - 1], d_sensor);
       status = vrpn_TRACKER_PARTIAL;
       sync_index++;
       continue;
     }

     // Decode the error status and output a debug message

     if (buffer[sync_index + 4] != ' ') {
       // An error has been flagged
       if (VRPN_LIBERTYHS_DEBUGA) fprintf(stderr,"[DEBUG]:Error Flag %i\n",buffer[sync_index + 4]);
     }

     //--------------------------------------------------------------------
     // Decode the X,Y,Z of the position and the W,X,Y,Z of the quaternion
     // (keeping in mind that we store quaternions as X,Y,Z, W).
     //--------------------------------------------------------------------
     // The reports coming from the LibertyHS are in little-endian order,
     // which is the opposite of the network-standard byte order that is
     // used by VRPN. Here we swap the order to big-endian so that the
     // routines below can pull out the values in the correct order.
     // This is slightly inefficient on machines that have little-endian
     // order to start with, since it means swapping the values twice, but
     // that is more than outweighed by the cleanliness gained by keeping
     // all architecture-dependent code in the vrpn_Shared.C file.
     //--------------------------------------------------------------------

     // Point at the first value in the report (position of the X value)
     bufptr = &buffer[sync_index + 8];

     // When copying the positions, convert from inches to meters, since the
     // LibertyHS reports in inches and VRPN reports in meters.
     float convFactor = VRPN_LIBERTYHS_METRIC_UNITS ? 1.0f : VRPN_INCHES_TO_METERS;
     pos[0] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * convFactor;
     pos[1] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * convFactor;
     pos[2] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr) * convFactor;


     // Change the order of the quaternion fields to match quatlib order
     d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
     d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
     d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);
     d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_float32>(bufptr);

     //--------------------------------------------------------------------
     // Decode the time from the LibertyHS system (unsigned 32bit int), add it to the
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

     // The frame number corresponding to the report
     frame_count = vrpn_unbuffer_from_little_endian<vrpn_uint32>(bufptr);   
     
     /*
     fprintf(stderr,"READ_TIME = %d,%d + liberty_zerotime %d,%d = TIMESTAMP %d,%d ; WATCHDOG = %d,%d\n", 
             (int)delta_time.tv_sec,(int)delta_time.tv_usec, 
             (int)liberty_zerotime.tv_sec, (int)liberty_zerotime.tv_usec,
             (int)timestamp.tv_sec,(int)timestamp.tv_usec,
             (int)watchdog_timestamp.tv_sec,(int)watchdog_timestamp.tv_usec);
     */
     /*
     print_latest_report();
     double delta = vrpn_TimevalMsecs(vrpn_TimevalDiff(watchdog_timestamp,liberty_zerotime)) - (double) read_time;
     fprintf(stderr,"--> LATENCY = %6.3f msec\n",delta);
     */


     //--------------------------------------------------------------------
     // Done with the decoding, send the report
     //--------------------------------------------------------------------

     send_report();

     // Check if there's still data to read in the buffer
     sync_index += bufcount;
     if (sync_index < read_len) {
       // There might be remaining reports from other trackers in buffer.
       // Go to Partial mode.
       status = vrpn_TRACKER_PARTIAL;
     }
     else {
       // There is no more report in buffer
       break;
     }

#ifdef VERBOSE2
     print_latest_report();
#endif
   }

   return 1;
}


int vrpn_Tracker_LibertyHS::test_markers()
{
  // Define a marker map request
  char mapmsg[4];
  mapmsg[0] = (char) (21); // Ctrl + U -> active marker map
  mapmsg[1] = '0';
  mapmsg[2] = (char) (13); // Return Key

  // Send a marker map command to the tracker
  if (write_usb_data(&mapmsg[0], 3) == 3) {
        vrpn_SleepMsecs(1000.0*1);   // Sleep for a second to let it respond
  } else {
	perror("  LibertyHS write failed");
	return 0;
  }

  // Read binary output record
  unsigned char markermap[vrpn_LIBERTYHS_MAX_MARKERMAP_LEN+1];
  int ret = read_usb_data((void*)markermap, vrpn_LIBERTYHS_MAX_MARKERMAP_LEN);
  if (ret != vrpn_LIBERTYHS_MAX_MARKERMAP_LEN) {
        fprintf(stderr,"  Got %d of %d bytes for marker map\n",ret,vrpn_LIBERTYHS_MAX_MARKERMAP_LEN);
        return 0;
  }
  
  // Flush what's left
  flush_usb_data();

  // Check marker map to identify launched markers
  int marker_found = 0;
  unsigned char mask = (char) (1);
  for (int bit = 1; bit <= vrpn_LIBERTYHS_MAX_STATIONS; bit++) {
        if (mask & markermap[10]) {
              marker_found++;
              fprintf(stderr,"Tracker #%i launched\n",bit);
        }
        mask *= 2;
  }

  return marker_found;
}


int vrpn_Tracker_LibertyHS::launch_markers()
{
  fprintf(stderr,"\nDetect and launch %i markers:\n\n",num_stations);

  // Clear the input buffer
  flush_usb_data();

  // Check if the markers are already launched
  int marker_found = test_markers ();
  if (marker_found > 0) {
        fprintf(stderr,"\nWARNING: %i markers are already launched! If you want to change markers,\n",marker_found);
        fprintf(stderr,  "         turn off the SEU (System Electronics Unit) and run this application again.\n");  
        return marker_found;
  }

  // Define a launch marker request
  char launchmsg[4];
  launchmsg[0] = 'L';         // 'Li' command line (i.e. launch new tracker near receptor i)
  launchmsg[1] = (char) ((int) '0' + receptor_index);
  launchmsg[2] = (char) (13); // Return Key  

  // Detect and launch num_stations markers
  while (marker_found < num_stations) {

        // Wait for the user to place a tracker for activation
        fprintf(stderr,"\n-->  PLACE A NEW POWERED UP MARKER UNDER RECEPTOR #%i\n     ",receptor_index);
        for (int i = 10; i > 0 ; i--) {
              fprintf(stderr,"%i...",i);
              vrpn_SleepMsecs(1000.0);
        }

        // Send a launch marker command to receptor 'receptor_index'
        fprintf(stderr,"  sending LAUNCH MARKER command\n     DON'T MOVE THE TRACKER!\n\n");
        if (write_usb_data(&launchmsg[0], 3) == 3) {
              vrpn_SleepMsecs(1000.0*2);  // Wait after each character to give it time to respond
        } else {
              perror("LibertyHS: Failed writing launch marker command to tracker");
              break;
        }

        // Check if the markers are already launched
        if (test_markers () == num_stations)
              marker_found = num_stations;
  }

  return marker_found;
}

// End of LIBUSB
#endif
