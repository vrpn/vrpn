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

#ifndef _VRPN_EVENT_ANALOG_H_
#define _VRPN_EVENT_ANALOG_H_

// includes, system
#include <vector>                       // for vector

// includes, project
#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Configure.h"             // for VRPN_API
#include "vrpn_Event.h"                 // for input_event

class VRPN_API vrpn_Connection;

class VRPN_API vrpn_Event_Analog: public vrpn_Analog {

public:

  // constructor
  // evdev is the event file name
  vrpn_Event_Analog( const char * name,
                     vrpn_Connection * connection,
                     const char * evdev_name);

  ~vrpn_Event_Analog();

protected:

  // read available events
  // returns number of structs read successfully
  int read_available_data();

protected:

  // typedefs for convenience
  typedef std::vector<struct vrpn_Event::input_event> event_vector_t;
  typedef event_vector_t::iterator event_iter_t;

  // handle to the event interface
  int fd;

  // maximal number of event structs read at once
  unsigned long max_num_events;

  // container for the event structs read
  event_vector_t  event_data;

};

#endif // _VRPN_EVENT_ANALOG_H_
