///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_ViewPoint.C
//
// Author:      David Borland
//
//				EventLab at the University of Barcelona
//
// Description: VRPN server class for Arrington Research ViewPoint EyeTracker.
//
//              The VRPN server connects to the eye tracker using the VPX_InterApp DLL.  
//              Whatever other control software is being used to connect to the eye tracker
//              (e.g. the ViewPoint software that comes with the tracker) to perform 
//              calibration, etc. should link to the same copy of the DLL, so they can share 
//              information.
//
//              -------------------------------------------------------------------------------
//
//              Tracker:
//
//              The tracker has two sensors, as the ViewPoint can optionally have binocular
//              tracking.  In the case of monocular tracking, only sensor 0 (EYE_A) will have 
//              valid information.  Retrieving smoothed or raw tracking data is controlled by 
//              the smoothedData parameter.
//
//              Position: The (x,y) gaze point in gaze space (smoothed or raw).
//              
//              Rotation: The (x,y) gaze angle as a quaternion (smoothed or raw).
//              
//              Velocity: The x- and y- components of the eye movement velocity in gaze space 
//                        (always smoothed).
//
//              -------------------------------------------------------------------------------
//
//              Analog:
//
//              There are a lot of additional data that can be retrieved from the tracker.
//              These values are always calculated from the smoothed gaze point.  Currently, 
//              the following are sent as analog values, but more can be added as needed.
//              Please see the ViewPoint documentation regarding what other data are available.
//
//              Because each channel needs to be duplicated in the case of a binocular tracker, 
//              the first n/2 values are for EYE_A, and the second n/2 values are for EYE_B.  
//              
//              EYE_A:
//  
//              Channel 0: The pupil aspect ratio, from 0.0 to 1.0.  Can be used to detect 
//                         blinks when it falls below a given threshold.
//
//              Channel 1: The total velocity (magnitude of eye movement velocity).  Can be 
//                         used to detect saccades.
//
//              Channel 2: The fixation seconds (length of time below the velocity criterion
//                         used to detect saccades).  0 if saccade is occurring.
//
//              EYE_B:
//
//              Channels 3-5: See EYE_A.
//
///////////////////////////////////////////////////////////////////////////////////////////////  

#include "vrpn_Tracker_ViewPoint.h"

#ifdef VRPN_USE_VIEWPOINT

#include VRPN_VIEWPOINT_H
#include "quat.h"

vrpn_Tracker_ViewPoint::vrpn_Tracker_ViewPoint(const char* name, vrpn_Connection* c, bool smoothedData) :
    vrpn_Tracker(name, c), vrpn_Analog(name, c), useSmoothedData(smoothedData)
{
	// Check the DLL version
	double version = VPX_GetDLLVersion();
	if (VPX_VersionMismatch(VPX_SDK_VERSION)) {
		fprintf(stderr, "vrpn_Tracker_ViewPoint::vrpn_Tracker_ViewPoint(): Warning, SDK version is %g, while DLL version is %g \n", version, VPX_SDK_VERSION);
	}
	else {
		printf("vrpn_Tracker_ViewPoint::vrpn_Tracker_ViewPoint(): SDK version %g matches DLL version %g \n", version, VPX_SDK_VERSION);
	}

	// Two sensors, one for each eye
	vrpn_Tracker::num_sensors = 2;

	// Currently 3 analog channels per eye
	const int channels_per_eye = 3;

    // Total number of channels is two times the number of channels per eye
	vrpn_Analog::num_channel = channels_per_eye * 2;

    // VRPN stuff
    register_server_handlers();
}

vrpn_Tracker_ViewPoint::~vrpn_Tracker_ViewPoint()
{
}

void vrpn_Tracker_ViewPoint::mainloop()
{
    // Call the server mainloop
	server_mainloop();

	// Get data from the DLL
    get_report();
}

void vrpn_Tracker_ViewPoint::get_report()
{
    // Get a time stamp
    struct timeval current_time;
    vrpn_gettimeofday(&current_time, NULL);

    // Set the time stamp for each device type
    vrpn_Tracker::timestamp = current_time;
    vrpn_Analog::timestamp = current_time;

	// Get tracker and analog data
	get_tracker();
	get_analog();	
}


void vrpn_Tracker_ViewPoint::get_tracker()
{
	// Get information for each eye
	for (int i = 0; i < 2; i++) {
		// The sensor
		d_sensor = i;


		// Which eye?
		VPX_EyeType eye;
		if (d_sensor == 0) eye = EYE_A;
		else eye = EYE_B;


		// Get tracker data from the DLL
		VPX_RealPoint gp, cv, ga;
		if (useSmoothedData) {
			// Use smoothed data, when available
			VPX_GetGazePointSmoothed2(eye, &gp);
			VPX_GetComponentVelocity2(eye, &cv);	// Always smoothed
			VPX_GetGazeAngleSmoothed2(eye, &ga);
		}
		else {
			// Use raw data
			VPX_GetGazePoint2(eye, &gp);
			VPX_GetComponentVelocity2(eye, &cv);	// Always smoothed
			VPX_GetGazeAngle2(eye, &ga);
		}


		// Set the tracker position from the gaze point
		pos[0] = gp.x;
		pos[1] = gp.y;
		pos[2] = 0.0;


		// Set the tracker velocity from the eye velocity
		vel[0] = cv.x;
		vel[1] = cv.y;
		vel[2] = 0.0;


		// Convert the gaze angle to a quaternion
		q_from_euler(d_quat, 0.0, Q_DEG_TO_RAD(ga.y), Q_DEG_TO_RAD(ga.x));


		// Send the data for this eye
		send_report();
	}
}

void vrpn_Tracker_ViewPoint::get_analog()
{
	// Get information for each eye
	for (int i = 0; i < 2; i++) {
		// Which eye?
		VPX_EyeType eye;
		if (i == 0) eye = EYE_A;
		else eye = EYE_B;


		// Analog channel index offset for second eye
		unsigned int eyeOffset = i * vrpn_Analog::num_channel / 2;


		// Get analog information from the DLL
		double ar, tv, fs;
		VPX_GetPupilAspectRatio2(eye, &ar);
		VPX_GetTotalVelocity2(eye, &tv);
		VPX_GetFixationSeconds2(eye, &fs);


		// Set the analog channels
		channel[0 + eyeOffset] = ar;
		channel[1 + eyeOffset] = tv;
		channel[2 + eyeOffset] = fs;
	}


	// Send all analog data
	vrpn_Analog::report_changes();
}


void vrpn_Tracker_ViewPoint::send_report()
{
	// Send tracker data
    if (d_connection) {
        char msgbuf[1000];
		int len = vrpn_Tracker::encode_to(msgbuf);
		if (d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf, 
                                       vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"vrpn_Tracker_ViewPoint: cannot write message: tossing\n");
		}
		len = vrpn_Tracker::encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, vrpn_Tracker::timestamp, velocity_m_id, d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"vrpn_Tracker_ViewPoint: cannot write message: tossing\n");
        }
    }
}

#endif