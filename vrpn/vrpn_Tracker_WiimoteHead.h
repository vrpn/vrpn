#ifndef __TRACKER_WIIMOTEHEAD_H
#define __TRACKER_WIIMOTEHEAD_H

#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"
#include <quat.h>

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#ifndef _WIN32
#include <sys/time.h>
#endif

// The time reported by this tracker is as of the last report it has had
// from the Wiimote, to ensure accurate timing.
class VRPN_API vrpn_Tracker_WiimoteHead : public vrpn_Tracker {
	public:
	vrpn_Tracker_WiimoteHead (const char* name,
				  vrpn_Connection * trackercon,
				  const char* wiimote,
				  float update_rate);

	virtual ~vrpn_Tracker_WiimoteHead (void);

	virtual void mainloop();
	virtual void reset(void);

	void update(q_matrix_type &);

	static int VRPN_CALLBACK handle_newConnection (void*, vrpn_HANDLERPARAM);
	static int VRPN_CALLBACK handle_dropLastConnection (void*, vrpn_HANDLERPARAM);

	long needwiimote_m_id;
	long refreshwiimote_m_id;
	long wmheadtrackserver_s_id;

	protected:

	double          d_update_interval; //< How long to wait between sends
	struct timeval  d_prevtime;     //< Time of the previous report
	double				d_blobDistance;

	std::vector<double> d_vX;
	std::vector<double> d_vY;
	std::vector<double> d_vSize;

	bool d_hasBlob;
	bool d_updated;
	bool d_needWiimote;

	vrpn_Analog_Remote* d_ana;
	const char* d_name;

	q_matrix_type d_initMatrix, d_currentMatrix;

	bool		d_gravDirty;
	q_vec_type	d_vGravAntepenultimate;
	q_vec_type	d_vGravPenultimate;
	q_vec_type	d_vGrav;

	q_type			d_qCorrectGravity;
	q_matrix_type	d_mCorrectGravity;

	bool	register_custom_types();
	void    update_matrix_based_on_values(double time_interval);
	void    convert_matrix_to_tracker(void);

	vrpn_bool shouldReport(double elapsedInterval) const;
	bool haveGravity() const;

	static void VRPN_CALLBACK handle_analog_update(void* userdata, const vrpn_ANALOGCB info);
	static void VRPN_CALLBACK handle_refresh_wiimote(void* userdata, const vrpn_ANALOGCB info);

};

#endif
