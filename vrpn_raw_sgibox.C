/*				vrpn_raw_sgibox.C
 *    This file contains the implementation for a controller to
 * the SGI dial and button boxes.  This is an implementation that
 * goes directly through the serial port rather than using the
 * SGI GL library.  The two connect together and then to a serial
 * port at 9600 baud, 8 bits, 1 start 1 stop, no parity.  The single-
 * byte command 0x20 resets both of them; they respond with 0x20.
 *    The dial box returns 3-byte values whenever a dial is moved.
 * There are 200 ticks per rotation, and there is an internal counter
 * that counts up to 16 bits and then rolls over.  The message format
 * that is returned is 3Z XX YY, where Z is the dial number and XXYY
 * is the new positional value for the dial.  The dials are clamped to
 * the range -0.5<-->0.5, and this is twice VRPN_DIAL_RANGE.  If the
 * dials are turned past this, the values are set to clamp.  As soon
 * as the dial is turned the other direction, it will lower the value;
 * this provides a "saturating" dial effect.
 *    The button box takes commands to turn the lights on or off; the
 * command is 0x75 followed by 4 bytes; the bits of byte 1 turn on or
 * off the first 8 lights, with MSB turning on light 7 and LSB controlling
 * byte 0.  Lights 8-15 are byte 2, 16-23 byte 3, 24-31 byte 4.  When a
 * button is pressed or released, a single-byte message is returned.
 * For press, D8 is button 0, D9 is 1... DF is 7; D0 is 8...D7 is 15;
 * C8 is 16...CF is 23, C0 is 24...C7 is 31.  For release, F8 is 0...
 * FF is 7, F0 is 8...F7 is 15, E8 is 16...EF is 23, E0 is 24...E7 is 31.
 *    The button box needs to have its buttons enabled and activated
 * using the 0x73 and 0x71 commands, each followed by FFFFFFFF to turn
 * on them all.  The dial box enable command is 0x50 followed by xxFF,
 * where xx is ignored and FF turns on all of the dials.
 */

//#define	VERBOSE

#include <stdio.h>                      // for perror, fprintf, printf, etc
#include <string.h>                     // for memcpy

#include "vrpn_Connection.h"            // for vrpn_HANDLERPARAM, etc
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_int16, etc
#include "vrpn_raw_sgibox.h"

static	const unsigned char	BBOX_RESET = 0x20;
static	const int		VRPN_DIAL_RANGE = 200;

static int VRPN_CALLBACK sgibox_raw_con_cb(void * userdata, vrpn_HANDLERPARAM p);
static int VRPN_CALLBACK sgibox_raw_alert_handler(void * userdata, vrpn_HANDLERPARAM);

vrpn_raw_SGIBox::vrpn_raw_SGIBox(char * name, vrpn_Connection * c,
				 char *serialPortName):
  vrpn_Analog(name, c), vrpn_Dial(name, c), vrpn_Button_Filter(name, c)
{
    char message[1024];
    serialfd = -1;
    
    // Open the serial port that will be used to communicate to
    // the dial and button box.  Then reset the boxes.
    serialfd = vrpn_open_commport(serialPortName, 9600);
    if (serialfd < 0) {
      sprintf(message,"vrpn_raw_SGIBox: error opening serial port: %s\n",serialPortName);
      perror(message);
      return;
    }
    reset();

    num_channel = vrpn_SGI_NUM_DIALS;
    num_dials = vrpn_SGI_NUM_DIALS;
    num_buttons = vrpn_SGI_NUM_BUTTONS;

    // We can use either autodeleted handler; choose the one in Analog
    register_autodeleted_handler(c->register_message_type(vrpn_got_first_connection), sgibox_raw_con_cb, this);
    register_autodeleted_handler(alert_message_id,sgibox_raw_alert_handler, this);

    set_alerts(1);	//turn on alerts from toggle filter class to notify
			//local sgibox that lights should be turned on/off
}

