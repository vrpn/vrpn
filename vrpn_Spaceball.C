// vrpn_Spaceball.C
// This is a driver for the 3DConnexions (was Labtec/Spacetec.) 
// http://www.3dconnexions.com/
// Spaceball 6DOF motion controller, with 2, 9, or 12 buttons and
// a ball that translational and rotational forces can be 
// applied to.  It uses an RS-232 interface and can be communicated
// with using either a raw serial protocol (done in this driver), 
// or by using the X-Windows or Microsoft Windows drivers supplied 
// by the hardware vendor.
//
// This driver was written by John E. Stone, based on his previous Spaceball 
// driver implementations.  The VRPN version of this code has only been 
// directly tested with the Spaceball 2003 version of the device so far, 
// though the original standalone libsball code it is based on is known 
// to work with the 3003 and 4000 FLX devices, so the VRPN driver should too.  
// If you have questions about this code in general, please email the author: 
// johns@megapixel.com  j.stone@acm.org  johns@ks.uiuc.edu
// http://www.megapixel.com/
// 
// Guidance for the VRPN version of this driver was obtained by 
// basing it on the vrpn_Magellan code, where possible.
//
// Comments about this code:
//   Spots in the code marked with XXX are either of particular interest
// as they could be improved, or because they are a VRPN-interpretation of
// functionality written in libsball that was changed for VRPN.   
// In particular, it might be useful for VRPN to know whether or not it is
// talking to a Spaceball 2003, 3003, or 4000, but this implementation does
// not set any status bits in VRPN to make such information available.
// Should the "lefty" mode of the 4000 be reported as a button?
// At present, none of this state is exposed to VRPN, though it might
// be useful in some cases.  Also, the VRPN code hides the differences
// between the 2003, 3003, and 4000 button handling, where libsball 
// gives them separate types of button status.  Either interpretation
// is useful, depending on what the application people want, but since
// VRPN is supposed to be very general purpose, I chose to make this
// driver hide as much of the device specific stuff as possible.
// The main issues between the devices are the actual number of buttons
// on the devices, and the interpretation of the "rezero" button,
// the interpretation of the "pick" button, and the interpretation of
// the "lefty" mode on the Spaceball 4000.  These can easily be 
// changed to better suit VRPN's needs however if some other behavior
// is preferable.  
//
// The current button mapping in VRPN (only verified on 2003 so far)
// is as follows:
//
//                    Spaceball Model
// VRPN Button      1003     2003     3003     4000
//        1                     1        L        1
//        2                     2        R        2
//        3                     3                 3
//        4                     4                 4
//        5                     5                 5
//        6                     6                 6
//        7                     7                 7
//        8                  8/Zero      Zero     8
//        9                  Pick                 9
//       10                                      10 ("aka A?")
//       11                                      11 ("aka B?")
//       12                                      12 ("aka C?")
//
// Enjoy,
//   John E. Stone
//   johns@megapixel.com
//   j.stone@acm.org
//   johns@ks.uiuc.edu
//

#include <stdio.h>                      // for fprintf, stderr
#include <string.h>                     // for NULL, strlen

#include "vrpn_Serial.h"                // for vrpn_flush_input_buffer, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include "vrpn_Spaceball.h"

// turn on for debugging code, leave off otherwise
#undef VERBOSE

#if defined(VERBOSE) 
#include <ctype.h> // for isprint()

#define DEBUG 1
#endif

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING       (-1) // Resetting the device
#define	STATUS_SYNCING		(0) // Looking for the first char of report
#define	STATUS_READING		(1) // Looking for the rest of the report
#define MAX_TIME_INTERVAL (2000000) // max time between reports (usec)

// This creates a vrpn_Spaceball and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
vrpn_Spaceball::vrpn_Spaceball (const char * name, vrpn_Connection * c,
			const char * port, int baud):
		vrpn_Serial_Analog(name, c, port, baud)
		, vrpn_Button_Filter(name, c)
		, _numbuttons(12)
		, _numchannels(6)
    , bufpos(0)
    , packlen(0)
    , escapedchar(0)
    , erroroccured(0)
    , resetoccured(0)
    , spaceball4000(0)
    , leftymode4000(0)
		, null_radius(8)
{
	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = _numbuttons;
	vrpn_Analog::num_channel = _numchannels;

        vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

	// Set the status of the buttons and analogs to 0 to start
	clear_values();

	// Set the mode to reset
	status = STATUS_RESETTING;
}

