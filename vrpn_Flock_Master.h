#ifndef FLOCKMASTER_H
#define FLOCKMASTER_H

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"

// only 13 receivers allowed in normal addressing mode
#define MAX_SENSORS 13

#ifndef _WIN32

// This is a class which provides a server for an ascension 
// Flock of Birds tracker.  The server will send out messages
// at whatever rate the flock of bird's stream mode feeds them.
// Default filtering is active.
// The timestamp is the time when the first character was read
// from the serial driver with "read".  No adjustment is currently
// made to this time stamp.

// Adelstein, et al, "Dynamic response of electromagnetic spatial
// displacement trackers", Presence 5(3) found that if all of the filters
// are turned off, and the time required to transport the data to the
// server is ignored, then the latency of the flock is about 2 ms for ori,
// 7.5 ms for pos for the 0-4hz range (ie, when the data is ready to leave
// the flock in stream mode, the pos is 7.5 ms old, and the ori is 2 ms
// old).  Later this data will be combined with the baud rate to provide a
// more accurate time stamp for certain flock modes.

// If this is running on a non-linux system, then the serial port driver
// is probably adding more latency -- see the vrpn README for more info.

class vrpn_Tracker_Flock_Master: public vrpn_Tracker_Serial {
private:
 unsigned long  duration(struct timeval t1, struct timeval t2) ;
 public:
  
  vrpn_Tracker_Flock_Master(char *name, vrpn_Connection *c, int cSensors=1,
		      char *port = "/dev/ttyd3", long baud = 38400);
    
  virtual ~vrpn_Tracker_Flock_Master();
  virtual void mainloop(void);
    
 protected:

  virtual void get_report(void);
  virtual void reset();
  void printError(unsigned char uchErrCode, unsigned char uchExpandedErrCode);
  int checkError();
  int cResets;
  int cSensors;
};

#endif  // #ifndef _WIN32

#endif




