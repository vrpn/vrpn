// vrpn_Tracker_DeadReckoning.h

// Set of filter classes to perform dead-reckoning prediction on an incoming stream of
// tracker reports and produce an outgoing stream of tracker reports predicted into the
// future by a specified time.

#pragma once
#include <vrpn_Tracker.h>
#include <string>
#include <vector>

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
        , std::string origTrackerName  //< Name of tracker to predict (*Name for one using our connection, Name@URL for one we should connect to
        , vrpn_int32 numSensors = 1    //< How many sensors to predict for?
        , vrpn_float64 predictionTime = 1.0 / 60.0   //< How far to predict into the future?
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
};
