#ifndef __TRACKER_3DMOUSE_H
#define __TRACKER_3DMOUSE_H

#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_API
#include "vrpn_Tracker.h"               // for vrpn_Tracker_Serial

class VRPN_API vrpn_Connection;

class VRPN_API vrpn_Tracker_3DMouse : public vrpn_Tracker_Serial, public vrpn_Button_Filter {
  
 public:

  vrpn_Tracker_3DMouse(const char *name, 
                          vrpn_Connection *c,
                          const char *port = "/dev/ttyS1",
						  long baud = 19200,
						  int filtering_count = 1);

  ~vrpn_Tracker_3DMouse();

  /// Called once through each main loop iteration to handle updates.
  virtual void mainloop();

    
 protected:

  virtual void reset();
  virtual int get_report(void);
  bool set_filtering_count(int count);
  virtual void clear_values(void);

  unsigned char		_buffer[2048];
  int			_filtering_count;
  int			_numbuttons;
  int			_count;
};

#endif
