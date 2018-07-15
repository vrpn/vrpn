// vrpn_Tracker_Isotrak.C
//	This file contains the code to operate a Polhemus Isotrack Tracker.
// This file is based on the vrpn_Tracker_Fastrack.C file, with modifications made
// to allow it to operate a Isotrack instead. The modifications are based
// on the old version of the Isotrack driver.
//	This version was written in the Spring 2006 by Bruno Herbelin.
//
// Revised by Charles B. Owen (cbowen@cse.msu.edu) 1-20-2012
// Revisions:
//
// The version before was reporting in centimeters. Revised to report in Meters per VRPN requirements.
// The version before negated the coordinates. I have no idea why. That has been fixed.
// Now reads the binary format instead of ASCII. It's faster and supports the stylus button.
// Now supports a stylus button for either channel.
	

#include <ctype.h>                      // for isprint
#include <stdio.h>                      // for fprintf, perror, sprintf, etc
#include <stdlib.h>                     // for atoi
#include <string.h>                     // for strlen, strtok

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_WARNING, etc
#include "vrpn_Button.h"                // for vrpn_Button_Server
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc
#include "vrpn_Tracker_Isotrak.h"
#include "vrpn_Types.h"                 // for vrpn_uint8, vrpn_float64, etc
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

const int BINARY_RECORD_SIZE = 20;

vrpn_Tracker_Isotrak::vrpn_Tracker_Isotrak(const char *name, vrpn_Connection *c, 
                    const char *port, long baud, int enable_filtering, int numstations,
                    const char *additional_reset_commands) :
    vrpn_Tracker_Serial(name,c,port,baud),
    do_filter(enable_filtering),
    num_stations(numstations>vrpn_ISOTRAK_MAX_STATIONS ? vrpn_ISOTRAK_MAX_STATIONS : numstations),
    num_resets(0)
{
        reset_time.tv_sec = reset_time.tv_usec = 0;
        if (additional_reset_commands == NULL) {
		add_reset_cmd[0] = '\0';
        } else {
                vrpn_strcpy(add_reset_cmd, additional_reset_commands);
        }

    // Initially, set to no buttons
    for(int i=0; i<vrpn_ISOTRAK_MAX_STATIONS;  i++) {
        stylus_buttons[i] = NULL;
    }
}

vrpn_Tracker_Isotrak::~vrpn_Tracker_Isotrak()
{

}


/** This routine sets the device for position + quaternion
    It puts a space at the end so that we can check to make
    sure we have complete good records for each report.

    Returns 0 on success and -1 on failure.
*/

