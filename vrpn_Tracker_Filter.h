/** @file
	@brief Header for a class of trackers that read from one tracker and
        apply a filter to the inputs, sending out filtered outputs under a
        different tracker name.  The initial instance is the one-Euro filter


	@date 2013

	@author
	Russ Taylor
*/

#pragma once

// Internal Includes
#include "quat.h"                       // for q_vec_type
#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_OneEuroFilter.h"

/** @brief Tracker filter based on the one-Euro filter by Jan Ciger
	<jan.ciger@reviatech.com>

    This class reads values from the specified tracker, then applies the
    one-Euro filter to its values and reports them as another tracker.
    This lets you filter any device type.  It was originally written to
    be used with the Razer Hydra tracker.
*/

class VRPN_API vrpn_Tracker_FilterOneEuro: public vrpn_Tracker {
  public:
    // name is the name that the filtered reports go out under
    // trackercon is the server connection to use to send filtered reports on
    // listen_tracker_name is the name of the tracker we listen to to filter
    //    If the tracker should use the server connection, then put * in
    //    front of the name.
    // channels tells how many channels from the listening tracker to
    //    filter (reports on other channels are ignored by this tracker)
    // The other parameters are passed to each One-Euro filter.
    vrpn_Tracker_FilterOneEuro(const char * name, vrpn_Connection * trackercon,
      const char *listen_tracker_name,
      unsigned channels, vrpn_float64 vecMinCutoff = 1.15,
      vrpn_float64 vecBeta = 0.5, vrpn_float64 vecDerivativeCutoff = 1.2,
      vrpn_float64 quatMinCutoff = 1.5, vrpn_float64 quatBeta = 0.5,
      vrpn_float64 quatDerivativeCutoff = 1.2);
    ~vrpn_Tracker_FilterOneEuro();

    virtual void mainloop();

  private:
    int  d_channels;                    // How many channels on our tracker?
    vrpn_OneEuroFilterVec *d_filters;   // Set of position filters, one/channel
    vrpn_OneEuroFilterQuat *d_qfilters; // Set of orientation filters, one/channel
    struct timeval  *d_last_report_times;     // Last time of report for each tracker.
    vrpn_Tracker_Remote   *d_listen_tracker;  // Tracker we get our reports from

    // Callback handler to deal with getting messages from the tracker we're
    // listening to.  It filters them and then sends them on.
    static void VRPN_CALLBACK handle_tracker_update(void *userdata, const vrpn_TRACKERCB info);
};

