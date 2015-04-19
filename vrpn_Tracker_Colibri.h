///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_Colibri.h
//
// Author:      Dmitry Mastykin
//
// Description: VRPN tracker class for Trivisio Colibri device (based on ColibriAPI).
//
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef VRPN_TRACKER_COLIBRI
#define VRPN_TRACKER_COLIBRI

#include "vrpn_Configure.h"   // IWYU pragma: keep

#ifdef VRPN_USE_COLIBRIAPI

#include "vrpn_Tracker.h"
#include "colibri_api.h"

class vrpn_Tracker_Colibri : public vrpn_Tracker {
public:
    // Constructor
    //
    // name:        VRPN tracker name
    //
    // c:           VRPN connection to use
    //
    // Hz:          Update rate in Hertz
    //
    //
    vrpn_Tracker_Colibri(const char* name, vrpn_Connection* c,
                         const char* path, int Hz, bool report_a_w);
    ~vrpn_Tracker_Colibri();

    /// This function should be called each time through the main loop
    /// of the server code. It checks for a report from the tracker and
    /// sends it if there is one.
    virtual void mainloop();

protected:
    virtual void get_report();
    virtual void send_report();

    TCHandle nw; // Network handle
	bool report_a_w;
};

#endif
#endif
