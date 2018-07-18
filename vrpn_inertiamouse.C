/*
 *   vrpn_inertiamouse.C
 */

#include <math.h>                       // for fabs
#include <stdio.h>                      // for fprintf, stderr, NULL
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Serial.h"
#include "vrpn_inertiamouse.h"

#undef VERBOSE

const double vrpn_inertiamouse::Vel_Decay = 0.66;

namespace {
    enum {
        STATUS_RESETTING = -1, // Resetting the device
        STATUS_SYNCING = 0,  // Looking for the first character of report
        STATUS_READING = 1,  // Looking for the rest of the report
        
        MAX_TIME_INTERVAL = 2000000 // max time between reports (usec)
    };
} // namespace
vrpn_inertiamouse::vrpn_inertiamouse (const char* name, 
                                      vrpn_Connection * c,
                                      const char* port, 
                                      int baud_rate)
    : vrpn_Serial_Analog (name, c, port, baud_rate)
    , vrpn_Button_Filter (name, c)
    , numbuttons_ (Buttons)
    , numchannels_ (Channels)
    , expected_chars_ (1)
    , bufcount_ (0)
    , null_radius_ (0)
    , dcb_ ()
    , lp_ ()
{
    vrpn_Button::num_buttons = numbuttons_;
    vrpn_Analog::num_channel = numchannels_;

    vel_ = new double[numchannels_];

    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
    
    clear_values();
    
    status_ = STATUS_RESETTING;
}

// factory method
vrpn_inertiamouse* 
vrpn_inertiamouse::create (const char* name, 
        vrpn_Connection * c,
        const char* port, 
        int baud_rate)
{
  vrpn_inertiamouse *ret = NULL;
  try { ret = new vrpn_inertiamouse(name, c, port, baud_rate); }
  catch (...) { return NULL; }
  return ret;
}


void vrpn_inertiamouse::clear_values(void)
{
    int	i;

    for (i = 0; i < numbuttons_; i++) {
        vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
    }
    for (i = 0; i < numchannels_; i++) {
        vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }
}

int vrpn_inertiamouse::reset(void)
{
    clear_values();
    vrpn_flush_input_buffer(serial_fd);
    
    status_ = STATUS_SYNCING;
    
    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
    return 0;
}

   
int vrpn_inertiamouse::get_report(void)
{
    int ret; // Return value from function call to be checked

    //
    // If we're SYNCing, then the next character we get should be the
    // start of a report.  If we recognize it, go into READing mode
    // and tell how many characters we expect total. If we don't
    // recognize it, then we must have misinterpreted a command or
    // something; reset the intertiamouse and start over
    //
    if (status_ == STATUS_SYNCING) {
        // Try to get a character.  If none, just return.
        ret = vrpn_read_available_characters (serial_fd, &buffer_[0], 1);
        if (ret != 1) { return 0; }

        switch (buffer_[0]) {
        case 'D':
            expected_chars_ = 25;
            break;
        case 'B':
            expected_chars_ = 4;
            break;
                
        default:
            fprintf(stderr, "vrpn_inertiamouse: Unknown command (%c), resetting\n", buffer_[0]);
            status_ = STATUS_RESETTING;
            return 0;
        }

        bufcount_ = 1;
        vrpn_gettimeofday (&timestamp, NULL);

        status_ = STATUS_READING;
    }

    ret = vrpn_read_available_characters(serial_fd, 
                                         &buffer_[bufcount_],
                                         expected_chars_ - bufcount_);
    if (ret == -1) {
	send_text_message("vrpn_inertiamouse: Error reading", 
                          timestamp, 
                          vrpn_TEXT_ERROR);
	status_ = STATUS_RESETTING;
	return 0;
    }
    bufcount_ += ret;

    if (bufcount_ < expected_chars_) {	// Not done -- go back for more
	return 0;
    }

    if (buffer_[expected_chars_ - 1] != '\n') {
        status_ = STATUS_SYNCING;
        send_text_message("vrpn_inertiamouse: No newline in record", 
                          timestamp, 
                          vrpn_TEXT_ERROR);
        return 0;
    }

    switch ( buffer_[0] ) {
    case 'D':
      {
        int i;
        int nextchar;
        for (i = 0, nextchar = 0; i < numchannels_; ++i) {
            int packet;
            packet =  (buffer_[++nextchar] & 0xf0) << 8;  
            packet |= (buffer_[++nextchar] & 0xf0) << 4;
            packet |= (buffer_[++nextchar] & 0xf0);
            packet |= (buffer_[++nextchar] & 0xf0) >> 4;

            int chnl = (packet >> 10) & 7;
            if (chnl >= Channels) {
               status_ = STATUS_SYNCING;
               send_text_message("vrpn_inertiamouse: Too-large channel value", 
		  timestamp, 
		  vrpn_TEXT_ERROR);
               return 0;
            }
            int acc = packet & 0x3ff; // 10 bits
            
            // normalize to interval [-1,1]
            // just a guess, block dc later
            double normval = ((double)(acc - 256) / 
                              (double)256);

//            normval *= 1.5;

            // update rotation data
            if ( (chnl == 4) || (chnl == 5) ) {
                channel[chnl] = normval;
                break;
            }
            normval = dcb_[chnl].filter (normval);
            normval = lp_[chnl].filter (normval);
           
            double dt = 0.25;

            // update velocity and position only when button[0] pressed
            if (buttons[0]) {
                
                double pos = vel_[chnl] * dt + normval * dt * dt / 2;

                vel_[chnl] += normval*dt;
                if(fabs (vel_[chnl]) < dt/2.0)
                    vel_[chnl] *= 0.90;
//                else
//                 if (fabs (vel_[chnl]) > 1.0)
//                     vel_[chnl] *= 1.0 / fabs (vel_[chnl]);

                channel[chnl] = pos;
//                channel[chnl] *= 0.95;
            } else {
                vel_[chnl] = 0.0;
                channel[chnl] = 0.0;
            }
        }
      }
        break;
    case 'B':
        buttons[0] = ((buffer_[1] & 1) != 0);
        buttons[1] = ((buffer_[1] & 2) != 0);
        break;
    default:
	fprintf(stderr, "vrpn_inertiamouse: Unknown [internal] command (%c), resetting\n", buffer_[0]);
	status_ = STATUS_RESETTING;
	return 0;
   }

    //
    // Done with the decoding, send the reports and go back to syncing
    //

    report_changes();
    status_ = STATUS_SYNCING;
    bufcount_ = 0;

    return 1;	// We got a full report.
}

void vrpn_inertiamouse::report_changes(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button::timestamp = timestamp;
    
    vrpn_Analog::report_changes(class_of_service);
    vrpn_Button::report_changes();
}

void vrpn_inertiamouse::report(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button::timestamp = timestamp;
    
    vrpn_Analog::report(class_of_service);
    vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It
// will take a course of action depending on the current status of the
// intertiamouse, either trying to reset it or trying to get a reading
// from it.
void vrpn_inertiamouse::mainloop()
{
    server_mainloop();

    switch(status_) {
    case STATUS_RESETTING:
	reset();
	break;

    case STATUS_SYNCING:
    case STATUS_READING:
	// Keep getting reports until all full reports are read.
        while (get_report());
        break;

    default:
        fprintf(stderr, "vrpn_inertiamouse: Unknown mode (internal error)\n");
	break;
    }
}
