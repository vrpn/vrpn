#include "vrpn_Tracker_PhaseSpace.h"

#define MM_TO_METERS (0.001)

#ifdef VRPN_INCLUDE_PHASESPACE
//#define DEBUG

//
inline void frame_to_time(int frame, float frequency, struct timeval &time) {
  float t = frame / frequency;
  time.tv_sec = int(t);
  time.tv_usec = int((t - time.tv_sec)*1E6);
}

//
vrpn_Tracker_PhaseSpace::vrpn_Tracker_PhaseSpace(const char *name, vrpn_Connection *c, const char* device, float frequency,int readflag, int slave)
  : vrpn_Tracker(name,c)
{
#ifdef DEBUG
  printf("%s %s %s %f %d\n", __PRETTY_FUNCTION__, name, device, frequency, readflag);
#endif

  if(d_connection) {
    // Register a handler for the update change callback
    if (register_autodeleted_handler(update_rate_id, handle_update_rate_request, this, d_sender_id))
      fprintf(stderr,"vrpn_Tracker: Can't register workspace handler\n");
  }

  this->slave = slave;
  this->frequency = frequency;

  numRigids = 0;
  numMarkers = 0;
  markers.reserve(VRPN_PHASESPACE_MAXMARKERS);
  rigids.reserve(VRPN_PHASESPACE_MAXRIGIDS);

  int owlflag = 0;
  if(slave) owlflag |= OWL_SLAVE;

  int ret = owlInit(device,owlflag);
  if(ret != owlflag) {
    fprintf(stderr, "owlInit error: 0x%x\n", ret);
    owlRunning = false;
    return;
  } else {
    owlRunning = true;
  }

  char msg[512];
  if(owlGetString(OWL_VERSION,msg)) {
    printf("OWL version: %s\n",msg);
  } else {
    printf("Unable to query OWL version.\n");
  }

  if(!slave) {
    //The master point tracker is index 0.  So all rigid trackers will start from 1.
    owlTrackeri(0, OWL_CREATE, OWL_POINT_TRACKER);
    if(!owlGetStatus()) {
      fprintf(stderr,"Error: Unable to create main point tracker.\n");
      return;
    }
  } else {
    printf("Ignoring tracker creation in slave mode.\n");
  }

  readMostRecent = readflag ? true : false;
  frame = 0;
}

//
vrpn_Tracker_PhaseSpace::~vrpn_Tracker_PhaseSpace()
{
#ifdef DEBUG
  printf("%s\n", __PRETTY_FUNCTION__);
#endif

  if(owlRunning)
    owlDone();
}


// This function should be called each time through the main loop
// of the server code. It polls for data from the OWL server and
// sends them if available.
void vrpn_Tracker_PhaseSpace::mainloop()
{
  get_report();

  // Call the generic server mainloop, since we are a server
  server_mainloop();
  return;
}

//
bool vrpn_Tracker_PhaseSpace::addMarker(int sensor,int led_id)
{
#ifdef DEBUG
  printf("%s %d %d\n", __PRETTY_FUNCTION__, sensor, led_id);
#endif

  if(!owlRunning) return false;
  if(slave) return false;

  if(numMarkers >= VRPN_PHASESPACE_MAXMARKERS) {
    fprintf(stderr, "Error: Maximum markers (%d) exceeded.\n", VRPN_PHASESPACE_MAXMARKERS);
    return false;
  }

  owlMarkeri(MARKER(0,sensor),OWL_SET_LED,led_id);

  if(!owlGetStatus())
    return false;

  numMarkers++;
  return true;
}

/*
This function must only be called after startNewRigidBody has been called.
*/
bool vrpn_Tracker_PhaseSpace::addRigidMarker(int sensor, int led_id, float x, float y, float z)
{
#ifdef DEBUG
  printf("%s %d %d %f %f %f\n", __PRETTY_FUNCTION__, sensor, led_id, x, y, z);
#endif

  if(!owlRunning) return false;
  if(slave) return false;

  if(numRigids == 0) {
    fprintf(stderr, "Error: Attempting to add rigid body marker with no rigid body defined.");
    return false;
  }
  if(numMarkers >= VRPN_PHASESPACE_MAXMARKERS) {
    fprintf(stderr, "Error: Maximum markers (%d) exceeded.\n", VRPN_PHASESPACE_MAXMARKERS);
    return false;
  }

  float xyz[3];
  xyz[0] = x;
  xyz[1] = y;
  xyz[2] = z;

  owlMarkeri(MARKER(numRigids,sensor),OWL_SET_LED,led_id);
  owlMarkerfv(MARKER(numRigids,sensor),OWL_SET_POSITION,&xyz[0]);

  if(!owlGetStatus())
    return false;

  numMarkers++;
  return true;
}

