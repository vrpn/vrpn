/** @file
	@brief Header for a class of trackers that read from one tracker and
        apply a filter to the inputs, sending out filtered outputs under a
        different tracker name.  The initial instance is the one-Euro filter.
        Others include various predictive and combining tracker filters.

	@date 2013

	@author
	Russ Taylor
*/

#pragma once

// Internal Includes
#include "quat.h"                       // for q_vec_type
#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_OneEuroFilter.h"
#include <string>
#include <vector>

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


/// Use angular velocity reports, if available, to predict the future orientation
// of the specified sensors from the specified tracker.  If there are no orientation
// velocity reports, use the two most-recent poses to estimate angular velocity and
// use that to predict.
//  Note: This class does not try to listen for angular acceleration.
//  Note: This class does not try to forward-predict position, it leaves that
// part of the tracker message alone.

class VRPN_API vrpn_Tracker_DeadReckoning_Rotation : public vrpn_Tracker_Server
{
  public:

    vrpn_Tracker_DeadReckoning_Rotation(
        std::string myName      //< Name of the tracking device we're exposing
        , vrpn_Connection *c    //< Connection to use to send reports on
        , std::string origTrackerName  //< Name of tracker to predict (*Name for one using our connection, Name@URL for one we should connect to)
        , vrpn_int32 numSensors = 1    //< How many sensors to predict for?
        , vrpn_float64 predictionTime = 1.0 / 60.0   //< How far to predict into the future?
        , bool estimateVelocity = true //< Should we estimate angular velocity if we don't get it?
                                       //< If false, this is basically just a pass-through filter, but estimating velocity can be choppy.
        );

    ~vrpn_Tracker_DeadReckoning_Rotation();

    // Handle ping/pong requests.
    void mainloop();
    
    /// Test the class to make sure it functions as it should.  Returns 0 on success,
    // prints an error message and returns an integer indicating what happened on failure.
    static int test(void);

  protected:

    vrpn_float64    d_predictionTime;   //< How far ahead to predict rotation
    vrpn_int32      d_numSensors;       //< How many sensors to predict for?
    vrpn_Tracker_Remote *d_origTracker; //< Original tracker we're predicting for

    typedef struct {
        bool d_receivedAngularVelocityReport;   //< If we get these, we don't estimate them
        vrpn_float64 d_rotationAmount[4];       //< How far did we rotate in the specified interval
        double d_rotationInterval;              //< Interval over which we rotated
        vrpn_float64 d_lastPosition[3];         //< What was our last reported position?
        vrpn_float64 d_lastOrientation[4];      //< What was our last orientation?
        struct timeval d_lastReportTime;        //< When did we receive it?
    } RotationState;
    std::vector<RotationState>  d_rotationStates;   //< State of rotation of each sensor.
    
    /// Static callback handler for tracker reports and tracker velocity reports
    static void VRPN_CALLBACK handle_tracker_report(void *userdata,
        const vrpn_TRACKERCB info);
    static void VRPN_CALLBACK handle_tracker_velocity_report(void *userdata,
        const vrpn_TRACKERVELCB info);

    /// Send a prediction based on the time of the new information; date the
    // prediction in the future from the original message by the prediction
    // interval.
    void sendNewPrediction(vrpn_int32 sensor);

    bool d_estimateVelocity;
};