int vrpn_raw_SGIBox::reset() {  /* Button/Dial box setup */
  int i;
  int ret;
  unsigned char inbuf[100];
#ifdef	VERBOSE
  unsigned char lightson[5] = {0x75, 0xff, 0xff, 0xff, 0xff};
  unsigned char lightsoff[5] = {0x75, 0x00, 0x00, 0x00, 0x00};
#endif
  unsigned char activatebuttons[5] = {0x73, 0xff, 0xff, 0xff, 0xff};
  unsigned char enablebuttons[5] = {0x71, 0xff, 0xff, 0xff, 0xff};
  unsigned char enabledials[3] = {0x50, 0xff, 0xff};

  
  // Clear the incoming serial buffer of all characters
  // Send the reset message to the dials and buttons. Wait for any return
  // messages, which should be 1 or 2 "0x20" bytes.

  if (serialfd != -1) {	// Write reset command
	  if (vrpn_flush_input_buffer(serialfd)  == -1) {
		perror("vrpn_raw_SGIBox::reset(): Can't flush incoming buffer");
		return -1;
	  }
	  if (vrpn_write_slowly(serialfd, &BBOX_RESET,1,1) != 1) {
		perror("vrpn_raw_SGIBox::reset(): Can't write reset command");
		return -1;
	  }
	  if (vrpn_write_slowly(serialfd, &BBOX_RESET,1,1) != 1) {
		perror("vrpn_raw_SGIBox::reset(): Can't write reset command");
		return -1;
	  }
  }
  vrpn_SleepMsecs(1000.0*1);	// Give the box time to respond
  if ( (ret=vrpn_read_available_characters(serialfd, inbuf, 2)) <= 0) {
	  //XXX Turn this into a vrpn text message
	  perror("vrpn_raw_SGIBox::reset(): Can't read or no data from serial port");
	  return -1;
  }
#ifdef	VERBOSE
  printf("vrpn_raw_SGIBox::reset(): Box's response to reset command: %02x\n", inbuf[0]);
#endif

  for (i = 0; i < ret; i++) {
	  if (inbuf[i] != BBOX_RESET) {
		  //XXX Turn this into a vrpn text message
		  fprintf(stderr,"vrpn_raw_SGIBox::reset(): Bad response to reset command : %02x- please restart sgiBox vrpn server\n",inbuf[i]);
		  return -1;
	  }
  }

#ifdef	VERBOSE
  if (serialfd != -1) {
	printf("vrpn_raw_SGIBox: flashing the lights on then off...\n");
	if (vrpn_write_slowly(serialfd, lightson,5,1) != 5) {
		perror("vrpn_raw_SGIBox::reset(): Can't turn the lights on");
		return -1;
	}
	vrpn_SleepMsecs(1000.0*5);
	if (vrpn_write_slowly(serialfd, lightsoff,5,1) != 5) {
		perror("vrpn_raw_SGIBox::reset(): Can't turn the lights off");
		return -1;
	}
  }
#endif
  
  // Active and enable all of the buttons, enable the dials
  
  if (serialfd != -1) {
    // for some reason, enabling the dials disables the buttons
    // so we have to enable the dials first
  	  if (vrpn_write_slowly(serialfd, enabledials,3,1) != 3) {
		perror("vrpn_raw_SGIBox::reset(): Can't enable dials");
		return -1;
	  }
#ifdef VERBOSE
	  else {
	    printf("vrpn_raw_SGIBOX::reset() : Enabled Dials\n");
	  }
#endif
	  // for some reasn the box doesn't always understand the enable buttons
	  // command the first time. So we send it twice to make sure
	  for (i=0; i < 2; i++) {

	    if (vrpn_write_slowly(serialfd, enablebuttons,5,1) != 5) {
	      perror("vrpn_raw_SGIBox::reset(): Can't enable buttons");
	      return -1;
	    }
#ifdef VERBOSE
	    else {
	      printf("vrpn_raw_SGIBOX::reset() : Enabled Buttons\n");
	    }
#endif 
	    if (vrpn_write_slowly(serialfd, activatebuttons,5,1) != 5) {
	      perror("vrpn_raw_SGIBox::reset(): Can't activate buttons\n");
	      return -1;
	    }
#ifdef VERBOSE
	    else {
	      printf("vrpn_raw_SGIBOX::reset() : Activated Buttons\n");
	    }
#endif
	  } // end of loop to send enable and activate commands

  }
  
  // Reset the button and, analog, and dial values to zero, since the dial box
  // and button box are now reset.
  
  for (i=0; i<vrpn_SGI_NUM_BUTTONS; i++) {
	buttons[i] = lastbuttons[i] = 0;	// The buttons are released
	//XXX Reset the button-light handling registers
  }

  for (i=0; i<vrpn_SGI_NUM_DIALS; i++) {
	mid_values[i] = 0;		// The middle of saturating analog range
	last[i] = channel[i] = 0;	// The analog values are reset to 0
	dials[i] = 0;			// Reset the dials to zero
	last_values[i] = 0;		// Reset the values used by dial code to zero
  }

  // Set the lights to how they should be
  send_light_command();

  return 0;
}

