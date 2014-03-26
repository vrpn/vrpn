/**************************************************************************************************/
/*                                                                                                */
/* Copyright (C) 2004 Bauhaus University Weimar                                                   */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                */
/**************************************************************************************************/
/*                                                                                                */
/*  module     :  vrpn_Event_Mouse.h                                                              */
/*  project    :                                                                                  */
/*  description:  mouse input using the event interface                                           */
/*                                                                                                */
/***************************************************************************************************/


#include "vrpn_Shared.h"                // for vrpn_gettimeofday
#include <vector>                       // for vector

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Event.h"                 // for input_event
// includes, file
#include "vrpn_Event_Mouse.h"
#include "vrpn_Types.h"                 // for vrpn_float64

// includes, system

// includes, project

#ifndef _WIN32

  // defines, local
  #define EV_KEY                  0x01
  #define EV_REL                  0x02
  #define REL_X                   0x00
  #define REL_Y                   0x01
  #define REL_Z                   0x02
  #define BTN_LEFT                0x110
  #define BTN_RIGHT               0x111
  #define BTN_MIDDLE              0x112
  #define REL_WHEEL               0x08

#endif


/**************************************************************************************************/
/* constructor (private) */
/**************************************************************************************************/
vrpn_Event_Mouse::vrpn_Event_Mouse( const char *name, 
                                    vrpn_Connection *c, 
                                    const char* evdev_name) : 
  vrpn_Event_Analog( name, c, evdev_name),
  vrpn_Button_Server(name,c)
{
  vrpn_Button::num_buttons = 3;
  vrpn_Analog::num_channel = 3;

  vrpn_gettimeofday(&timestamp, 0);
  vrpn_Analog::timestamp = timestamp;
  vrpn_Button::timestamp = timestamp;

  clear_values();
}

/**************************************************************************************************/
/* destructor                                                                                     */
/* the work is done in the destructor of the base class                                           */
/**************************************************************************************************/
vrpn_Event_Mouse::~vrpn_Event_Mouse() {

}

/**************************************************************************************************/
/* mainloop */
/**************************************************************************************************/
void
vrpn_Event_Mouse::mainloop() {

  server_mainloop();

  if ( ! d_connection->connected()) {

    return;
  }

  // read and interpret data from the event interface
  process_mouse_data();

  // update message buffer
  vrpn_Analog::report_changes();
  vrpn_Button::report_changes();

  // send messages
  d_connection->mainloop();
}

/**************************************************************************************************/
/* helper function for mainloop */
/**************************************************************************************************/
void
vrpn_Event_Mouse::process_mouse_data() {

  // try to read data
  if ( 0 == vrpn_Event_Analog::read_available_data()) {

    return;
  }

  #if defined(_WIN32)

    fprintf( stderr, "vrpn_Event_Mouse::process_mouse_data(): Not yet implemented on this architecture.");

  #else // if defined(LINUX)

    int index;

    // process data stored by the base class
    for( event_iter_t iter = event_data.begin(); iter != event_data.end(); ++iter) {

      switch ((*iter).type) {
        case EV_REL:
          switch ((*iter).code) {
            case REL_X:	
              channel[0] = (signed int)(*iter).value;
              break;
            case REL_Y:	
              channel[1] = (signed int)(*iter).value;
              break;
            case REL_WHEEL: 
              channel[2] = (signed int)(*iter).value;
              break;
          }
          break;
        case EV_KEY:
          switch ((*iter).code) {
            case BTN_LEFT: 
              index = 0; 
              break;
            case BTN_RIGHT: 
              index = 1; 
              break;
            case BTN_MIDDLE: 
              index = 2; 
              break;
           default: 
             return;
          }
          switch ((*iter).value) {
            case 0:	    
            case 1:	    
              buttons[index]=(*iter).value;
              break;
            case 2: 
              break;
            default: 
              return;
          }
          break;
      }
    } // end for loop

    #ifdef DEBUG
    {
      int i;

      printf("channel: ");
      for (i = 0; i < vrpn_Analog::num_channel; ++i) {

        //printf("float %4.2f ",channel[i]);
        printf("channel %d mit %f; ",i, channel[i]);
      }
      printf("\n");

      printf("buttons: ");
      for (i = 0; i < vrpn_Button::num_buttons; ++i) {
        
        //printf("float %4.2f ",buttons[i]);
        printf("button %d mit %d; ",i,buttons[i]);
      }
      printf("\n");
    }
    #endif

  #endif // if defined(LINUX) 

  vrpn_gettimeofday(&timestamp, 0);
  vrpn_Analog::timestamp = timestamp;
  vrpn_Button::timestamp = timestamp;
}



/**************************************************************************************************/
/* clear values, reset to 0 */
/**************************************************************************************************/
void
vrpn_Event_Mouse::clear_values() {

  int i;
  for ( i = 0; i < vrpn_Button::num_buttons; ++i) {

    vrpn_Button::buttons[i] = 0;
    vrpn_Button::lastbuttons[i] = 0;
  }

  for ( i = 0; i < vrpn_Analog::num_channel; ++i) {

    vrpn_Analog::channel[i] = 0;
    vrpn_Analog::last[i] = 0;
  }
}
