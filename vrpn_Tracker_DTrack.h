// vrpn_Tracker_DTrack.h
//
// Advanced Realtime Tracking GmbH's (http://www.ar-tracking.de) DTrack client
//
// developed by David Nahon for Virtools VR Pack (http://www.virtools.com)
// improved by Advanced Realtime Tracking GmbH (http://www.ar-tracking.de)

#ifndef VRPN_TRACKER_DTRACK_H
#define VRPN_TRACKER_DTRACK_H

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
	#include <sys/time.h>
#endif

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"


// Globals for storing DTrack data:

#define vrpn_DTRACK_MAX_NBODY 20
#define vrpn_DTRACK_MAX_NFLYSTICK 4


typedef struct{
	unsigned long id;     // id number
	float loc[3];         // location (in mm)
	float rot[9];         // rotation matrix (column-wise)
} dtrack_body_type;

// DTrack flystick data (6d + buttons):

typedef struct{
	unsigned long id;     // id number
	float quality;        // quality (0 <= qu <= 1, no tracking if -1)
	unsigned long bt;     // pressed buttons (binary coded)
	float loc[3];         // location (in mm)
	float rot[9];         // rotation matrix (column-wise)
} dtrack_flystick_type;


// --------------------------------------------------------------------------

class VRPN_API vrpn_Tracker_DTrack : public vrpn_Tracker, public vrpn_Button, public vrpn_Analog
{
  
 public:

// Constructor:
// name (i): device name
// c (i): vrpn_Connection
// dtrackPort (i): DTrack udp port
// timeToReachJoy (i): time needed to reach the maximum value of the joystick
// fixNbody, fixNflystick (i): fixed numbers of DTrack bodies and flysticks (-1 if not wanted)
// fixId (i): renumbering of targets; must have exact (fixNbody + fixNflystick) elements (NULL if not wanted)
// actTracing (i): activate trace output

	vrpn_Tracker_DTrack(const char *name, vrpn_Connection *c,
	                    unsigned short dtrackPort, float timeToReachJoy = 1.f,
	                    int fixNbody = -1, int fixNflystick = -1, unsigned long* fixId = NULL,
	                    bool actTracing = false);

	~vrpn_Tracker_DTrack();

	/// This function should be called each time through the main loop
	/// of the server code. It checks for a report from the tracker and
	/// sends it if there is one.

	virtual void mainloop();


 private:

	// general:
	
	struct timeval tim_first;      // timestamp of first frame
	struct timeval tim_last;       // timestamp of last frame
	
	bool tracing;			          // activate debug output
	unsigned long tracing_frames;  // frame counter for debug output

	// DTrack data:

	int fix_nbody;                 // fixed number of bodies (or -1)
	int fix_nflystick;             // fixed number of flysticks (or -1)

	unsigned long fix_idbody[vrpn_DTRACK_MAX_NBODY + vrpn_DTRACK_MAX_NFLYSTICK];  // fixed vrpn body IDs
	unsigned long fix_idflystick[vrpn_DTRACK_MAX_NFLYSTICK];                      // fixed vrpn flystick IDs

	int warning_nbodycal;          // already warned cause of missing '6dcal' data

	int max_nbody;                 // max. number of tracked bodies
	int max_nflystick;             // max. number of tracked flysticks

	dtrack_body_type dtr_body[vrpn_DTRACK_MAX_NBODY];              // temporary
	dtrack_flystick_type dtr_flystick[vrpn_DTRACK_MAX_NFLYSTICK];  // temporary

	// preparing data for VRPN:
	// these functions convert DTrack data to vrpn data

	double joy_x[vrpn_DTRACK_MAX_NFLYSTICK];  // current value of 'joystick' channel (hor)
	double joy_y[vrpn_DTRACK_MAX_NFLYSTICK];  // current value of 'joystick' channel (ver)
	double joy_incPerSec;                     // increase of 'joystick' channel (in 1/sec)
	
	int dtrack2vrpn_body(unsigned long id, char* str_dtrack, unsigned long id_dtrack,
	                     float* loc, float* rot, struct timeval timestamp);
	int dtrack2vrpn_flystickbuttons(unsigned long id,  unsigned long id_dtrack,
	                                unsigned long but, double dt, struct timeval timestamp);
	double dtrack2vrpn_butToChannel(double curVal,
                          unsigned char incBut, unsigned char decBut, double dt);

	// communicating with DTrack:
	// these functions receive and parse data packets from DTrack

	int udpsock;          // socket number for udp
	char* udpbuf;         // udp buffer

	int dtrack_init(unsigned short dtrack_port);
	int dtrack_exit(void);
	int dtrack_receive(unsigned long* framenr,
		int* nbodycal, int* nbody, dtrack_body_type* body, int max_nbody,
		int* nflystick, dtrack_flystick_type* flystick, int max_nflystick
	);
};

#endif

