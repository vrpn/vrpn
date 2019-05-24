///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_Occipital.h
//
// Author:      Russell Taylor
//              ReliaSolve working for Vality
//
// Description: VRPN tracker class for Occipital Structure Core using PerceptionEngine library
//
/////////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef VRPN_TRACKER_OCCIPITAL
#define VRPN_TRACKER_OCCIPITAL

#include "vrpn_Configure.h"   // IWYU pragma: keep

#ifdef VRPN_USE_STRUCTUREPERCEPTIONENGINE

#include "vrpn_Tracker.h" 
#include <ST/XRSession.h>

class vrpn_Tracker_OccipitalStructureCore : public vrpn_Tracker {
public:
    // Constructor
    //
    // name:        VRPN tracker name
    //
    // c:           VRPN connection to use
    //
    // numSensors:  The number of devices to connect to
    //
    // Hz:          Update rate in Hertz
    //
    // bufLen:      The buffer length for reading data. 
    //
    //              From the reference manual:
    //
    // An short buffer (0) ensures minimal delay until the sensor measurement is available at the risk
    // of lost measurements. A long buffer guarantees that no data is dropped, at
    // the same time if data is not read fast enough there is a potential risk of a
    // bufLenfrequency before the measurement becomes available.
    //
    vrpn_Tracker_OccipitalStructureCore(const char* name, vrpn_Connection* c);
    ~vrpn_Tracker_OccipitalStructureCore();

    /// This function should be called each time through the main loop
    /// of the server code. It checks for a report from the tracker and
    /// sends it if there is one.
    virtual void mainloop();

protected:
    void reset();
    void get_report();
    void send_report();
    static int VRPN_CALLBACK handle_update_rate_request(void* userdata,
                                                        vrpn_HANDLERPARAM p);

    ST::XRSession m_session;
    double m_update_rate = 120;
    struct timeval m_last_report = {};
};

#endif
#endif
