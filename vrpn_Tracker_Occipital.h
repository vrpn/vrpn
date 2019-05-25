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
    // updateRate:          Update rate in Hertz
    //
    vrpn_Tracker_OccipitalStructureCore(const char* name, vrpn_Connection* c,
      float updateRate = 120);
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
    double m_update_rate;
    struct timeval m_last_report = {};
};

#endif
#endif
