// vrpn_Tracker_Isotrak.C
//	This file contains the code to operate a Polhemus Isotrack Tracker.
// This file is based on the vrpn_Tracker_Fastrack.C file, with modifications made
// to allow it to operate a Isotrack instead. The modifications are based
// on the old version of the Isotrack driver.
//	This version was written in the Spring 2006 by Bruno Herbelin.

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
#include "vrpn_Tracker_Isotrak.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"

#define	FT_INFO(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_NORMAL) ; if (d_connection && d_connection->connected()) d_connection->send_pending_reports(); }
#define	FT_WARNING(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_WARNING) ; if (d_connection && d_connection->connected()) d_connection->send_pending_reports(); }
#define	FT_ERROR(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_ERROR) ; if (d_connection && d_connection->connected()) d_connection->send_pending_reports(); }

vrpn_Tracker_Isotrak::vrpn_Tracker_Isotrak(const char *name, vrpn_Connection *c, 
                    const char *port, long baud, int enable_filtering, int numstations,
                    const char *additional_reset_commands) :
    vrpn_Tracker_Serial(name,c,port,baud),
    do_filter(enable_filtering),
    num_stations(numstations>vrpn_ISOTRAK_MAX_STATIONS ? vrpn_ISOTRAK_MAX_STATIONS : numstations)
{
        reset_time.tv_sec = reset_time.tv_usec = 0;
        if (additional_reset_commands == NULL) {
                sprintf(add_reset_cmd, "");
        } else {
                strncpy(add_reset_cmd, additional_reset_commands, sizeof(add_reset_cmd)-1);
        }
}

vrpn_Tracker_Isotrak::~vrpn_Tracker_Isotrak()
{

}


/** This routine augments the standard Isotrack report 
    It returns the number of characters total to expect for a report for the
    given sensor.
/// example report string :
/// 01   28.57 -10.28  13.95-0.9713-0.0093 0.1473 0.1855

/// Documentation ISOTRAK 2 USER's MANUAL p60
*/

int vrpn_Tracker_Isotrak::report_length(int /*sensor*/)
{
    int	len;

    len = 3;	// Basic report, "0" + Isotrack station char + space separator
    len += 3*7;	// + 3 x position string 
    len += 4*7;	// + 4 x quaternion string
    len += 1;   // + space at the end of report

    return len;
}

/** This routine sets the device for position + quaternion
    It puts a space at the end so that we can check to make
    sure we have complete good records for each report.

    Returns 0 on success and -1 on failure.
*/

int vrpn_Tracker_Isotrak::set_sensor_output_format(int /*sensor*/)
{
    char    outstring[16];

    /// Set output format for the station to be position, quaternion, and a space
    sprintf(outstring, "O2,11,0\015");
    if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring,
            strlen(outstring)) == (int)strlen(outstring)) {
        vrpn_SleepMsecs(50);	// Sleep for a bit to let command run
    } else {
        FT_ERROR("Write failed on format command");
        status = vrpn_TRACKER_FAIL;
        return -1;
    }
    return 0;
}

// This routine will reset the tracker and set it to generate the types
// of reports we want.
// This was based on the Isotrak User Manual from Polhemus (2001 Edition, Rev A)

