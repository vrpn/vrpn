#ifndef SPACE_H
#define SPACE_H

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"

class vrpn_Tracker_3Space: public vrpn_Tracker_Serial {
  
 public:
  
  vrpn_Tracker_3Space(char *name, vrpn_Connection *c, 
		      char *port = "/dev/ttyS1", long baud = 19200) :
    vrpn_Tracker_Serial(name,c,port,baud) {};
    
  virtual void mainloop(const struct timeval * timeout=NULL);
    
 protected:
  
  virtual void get_report(void);

  virtual void reset();

};

#endif