int vrpn_Tracker_Isotrak::set_sensor_output_format(int /*sensor*/)
{
    char    outstring[16];

    // Set output format for the station to be position, quaternion
    // Don't need the space anymore, though
    sprintf(outstring, "O2,11\r");

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

// This routine will reset the tracker and set it to generate the types
// of reports we want.
// This was based on the Isotrak User Manual from Polhemus (2001 Edition, Rev A)

void vrpn_Tracker_Isotrak::reset()
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
    // After a few tries with this, use a [return] character, and then use the ^Y to reset. 
    
    resetLen = 0;
    num_resets++;		  	
    
    // We're trying another reset
    if (num_resets > 1) {	        // Try to get it out of a query loop if its in one
            reset[resetLen++] = (unsigned char) (13); // Return key -> get ready
    }
    
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
                    perror("Isotrack: Failed writing to tracker");
                    status = vrpn_TRACKER_FAIL;
                    return;
            }
    }
    //XXX Take out the sleep and make it keep spinning quickly
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
        VRPN_MSG_ERROR("Bad status report from Isotrack, retrying reset");
        return;
    }
    else if ( (statusmsg[0]!='2') ) {
        int i;
        statusmsg[sizeof(statusmsg) - 1] = '\0';	// Null-terminate the string
        fprintf(stderr, "  Isotrack: bad status (");
        for (i = 0; i < ret; i++) {
            if (isprint(statusmsg[i])) {
                    fprintf(stderr,"%c",statusmsg[i]);
            } else {
                    fprintf(stderr,"[0x%02X]",statusmsg[i]);
            }
        }
        fprintf(stderr,")\n");
        VRPN_MSG_ERROR("Bad status report from Isotrack, retrying reset");
        return;
    } else {
        VRPN_MSG_WARNING("Isotrack gives correct status (this is good)");
        num_resets = 0; 	// Success, use simple reset next time
    }
    
    //--------------------------------------------------------------------
    // Now that the tracker has given a valid status report, set all of
    // the parameters the way we want them. We rely on power-up setting
    // based on the receiver select switches to turn on the receivers that
    // the user wants.
    //--------------------------------------------------------------------
    
    // Set output format. This is done once for the Isotrak, not per channel.
    if (set_sensor_output_format(0)) {
        return;
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
            VRPN_MSG_WARNING("Isotrack reset ALIGNMENT reference frame (this is good)");
    }
    
    // reset BORESIGHT
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "b1\r", 3) != 3) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            VRPN_MSG_WARNING("Isotrack reset BORESIGHT (this is good)");
    }
    
    // Set data format to METRIC mode
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "u", 1) != 1) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            VRPN_MSG_WARNING("Isotrack set to metric units (this is good)");
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
    // F = ASCII, f = binary
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "f", 1) != 1) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            VRPN_MSG_WARNING("Isotrack set to BINARY mode (this is good)");
    }
    
    
    // Set tracker to continuous mode
    if (vrpn_write_characters(serial_fd, (const unsigned char *) "C", 1) != 1) {
            perror("  Isotrack write failed");
            status = vrpn_TRACKER_FAIL;
            return;
    } else {
            VRPN_MSG_WARNING("Isotrack set to continuous mode (this is good)");
    }

    VRPN_MSG_WARNING("Reset Completed.");

    status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading

    // Ok, device is ready, we want to calibrate to sensor 1 current position/orientation
    while(get_report() != 1);

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
    
    // The first byte of a binary record has the high order bit set
    
    if (status == vrpn_TRACKER_SYNCING) {
        // Try to get a character.  If none, just return.
        if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) {
            return 0;
        }

        // The first byte of a record has the high order bit set
        if(!(buffer[0] & 0x80)) {
            sprintf(errmsg,"While syncing (looking for byte with high order bit set, "
                    "got '%x')", buffer[0]);
            VRPN_MSG_WARNING(errmsg);
            vrpn_flush_input_buffer(serial_fd);
    
            return 0;
        }
    
        // Got the first byte of a report -- go into TRACKER_PARTIAL mode
        // and record that we got one character at this time. 
        bufcount = 1;
        vrpn_gettimeofday(&timestamp, NULL);
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
                    BINARY_RECORD_SIZE - bufcount);
    if (ret == -1) {
            VRPN_MSG_ERROR("Error reading report");
            status = vrpn_TRACKER_FAIL;
            return 0;
    }
    
    bufcount += ret;
    
    if (bufcount < BINARY_RECORD_SIZE) {	// Not done -- go back for more
            return 0;
    }
    
    // We now have enough characters for a full report
    // Check it to ensure we do not have a high bit set other
    // than on the first byte
    for(int i=1;  i<BINARY_RECORD_SIZE;  i++)
    {
        if (buffer[i] & 0x80) {
            status = vrpn_TRACKER_SYNCING;
        
            sprintf(errmsg,"Unexpected sync character in record");
            VRPN_MSG_WARNING(errmsg);

            //VRPN_MSG_WARNING("Not '0' in record, re-syncing");
            vrpn_flush_input_buffer(serial_fd);
            return 0;
        }
    }
    
    // Create a buffer for the decoded message
    unsigned char decoded[BINARY_RECORD_SIZE];
    int d = 0;

    int fullgroups = BINARY_RECORD_SIZE / 8;

    // The following decodes the Isotrak binary format. It consists of
    // 7 byte values plus an extra byte of the high bit for these 
    // 7 bytes. First, loop over the 7 byte ranges (8 bytes in binary)
    int i;
    for(i = 0;  i<fullgroups;  i++)
    {
        vrpn_uint8 *group = &buffer[i * 8];
        vrpn_uint8 high = buffer[i * 8 + 7];

        for(int j=0;  j<7;  j++)
        {
            decoded[d] = *group++;
            if(high & 1) 
                decoded[d] |= 0x80;

            d++;
            high >>= 1;
        }
    }

    // We'll have X bytes left at the end
    int left = BINARY_RECORD_SIZE - fullgroups * 8;
    vrpn_uint8 *group = &buffer[fullgroups * 8];
    vrpn_uint8 high = buffer[fullgroups * 8 + left - 1];

    for(int j=0;  j<left-1;  j++)
    {
        decoded[d] = *group++;
        if(high & 1) 
            decoded[d] |= 0x80;

        d++;
        high >>= 1;
    }

    // ASCII value of 1 == 49 subtracing 49 gives the sensor number
    d_sensor = decoded[1] - 49;	// Convert ASCII 1 to sensor 0 and so on.
    if ( (d_sensor < 0) || (d_sensor >= num_stations) ) {
        status = vrpn_TRACKER_SYNCING;
        sprintf(errmsg,"Bad sensor # (%d) in record, re-syncing", d_sensor);
        VRPN_MSG_WARNING(errmsg);
        vrpn_flush_input_buffer(serial_fd);
        return 0;
    }



    // Extract the important information
    vrpn_uint8 *item = &decoded[3];

    // This is a scale factor from the Isotrak manual
    // This will convert the values to meters, the standard vrpn format
    double mul = 1.6632 / 32767.; 
    float div = 1.f / 32767.f;  // Fractional amount for angles

    pos[0] = ( (vrpn_int8(item[1]) << 8) + item[0]) * mul;   item += 2;
    pos[1] = ( (vrpn_int8(item[1]) << 8) + item[0]) * mul;   item += 2;
    pos[2] = ( (vrpn_int8(item[1]) << 8) + item[0]) * mul;   item += 2;
    d_quat[3] = ( (vrpn_int8(item[1]) << 8) + item[0]) * div;   item += 2;
    d_quat[0] = ( (vrpn_int8(item[1]) << 8) + item[0]) * div;   item += 2;
    d_quat[1] = ( (vrpn_int8(item[1]) << 8) + item[0]) * div;   item += 2;
    d_quat[2] = ( (vrpn_int8(item[1]) << 8) + item[0]) * div;

    //--------------------------------------------------------------------
    // If this sensor has button on it, decode the button values
    // into the button device and mainloop the button device so that
    // it will report any changes.
    //--------------------------------------------------------------------

    if(stylus_buttons[d_sensor] != NULL)
    {
        char button = decoded[2];
        if(button == '@' || button == '*')
        {
            stylus_buttons[d_sensor]->set_button(0, button == '@');

        }

	    stylus_buttons[d_sensor]->mainloop();
    }
    
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
}


// this routine is called when an "Stylus" button is encountered
// by the tracker init string parser it sets up the VRPN button
// device
int vrpn_Tracker_Isotrak::add_stylus_button(const char *button_device_name, int sensor)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= num_stations) ) {
	return -1;
    }

    // Add a new button device and set the pointer to point at it.
    try { stylus_buttons[sensor] = new vrpn_Button_Server(button_device_name, d_connection, 1); }
    catch (...) {
	    VRPN_MSG_ERROR("Cannot open button device");
	    return -1;
    }

    return 0;
}

