/**************************************************************************************************/
/*                                                                                                */
/* Copyright (C) 2004 Bauhaus University Weimar                                                   */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                */
/**************************************************************************************************/
/*                                                                                                */
/* module     :  vrpn_Linux.h                                                                     */
/* project    :                                                                                   */
/* description:  provide functionality for Event interface                                        */
/*                                                                                                */
/**************************************************************************************************/

// includes, system

#include <stdio.h>                      // for perror
#if ! defined(_WIN32)
  #include <fcntl.h>                      // for open, O_RDONLY
  #include <unistd.h>                     // for close, read
#endif

// includes project
#include "vrpn_Event.h"


namespace vrpn_Event {

  /************************************************************************************************/
  /* open the specified event interface */
  /************************************************************************************************/
  int 
  vrpn_open_event( const char* file) {

    #if defined(_WIN32)

      fprintf( stderr, "vrpn_Event::vrpn_open_event(): Not yet implemented on this architecture.");
      return -1;

    #else  // #if defined(LINUX)

      return open( file, O_RDONLY);
    
    #endif
  }

  /************************************************************************************************/
  /* close the event interface */
  /************************************************************************************************/
  void 
  vrpn_close_event( const int fd) {

    #if defined(_WIN32)

      fprintf( stderr, "vrpn_Event::vrpn_close_event(): Not yet implemented on this architecture.");

    #else  // #if defined(LINUX)

      close(fd);

    #endif
  }

  /************************************************************************************************/
  /* read data from the interface */
  /************************************************************************************************/
  int
  vrpn_read_event( int fd, input_event * data, int max_elements) {

    #if defined(_WIN32)

      fprintf( stderr, "vrpn_Event::vrpn_read_event(): Not yet implemented on this architecture.");
      return -1;

    #else  /// #if defined(LINUX)

      int read_bytes = read(fd, data, sizeof(struct input_event) * max_elements);

      if (read_bytes < (int) sizeof(struct input_event)) {

        perror("vrpn_Event_Linux::vrpn_read_event() : short read");
      }    
  
    return (read_bytes / sizeof(struct input_event));

    #endif 
  }    

} // end namespace vrpn_Event

