/*
 * vrpn_Tracker_GameTrak.cpp
 *
 *  Created on: Nov 22, 2010
 *      Author: janoc
 */

#include <string.h>

#include "vrpn_Tracker_GameTrak.h"

vrpn_Tracker_GameTrak::vrpn_Tracker_GameTrak(const char * name, vrpn_Connection * trackercon, const char *joystick_dev, int *mapping) :
    vrpn_Tracker(name, trackercon)
{
    _name = name;
    _con = trackercon;
    _joydev = joystick_dev;

    memset(_sensor0, 0, 3 * sizeof(float));
    memset(_sensor1, 0, 3 * sizeof(float));

    memcpy(_mapping, mapping, 6 * sizeof(int));

    // try to open a client connection to the joystick device
    // if the name starts with '*', use the server connection only
    if (_joydev[0] == '*')
        _analog = new vrpn_Analog_Remote(&(_joydev.c_str()[1]), d_connection);
    else
        _analog = new vrpn_Analog_Remote(_joydev.c_str());

    if (_analog == NULL)
    {
        fprintf(stderr, "vrpn_Tracker_GameTrak: "
            "Can't open joystick %s\n", _joydev.c_str());
    }
    else
    {
        _analog->register_change_handler(this, handle_update);
    }
}

vrpn_Tracker_GameTrak::~vrpn_Tracker_GameTrak()
{
    // TODO: Auto-generated destructor stub
}

void vrpn_Tracker_GameTrak::mainloop()
{
    if (!_analog)
        return;

    // server update
    server_mainloop();

    // master joystick update
    _analog->mainloop();

    if (_should_report)
    {
        vrpn_Tracker::timestamp = _timestamp;

        // send tracker orientation
        memset(d_quat, 0, sizeof(vrpn_float64) * 4); // no position
        d_quat[3] = 1;

        d_sensor = 0;
        pos[0] = _sensor0[0];
        pos[1] = _sensor0[1];
        pos[2] = _sensor0[2];

        char msgbuf[1000];
        int len = vrpn_Tracker::encode_to(msgbuf);
        if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY))
        {
            fprintf(stderr, "GameTrak tracker: can't write message: tossing\n");
        }

        d_sensor = 1;
        pos[0] = _sensor1[0];
        pos[1] = _sensor1[1];
        pos[2] = _sensor1[2];

        len = vrpn_Tracker::encode_to(msgbuf);
        if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY))
        {
            fprintf(stderr,"GameTrak tracker: can't write message: tossing\n");
        }

        _should_report = false;
    }
}

void VRPN_CALLBACK vrpn_Tracker_GameTrak::handle_update(void *userdata, const vrpn_ANALOGCB info)
{
    // Code based on the comment by Gilles Pinault (gilles[AT]softkinetic.com) at:
    // http://cb.nowan.net/blog/2006/09/25/gametrak-a-first-impression/
    // This calculation makes a left handed y-up coordinate system.
    // This will be fixed at the end.

    vrpn_Tracker_GameTrak *gametrak = (vrpn_Tracker_GameTrak *) userdata;

    vrpn_float64 s1x, s1y, s1z;
    vrpn_float64 s2x, s2y, s2z;

    s1x = info.channel[gametrak->_mapping[0]];
    s1y = info.channel[gametrak->_mapping[1]];
    s1z = info.channel[gametrak->_mapping[2]];

    s2x = info.channel[gametrak->_mapping[3]];
    s2y = info.channel[gametrak->_mapping[4]];
    s2z = info.channel[gametrak->_mapping[5]];

    vrpn_float64 coeffDist = 0.7258; // to have the distance in meters
    vrpn_float64 calibDist = 1.56; // re-centering the distance to the middle of the cord

    // calculate distance from the base, with 0 in the middle of the cord
    vrpn_float64 d1 = calibDist - (s1z / coeffDist);
    vrpn_float64 d2 = calibDist - (s2z / coeffDist);

    // to be able calculate angles in radians -> <-0.5, 0.5>
    vrpn_float64 angle1x = s1x / 2.0;
    vrpn_float64 angle1y = s1y / 2.0;
    vrpn_float64 angle2x = s2x / 2.0;
    vrpn_float64 angle2y = s2y / 2.0;

    // sensor 0 (left one)
    vrpn_float64 tanA1 = tan(angle1x);
    vrpn_float64 tanB1 = tan(angle1y);

    s1y = sqrt((d1 * d1) / (1 + tanA1 * tanA1 + tanB1 * tanB1));
    s1x = s1y * tanA1 - 0.065; // 6.5cm offset from the center of the GameTrak base
    s1z = (s1y * tanB1);

    // sensor 1 (right one)
    vrpn_float64 tanA2 = tan(angle2x);
    vrpn_float64 tanB2 = tan(angle2y);

    s2y = sqrt((d2 * d2) / (1 + tanA2 * tanA2 + tanB2 * tanB2));
    s2x = s2y * tanA2 + 0.065; // 6.5cm offset from the center of the GameTrak base
    s2z = (s2y * tanB2);

    // copy sensor data and make it right-handed, z-up
    gametrak->_sensor0[0] = -s1x;
    gametrak->_sensor0[1] = s1z;
    gametrak->_sensor0[2] = s1y;
    gametrak->_sensor1[0] = -s2x;
    gametrak->_sensor1[1] = s2z;
    gametrak->_sensor1[2] = s2y;
    memcpy(&(gametrak->_timestamp), &(info.msg_time), sizeof(struct timeval));
    gametrak->_should_report = true;
}
