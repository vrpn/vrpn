#ifndef __TRACKER_DTRACK_H
#define __TRACKER_DTRACK_H

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"

// Globals to store DTrack data:

//// from example_without_remote_control.c
#ifdef	VRPN_USE_DTRACK
#include "dtracklib.h"

#define vrpn_DT_MAX_NBODY 10
#define vrpn_DT_MAX_NFLYSTICK 1
#define vrpn_DT_MAX_NMARKER 100

class vrpn_Tracker_DTrack : public vrpn_Tracker, public vrpn_Button, public vrpn_Analog {
  
 public:

  vrpn_Tracker_DTrack(const char *name, 
                          vrpn_Connection *c,
                          int UdpPort, float timeToReachJoy=1.f);

  ~vrpn_Tracker_DTrack();

  /// This function should be called each time through the main loop
  /// of the server code. It polls for a report from the tracker and
  /// sends it if there is one. It will reset the tracker if there is
  /// no data from it for a few seconds.

  virtual void mainloop();
    
 protected:
  
	double butToChannel(double curVal, unsigned char incBut, unsigned char decBut, double dt);
	void dtrackMatrix2vrpnquat(const float* rot, vrpn_float64* quat);

	timeval m_firstTv;
	bool m_first;
	double m_lastTime, m_now; // previous and current time in sec

	double m_x, m_y;
	double m_incPerSec;		// increase joystick channel of m_incPerSec when button on
	double m_cumulatedDt;	// because on a fast cpu dt may be too small

	unsigned long framenr;
	double m_timestamp;

	dtracklib_body_type body[vrpn_DT_MAX_NBODY];
	int nbody, nbodycal;

	dtracklib_flystick_type flystick[vrpn_DT_MAX_NFLYSTICK];
	int nflystick;

	dtracklib_marker_type marker[vrpn_DT_MAX_NMARKER];
	int nmarker;
};

#endif
#endif

