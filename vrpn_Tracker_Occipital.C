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

#include "vrpn_Tracker_Occipital.h"

#ifdef VRPN_USE_STRUCTUREPERCEPTIONENGINE

vrpn_Tracker_OccipitalStructureCore::vrpn_Tracker_OccipitalStructureCore(
    const char* name, vrpn_Connection* c) :
    vrpn_Tracker(name, c)
{
    /// @todo
    num_sensors = 0;
    
    // VRPN stuff
    register_server_handlers();
}

vrpn_Tracker_OccipitalStructureCore::~vrpn_Tracker_OccipitalStructureCore()
{
    /// @todo
}

void vrpn_Tracker_OccipitalStructureCore::mainloop()
{
    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // Get latest data
    get_report();
}

void vrpn_Tracker_OccipitalStructureCore::get_report()
{
    vrpn_gettimeofday(&timestamp, NULL);

    /// @todo

    for (int i = 0; i < num_sensors; i++) {
        /// @todo

        // The sensor number
        d_sensor = i;

        // No need to fill in position, as we don´t get position information
        /// @todo Fill in d_pos and d_quat

        // Send the data
        send_report();
    }
}

void vrpn_Tracker_OccipitalStructureCore::send_report()
{
    if (d_connection) {
        char msgbuf[1000];
        int len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf, 
                                       vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr, "Tracker: cannot write message: tossing\n");
		}
    }
}

#endif