/*
Starts a new rigid body definition and creates a tracker for it on
the server.  Use addRigidMarker() to add markers to the tracker.
Trackers must be disabled (using enableTracker(false)) before being
modified or created.
*/
bool vrpn_Tracker_PhaseSpace::startNewRigidBody(int sensor)
{
#ifdef DEBUG
  printf("%s %d\n", __PRETTY_FUNCTION__, sensor);
#endif

  if(!owlRunning) return false;
  if(slave) return false;

  if(numRigids >= VRPN_PHASESPACE_MAXRIGIDS) {
    fprintf(stderr, "error: maximum rigid bodies (%d) exceeded\n", VRPN_PHASESPACE_MAXRIGIDS);
    return false;
  }

  owlTrackeri(++numRigids, OWL_CREATE, OWL_RIGID_TRACKER);

  if(!owlGetStatus()) {
    numRigids--;
    return false;
  }

  //remember the sensor that this rigid tracker is mapped to.
  r2s_map[numRigids] = sensor;
  return true;
}

/*
Enables all the trackers and sets the streaming frequency.
Note: Trackers according to VRPN and Trackers as defined by OWL
are two entirely different things.
*/
bool vrpn_Tracker_PhaseSpace::enableTracker(bool enable)
{
#ifdef DEBUG
  printf("%s %d\n", __PRETTY_FUNCTION__, enable);
#endif

  if(!owlRunning) return false;

  // Scale the reports for this tracker to be in meters rather than
  // in MM, to match the VRPN standard.
  owlScale(MM_TO_METERS);

  if(!slave) {
    int option = enable ? OWL_ENABLE : OWL_DISABLE;
    owlTracker(0,option); //enables/disables the point tracker
    if(!owlGetStatus())
      return false;

    //enable/disable the rigid trackers
    for(int i = 1; i <= numRigids; i++) {
      owlTracker(i,option);
      if(!owlGetStatus())
        return false;
    }
  }

  if(enable) {
    setFrequency(frequency);
    owlSetInteger(OWL_EVENTS, OWL_ENABLE);
    owlSetInteger(OWL_STREAMING,OWL_ENABLE);
    if(!owlGetStatus())
      return false;
    printf("Streaming enabled.\n");
  } else {
    owlSetInteger(OWL_EVENTS, OWL_DISABLE);
    owlSetInteger(OWL_STREAMING,OWL_DISABLE);
    if(!owlGetStatus())
      return false;
    printf("Streaming disabled.\n");
  }
  return true;
}

int debugcounter = 0;

//
int vrpn_Tracker_PhaseSpace::read_frame(void)
{
  int ret = 0;
  char buffer[1024];
  OWLEvent e = owlGetEvent();
  switch(e.type) {
  case 0:
    break;
  case OWL_DONE:
    owlRunning = false;
    send_text_message("owl stopped\n", timestamp, vrpn_TEXT_ERROR);
    return 0;
  case OWL_FREQUENCY:
    owlGetFloatv(OWL_FREQUENCY, &frequency);
    fprintf(stdout, "frequency: %d\n", frequency);
    break;
  case OWL_BUTTONS: break;
  case OWL_MARKERS:
    markers.resize(VRPN_PHASESPACE_MAXMARKERS);
    ret = owlGetMarkers(&markers.front(), markers.size());
    if(ret > 0) markers.resize(ret);
    break;
  case OWL_RIGIDS:
    rigids.resize(VRPN_PHASESPACE_MAXRIGIDS);
    ret = owlGetRigids(&rigids.front(), rigids.size());
    if(ret > 0) rigids.resize(ret);
    break;
  case OWL_COMMDATA:
    owlGetString(OWL_COMMDATA, buffer);
    break;
  case OWL_TIMESTAMP:
    owlGetIntegerv(OWL_TIMESTAMP, &ret);
    break;
  case OWL_PLANES:
    planes.resize(VRPN_PHASESPACE_MAXPLANES);
    ret = owlGetPlanes(&planes.front(), planes.size());
    if(ret > 0) planes.resize(ret);
    break;
  case OWL_DETECTORS:
    detectors.resize(VRPN_PHASESPACE_MAXDETECTORS);
    ret = owlGetDetectors(&detectors.front(), detectors.size());
    if(ret > 0) detectors.resize(ret);
    break;
  case OWL_PEAKS:
    peaks.resize(VRPN_PHASESPACE_MAXPEAKS);
    ret = owlGetPeaks(&peaks.front(), peaks.size());
    if(ret > 0) peaks.resize(ret);
    break;
  case OWL_IMAGES:
    images.resize(VRPN_PHASESPACE_MAXIMAGES);
    ret = owlGetImages(&images.front(), images.size());
    if(ret > 0) images.resize(ret);
    break;
  case OWL_CAMERAS:
    cameras.resize(VRPN_PHASESPACE_MAXCAMERAS);
    ret = owlGetCameras(&cameras.front(), cameras.size());
    if(ret > 0) cameras.resize(ret);
    break;
  case OWL_STATUS_STRING: break;
  case OWL_CUSTOM_STRING: break;
  case OWL_FRAME_NUMBER:
    if(owlGetIntegerv(OWL_FRAME_NUMBER, &ret))
      frame = ret;
    break;
  default:
    fprintf(stderr, "unsupported OWLEvent type: 0x%x\n", e.type);
    break;
  }
  return e.type;
}

