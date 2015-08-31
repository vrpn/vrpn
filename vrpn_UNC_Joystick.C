#include <stdio.h>                      // for fprintf, stderr, perror, etc
#include <string.h>                     // for strlen

#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_int32
#include "vrpn_UNC_Joystick.h"

// This class runs the UNC custom serial joystick.  It includes two
// buttons, a slider, and two 3-axis joysticks.  It is based on a
// single-board computer.  This driver is based on the px_sjoy.c
// code.

static const vrpn_float64 JoyScale[] = {1019, 200, 200, 350, 200, 200, 350};

vrpn_Joystick::vrpn_Joystick(char * name, 
		    vrpn_Connection * c, char * portname,int baud, 
			     vrpn_float64 update_rate):
      vrpn_Serial_Analog(name, c, portname, baud),
	  vrpn_Button_Filter(name, c)
{ 
  num_buttons = 2;  // Has 2 buttons
  num_channel = 7;	// Has a slider and two 3-axis joysticks
  if (update_rate != 0) {
    MAX_TIME_INTERVAL = (unsigned long)(1000000/update_rate);
  } else {
    MAX_TIME_INTERVAL = (unsigned long)(-1);
  }
  status = vrpn_ANALOG_RESETTING;
}

void vrpn_Joystick::report(struct timeval current_time)
{
    // Store the current time in the structures
    vrpn_Analog::timestamp.tv_sec = current_time.tv_sec;
    vrpn_Analog::timestamp.tv_usec = current_time.tv_usec;
    vrpn_Button::timestamp.tv_sec = current_time.tv_sec;
    vrpn_Button::timestamp.tv_usec = current_time.tv_usec;

    vrpn_Button::report_changes(); // report any button event;

    // Send the message on the connection;
    if (vrpn_Analog::d_connection) {
      char	msgbuf[1000];
      vrpn_int32	len = vrpn_Analog::encode_to(msgbuf);
      if (vrpn_Analog::d_connection->pack_message(len, vrpn_Analog::timestamp,
				   channel_m_id, vrpn_Analog::d_sender_id, msgbuf,
				   vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"UNC Joystick: Cannot pack message, tossing\n");
	  }
	} else {
	  fprintf(stderr,"UNC Joystick: No valid Connection\n");
	}
}

void vrpn_Joystick::mainloop(void) {
  struct timeval current_time;
  vrpn_gettimeofday(&current_time, NULL);

  // Since we are a server, call the generic server mainloop()
  server_mainloop();
    
  switch (status) {

  case vrpn_ANALOG_SYNCING:
  case vrpn_ANALOG_PARTIAL:
    {	
	// As long as we keep getting reports, send them.
        // XXX Ideally, this time would be the time of the first
        // character read from the serial port for this report.
	while (get_report()) {
	  vrpn_gettimeofday(&current_time, NULL);
	  report(current_time);
	}

	// If it has been longer than the requested report interval
	// since we sent a report, send a complete report
	// anyway (a repeat of the previous one).
	if ( (static_cast<unsigned long>(vrpn_TimevalDuration(current_time, vrpn_Analog::timestamp)) 
	     > MAX_TIME_INTERVAL) && (MAX_TIME_INTERVAL != (unsigned long)(-1)) ) {

	  // send out the last report again;
	  report(current_time);
	}
    }
    break;

  case vrpn_ANALOG_RESETTING:
  case vrpn_ANALOG_FAIL:
    reset();
    break;
  }
}

/****************************************************************************/
/* Send request for status to joysticks and reads response.
 * Resets all of the rest values for the analog channels.
 */
void vrpn_Joystick::reset() {
  char request[256];
  int write_rt, bytesread;

    fprintf(stderr, "Going into vrpn_Joystick::reset()\n");

    /* Request baseline report for comparison */
    request[0] = 'r';
    request[1] = 0;
    write_rt = vrpn_write_characters(serial_fd, (unsigned char*)request, strlen(request));
    if (write_rt < 0) {
      fprintf(stderr, "vrpn_Joystick::reset: write failed\n");
      status = vrpn_ANALOG_FAIL;
      return;
    }
    vrpn_SleepMsecs(1000.0*1);

	// Read the report and parse each entry.  For each one, have
	// the parse routine store the restvalue, so that it will be
	// subtracted from subsequent readings.
    bytesread= vrpn_read_available_characters(serial_fd, serialbuf, 16);
    if (bytesread != 16) {
      fprintf(stderr, "vrpn_Joystick::reset: got only %d char from \
joystick, should be 16, trying again\n", bytesread);
      status = vrpn_ANALOG_FAIL;
    } else {  
	  // Parse the analog reports in the serialbuf and reset each
	  // channel's rest value.  The button report is not
	  // parsed.
      for (int i=0; i< num_channel; i++) { 
		  parse(i*2, 1);
      }
      status = vrpn_ANALOG_SYNCING;

      /* Request only send each channel when state has changed */
      request[0] = 'j';
      vrpn_write_characters(serial_fd, (unsigned char*)request, strlen(request));
    }
}

