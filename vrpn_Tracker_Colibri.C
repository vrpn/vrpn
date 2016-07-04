///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_Colibri.C
//
// Author:      Dmitry Mastykin
//
// Description: VRPN tracker class for Trivisio Colibri device (based on ColibriAPI).
//
///////////////////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <math.h>
#include "quat.h"
#include "vrpn_Tracker_Colibri.h"

#ifdef VRPN_USE_COLIBRIAPI

vrpn_Tracker_Colibri::vrpn_Tracker_Colibri(const char* name, vrpn_Connection* c,
                                           const char* path, int Hz, bool report_a_w)
                    : vrpn_Tracker(name, c), nw(NULL), report_a_w(report_a_w)
{
    TCResult result;
    printf("ColibriAPI version: %s\n", TC_APIVersion());

    if (TC_Initialize(colibri_api_strings) != TCR_SUCCESS) {
        printf("Initialize ColibriAPI failed!\n");
        exit(EXIT_FAILURE);
    }

    if (!path) // No configuration file
        result = TC_NewFromHardware(&nw, TCO_NEW_WIRED | TCO_NEW_WIRELESS);
    else {
        TC_New(&nw, 0);
        result = TC_LoadConf(nw, path);
        if (result == TCR_SUCCESS)
            result = TC_New(&nw, TCO_NEW_WIRED | TCO_NEW_WIRELESS);
    }

    bool ok;
    for (ok = false; !ok; ok = true) {
        if (result == TCR_FILE) {
            printf("File %s not found or bad format.\n", path);
            break;
        }

        // Query the number of connected devices
        num_sensors = TC_GetNodeInfo(nw, 0, NULL);
        printf("%d Colibri sensors found\n", num_sensors);

        if (result != TCR_SUCCESS) {
            printf("%s not found (error %d).\n", TC_GetFailedID(nw)[0]?TC_GetFailedID(nw):"Device", result);
            if (result == TCR_IO_REMOTE) {
                printf("Dongle will scan for wireless Colibri next 20 seconds...\n"
                       "If sensor is off - just turn it on; if you think it's on another channel: "
                       "put it in scanning mode (push button for 2 seconds, LED will flash fast).\n"
                       "Then restart server.\n");
                const char (*list)[8];
                unsigned int nodes_found;
                int gates = TC_GetGateInfo(nw, 0, NULL);
                for (int j = 0; j < gates; ++j)
                    TC_Discover(nw, j, &list, &nodes_found);
            }
            break;
        }

        TC_SaveConf(nw, "./current.conf");
        for (int i = 0; i < num_sensors; ++i) {
            TCInfo info;
            TC_GetNodeInfo(nw, i, &info);
            printf("%d: %s (FW %d.%.3d)\n", i, info.ID, info.FW_ver/1000, info.FW_ver%1000);
        }

        if (!path) { // No configuration file
            result = TC_QuickConfig(nw, TCS_ORIENT | (report_a_w ? TCS_ACC|TCS_GYR : 0));
            if (result != TCR_SUCCESS) {
                printf("Colibri sensor error.\n");
                break;
            }
        }

        // Start the devices
		vel_quat_dt = 1.f/Hz;
        result = TC_Run(nw, Hz);
        if (result == TCR_SUCCESS) {
            printf("Network started...\n");
        } else {
            printf("Failed to start network.\n");
            break;
        }
    }

    if (!ok) {
        TC_Close(nw);
        exit(EXIT_FAILURE);
    }

    // VRPN stuff
    register_server_handlers();
}

vrpn_Tracker_Colibri::~vrpn_Tracker_Colibri()
{
    if (!nw)
        return;

    if (TCR_SUCCESS == TC_Stop(nw))
        printf("Network stopped.\n");
    else
        printf("Failed to stop network.\n");

    TC_Close(nw);
}

void vrpn_Tracker_Colibri::mainloop()
{
    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // Get latest data
    get_report();
}

void vrpn_Tracker_Colibri::get_report()
{
    //vrpn_gettimeofday(&timestamp, NULL);

    const TCSample **data;
    while (TCR_SUCCESS == TC_GetNextData(nw, &data)) {
        for (int i = 0; i < num_sensors; ++i) {
            if (data[i]) {
                // The sensor number
                d_sensor = i;

                // Time of the report
                long tts, ttu;
                tts = data[i]->t / 10000;
                ttu = (data[i]->t - tts * 10000) * 100;
                timestamp.tv_sec = tts;
                timestamp.tv_usec = ttu;

                // No need to fill in position, as we don't get position information

                // Orientation of the sensor
                d_quat[Q_X] = data[i]->q[1];
                d_quat[Q_Y] = data[i]->q[2];
                d_quat[Q_Z] = data[i]->q[3];
                d_quat[Q_W] = data[i]->q[0];
				q_conjugate(d_quat, d_quat); // VRPN defines a right-handed coordinate system

                if (report_a_w) {
                    // Acceleration of the sensor
                    acc[0] = data[i]->a[0];
                    acc[1] = data[i]->a[1];
                    acc[2] = data[i]->a[2];

                    // Delta Orientation of the sensor
                    const vrpn_float64 x = data[i]->g[0] * vel_quat_dt;
                    const vrpn_float64 y = data[i]->g[1] * vel_quat_dt;
                    const vrpn_float64 z = data[i]->g[2] * vel_quat_dt;
                    const vrpn_float64 angle = sqrt(x*x + y*y + z*z);  //module of angular velocity
                    if (angle > 0.0) {
                        vel_quat[Q_X] = x*sin(angle/2.0f)/angle;
                        vel_quat[Q_Y] = y*sin(angle/2.0f)/angle;
                        vel_quat[Q_Z] = z*sin(angle/2.0f)/angle;
                        vel_quat[Q_W] = cos(angle/2.0f);
                    } else { //to avoid illegal expressions
                        vel_quat[Q_X] = vel_quat[Q_Y] = vel_quat[Q_Z] = 0.0f;
                        vel_quat[Q_W] = 1.0f;
                    }
                }

                // Send the data
                send_report();
            }
        }
    }
}

void vrpn_Tracker_Colibri::send_report()
{
    if (d_connection) {
        char msgbuf[1000];

        int len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY))
            fprintf(stderr, "Tracker: cannot write message: tossing\n");

        if (report_a_w) {
            len = encode_acc_to(msgbuf);
            if (d_connection->pack_message(len, timestamp, accel_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY))
                fprintf(stderr,"Tracker: cannot write message: tossing\n");

            len = encode_vel_to(msgbuf);
            if (d_connection->pack_message(len, timestamp, velocity_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY))
                fprintf(stderr,"Tracker: cannot write message: tossing\n");
        }
    }
}

#endif //VRPN_USE_COLIBRIAPI