//
int vrpn_Tracker_PhaseSpace::get_report(void)
{
  if(!owlRunning) return 0;

  int maxiter = 1;
  if(readMostRecent) maxiter = 1024; // not technically most recent, but if the client is slow, avoids infinite loop.

  int ret = 1;
  int oldframe = frame;
  for(int i = 0; i < maxiter && ret; i++) {
    int cframe = frame;
    while(ret && cframe == frame) {
      ret = read_frame();
    }
  }
  // no new data? abort.
  if(oldframe == frame)
    return 0;

#ifdef DEBUG
  char buffer[1024];
  owlGetString(OWL_FRAME_BUFFER_SIZE, buffer);
  printf("%s\n", buffer);
#endif

  for(int i = 0; i < markers.size(); i++)
    {
      if(markers[i].cond <= 0) continue;

      //set the sensor
      d_sensor = markers[i].id;

      pos[0] = markers[i].x;
      pos[1] = markers[i].y;
      pos[2] = markers[i].z;

      //raw positions have no rotation
      d_quat[0] = 0;
      d_quat[1] = 0;
      d_quat[2] = 0;
      d_quat[3] = 1;

      // send time out in OWL time
      if(frequency) frame_to_time(frame, frequency, timestamp);
      else memset(&timestamp, 0, sizeof(timestamp));

      //send the report
      send_report();
    }
  for(int j = 0; j < rigids.size(); j++)
    {
      if(rigids[j].cond <= 0) continue;

      //set the sensor
      d_sensor = r2s_map[rigids[j].id];
      if(slave && d_sensor == 0) {
        // rigid bodies aren't allowed to be sensor zero in slave mode
        r2s_map[rigids[j].id] = rigids[j].id;
      }

      //set the position
      pos[0] = rigids[j].pose[0];
      pos[1] = rigids[j].pose[1];
      pos[2] = rigids[j].pose[2];

      //set the orientation quaternion
      //OWL has the scale factor first, whereas VRPN has it last.
      d_quat[0] = rigids[j].pose[4];
      d_quat[1] = rigids[j].pose[5];;
      d_quat[2] = rigids[j].pose[6];;
      d_quat[3] = rigids[j].pose[3];;

      // send time out in OWL time
      if(frequency) frame_to_time(frame, frequency, timestamp);
      else memset(&timestamp, 0, sizeof(timestamp));

      //send the report
      send_report();
    }

  return markers.size() || rigids.size() > 0 ? 1 : 0;
}

//
void vrpn_Tracker_PhaseSpace::send_report(void)
{
  if(d_connection)
    {
      char	msgbuf[VRPN_PHASESPACE_MSGBUFSIZE];
      int	len = encode_to(msgbuf);
      if(d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf,
                                    vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr,"PhaseSpace: cannot write message: tossing\n");
      }
    }
}

//
void vrpn_Tracker_PhaseSpace::setFrequency(float freq)
{
#ifdef DEBUG
  printf("%s %f\n", __PRETTY_FUNCTION__, freq);
#endif
  if(freq < 0 || freq > OWL_MAX_FREQUENCY) {
    fprintf(stderr,"Error:  Invalid frequency requested, defaulting to %f hz\n", OWL_MAX_FREQUENCY);
    freq = OWL_MAX_FREQUENCY;
  }

  frequency = freq;

  if(owlRunning) {
    float tmp = -1;
    owlSetFloat(OWL_FREQUENCY, frequency);
    owlGetFloatv(OWL_FREQUENCY, &tmp);
    if(!owlGetStatus() || tmp == -1)
      fprintf(stderr,"Error: unable to set frequency\n");
  }
  return;
}

//
int vrpn_Tracker_PhaseSpace::handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p)
{
#ifdef DEBUG
  printf("%s\n", __PRETTY_FUNCTION__);
#endif
  vrpn_Tracker_PhaseSpace* thistracker = (vrpn_Tracker_PhaseSpace*)userdata;
  vrpn_float64 update_rate = 0;
  vrpn_unbuffer(&p.buffer,&update_rate);
  thistracker->setFrequency((float) update_rate);
  return 0;
}

#endif
