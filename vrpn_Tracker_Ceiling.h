#include "vrpn_Tracker.h"
#include "tracker.h"


class vrpn_Tracker_Ceiling: public vrpn_Tracker {
  
public:
  vrpn_Tracker_Ceiling(char *name,vrpn_Connection *c,
		       int   sensors = 1,
		       float Hz = 60,
		       int   allowthreads = 1,
		       float predict = 0);
  
  virtual void mainloop(void);  // called by the server
  
  void sendTrackerReport(void);  // actually issues the report
  
  void setupLogfile(char *name, int timeout);
  
protected:
  void writeLogfile(void);
  
  Tracker Track;
  TrackerState *logts;
  
  float update_rate;
  float predict;
  
  double interval_time;
  
  int num_sensors;
  
  // log file (for recording data as it is sent out)
  FILE *logfile;
  int logendtime;  // when to quite and write logfile
  int logindex, logmaxindex;  // index into logts array

  // stats for computing rates
  int trkcount, reptcount;
};


