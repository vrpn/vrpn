/**
 * Button driver for Virtual Presence Joystick
 *
 * This joystick is a device built by Virtual Presence Ltd 
 * (http://www.vrweb.com ) consisting of an 8-button joystick handle
 * containing an Ascension Flock-of-Birds sensor.
 *
 * This driver reads the button states as transmitted via the serial line.
 *
 * The buttons map as follows:
 *
 * 	0	Bottom
 * 	1	Middle
 * 	2	Top
 * 	3	Trigger
 * 	4	Hat Up
 * 	5	Hat Right
 * 	6	Hat Down
 * 	7	Hat Left
 *
 * Use the Flock driver for position data.
 *
 * Written September 2003 by Matt Harvey <m.j.harvey@ucl.ac.uk>
 * Derived from vrpn_ADBox.C
*/

#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday
#include "vrpn_VPJoystick.h"

class VRPN_API vrpn_Connection;

#define STATE_SYNCHING  (0)
#define STATE_READING   (1)
#define STATE_RECEIVED  (2)

#define SYNC_BYTE          (0xff)

#define VP_BUTTON_1  4096
#define VP_BUTTON_2  256
#define VP_BUTTON_3  1
#define VP_BUTTON_4  512
#define VP_BUTTON_5  2048
#define VP_BUTTON_6  2
#define VP_BUTTON_7  1024
#define VP_BUTTON_8  4

#define VP_HAT_UP    VP_BUTTON_5
#define VP_HAT_DOWN  VP_BUTTON_7
#define VP_HAT_LEFT  VP_BUTTON_8
#define VP_HAT_RIGHT VP_BUTTON_6

#define VP_TRIGGER         VP_BUTTON_4
#define VP_BUTTON_TOP      VP_BUTTON_3
#define VP_BUTTON_MIDDLE   VP_BUTTON_2
#define VP_BUTTON_BOTTOM   VP_BUTTON_1


#define VP_HAT_ALL    ( VP_HAT_UP | VP_HAT_DOWN | VP_HAT_LEFT | VP_HAT_RIGHT )
#define VP_BUTTON_ALL ( VP_TRIGGER | VP_BUTTON_TOP | VP_BUTTON_MIDDLE | VP_BUTTON_BOTTOM )

#include <stdio.h>                      // for fprintf, stderr

vrpn_VPJoystick::vrpn_VPJoystick(char* name, vrpn_Connection *c,
                       const char *port, long baud)
  :  vrpn_Button_Filter(name, c),
     serial_fd(0), state( STATE_SYNCHING )
{
  int i;
	
  // Open the serial port
  if ( (serial_fd=vrpn_open_commport(port, baud)) == -1) {
    fprintf(stderr,"vrpn_VPJoystick: Cannot Open serial port\n");
  }
  
  // find out what time it is - needed?
  vrpn_gettimeofday(&timestamp, 0);
	vrpn_Button::timestamp = timestamp;

	num_buttons = vrpn_VPJOY_NUM_BUTTONS;

	button_masks[0] = VP_BUTTON_1;
	button_masks[1] = VP_BUTTON_2;
	button_masks[2] = VP_BUTTON_3;
	button_masks[3] = VP_BUTTON_4;
	button_masks[4] = VP_BUTTON_5;
	button_masks[5] = VP_BUTTON_6;
	button_masks[6] = VP_BUTTON_7;
	button_masks[7] = VP_BUTTON_8;
	
	for( i=0; i< num_buttons; i++ ) {
		buttons[i] = lastbuttons[i] = VRPN_BUTTON_OFF;			  
	}
}


vrpn_VPJoystick::~vrpn_VPJoystick()
{
  vrpn_close_commport(serial_fd);
}

void vrpn_VPJoystick::mainloop() 
{
	server_mainloop();

        // XXX Why not have a timeout of zero?  This would cause faster
        // update rates for other devices in the same server.
	struct timeval timeout = { 0,200000 };

        if (serial_fd == -1) {
          fprintf(stderr,"vrpn_VPJOystick::mainloop(): Bad serial port descriptor\n");
          return;
        }

	if( state == STATE_SYNCHING ) {

		// Read bytes from the incoming stream until
		// the synch byte is received
			  			  
		message_buffer[0] = 0;
		if( vrpn_read_available_characters( serial_fd, message_buffer, 1, &timeout ) ) {
			if( message_buffer[0] == SYNC_BYTE ) {
				state = STATE_READING;						  		
				bytes_read = 1;
  			}						  
		}

	}
        
        // The state may just have been set to this, so don't do this in a switch
        // or if-then-else statement because we may take another whole loop iteration
        // before checking it again.
        if( state == STATE_READING ) {
			  
		// Read the remaining bytes of the packet
		//
			  	  
		bytes_read += vrpn_read_available_characters( serial_fd, message_buffer + bytes_read, vrpn_VPJOY_MESSAGE_LENGTH - bytes_read, &timeout );
		if( bytes_read == vrpn_VPJOY_MESSAGE_LENGTH ) {
			state = STATE_RECEIVED;					  	
		}

	}
        
        // The state may just have been set to this, so don't do this in a switch
        // or if-then-else statement because we may take another whole loop iteration
        // before checking it again.
        if( state == STATE_RECEIVED ) {

		// decode a received packet
			  			  
		int i;
		int flag = ((int) message_buffer[1])*256 + message_buffer[2];
   	        flag = (~flag) & ( VP_BUTTON_ALL | VP_HAT_ALL );

		for( i = 0 ; i < num_buttons; i++ ) {
			buttons[ i ] =  static_cast<unsigned char>( ( ( flag & button_masks[i] ) == button_masks[i] ) ? VRPN_BUTTON_ON : VRPN_BUTTON_OFF );
		}
		
		vrpn_Button::report_changes();
		state = STATE_SYNCHING;			  
	}
}
