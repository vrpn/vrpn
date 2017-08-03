#ifndef VRPN_3SPACE_H
#define VRPN_3SPACE_H

#include "vrpn_Configure.h"             // for VRPN_API
#include "vrpn_Tracker.h"               // for vrpn_Tracker_Serial

class VRPN_API vrpn_Connection;

class VRPN_API vrpn_Tracker_3Space: public vrpn_Tracker_Serial {
  
 public:
  
  vrpn_Tracker_3Space(char *name, vrpn_Connection *c,
		      const char *port = "/dev/ttyS1", long baud = 19200) :
  vrpn_Tracker_Serial(name,c,port,baud), d_numResets(0) {};
    
 protected:
  
  /// Returns 0 if didn't get a complete report, 1 if it did.
  virtual int get_report(void);

  virtual void reset();
  int d_numResets;

};

#endif
