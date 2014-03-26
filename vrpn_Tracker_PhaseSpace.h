#ifndef VRPN_TRACKER_PHASESPACE_H
#define VRPN_TRACKER_PHASESPACE_H

#include "vrpn_Configure.h"   // IWYU pragma: keep

#ifdef  VRPN_INCLUDE_PHASESPACE
#include <map>
#include <vector>

#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"

#include "owl.h"
#include "owl_planes.h"
#include "owl_peaks.h"
#include "owl_images.h"

#define VRPN_PHASESPACE_MAXMARKERS 256
#define VRPN_PHASESPACE_MAXRIGIDS 32
#define VRPN_PHASESPACE_MAXCAMERAS 128
#define VRPN_PHASESPACE_MAXDETECTORS 128
#define VRPN_PHASESPACE_MAXPLANES 256
#define VRPN_PHASESPACE_MAXPEAKS 512
#define VRPN_PHASESPACE_MAXIMAGES 512
#define VRPN_PHASESPACE_MSGBUFSIZE 1024

class VRPN_API vrpn_Tracker_PhaseSpace : public vrpn_Tracker {

public:

  vrpn_Tracker_PhaseSpace(const char *name, 
                          vrpn_Connection *c,
                          const char* device,
                          float frequency,
                          int readflag,
                          int slaveflag=0);


  ~vrpn_Tracker_PhaseSpace();

  virtual void mainloop();

  bool addMarker(int sensor,int led_id);
  bool addRigidMarker(int sensor, int led_id, float x, float y, float z);
  bool startNewRigidBody(int sensor);
  bool enableTracker(bool enable);
  void setFrequency(float freq);

  static int VRPN_CALLBACK handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p);  

protected:

  int numRigids;
  int numMarkers;
  bool owlRunning;
  float frequency;
  bool readMostRecent;
  bool slave;
  int frame;

  typedef std::map<int, vrpn_int32> RigidToSensorMap;
  RigidToSensorMap r2s_map;
  std::vector<OWLMarker> markers;
  std::vector<OWLRigid> rigids;
  std::vector<OWLCamera> cameras;
  std::vector<OWLPlane> planes;
  std::vector<OWLPeak> peaks;
  std::vector<OWLImage> images;
  std::vector<OWLDetectors> detectors;

protected:
  int read_frame(void);
    
  virtual int get_report(void);
  virtual void send_report(void);
};
#endif

#endif
