/*
 * vrpn_Tracker_GameTrak.cpp
 *
 *  Created on: Nov 22, 2010
 *      Author: janoc
 */

#include <math.h>                       // for cos, sin
#include <stdio.h>                      // for fprintf, stderr
#include <string.h>                     // for memset, memcpy, NULL

#include "quat.h"                       // for Q_PI
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Tracker_GameTrak.h"

vrpn_Tracker_GameTrak::vrpn_Tracker_GameTrak(const char * name, vrpn_Connection * trackercon, const char *joystick_dev, int *mapping) :
    vrpn_Tracker(name, trackercon)
{
    memset(_sensor0, 0, 3 * sizeof(float));
    memset(_sensor1, 0, 3 * sizeof(float));

    memcpy(_mapping, mapping, 6 * sizeof(int));

    // try to open a client connection to the joystick device
    // if the name starts with '*', use the server connection only
    if (joystick_dev[0] == '*') {
      _analog = new vrpn_Analog_Remote(&(joystick_dev[1]), d_connection);
    } else {
      _analog = new vrpn_Analog_Remote(joystick_dev);
    }

    if (_analog == NULL) {
        fprintf(stderr, "vrpn_Tracker_GameTrak: "
            "Can't open joystick %s\n", joystick_dev);
    } else {
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
    /*
     * The spherical->carthesian conversion code is based on the mail from: 
     * Marc LE RENARD lerenard@esiea-ouest.fr, 
     * ESIEA
     * Laboratoire RVSE
     * (Réalité Virtuelle et Systèmes Embarqués)
     * 38 Rue des docteurs Calmette & Guérin
     * 53000 LAVAL
     */

    vrpn_Tracker_GameTrak *gametrak = (vrpn_Tracker_GameTrak *) userdata;

    vrpn_float64 s1x, s1y, s1z;
    vrpn_float64 s2x, s2y, s2z;

    s1x = info.channel[gametrak->_mapping[0]];
    s1y = info.channel[gametrak->_mapping[1]];
    s1z = info.channel[gametrak->_mapping[2]];

    s2x = info.channel[gametrak->_mapping[3]];
    s2y = info.channel[gametrak->_mapping[4]];
    s2z = info.channel[gametrak->_mapping[5]];

    vrpn_float64 coef = (32.5 / 180.0) * Q_PI;

    vrpn_float64 distance0 = 1.5 * (1 - s1z);
    vrpn_float64 angleX0   = -s1x * coef;
    vrpn_float64 angleY0   = -s1y * coef;

    vrpn_float64 distance1 = 1.5 * (1 - s2z);
    vrpn_float64 angleX1   = -s2x * coef;
    vrpn_float64 angleY1   = -s2y * coef;

    // printf("%3.3f %3.3f %3.3f     %3.3f %3.3f %3.3f\n", angleX0, angleY0, distance0, angleX1, angleY1, distance1);

    gametrak->_sensor0[0] = sin(angleX0) * distance0 + 0.065;
    gametrak->_sensor0[1] = cos(angleX0) * sin(angleY0) * distance0;
    gametrak->_sensor0[2] = cos(angleX0) * cos(angleY0) * distance0;

    gametrak->_sensor1[0] = sin(angleX1) * distance1 - 0.065;
    gametrak->_sensor1[1] = cos(angleX1) * sin(angleY1) * distance1;
    gametrak->_sensor1[2] = cos(angleX1) * cos(angleY1) * distance1;

    //printf("    %3.3f %3.3f %3.3f     %3.3f %3.3f %3.3f\n", gametrak->_sensor0[0], gametrak->_sensor0[1],  gametrak->_sensor0[2],
    //                                                    gametrak->_sensor1[0], gametrak->_sensor1[1],  gametrak->_sensor1[2]);

    memcpy(&(gametrak->_timestamp), &(info.msg_time), sizeof(struct timeval));
    gametrak->_should_report = true;
}

