/**************************************************************************************************/
/*                                                                                                */
/* Copyright (C) 2004 Bauhaus University Weimar                                                   */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                */
/**************************************************************************************************/
/*                                                                                                */
/* module     :  vrpn_Event_Analog.h                                                              */
/* project    :                                                                                   */
/* description:  base class for devices using event interface                                     */
/*                                                                                                */
/**************************************************************************************************/


// includes, system
#include <stdio.h>                      // for fprintf, stderr, NULL
#include "vrpn_Shared.h"                // for vrpn_gettimeofday

// includes, file
#include "vrpn_Event_Analog.h"

class VRPN_API vrpn_Connection;


/**************************************************************************************************/
/* constructor */
/**************************************************************************************************/
vrpn_Event_Analog::vrpn_Event_Analog ( const char * name, 
                                       vrpn_Connection * c,
                                       const char* evdev_name) : 
  vrpn_Analog( name, c),
  fd(-1),
  max_num_events( 16),
  event_data( event_vector_t( 16))
{
  #if defined(_WIN32)

   fprintf( stderr, "vrpn_Event_Analog(): Not yet implemented on this architecture.");

  #else  // #if defined(LINUX)

    // check if filename valid
    if (0 == evdev_name) {

      fprintf(stderr,"vrpn_Event_Analog: No file name.\n");
      status = vrpn_ANALOG_FAIL;

      return;
    }

    // open event interface
    if ( -1 == (fd = vrpn_Event::vrpn_open_event( evdev_name))) {

      fprintf(stderr,"vrpn_Event_Analog: Failed to open event interface file.\n");

      status = vrpn_ANALOG_FAIL;
    }
    else {

      // Reset the tracker 
      status = vrpn_ANALOG_RESETTING;
    }
 
    // set the time 
    vrpn_gettimeofday(&timestamp, NULL);

  #endif // #if defined(LINUX) 
}

/**************************************************************************************************/
/* destructor */
/**************************************************************************************************/
vrpn_Event_Analog::~vrpn_Event_Analog () {

  #if defined(_WIN32)

    fprintf( stderr, "~vrpn_Event_Analog(): Not yet implemented on this architecture.");

  #else  // #if defined(LINUX)

    // close handle if still valid
    if ( -1 != fd) {

      vrpn_Event::vrpn_close_event( fd);
    }

  #endif // #if defined(LINUX) 
}

/***************************************************************************************************/
/* read the data */
/***************************************************************************************************/
int
vrpn_Event_Analog::read_available_data () {

  #if defined(_WIN32)

    return vrpn_Event::vrpn_read_event( fd, &(event_data.front()), max_num_events); 

  #else // not Windows
  
    // check for updates at max_num_events
    if (max_num_events != event_data.size()) {

      event_data.resize( max_num_events);

      max_num_events = event_data.size();
    }

    // read data
    return vrpn_Event::vrpn_read_event( fd, &(event_data.front()), max_num_events); 

  #endif  // not Windows
}