// This checks one bank of eight buttons to see if the command refers
// to a press event in that bank.  If it does, it will set the button.
// If not, it does nothing.
void vrpn_raw_SGIBox::check_press_bank(int base_button, unsigned char base_command,
				       unsigned char command) {
	if ( (command >= base_command) && (command < (base_command+8)) ) {
		buttons[base_button + (command-base_command)] = 1;
	}
}

// This checks one bank of eight buttons to see if the command refers
// to a release event in that bank.  If it does, it will clear the button.
// If not, it does nothing.
void vrpn_raw_SGIBox::check_release_bank(int base_button, unsigned char base_command,
					 unsigned char command) {
	if ( (command >= base_command) && (command < (base_command+8)) ) {
		buttons[base_button + (command-base_command)] = 0;
	}
}


// See what reports have come in over the serial port.  When something
// comes in, figure out if it is a dial report or a button report
// and act on it accordingly.  If we get a partial report, go ahead
// and wait for the rest of the report before leaving the loop.
// XXX This should be modified to handle partial reports the same way
// that the trackers do.

void vrpn_raw_SGIBox::get_report() {
	unsigned char	command;
	int	ret;

#ifdef	VERBOSE
	printf("."); fflush(stdout);
#endif
	// Read a character if there is one.  See if it matches one of
	// the known command start bytes.  If it does not, there is
	// something wrong.
	ret = vrpn_read_available_characters(serialfd, &command, 1);
	if (ret == 0) {	// Nothing there, we're done
		return;
	}
	if (ret == -1) { // Error in the read; try resetting.
#ifdef VERBOSE
	    perror("vrpn_raw_SGIBOX::get_report(): error reading serial port - reseting...");
#endif
	    reset();
	    return;
	}

#ifdef	VERBOSE
	printf("vrpn_raw_SGIBox::get_report(): Got %02x\n", command);
#endif
	// If this is a reset command, we can skip it and get the next command
	// next time.
	if (command == 0x20) {
	  perror("vrpn_raw_SGIBOX::get_report(): Got reset response when we didn't expect it - reseting...\n");
	  reset();
	  return;
	}
	
	// See if this is a button report, which are only a single byte.
	if ( command >= 0xC0 ) {
		// Due to the strange layout of the commands to buttons,
		// we need to check each group of 8 button messages in chunks,
		// both for the press commands and the release commands.
		check_press_bank(24, 0xC0, command);
		check_press_bank(16, 0xC8, command);
		check_press_bank(8, 0xD0, command);
		check_press_bank(0, 0xD8, command);

		check_release_bank(24, 0xE0, command);
		check_release_bank(16, 0xE8, command);
		check_release_bank(8, 0xF0, command);
		check_release_bank(0, 0xF8, command);
	}

	vrpn_Button_Filter::report_changes();
	
	
	// Parse the dial turn results, which are more than single-byte
	// results so will require reading in the rest of the command.
	// We will block until either we get them or get an error or time
	// out.  If there is an error or timeout, try resetting.
	if ( (command >= 0x30) && (command <= 0x37) ) {
#ifdef VERBOSE
	  printf("vrpn_raw_SGIBOX::get_report(): Got dial event\n");
#endif
		unsigned char dial_value[2];
		int i = command - 0x30;		// Which dial
		vrpn_int16 value;
		struct timeval timeout = {0, 10000};	// 10 milliseconds

		// Attempt to read both new values until we run out of time
		// and give up.  If we give up or have an error, try a reset.
		if (vrpn_read_available_characters(serialfd, dial_value, 2, &timeout) != 2) {
		  perror("vrpn_raw_SGIBOX: starting getting a dial command from box, but message wasn't completed -reseting ...");
		  reset();
		  return;
		}
#ifdef VERBOSE
		printf("vrpn_raw_SGIBOX::get_report(): Dial event %02x:[ %02x %02x] \n",
		 command,dial_value[0],dial_value[1]);
#endif

		// Make sure to sign-extend the 16-bit value properly.
		value = static_cast<short>((dial_value[0]<<8) | dial_value[1]);

		// Figure out the value and adjust the corresponding analog value.
		// The analogs are set up to clamp at both the to and bottom end of
		// the scale, but will back off immediately if you turn it the other
		// direction.  The range of outputs (channels) is from -0.5 to 0.5.
		int temp = value - mid_values[i];
		if (temp > VRPN_DIAL_RANGE) {
			channel[i] = 0.5;
			mid_values[i] = value - VRPN_DIAL_RANGE; 
		} else if (temp < -VRPN_DIAL_RANGE) {
			channel[i] = -0.5;
			mid_values[i] = value + VRPN_DIAL_RANGE;
		} else {
			channel[i] = temp/400.0;
		}

		// Figure out the change for the dial report, and report fraction
		// turned.
		dials[i] = (value-last_values[i])/(double)(VRPN_DIAL_RANGE);
		last_values[i] = value;
	}
	vrpn_Analog::report_changes();
	vrpn_Dial::report_changes();
	
	// check for an unrecognized command from the box
	if (! (
	     ( (command >= 0x30) && (command <= 0x37) ) ||
	     (command >= 0xC0)
	     ))
	  {
	    perror("vrpn_raw_SGIBOX: unrecognized command from sgiBox - reseting...");
	    reset();
	  }
}