void	vrpn_Spaceball::clear_values(void)
{
	int	i;

	for (i = 0; i < _numbuttons; i++) {
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
	}
	for (i = 0; i < _numchannels; i++) {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
	}
}





// This routine will reset the Spaceball, zeroing the position,
// mode and making the device beep.
int	vrpn_Spaceball::reset(void) {

        /* Spaceball initialization string. Newer documentation suggests  */
        /* eliminating several init settings, leaving them at defaults.   */
	const char *reset_str = "CB\rNT\rFTp\rFRp\rP@r@r\rMSSV\rZ\rBcCcC\r";

	// Set the values back to zero for all buttons, analogs and encoders
	clear_values();

        /* Reset some state variables back to zero */
        bufpos = 0;
        packtype = 0;
        packlen = 0;
        escapedchar = 0;
        erroroccured = 0;
        resetoccured = 0;
        spaceball4000 = 0; /* re-determine which type it is     */
        leftymode4000 = 0; /* re-determine if its in lefty mode */
	null_radius = 8;  // The NULL radius is now set to 8

	// Send commands to the device to cause it to reset and beep.
	vrpn_flush_input_buffer(serial_fd);
	vrpn_write_slowly(serial_fd, (const unsigned char *)reset_str, strlen(reset_str), 5);

	// We're now waiting for a response from the box
	status = STATUS_SYNCING;

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}



//   This function will read characters until it has a full report, then
// put that report into the time, analog, or button fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report.
//   Reports start with different characters, and the length of the report
// depends on what the first character of the report is.  We switch based
// on the first character of the report to see how many more to expect and
// to see how to handle the report.
//   Returns 1 if there is a complete report found, 0 otherwise.  This is
// so that the calling routine can know to check again at the end of complete
// reports to see if there is more than one report buffered up.
   
