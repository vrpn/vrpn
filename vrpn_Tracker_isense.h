#ifndef __TRACKER_ISENSE_H
#define __TRACKER_ISENSE_H

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"
#ifdef  VRPN_INCLUDE_INTERSENSE
#ifdef __APPLE__
#define MACOSX
#endif
#include "isense.h"
#endif

class vrpn_Tracker_InterSense : public vrpn_Tracker {
  
 public:

  vrpn_Tracker_InterSense(const char *name, 
                          vrpn_Connection *c,
                          int commPort);

  ~vrpn_Tracker_InterSense();


  /// This function should be called each time through the main loop
  /// of the server code. It polls for a report from the tracker and
  /// sends it if there is one. It will reset the tracker if there is
  /// no data from it for a few seconds.

  virtual void mainloop();
    
 protected:
  
  virtual void get_report(void);
  virtual void reset();

  int m_CommPort;
#ifdef VRPN_INCLUDE_INTERSENSE
  ISD_TRACKER_HANDLE m_Handle;
  ISD_TRACKER_INFO_TYPE m_TrackerInfo;
  ISD_STATION_INFO_TYPE m_StationInfo[ISD_MAX_STATIONS];
#endif
};


#endif