void vrpn_Tracker_Isotrak::reset()
{
    static int numResets = 0;	// How many resets have we tried?
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
    // After a few tries with this, use a [return] character, and then use the ^Y to reset. 
    
    resetLen = 0;
    numResets++;		  	
    
    // We're trying another reset
    if (numResets > 1) {	        // Try to get it out of a query loop if its in one
            reset[resetLen++] = (unsigned char) (13); // Return key -> get ready
    }
    
    if (numResets > 2) {
        reset[resetLen++] = (unsigned char) (25); // Ctrl + Y -> reset the tracker
    }
    
    reset[resetLen++] = 'c'; // Put it into polled (not continuous) mode
    
    
    sprintf(errmsg, "Resetting the tracker (attempt %d)", numResets);
    FT_WARNING(errmsg);
    
    for (i = 0; i < resetLen; i++) {
            if (vrpn_write_characters(serial_fd, &reset[i], 1) == 1) {
                    fprintf(stderr,".");
                    vrpn_SleepMsecs(1000.0*2);  // Wait after each character to give it time to respond
            } else {
                    perror("Isotrack: Failed writing to tracker");
                    status = vrpn_TRACKER_FAIL;
                    return;
            }
    }
    //XXX Take out the sleep and make it keep spinning quickly
    if (numResets > 2) {
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
        FT_WARNING(errmsg);
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
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    }
    
    // Read Status
    unsigned char statusmsg[22];
    
    // Attempt to read 21 characters.  
    ret = vrpn_read_available_characters(serial_fd, statusmsg, 21);
    
    if ( (ret != 21) ) {
            fprintf(stderr,
            "  Got %d of 21 characters for status\n",ret);
        FT_ERROR("Bad status report from Isotrack, retrying reset");
        return;
    }
    else if ( (statusmsg[0]!='2') ) {
        int i;
        statusmsg[22] = '\0';	// Null-terminate the string
        fprintf(stderr, "  Isotrack: bad status (");
        for (i = 0; i < ret; i++) {
            if (isprint(statusmsg[i])) {
                    fprintf(stderr,"%c",statusmsg[i]);
            } else {
                    fprintf(stderr,"[0x%02X]",statusmsg[i]);
            }
        }
        fprintf(stderr,")\n");
        FT_ERROR("Bad status report from Isotrack, retrying reset");
        return;
    } else {
        FT_WARNING("Isotrack gives correct status (this is good)");
        numResets = 0; 	// Success, use simple reset next time
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
    // Set filtering for both position (x command) and orientation (v command)
    // to the values that are recommended as a "jumping off point" in the
    // Isotrack manual.

    if (do_filter) {
        if (vrpn_write_characters(serial_fd, (const unsigned char *)"x0.2,0.2,0.8,0.8\015", 17) == 17) {
            vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
        } else {
            perror("  Isotrack write position filter failed");
            status = vrpn_TRACKER_FAIL;
            return;
        }
        if (vrpn_write_characters(serial_fd, (const unsigned char *)"v0.2,0.2,0.8,0.8\015", 17) == 17) {
            vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
        } else {
            perror("  Isotrack write orientation filter failed");
            status = vrpn_TRACKER_FAIL;
            return;
        }
    }
    
    // RESET Alignment reference frame
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "R1\r", 3) != 3) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            FT_WARNING("Isotrack reset ALIGNMENT reference frame (this is good)");
    }
    
    // reset BORESIGHT
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "b1\r", 3) != 3) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            FT_WARNING("Isotrack reset BORESIGHT (this is good)");
    }
    
    // Set data format to ASCII mode
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "F", 1) != 1) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            FT_WARNING("Isotrack set to ASCII mode (this is good)");
    }
    
    // Set data format to METRIC mode
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "u", 1) != 1) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            FT_WARNING("Isotrack set to metric units (this is good)");
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

        printf("  Isotrack writing extended reset commands...\n");

        // Make a copy of the additional reset string, since it is consumed
        strncpy(add_cmd_copy, add_reset_cmd, sizeof(add_cmd_copy));

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
    
    
    // Set tracker to continuous mode
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "C", 1) != 1) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            FT_WARNING("Isotrack set to continuous mode (this is good)");
    }

    FT_WARNING("Reset Completed.");

    status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading

    // Ok, device is ready, we want to calibrate to sensor 1 current position/orientation
    while(get_report() != 1);

    // CBO: I have commented out the following code, as it sets the alignment and boresight
    // in a way that does not make sense. 

    //// Set ALIGNMENT : current position as origin
    //char    outstring[68];
    //sprintf(outstring, "A1,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r", -pos[0], -pos[1], -pos[2], -pos[0] + 1.0, -pos[1], -pos[2], -pos[0], -pos[1] + 1.0, -pos[2]);
    //
    //if (vrpn_write_characters(serial_fd, (const unsigned char *)outstring, strlen(outstring)) == (int)strlen(outstring)) {
    //        vrpn_SleepMsecs(50);	// Sleep for a bit to let command run
    //} else {
    //        FT_ERROR("  Isotrack write failed on set ALIGNMENT command");
    //        status = vrpn_TRACKER_FAIL;
    //        return;
    //}
    //
    //// set BORESIGHT : current orientation as identity
    //if (vrpn_write_characters(serial_fd, (const unsigned char *) "B1\r", 3) != 3) {
    //        FT_ERROR("  Isotrack write failed on set BORESIGHT");
    //        status = vrpn_TRACKER_FAIL;
    //        return;
    //} 

    //FT_WARNING("Calibration Completed.");


    // Done with reset.
    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
    
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
// conditions). The report follows, 7 values in 7 characters each. The first three
// are position in X,Y and Z. The next four are the quaternion in the
// order W, X,Y,Z.
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize with the Isotrack by waiting
// until the start-of-report character ('0') comes around again.
// The routine that calls this one makes sure we get a full reading often
// enough (ie, it is responsible for doing the watchdog timing to make sure
// the tracker hasn't simply stopped sending characters).

