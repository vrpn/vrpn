/*
 * vrpn_Tracker_Wintracker.cpp
 *
 *  Created on: Dec 7, 2012
 *      Author: Emiliano Pastorelli - Institute of Cybernetics, Tallinn (Estonia)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "quat.h"
#include "vrpn_Tracker_Wintracker.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

const unsigned VENDOR_ID = 0x09d9;
const unsigned PRODUCT_ID = 0x64df;

using namespace std;

//WintrackerIII Data length + 1 Byte for HID Report ID (0x0 as the device has no report IDs)

#ifdef VRPN_USE_HID
vrpn_Tracker_Wintracker::vrpn_Tracker_Wintracker(const char * name, vrpn_Connection * trackercon, const char s0,  const char s1,  const char s2, const char ext, const char hemisphere):
vrpn_Tracker(name, trackercon), vrpn_HidInterface(new vrpn_HidProductAcceptor(VENDOR_ID, PRODUCT_ID), VENDOR_ID, PRODUCT_ID)
{
    _name = name;
    _con = trackercon;

    memset(d_quat, 0, 4 * sizeof(float));
    memset(pos, 0, 3 * sizeof(float));

    vrpn_Tracker::num_sensors=3;

    //configure WintrackerIII sensors activation state                    
    const vrpn_uint8 sensor0 = s0;
    const vrpn_uint8 sensor1 = s1;
    const vrpn_uint8 sensor2 = s2;
    const vrpn_uint8 set_sensors_cmd[] = {0x0,'S','A',sensor0,sensor1,sensor2,'\n'};
    //Send sensors activation data to the WintrackerIII device
    send_data(sizeof(set_sensors_cmd),set_sensors_cmd);
	
    cout << "WintrackerIII Vrpn Server up and running..." << endl;
    cout << "Sensors activation state : " << endl;
    cout << "Sensor 0: " << sensor0 << " - " << "Sensor 1: " << sensor1 << " - " << "Sensor 2: " << sensor2 <<endl;

    //if parameter hemisphere is set to Z, change the hemisphere of operation to the upper one (Z<0)
    //by default set to front one (X>0)
    if(hemisphere=='Z'){
    	cout << "Hemisphere of operation : Upper(Z<0)" << endl;
	const vrpn_uint8 set_hemi[] = {0x0,'S','H','U','\n'};
	send_data(sizeof(set_hemi),set_hemi);
    }
    else{
    	cout << "Hemisphere of operation : Front(X>0)" << endl;
    }

    //if parameter ext == 1, activate VRSpace Range Extender
    if(ext=='1'){
    	cout << "Range Extender : Activated" << endl;
	const vrpn_uint8 set_extender_cmd[] = {0x0,'S','L','\n'};
    	send_data(sizeof(set_extender_cmd),set_extender_cmd);
    }
    else{
    	cout << "Range Extender : Not Activated" << endl;
    }

    vrpn_gettimeofday(&_timestamp, NULL);
}


void vrpn_Tracker_Wintracker::on_data_received(size_t bytes, vrpn_uint8 *buff)
{

    if (bytes == 24) {

        vrpn_uint8 recordType = vrpn_unbuffer_from_little_endian<vrpn_int8>(buff);
        //second byte of the buffer, identifying the sensor number
        vrpn_uint8 recordNumber = vrpn_unbuffer_from_little_endian<vrpn_int8>(buff);

        //WintrackerIII identifies the sensors using recordNumber
        //48=sensor 0
        //49=sensor 1
        //50=sensor 2
        if(((int)recordNumber)==48){
        	d_sensor = 0;
        }else if(((int)recordNumber)==49){
        	d_sensor = 1;
        }else if(((int)recordNumber)==50){
        	d_sensor = 2;
        }

        //position data (transformed in meters from WintrackerIII 0.1 mm data
        pos[0]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;
        pos[1]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;
        pos[2]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;

        //azimuth, elevation and roll data, not used by the vrpn server
        vrpn_uint16 azimuth = vrpn_unbuffer_from_little_endian<vrpn_int16>(buff);
        vrpn_uint16 elevation = vrpn_unbuffer_from_little_endian<vrpn_int16>(buff);
        vrpn_uint16 roll = vrpn_unbuffer_from_little_endian<vrpn_int16>(buff);

        //normalized quaternion data (Vrpn Order Qx,Qy,Qz,Qw - WintrackerIII Order Qw,Qx,Qy,Qz)
        d_quat[3]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;
        d_quat[0]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;
        d_quat[1]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;
        d_quat[2]=vrpn_unbuffer_from_little_endian<vrpn_int16>(buff)/10000.00;

        vrpn_gettimeofday(&_timestamp, NULL);
        vrpn_Tracker::timestamp = _timestamp;

        char msgbuf[1000];

        int len = vrpn_Tracker::encode_to(msgbuf);

        if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)){
            fprintf(stderr, "FAIL \n");
        }

        // Use unused variables to avoid compiler warnings.
        recordType = recordType + 1;
        azimuth = azimuth + 1;
        elevation = elevation + 1;
        roll = roll + 1;
    } else {
    	fprintf(stderr, "FAIL : Cannot read input from Wintracker \n");
    }
}


void vrpn_Tracker_Wintracker::mainloop()
{
    if (connected())
    {
        // device update
        update();

        // server update
        server_mainloop();

    }
}

#endif


