// vrpn_Tracker_Fastrak.h
//	This file contains the class header for a Polhemus Fastrak Tracker.
// This file is based on the vrpn_3Space.h file, with modifications made
// to allow it to operate a Fastrak instead. The modifications are based
// on the old version of the Fastrak driver, which had been mainly copied
// from the Trackerlib driver and was very difficult to understand.
//	This version was written in the Summer of 1999 by Russ Taylor.

#ifndef VRPN_TRACKER_FASTRAK_H
#define VRPN_TRACKER_FASTRAK_H

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"

class vrpn_Tracker_Fastrak: public vrpn_Tracker_Serial {
  
 public:

  // The constructor is given the name of the tracker (the name of
  // the sender it should use), the connection on which it is to
  // send its messages, the name of the serial port it is to open
  // (default is /dev/ttyS1 (first serial port in Linux)), the baud
  // rate at which it is to communicate (default 19200), whether
  // filtering is enabled (default yes), and the number of stations
  // that are possible on this Fastrak (default 4). The station select
  // switches on the front of the Fastrak determine which stations are
  // active. The final parameter is a string that can contain additional
  // commands that are set to the tracker as part of its reset routine.
  // These might be used to set the hemisphere or other things that are
  // not normally included; see the Fastrak manual for a list of these.
  // There can be multiple lines of them but putting <CR> into the string.

  vrpn_Tracker_Fastrak(char *name, vrpn_Connection *c, 
		      char *port = "/dev/ttyS1", long baud = 19200,
		      int enable_filtering = 1, int numstations = 4,
		      const char *additional_reset_commands = NULL);

  // This function should be called each time through the main loop
  // of the server code. It polls for a report from the tracker and
  // sends it if there is one. It will reset the tracker if there is
  // no data from it for a few seconds.

  virtual void mainloop();
    
 protected:
  
  virtual void get_report(void);
  virtual void reset();

  // Swap the endian-ness of the 4-byte entry in the buffer.
  // This is used to make the little-endian IEEE float values
  // returned by the Fastrak into the big-endian format that is
  // expected by the VRPN unbuffer routines.

  void swap_endian4(char *buffer);

  struct timeval reset_time;
  int	do_filter;	// Should we turn on filtering for pos/orient?
  int	num_stations;	// How many stations maximum on this Fastrak?
  char	add_reset_cmd[1024];	// Additional reset commands to be sent

};

#endif