int vrpn_Tracker_Isotrak::get_report(void)
{
    char errmsg[512];	// Error message to send to VRPN
    int ret;		// Return value from function call to be checked
    char *bufptr;	// Points into buffer at the current value to read
    
    //--------------------------------------------------------------------
    // Each report starts with an ASCII '0' character. If we're synching,
    // read a byte at a time until we find a '0' character.
    //--------------------------------------------------------------------
    // example report string :
    // 01   28.57 -10.30  13.98-0.9712-0.0091 0.1476 0.1857
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
            FT_WARNING(errmsg);
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
    // the first Isotrack station is '1' and the first VRPN sensor is 0.
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
            FT_WARNING(errmsg);
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
            FT_ERROR("Error reading report");
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
            
            sprintf(errmsg,"Not '0' in record, re-syncing '%s'", buffer);
            FT_WARNING(errmsg);

            //FT_WARNING("Not '0' in record, re-syncing");
            vrpn_flush_input_buffer(serial_fd);
            return 0;
    }
    
    // Sensor checking was handled when we received the character for it
    if ( (buffer[2] != ' ') && !isalpha(buffer[2]) ) {
            status = vrpn_TRACKER_SYNCING;
            FT_WARNING("Bad 3rd char in record, re-syncing");
            vrpn_flush_input_buffer(serial_fd);
            return 0;
    }
    
    if (buffer[bufcount-1] != ' ') {
            status = vrpn_TRACKER_SYNCING;
            FT_WARNING("No space character at end of report, re-syncing");
            vrpn_flush_input_buffer(serial_fd);
            return 0;
    }
    
    //--------------------------------------------------------------------
    // Decode the X,Y,Z of the position and the W,X,Y,Z of the quaternion
    // (keeping in mind that we store quaternions as X,Y,Z, W).
    //--------------------------------------------------------------------
    // The reports coming from the Isotrack should be in ASCII in this format :
    // "01   28.57 -10.30  13.98-0.9712-0.0091 0.1476 0.1857 "
    // which corresponds to 
    //           X      Y      Z     Qx     Qy     Qz     Qw
    // with 7 characters per number
    char valuestring[8];
    bufptr = (char *)&buffer;
    q_xyz_quat_type data;
    
    strncpy(valuestring, &bufptr[3],  7);  /* x */  data.xyz[0] = (float) atof(valuestring);
    strncpy(valuestring, &bufptr[10], 7);  /* y */  data.xyz[1] = (float) atof(valuestring);
    strncpy(valuestring, &bufptr[17], 7);  /* z */  data.xyz[2] = (float) atof(valuestring);
    
    strncpy(valuestring, &bufptr[24], 7);  /* W */  data.quat[3] = (float) atof(valuestring);
    strncpy(valuestring, &bufptr[31], 7);  /* X */  data.quat[0] = (float) atof(valuestring);
    strncpy(valuestring, &bufptr[38], 7);  /* Y */  data.quat[1] = (float) atof(valuestring);
    strncpy(valuestring, &bufptr[45], 7);  /* Z */  data.quat[2] = (float) atof(valuestring);
    
    // copy data to internal data structures : 
    //memcpy(pos, data.xyz, sizeof(pos));
    
    pos[0] = - data.xyz[0];
    pos[1] = - data.xyz[1];
    pos[2] = - data.xyz[2];
    
    memcpy(d_quat, data.quat, sizeof(d_quat));
    //q_invert(d_quat, data.quat);            // ! need to invert the quaternion (i don't know why)
    
    
    //--------------------------------------------------------------------
    // Done with the decoding, 
    // set the report to ready
    //--------------------------------------------------------------------
    status = vrpn_TRACKER_SYNCING;
    bufcount = 0;
    
    #ifdef VERBOSE2
        print_latest_report();
    #endif
    
    return 1;


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


