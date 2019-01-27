///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_TrivisioColibri.C
//
// Author:      David Borland
//              Institut d'Investigacions Biomèdiques August Pi i Sunyer (IDIBAPS)
//              Virtual Embodiment and Robotic Re-Embodiment (VERE) Project – 257695
//
// Description: VRPN tracker class for Trivisio Colibri device.
//
/////////////////////////////////////////////////////////////////////////////////////////////// 

#include "vrpn_Tracker_TrivisioColibri.h"

#ifdef VRPN_USE_TRIVISIOCOLIBRI

// XXX: Some horrible hackery here...
//
// The TravisioTypes.h header file has an #if WIN32 test.  This is problematic, because
// WIN32 is not guaranteed to be defined to a value of 1 on Windows.  Thus, this test can fail
// inadvertently.  For example, #define WIN32 or #define WIN32 WIN32 will both produce 
// unwanted results, but for different reasons.  #define WIN32 will cause the #if WIN32 
// test to fail because WIN32 has not been assigned a value.  #define WIN32 WIN32 will cause
// #if WIN32 to produce a compiler error because WIN32 does not have an integer value.
//
// The correct thing to do would be to fix TrivisioTypes.h to use #ifdef _WIN32, as _WIN32 is
// always defined on Windows.  However, so that users don't have to manually edit the 
// Trivisio SDK source files, we redefine WIN32 to have a value of 1, include the Trivisio
// headers, and then redefine WIN32 with no value, which is what windows.h does...
//
#ifdef _WIN32
#   ifdef WIN32
#       undef WIN32
#   endif
#   define WIN32 1
#   include VRPN_TRIVISIOCOLIBRI_H
#   undef WIN32
#   define WIN32
#else
#   include VRPN_TRIVISIOCOLIBRI_H
#endif

vrpn_Tracker_TrivisioColibri::vrpn_Tracker_TrivisioColibri(const char* name, vrpn_Connection* c,
                                                           int numSensors, int Hz, int bufLen) :
    vrpn_Tracker(name, c)
{
    // Query the number of connected devices
    struct TrivisioSensor* sensorList;
    try { sensorList = new TrivisioSensor[numSensors]; }
    catch (...) {
      printf("vrpn_Tracker_TrivisioColibri::vrpn_Tracker_TrivisioColibri: Out of memory\n");
      status = vrpn_TRACKER_FAIL;
      return;
    }
    int sensorCount = colibriGetDeviceList(sensorList, numSensors);

    // If sensorCount == -1, there are more sensors connected than we specified, which is fine.
    // Else, sensorCount == the number of sensors connected, which is all we can use.
    if (sensorCount < 0) {
        num_sensors = numSensors;
    }
    else {
        num_sensors = sensorCount;
    }


    // Print info   
    if (num_sensors < 1) {
        printf("Warning: No Colibri sensors found\n");
        status = vrpn_TRACKER_FAIL;
        return;
    }

    printf("Using %d Colibri sensors\n", num_sensors);
    for (int i = 0; i < num_sensors; i++) {
		printf("%s:\t %s (FW %d.%d)\n", sensorList[i].dev, sensorList[i].ID,
		                                sensorList[i].FWver, sensorList[i].FWsubver);
    }
	printf("\n\n");


    // From the sample code:
    //
    // Diagonal matrices with diagonal element .68 yields approx 20Hz
	// bandwidth @ 100Hz
    //
	float Ka[9] = { 0.68f,    0.00f,    0.00f,
	                0.00f,    0.68f,    0.00f,
	                0.00f,    0.00f,    0.68f };
	float Kg[9] = { 0.68f,    0.00f,    0.00f,
	                0.00f,    0.68f,    0.00f,
	                0.00f,    0.00f,    0.68f };


    // Create the array of device handles
    try { imu = new void*[num_sensors]; }
    catch (...) {
      printf("vrpn_Tracker_TrivisioColibri::vrpn_Tracker_TrivisioColibri: Out of memory\n");
      status = vrpn_TRACKER_FAIL;
      return;
    }
    
    // Create device handles and configure them
    for (int i = 0; i < num_sensors; i++) {
        imu[i] = colibriCreate(bufLen);

        if (colibriOpen(imu[i], 0, sensorList[i].dev) < 0) {
            printf("Warning: Could not access Colibri device on %s\n", sensorList[i]);
            continue;
        }

        struct ColibriConfig conf;

        // Taken from sample code
        colibriGetConfig(imu[i], &conf);
	    conf.raw = 0;
	    conf.freq = Hz;
        conf.sensor = (ColibriConfig::Sensor)1023;
	    conf.ascii = 0;
	    colibriSetConfig(imu[i], &conf);

	    colibriSetKa(imu[i], Ka);
	    colibriSetKaStatus(imu[i], 1);
	    colibriSetKg(imu[i], Kg);
	    colibriSetKgStatus(imu[i], 1);
	    colibriSetJitterStatus(imu[i], 1);

        // Print info
        char id[8];

        printf("Colibri IMU %d\n", i);
	    colibriGetID(imu, id);
	    printf("\tDevice ID:        %s\n", id);
	    printf("\tSensor config:    %d\n", conf.sensor);
	    printf("\tMagnetic div:     %d\n", (unsigned)conf.magDiv);
	    printf("\tFrequency:        %d\n", conf.freq);
	    printf("\tASCII output:     %d\n", conf.ascii);
	    printf("\tAutoStart:        %d\n", conf.autoStart);
	    printf("\tRAW mode:         %d\n", conf.raw);
	    printf("\tJitter reduction: %d\n", colibriGetJitterStatus(imu[i]));
	    printf("\n\n");
    }


    // Start the devices
    for (int i = 0; i < num_sensors; i++) {
        colibriStart(imu[i]);
    }

    
    // VRPN stuff
    register_server_handlers();
}

vrpn_Tracker_TrivisioColibri::~vrpn_Tracker_TrivisioColibri()
{
    for (int i = 0; i < num_sensors; i++) {
        colibriStop(imu[i]);
        colibriClose(imu[i]);
    }

    try {
      delete[] imu;
    } catch (...) {
      fprintf(stderr, "vrpn_Tracker_TrivisioColibri::~vrpn_Tracker_TrivisioColibri(): delete failed\n");
      return;
    }
}

void vrpn_Tracker_TrivisioColibri::mainloop()
{
    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // Get latest data
    get_report();
}

void vrpn_Tracker_TrivisioColibri::get_report()
{
    vrpn_gettimeofday(&timestamp, NULL);

    for (int i = 0; i < num_sensors; i++) {
        TrivisioIMUData data;
        colibriGetData(imu[i], &data);

        // The sensor number
        d_sensor = i;

        // No need to fill in position, as we don´t get position information

        // The orientation
        d_quat[0] = data.q_x;
        d_quat[1] = data.q_y;
        d_quat[2] = data.q_z;
        d_quat[3] = data.q_w;

        // Send the data
        send_report();
    }
}

void vrpn_Tracker_TrivisioColibri::send_report()
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