int vrpn_Spaceball::get_report(void)
{
  unsigned char rawbuf[1024]; // raw unprocessed incoming characters
  int i, num, packs;
#if defined(DEBUG)
  int j;
#endif
  packs = 0; /* no packs received yet */

  // read up to 1023 unprocessed characters from the serial device at once
  num = vrpn_read_available_characters(serial_fd, rawbuf, 1023);

  // if we receive 1 or more chars, we will see if this completes any
  // pending packets we're trying to process.
  if (num > 0) {
    for (i=0; i<num; i++) {
      /* process potentially occurring escaped character sequences */
      if (rawbuf[i] == '^') {
        if (!escapedchar) {
          escapedchar = 1;
          continue; /* eat the escape character from buffer */
        } 
      }

      /* if in escaped mode, we convert the escaped char to final form */
      if (escapedchar) {
        escapedchar = 0; /* we're back out of escape mode after this */

        switch (rawbuf[i]) {
          case '^': /* leave char in buffer unchanged */
            break;

          case 'Q':
          case 'S':
          case 'M':
            rawbuf[i] &= 0x1F; /* convert character to unescaped form */
            break;

          default:
#if defined(DEBUG)
            printf("\nGot a bad escape sequence! 0x%02x", rawbuf[i]);
            if (isprint(rawbuf[i])) 
              printf(" (%c)", rawbuf[i]);
            else 
              printf(" (unprintable)");
            printf("\n");
#endif
            break;
        }
      } 


      /* figure out what kind of packet we received */
      if (bufpos == 0) {
        status = STATUS_SYNCING; /* update our status */

        switch(rawbuf[i]) {
          case 'D':  /* Displacement packet */
            packtype = 'D'; 
            packlen = 16;    /* D packets are 15 bytes long */
            break;
  
          case 'K':  /* Button/Key packet */
            packtype = 'K';
            packlen = 4;     /* K packets are 3 bytes long */
            break;

          case '.': /* Spaceball 4000 FLX "advanced" button press event */
            packtype = '.';
            packlen = 4;     /* . packets are 3 bytes long */
            break;

          case 'C': /* Communications mode packet */
            packtype = 'C';
            packlen = 4;
            break;

          case 'F': /* Spaceball sensitization mode packet */
            packtype = 'F';
            packlen = 4;
            break;

          case 'M': /* Movement mode packet */
            packtype = 'M';
            packlen = 5;
            break;

          case 'N': /* Null region packet */
            packtype = 'N';
            packlen = 3;
            break;

          case 'P': /* Update rate packet */ 
            packtype = 'P';
            packlen = 6;
            break;

          case '\v': /* XON at poweron */
            packtype = '\v';
            packlen = 1;
            break;

          case '\n': /* carriage return at poweron */
          case '\r': /* carriage return at poweron */
            packtype = '\r';
            packlen = 1;
            break;

          case '@': /* Spaceball Hard/Soft Reset packet */
            resetoccured=1;
            packtype = '@';
            packlen = 62;    /* Resets aren't longer than 62 chars */
            break;
 
          case 'E': /* Error packet */
            packtype = 'E';
            packlen = 8;     /* E packets are up to 7 bytes long */
            break;

          case 'Z': /* Zero packet (Spaceball 2003/3003/4000 FLX) */
            packtype = 'Z';
            packlen = 14;    /* Z packets are hardware dependent */
            break;
 
          default:  /* Unknown packet! */
#if defined(DEBUG)
            printf("\nUnknown packet (1) [%d]: 0x%02x \n ", i, rawbuf[i]); 
            printf("                char:  ");
            if (isprint(rawbuf[i])) 
              printf("%c", rawbuf[i]);
            else 
              printf(" (unprintable)");
            printf("\n");
#endif
            continue;
        }
      }


      buf[bufpos] = rawbuf[i]; /* copy processed chars into long-term buffer */
      bufpos++;                /* go to next buffer slot */

      /* Reset packet processing */
      if (packtype == '@') {
        if (rawbuf[i] != '\r')
          continue;
        else 
          packlen = bufpos;
      }   

      /* Error packet processing */
      if (packtype == 'E') {
        if (rawbuf[i] != '\r')
          continue;
        else 
          packlen = bufpos;
      } else if (bufpos != packlen)
        continue;

      status = STATUS_READING;        // ready to process event packet
      vrpn_gettimeofday(&timestamp, NULL); // set timestamp of this event

      switch (packtype) {
        case 'D':  /* ball displacement event */
          {
            int nextchar, chan;

            /* number of 1/16ths of milliseconds since last */
            /* ball displacement packet */ 

            nextchar = 1;  // this is where the timer data is, if we want it.
            nextchar = 3;  // Skip the zeroeth character (the command)
            for (chan = 0; chan < _numchannels; chan++) {
              vrpn_int16 intval;
              intval  = (buf[nextchar++]) << 8;
              intval |= (buf[nextchar++]);

              // If the absolute value of the integer is <= the NULL 
              // radius, it should be set to zero.
              if ( (intval <= null_radius) && (intval >= - null_radius) ) {
                intval = 0;
              }

              // maximum possible values per axis are +/- 32768, although the
              // largest value I've ever observed among several devices
              // is only 20,000.  For now, we'll divide by 32768, and if this is
              // too insensitive, we can do something better later.
              double realval = intval / 32768.0;
              channel[chan] = realval;
              // printf("XXX Channel[%d] = %f   %d \n", z, realval, intval);
            }
            channel[2] = -channel[2]; // Negate Z translation
            channel[5] = -channel[5]; // Negate Z rotation
          }
          break;
 
        case 'K': /* button press event */
          /* Spaceball 2003A, 2003B, 2003 FLX, 3003 FLX, 4000 FLX       */
          /* button packet. (4000 only for backwards compatibility)     */
          /* The lowest 5 bits of the first byte are buttons 5-9        */
          /* Button '8' on a Spaceball 2003 is the rezero button        */
          /* The lowest 4 bits of the second byte are buttons 1-4       */
          /* For Spaceball 2003, we'll map the buttons 1-7 normally     */   
          /* skip 8, as its a hardware "rezero button" on that device   */
          /* and call the "pick" button "8".                            */
          /* On the Spaceball 3003, the "right" button also triggers    */
          /* the "pick" bit.  We OR the 2003/3003 rezero bits together  */

          /* if we have found a Spaceball 4000, then we ignore the 'K'  */
          /* packets entirely, and only use the '.' packets.            */ 
          if (spaceball4000)
            break;

// XXX    my original libsball button decoding code, for reference
//           buttons = 
//             ((buf[1] & 0x10) <<  3) | /* 2003 pick button is "8" */
//             ((buf[1] & 0x20) <<  9) | /* 3003 rezero button      */
//             ((buf[1] & 0x08) << 11) | /* 2003 rezero button      */
//             ((buf[1] & 0x07) <<  4) | /* 5,6,7    (2003/4000)    */
//             ((buf[2] & 0x30) <<  8) | /* 3003 Left/Right buttons */ 
//             ((buf[2] & 0x0F));        /* 1,2,3,4  (2003/4000)    */ 

            // Mapping the buttons is tricky since different models
            // of the Spaceball have different button sets.
            // 2003 has 9 buttons (rezero included)
            // 3003 has 3 buttons (rezero included)
            // 4000 has 12 buttons, and can be in "lefty" or "righty" mode.
            // We'll skip reporting lefty/righty mode for now though.

            // Spaceball 2003/4000 buttons 1-4 mapped to slots 0-3
            // Spaceball 3003 L/R buttons are mapped as slots 0/1
            buttons[0] = static_cast<unsigned char>(((buf[2] & 0x01) != 0) | ((buf[2] & 0x10) != 0));
            buttons[1] = static_cast<unsigned char>(((buf[2] & 0x02) != 0) | ((buf[2] & 0x20) != 0));
            buttons[2] = static_cast<unsigned char>(((buf[2] & 0x04) != 0));
            buttons[3] = static_cast<unsigned char>(((buf[2] & 0x08) != 0));

            // Spaceball 2003/4000 buttons 5,6,7 mapped to slots 4-6
            buttons[4] = static_cast<unsigned char>(((buf[1] & 0x01) != 0));
            buttons[5] = static_cast<unsigned char>(((buf[1] & 0x02) != 0));
            buttons[6] = static_cast<unsigned char>(((buf[1] & 0x04) != 0));

            // Spaceball 2003/3003 rezero buttons are mapped to slot 7
            // The rezero button's function is sometimes hard-wired, 
            // and may or may not be used by applications, this is up
            // in the air still.  For now, I'll map it to slot 7 which
            // keeps the numbering in a sane order on the 2003 device anyway.
            buttons[7] = static_cast<unsigned char>(((buf[1] & 0x20) != 0) | ((buf[1] & 0x08) != 0)); 

            // Spaceball 2003 pick button mapped to slot 8
            // Note: the pick button is the button embedded into the front
            //           surface of the control sphere.
            buttons[8] = static_cast<unsigned char>(((buf[1] & 0x10) != 0));
          break;


        case '.': /* button press event (4000) */
          /* Spaceball 4000 FLX "expanded" button packet, with 12 buttons */

          /* extra packet validity check, since we use this packet type */
          /* to override the 'K' button packets, and determine if its a */
          /* Spaceball 4000 or not...                                   */
          if (buf[3] != '\r') {
            break; /* if not terminated with a '\r', probably garbage */
          }

          /* if we got a valid '.' packet, this must be a Spaceball 4000 */
#if defined(DEBUG)
          if (!spaceball4000)
            printf("\nDetected a Spaceball 4000 FLX\n");
#endif
          spaceball4000 = 1; /* Must be talking to a Spaceball 4000 */
    
// XXX    my original libsball button decoding code, for reference
//          buttons = 
//            (((~buf[1]) & 0x20) << 10) |  /* "left handed" mode  */
//            ((buf[1] & 0x1F) <<  7)    |  /* 8,9,10,11,12        */
//            ((buf[2] & 0x3F)      )    |  /* 1,2,3,4,5,6         */
//            ((buf[2] & 0x80) >>  1);      /* 7                   */

          /* Spaceball 4000 series "expanded" button press event      */
          /* includes data for 12 buttons, and left/right orientation */
          buttons[0]  = ((buf[2] & 0x01) != 0); // SB 4000 button 1
          buttons[1]  = ((buf[2] & 0x02) != 0); // SB 4000 button 2
          buttons[2]  = ((buf[2] & 0x04) != 0); // SB 4000 button 3
          buttons[3]  = ((buf[2] & 0x08) != 0); // SB 4000 button 4
          buttons[4]  = ((buf[2] & 0x10) != 0); // SB 4000 button 5
          buttons[5]  = ((buf[2] & 0x20) != 0); // SB 4000 button 6
          buttons[6]  = ((buf[2] & 0x80) != 0); // SB 4000 button 7

          buttons[7]  = ((buf[1] & 0x01) != 0); // SB 4000 button 8
          buttons[8]  = ((buf[1] & 0x02) != 0); // SB 4000 button 9
          buttons[9]  = ((buf[1] & 0x04) != 0); // SB 4000 button 10
          buttons[10] = ((buf[1] & 0x08) != 0); // SB 4000 button 11
          buttons[11] = ((buf[1] & 0x10) != 0); // SB 4000 button 12

          // XXX lefty/righty mode handling goes here if we wish to 
          //     represent it as a "button" etc..
          //buttons[??] = ((~buf[1]) & 0x20) != 0) // SB 4000 "lefty" mode bit
 
#if defined(DEBUG)
          if (leftymode4000 != ((buf[1] & 0x20) == 0))
             printf("\nSpaceball 4000 mode changed to: %s\n",
               (((buf[1] & 0x20) == 0) ? "left handed" : "right handed"));
#endif
          /* set "lefty" orientation mode if "lefty bit" is _clear_ */
          if ((buf[1] & 0x20) == 0) 
            leftymode4000 = 1; /* left handed mode */
          else 
            leftymode4000 = 0; /* right handed mode */
          break;

        case 'C': /* Communications mode packet */
        case 'F': /* Spaceball sensitization packet */
        case 'P': /* Spaceball update rate packet */
        case 'M': /* Spaceball movement mode packet */
        case 'N': /* Null region packet */
        case '\r': /* carriage return at poweron */
        case '\v': /* XON at poweron */
          /* eat and ignore these packets */
          break;

        case '@': /* Reset packet */
#if defined(DEBUG)
          printf("Spaceball reset: ");
          for (j=0; j<packlen; j++) {
            if (isprint(buf[j])) 
              printf("%c", buf[j]);
          }
          printf("\n");
#endif
          /* if we get a reset packet, we have to re-initialize       */
          /* the device, and assume that its completely schizophrenic */
          /* at this moment, we must reset it again at this point     */
          resetoccured=1;
          reset(); // was sball_hwreset()
          break;
  

        case 'E': /* Error packet, hardware/software problem */
          erroroccured++;
#if defined(DEBUG)
          printf("\nSpaceball Error!!    "); 
          printf("Error code: ");
          for (j=0; j<packlen; j++) {
            printf(" 0x%02x ", buf[j]);
          } 
          printf("\n");
#endif
          break;

        case 'Z': /* Zero packet (Spaceball 2003/3003/4000 FLX) */
          /* We just ignore these... */
          break;          

        default: 
#if defined(DEBUG)
            printf("Unknown packet (2): 0x%02x\n", packtype); 
            printf("              char:  ");
            if (isprint(packtype)) 
              printf("%c", packtype);
            else 
              printf(" (unprintable)");
            printf("\n");
#endif
          break;
      }   
        
      /* reset */ 
      bufpos = 0;   
      packtype = 0; 
      packlen = 1;
      packs++;
    }
  }

  report_changes();        // Report updates to VRPN

  if (packs > 0) 
    return 1; // got at least 1 full report
  else 
    return 0; // didn't get any full reports
}

void	vrpn_Spaceball::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

void	vrpn_Spaceball::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the Spaceball,
// either trying to reset it or trying to get a reading from it.
void	vrpn_Spaceball::mainloop()
{
  server_mainloop();

  switch(status) {
    case STATUS_RESETTING:
	reset();
	break;

    case STATUS_SYNCING:
    case STATUS_READING:
	// Keep getting reports until all full reports are read.
	while (get_report()) {};
        break;

    default:
	fprintf(stderr,"vrpn_Spaceball: Unknown mode (internal error)\n");
	break;
  }
}


