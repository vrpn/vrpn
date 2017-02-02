#ifndef VRPN_TRACKER_PHASESPACE_H
#define VRPN_TRACKER_PHASESPACE_H

#include "vrpn_Configure.h"   // IWYU pragma: keep

#ifdef  VRPN_INCLUDE_PHASESPACE
#include <string>

#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"

#include "owl.hpp"

class VRPN_API vrpn_Tracker_PhaseSpace : public vrpn_Tracker, public vrpn_Button_Filter, public vrpn_Clipping_Analog_Server {

 public:

  vrpn_Tracker_PhaseSpace(const char *name,
                          vrpn_Connection *c);

  vrpn_Tracker_PhaseSpace(const char *name,
                          vrpn_Connection *c,
                          const char* device,
                          float frequency,
                          int readflag,
                          int slaveflag=0);


  ~vrpn_Tracker_PhaseSpace();

  bool debug;

  // vrpn_Tracker
  virtual void mainloop();
  static int VRPN_CALLBACK handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p);

  // parse a tracker specification file and create PhaseSpace trackers
  bool load(FILE* file);

  // connect to the Impulse system
  bool InitOWL();

  // start streaming
  bool enableStreaming(bool enable);

 protected:
  // vrpn_Tracker
  virtual int get_report(void);
  virtual void send_report(void);

 protected:

  // libowl2
  OWL::Context context;
  std::string device;
  std::string options;

  // 
  bool drop_frames;

  class SensorManager;
  SensorManager* smgr;

 protected:

  bool create_trackers();

  void set_pose(const OWL::Rigid &r);
  void report_marker(vrpn_int32 sensor, const OWL::Marker &m);
  void report_rigid(vrpn_int32 sensor, const OWL::Rigid &r, bool is_stylus=false);
  void report_button(vrpn_int32 sensor, int value);
  void report_button_analog(vrpn_int32 sensor, int value);

};

#endif //VRPN_INCLUDE_PHASESPACE
#endif //VRPN_TRACKER_PHASESPACE_H
