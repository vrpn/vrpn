#ifndef __TRACKER_WIIMOTEHEAD_H
#define __TRACKER_WIIMOTEHEAD_H

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"

#include <quat.h>

// This parameter is passed to the constructor for the AnalogFly; it describes
// the wiimote device (vrpn_Wiimote) to get IR from.

class VRPN_API vrpn_TWH_wiimote {
	public:

	vrpn_TWH_wiimote (void)
	{ name = NULL; };

	char * name;	//< Name of the Analog device corresponding to this Wiimote
};

class VPRN_API vrpn_Tracker_WiimoteHead; // forward reference
class VRPN_API vrpn_TWH_blob {
	public:

	vrpn_TWH_blob (void)
	{ name = NULL; first_channel = NULL; ana = NULL; wh = NULL; };

	char * name;	//< Name of the Analog device corresponding to this Wiimote
	int first_channel;	//< Which channel to start at from the Analog device

	vrpn_Analog_Remote * ana;
	vrpn_Tracker_WiimoteHead* wh;

	double x;
	double y;
	double size;
};

// The time reported by absolute trackers is as of the last report they have
// had from their analog devices.  The time reported by differential trackers
// is the local time that the report was generated.  This is to allow a
// gen-locked camera tracker to have its time values passed forward through
// the AnalogFly class.

// If reportChanges is TRUE, updates are ONLY sent if there has been a
// change since the last update, in which case they are generated no faster
// than update_rate.

class VRPN_API vrpn_Tracker_WiimoteHead : public vrpn_Tracker {
	public:
	vrpn_Tracker_WiimoteHead (const char* name,
				  vrpn_Connection * trackercon,
				  vrpn_TWH_wiimote * wiimote,
				  float update_rate,
				  vrpn_bool absolute = vrpn_TRUE,
				  vrpn_bool reportChanges = VRPN_FALSE);

	virtual ~vrpn_Tracker_WiimoteHead (void);

	virtual void mainloop();
	virtual void reset(void);

	void update(q_matrix_type &);

	static void VRPN_CALLBACK handle_joystick(void*, const vrpn_ANALOGCB);
	static int VRPN_CALLBACK handle_newConnection (void*, vrpn_HANDLERPARAM);

	protected:

	double          d_update_interval; //< How long to wait between sends
	struct timeval  d_prevtime;     //< Time of the previous report
	vrpn_bool       d_absolute;     //< Report absolute (vs. differential)?
	vrpn_bool       d_reportChanges;

	vrpn_TWH_blob   d_blobs[4];

	q_matrix_type d_initMatrix, d_currentMatrix;

	void    update_matrix_based_on_values(double time_interval);
	void    convert_matrix_to_tracker(void);

	vrpn_bool shouldReport(double elapsedInterval) const;

	int setup_blob(vrpn_TWH_blob* blob);
	int teardown_blob(vrpn_TWH_blob* blob);

	static void VRPN_CALLBACK handle_analog_update(void* userdata, const vrpn_ANALOGCB info);
};

#endif