void vrpn_raw_SGIBox::mainloop()
{
  server_mainloop();
  get_report();
}

int	vrpn_raw_SGIBox::send_light_command(void)  {
  int bank, i;
  unsigned char lights[4];	// Array to hold the light-control bits
  unsigned char msg[5];		// Message to send to turn on lights

  // Figure out which lights should be on, and pack them into the
  // four bytes that describe which should be on.
  for (bank = 0; bank < 4; bank++) {
    lights[bank] = 0; //one bit per button light
    for (i = 0; i < 8; i++) {
	int buttonLightNumber = bank*8 + i;
	if (buttonstate[buttonLightNumber]==vrpn_BUTTON_TOGGLE_ON) {
		lights[bank]=static_cast<unsigned char>(lights[bank]|1<<i);
	}
    }
  }

  // Prepare the control message to turn the lights on, then
  // send it.
  
  msg[0] = 0x75; memcpy(&msg[1],lights,4);
  if (vrpn_write_slowly(serialfd, msg, 5,1) != 5) {
	  perror("Could not write light control message");
	  //XXX Should be vrpn_Text message
	  reset();
	  return -1;
  }
  return 0;
}

// Turn on all the lights that are currently enabled.  This ignores
// the HANDLERPARAM parameter, since it is not required to figure this
// out.

static int VRPN_CALLBACK sgibox_raw_alert_handler(void * userdata, vrpn_HANDLERPARAM)
{
  vrpn_raw_SGIBox *me=(vrpn_raw_SGIBox *)userdata;

  me->send_light_command();
  return 0;
}

static int VRPN_CALLBACK sgibox_raw_con_cb(void * userdata, vrpn_HANDLERPARAM)
{
     
  printf("vrpn_raw_SGIBox::Get first new connection, reset the box\n");
  ((vrpn_raw_SGIBox *)userdata) ->reset();
  return 0;
}