// SYNCING means that we are looking for a character that has
// the high bit clear (the first character of a report has this,
// while the second has this bit set).
// PARTIAL means that we have gotten the first character and are
// looking for the second character (which will have the high bit
// set).
int vrpn_Joystick::get_report() {
  int bytesread = 0;
  
  if (status == vrpn_ANALOG_SYNCING) {
    bytesread =vrpn_read_available_characters(serial_fd, serialbuf, 1);
	if (bytesread == -1) {
		perror("vrpn_Joystick::get_report() 1st read failed");
		return 0;
	}
    if (bytesread == 1) {
	if ((serialbuf[0] >> 7) == 0) {
	    status = vrpn_ANALOG_PARTIAL;
	} else {
	    fprintf(stderr,"vrpn_Joystick::get_report(): Bad 1st byte (re-syncing)\n");
		return 0;
	}
    }
  }

  // Since we are not syncing, we must be in partial mode and
  // looking for the second character.
  bytesread = vrpn_read_available_characters(serial_fd, serialbuf+1, 1);
  if (bytesread == -1) {
	perror("vrpn_Joystick::get_report() 2nd read failed");
	return 0;
  }
  if (bytesread  == 0) {
	return 0;
  }
  // If the high bit is not set, then what we have here is a
  // first character, not a second.  Go ahead and put it into
  // the first byte and continue on in partial mode.  Scary but
  // true: this seems to get all of the reports accurately,
  // when tossing it out loses reports.  Looks like there are
  // sometimes duplicate first characters in the report ?!?!?
  if ( (serialbuf[1] >> 7) == 0 ) {	// Should have high bit set
	  serialbuf[0] = serialbuf[1];
	  status = vrpn_ANALOG_PARTIAL;
	  return 0;
  }

  parse(0);	// Parse starting at the beginning of the serialbuf.
  status = vrpn_ANALOG_SYNCING;
  return 1;	// Indicates that we have gotten a report
}

/****************************************************************************/
/* Decodes bytes as follows:
     First byte of set received (from high order bit down):
        -bit == 0 to signify first byte
        -empty bit
        -3 bit channel label
        -3 bits (out of 10) of channel reading (high-order bits)
     Second byte of set received (from high order bit down):
        -bit == 1 to signify second byte
        -7 bits (out of 10) of channel reading (low-order bits)
*/

void vrpn_Joystick::parse (int index, int reset_rest_pos)
{
   unsigned int temp;
   static const unsigned int mask1 = 7, mask2 = 127, left = 1, right = 2;
   
   int chan;	// Channel number extracted from report
   int value;	// Integer value extracted from report
   
   // Isolate the channel part of the first byte by shifting it
   // to the lowest bits and then masking off the rest.
   chan = serialbuf[index] >> 3;
   chan = chan & mask1;

   // If channel number is > 7, this is illegal.
   if (chan > 7) {
	   fprintf(stderr,"vrpn_Joystick::parse(): Invalid channel (%d)\n",chan);
	   status = vrpn_ANALOG_FAIL;
	   return;
   }
   
   // Channel 7 is the button report.
   if (chan == 7) {
     /******************************************************************
     *  The least most significant bit should be the left button and the next
     *  should be the right button.  In the original reading the two bits are
     *  opposite this.  We swap the two least significant bits. */;
     
     buttons[0] = static_cast<unsigned char>(!(serialbuf[index+1] & left)?1:0);
     buttons[1] = static_cast<unsigned char>(!(serialbuf[index+1] & right)?1:0);
#ifdef VERBOSE
     printf("Joystick::parse: Buttons: %d %d\n",
		 buttons[1], buttons[0]);
#endif
     return;
   }

   // The other channels are the analog channels [0..6]
   // Strip off the high-order 3 bits from the first byte and
   // shift them up to their proper location.
   // Then strip off check bit from the second byte and insert
   // the value bits to form the full value.
   temp = serialbuf[index] & mask1;  /* isolate value bits */
   temp = temp << 7;              /* position value bits */
   value = (serialbuf[index+1] & mask2) | temp;

   // If we're resetting, set the rest positions to this value.
   // Don't do this for the slider (channel 0), since it is not
   // auto-centering.
   if (reset_rest_pos) {
	   if (chan == 0) {
		   restval[chan] = 512;
	   } else {
		   restval[chan]= value;
	   }
   }

   // Subtract the current value from the rest value to form the
   // relative location, then scale to get into the range
   // [-0.5 --- 0.5].  Clamp to make sure we are in this range.
   channel[chan] = (restval[chan] - value)/JoyScale[chan];
   if (channel[chan] > 0.5) { channel[chan] = 0.5; }
   if (channel[chan] < -0.5) { channel[chan] = -0.5; }

   // The slider should be inverted, to match the conventional
   // reading.
   if (chan == 0) {channel[chan] = -channel[chan];}

#ifdef VERBOSE
   printf("Joystick::parse: channel[%d] = %f\n", chan, channel[chan]);
#endif
}
