/*
 * vrpn_Tracker_SpacePoint.h
 *
 *  Created on: Nov 22, 2010
 *      Author: janoc
 */

#ifndef VRPN_TRACKER_SPACEPOINT_H_
#define VRPN_TRACKER_SPACEPOINT_H_

#include <stddef.h>                     // for size_t

#include "vrpn_Button.h"                // for vrpn_Button
#include "vrpn_Configure.h"             // for VRPN_API, VRPN_USE_HID
#include "vrpn_HumanInterface.h"        // for vrpn_HidInterface
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_Types.h"                 // for vrpn_uint8

class VRPN_API vrpn_Connection;

#ifdef VRPN_USE_HID

class VRPN_API vrpn_Tracker_SpacePoint: public vrpn_Tracker, vrpn_Button, vrpn_HidInterface
{
    public:
        vrpn_Tracker_SpacePoint(const char * name, vrpn_Connection * trackercon, int index = 0);

        virtual void mainloop ();

        virtual void on_data_received(size_t bytes, vrpn_uint8 *buffer);

    protected:
        bool _should_report;
        struct timeval _timestamp;
};

#endif

#endif /* VRPN_TRACKER_SPACEPOINT_H_ */
