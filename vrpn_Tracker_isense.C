
#include "vrpn_Tracker_isense.h"

#ifdef  VRPN_INCLUDE_INTERSENSE
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef linux
#include <termios.h>
#endif

#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR
#include "quat.h" 
#include "isense.c"

#define MAX_TIME_INTERVAL       (5000000) // max time between reports (usec)

void vrpn_Tracker_InterSense::getTrackerInfo(char *msg)
{
	sprintf(msg, "Port%d (Intersense lib %g) (Firmware Rev %g)", m_TrackerInfo.Port, m_TrackerInfo.LibVersion, m_TrackerInfo.FirmwareRev);
	switch(m_TrackerInfo.TrackerType) {
		case ISD_NONE:
			sprintf(msg, "%s (Unknown series:", msg);
			break;
		case ISD_PRECISION_SERIES:
			sprintf(msg, "%s (Precision series:", msg);
			break;
		case ISD_INTERTRAX_SERIES:
			sprintf(msg, "%s (InterTrax series:", msg);
			break;
	}
	switch(m_TrackerInfo.TrackerModel) {
		case ISD_UNKNOWN: 
			sprintf(msg, "%s Unknown model)", msg);
			break;

		case ISD_IS300:
			sprintf(msg, "%s IS300)", msg);
			break;

		case ISD_IS600:
			sprintf(msg, "%s IS600)", msg);
			break;

		case ISD_IS900:
			sprintf(msg, "%s IS900)", msg);
			break;

		case ISD_IS1200:
			sprintf(msg, "%s IS1200)", msg);
			break;

		case ISD_INTERTRAX:
			sprintf(msg, "%s InterTrax)", msg);
			break;

		case ISD_INTERTRAX_2:
			sprintf(msg, "%s InterTrax2)", msg);
			break;

		case ISD_INTERTRAX_LS:
			sprintf(msg, "%s InterTrax LS)", msg);
			break;

		case ISD_INTERTRAX_LC:
			sprintf(msg, "%s InterTrax LC)", msg);
			break;

		case ISD_ICUBE2:
			sprintf(msg, "%s InertiaCube2)", msg);
			break;

		case ISD_ICUBE2_PRO:
			sprintf(msg, "%s InertiaCube2 Pro)", msg);
			break;

		case ISD_ICUBE3:
			sprintf(msg, "%s InertiaCube3)", msg);
			break;
	}
	sprintf(msg, "%s\n", msg);
}

vrpn_Tracker_InterSense::vrpn_Tracker_InterSense(const char *name, 
                                         vrpn_Connection *c,
                                         int commPort, const char *additional_reset_commands, 
										 int is900_timestamps, int reset_at_start) :
vrpn_Tracker(name,c), do_is900_timestamps(is900_timestamps), 
m_reset_at_start(reset_at_start)
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  char errStr[1024];
  int i;

  register_server_handlers();

	if (additional_reset_commands == NULL) {
		sprintf(add_reset_cmd, "");
	} else {
		vrpn_strcpy(add_reset_cmd, additional_reset_commands);
	}

	// Initially, set to no buttons or analogs on the stations.  The
	// methods to add buttons and analogs must be called to add them.
	for (i = 0; i < ISD_MAX_STATIONS; i++) {
	    is900_buttons[i] = NULL;
	    is900_analogs[i] = NULL;
	}

  m_CommPort = commPort;
  m_Handle = ISD_OpenTracker(NULL, commPort, FALSE, FALSE);

  if(m_Handle == -1)
  {
    sprintf(errStr,"Failed to open tracker '%s' on COM%d: ISLIB_OpenTracker returned -1",name,commPort);
    status = vrpn_TRACKER_FAIL;
    return;
  }

  //  ISD_TRACKER_INFO_TYPE trackerInfo;

  ISD_GetTrackerConfig(m_Handle,&m_TrackerInfo,FALSE);

  for (i = 0; i < ISD_MAX_STATIONS; i++) {
       if (set_sensor_output_format(i)) {
		    sprintf(errStr,"Failed to reset sensor %d on tracker '%s' on COM%d",i, name,commPort);
			status = vrpn_TRACKER_FAIL;
     		return;
       }
  }


  //what is the update rate of this tracker?
  //we might want to set the update rate of the mainloop to based on this value.
  //for now we just print it out.
  getTrackerInfo(errStr);
  vrpn_gettimeofday(&timestamp, NULL);
  VRPN_MSG_INFO(errStr);
  fprintf(stderr,errStr);	
  
  status =   vrpn_TRACKER_SYNCING;
#else
  fprintf(stderr,"Intersense library not compiled into this version.  Use Fastrak driver for IS-600/900 or recompile with VRPN_INCLUDE_INTERSENSE defined\n");
  status = vrpn_TRACKER_FAIL;
#endif
}
 
vrpn_Tracker_InterSense::~vrpn_Tracker_InterSense()
{
#ifdef  VRPN_INCLUDE_INTERSENSE

    int i;

    ISD_CloseTracker(m_Handle);

    // Delete any button and analog devices that were created
    for (i = 0; i < ISD_MAX_STATIONS; i++) {
		if (is900_buttons[i]) {
                    try {
                      delete is900_buttons[i];
                    } catch (...) {
                      fprintf(stderr, "vrpn_Tracker_InterSense::~vrpn_Tracker_InterSense(): delete failed\n");
                      return;
                    }
                    is900_buttons[i] = NULL;
		}
		if (is900_analogs[i]) {
                    try {
                      delete is900_analogs[i];
                    } catch (...) {
                      fprintf(stderr, "vrpn_Tracker_InterSense::~vrpn_Tracker_InterSense(): delete failed\n");
                      return;
                    }
                    is900_analogs[i] = NULL;
		}
    }
#endif
}

/** This routine augments the basic sensor-output setting function of the Intersense
    to allow the possibility of requesting timestamp, button data, and/or analog
    data from the device.  It sets the device for position + quaternion + any of
    the extended fields.  

    Returns 0 on success and -1 on failure.
*/

int vrpn_Tracker_InterSense::set_sensor_output_format(int station)
{
  char errStr[1024];

		//Get the info about the station
        ISD_GetStationConfig(m_Handle,&m_StationInfo[station],station+1,0);

    	//ISD_ResetHeading(m_Handle,station+1); //Not sure if this is needed
		// nahon@virtools.com - 
		if (m_reset_at_start)
		  ISD_Boresight(m_Handle, station+1, true); // equivalent to ISD_ResetHeading for itrax, see isense.h


	    // First, try to set the orientation reporting format to quaternions if possible.
		// But, some models of intersense trackers (like the intertrax series) will only report
		// in euler angles

	    // Try to set the tracker to report in quaternion format
		// (to avoid gimbal lock)
		// nahon@virtools.com : Let's try, even if we are not a precision series
		// It seems that this is OK with Intertrax2, what would happen to old Intertrax ?
//		if (m_TrackerInfo.TrackerType == ISD_PRECISION_SERIES &&
		if (
			m_StationInfo[station].AngleFormat == ISD_EULER)
	    {
			m_StationInfo[station].AngleFormat = ISD_QUATERNION;
			if(!ISD_SetStationConfig(m_Handle,&m_StationInfo[station],station+1,FALSE))
		    {
				sprintf(errStr,"Warning: Your tracker doesn't seem to support the quaternion format - couldn't set station config for Station%d. ",station+1);
				vrpn_gettimeofday(&timestamp, NULL);
				VRPN_MSG_WARNING(errStr);

				m_StationInfo[station].AngleFormat = ISD_EULER;
			}
		}

		// do is900 special things
	    // IS900 states (timestamp, button, analog).
		if(m_TrackerInfo.TrackerModel == ISD_IS900) {
			if (do_is900_timestamps) {
				m_StationInfo[station].TimeStamped = TRUE;
				if(!ISD_SetStationConfig(m_Handle,&m_StationInfo[station],station+1,FALSE))
			    {
					sprintf(errStr,"Warning: Your tracker doesn't seem to support the IS900 timestamps - couldn't set station config for Station%d. ",station+1);
					vrpn_gettimeofday(&timestamp, NULL);
					VRPN_MSG_WARNING(errStr);
					m_StationInfo[station].TimeStamped = FALSE;
				}
			}

			if (is900_buttons[station] || is900_analogs[station]) {
				m_StationInfo[station].GetInputs = TRUE;
				if(!ISD_SetStationConfig(m_Handle,&m_StationInfo[station],station+1,FALSE))
			    {
					sprintf(errStr,"Warning: Your tracker doesn't seem to support the IS900 buttons/analogs - couldn't set station config for Station%d. ",station+1);
					vrpn_gettimeofday(&timestamp, NULL);
					VRPN_MSG_WARNING(errStr);
					m_StationInfo[station].GetInputs = FALSE;
				}
			}
		}
		return 0;
}

void vrpn_Tracker_InterSense::reset()
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  char errStr[1024]; 
  int i;
  
  m_Handle = ISD_OpenTracker(NULL,m_CommPort,FALSE,FALSE);

  if(m_Handle == -1)
  {
    sprintf(errStr,"InterSense: Failed to open tracker '%s' on COM%d: ISD_OpenTracker returned -1",d_servicename,m_CommPort);
    fprintf(stderr,errStr);
    vrpn_gettimeofday(&timestamp, NULL);
	VRPN_MSG_ERROR(errStr);

    status = vrpn_TRACKER_FAIL;
  }
  else 
  {

    //--------------------------------------------------------------------
    // Now that the tracker has given a valid status report, set all of
    // the parameters the way we want them. We rely on power-up setting
    // based on the receiver select switches to turn on the receivers that
    // the user wants.
    //--------------------------------------------------------------------

    // Set output format for each of the possible stations.

    for (i = 0; i < ISD_MAX_STATIONS; i++) {
       if (set_sensor_output_format(i)) {
		   return;
       }
    }

    // Send the additional reset commands, if any, to the tracker.
    // These commands come in lines, with character \015 ending each
    // line. 
    if (strlen(add_reset_cmd) > 0) {
		printf("  Intersense writing extended reset commands...\n");
		if(!ISD_SendScript(m_Handle,add_reset_cmd))
	    {
			sprintf(errStr,"Warning: Your tracker failed executing the additional command string. ");
			vrpn_gettimeofday(&timestamp, NULL);
			VRPN_MSG_WARNING(errStr);
		}
	}
  
    // If we are using the IS-900 timestamps, clear the timer on the device and
    // store the time when we cleared it.  First, drain any characters in the output
    // buffer to ensure we're sending right away.  Then, send the reset command and
    // store the time that we sent it, plus the estimated time for the characters to
    // get across the serial line to the device at the current baud rate.
    // Set time units to milliseconds (MT) and reset the time (MZ).
    if (do_is900_timestamps) {
    	char	clear_timestamp_cmd[] = "MT\015MZ\015";
		if(!ISD_SendScript(m_Handle,clear_timestamp_cmd))
	    {
			sprintf(errStr,"Warning: Your tracker failed executing the additional command string. ");
			vrpn_gettimeofday(&timestamp, NULL);
			VRPN_MSG_WARNING(errStr);
		}

		vrpn_gettimeofday(&is900_zerotime, NULL);
    }

    // Done with reset.
    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
    VRPN_MSG_WARNING("Reset Completed (this is good)");

    status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
  }
#else
  fprintf(stderr,"Intersense library not compiled into this version.  Use Fastrak driver for IS-600/900 or recompile with VRPN_INCLUDE_INTERSENSE defined\n");
  status = vrpn_TRACKER_FAIL;
#endif
}

// This function will get the reports from the intersense dll, then
// put that report into the time, sensor, pos and quat fields, and
// finally call send_report to send it.
void vrpn_Tracker_InterSense::get_report(void)
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  q_vec_type angles;
  ISD_TRACKING_DATA_TYPE data;
  int i;

  if(ISD_GetTrackingData(m_Handle,&data)) {
    for(int station=0;station<ISD_MAX_STATIONS;station++) {
      if(data.Station[station].NewData == TRUE) {

       d_sensor = station;

        //--------------------------------------------------------------------
		// If we are doing IS900 timestamps, decode the time, add it to the
        // time we zeroed the tracker, and update the report time.  Remember
        // to convert the MILLIseconds from the report into MICROseconds and
        // seconds.
        //--------------------------------------------------------------------

        if (do_is900_timestamps) {
          vrpn_float32 read_time = data.Station[station].TimeStamp;

          struct timeval delta_time;   // Time since the clock was reset

          // Convert from the float in MILLIseconds to the struct timeval
          delta_time.tv_sec = (long)(read_time / 1000);	// Integer trunction to seconds
          read_time -= delta_time.tv_sec * 1000;	// Subtract out what we just counted
          delta_time.tv_usec = (long)(read_time * 1000);	// Convert remainder to MICROseconds

          // Store the current time
          timestamp = vrpn_TimevalSum(is900_zerotime, delta_time);
        } else {
	  vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	}

        //--------------------------------------------------------------------
        // If this sensor has an IS900 button on it, decode
        // the button values into the button device and mainloop the button
        // device so that it will report any changes.  Each button is stored
        // in one bit of the byte, with the lowest-numbered button in the
        // lowest bit.
        //--------------------------------------------------------------------

        if (is900_buttons[station]) {
	    for (i = 0; i < is900_buttons[station]->number_of_buttons(); i++) {
	      is900_buttons[station]->set_button(i, data.Station[station].ButtonState[i]);
	    }
            is900_buttons[station]->mainloop();
        }

        //--------------------------------------------------------------------
        // If this sensor has an IS900 analog on it, decode the analog values
        // into the analog device and mainloop the analog device so that it
        // will report any changes.  The first byte holds the unsigned char
        // representation of left/right.  The second holds up/down.  For each,
        // 0 means min (left or rear), 127 means center and 255 means max.
        //--------------------------------------------------------------------

        if (is900_analogs[station]) {
	  // Normalize the values to the range -1 to 1
	  is900_analogs[station]->setChannelValue(0, (data.Station[station].AnalogData[0] - 127) / 128.0);
	  is900_analogs[station]->setChannelValue(1, (data.Station[station].AnalogData[1] - 127) / 128.0);

	  // Report the new values
	  is900_analogs[station]->report_changes();
	  is900_analogs[station]->mainloop();
	}

	// Copy the tracker data into our internal storage before sending
	// (no unit problem as the Position vector is already in meters, see ISD_STATION_STATE_TYPE)
	// Watch: For some reason, to get consistent rotation and translation axis permutations,
	//        we need non direct mapping.
        // RMT: Based on a report from Christian Odom, it seems like the Quaternions in the
        //      Isense are QXYZ, whereas in Quatlib (and VRPN) they are XYZQ.  Once these
        //      are switched correctly, the positions can be read without strange swapping.
	pos[0] = data.Station[station].Position[0];
	pos[1] = data.Station[station].Position[1];
	pos[2] = data.Station[station].Position[2];

	if(m_StationInfo[station].AngleFormat == ISD_QUATERNION) {	
		d_quat[0] = data.Station[station].Quaternion[1];
		d_quat[1] = data.Station[station].Quaternion[2];
		d_quat[2] = data.Station[station].Quaternion[3];
		d_quat[3] = data.Station[station].Quaternion[0];
        } else {
	        // Just return Euler for now...
	        // nahon@virtools needs to convert to radians
		angles[0] = VRPN_DEGREES_TO_RADIANS*data.Station[station].Euler[0];
		angles[1] = VRPN_DEGREES_TO_RADIANS*data.Station[station].Euler[1];
		angles[2] = VRPN_DEGREES_TO_RADIANS*data.Station[station].Euler[2];

		q_from_euler(d_quat, angles[0], angles[1], angles[2]);	
	}

	// have to just send it now
        status = vrpn_TRACKER_REPORT_READY;
//	fprintf(stderr, "sending message len %d\n", len);
	send_report();

	//printf("Isense %f, %f, %f\n",pos[0],pos[1],pos[2]);
	//printf("Isense a:%f, %f, %f : ",angles[0],angles[1],angles[2]); //if the tracker reports a quat, these will be garbage
	//printf("q: %f, %f, %f, %f\n",d_quat[0],d_quat[1],d_quat[2],d_quat[3]);	
      }
    }

  }
#endif
}

void vrpn_Tracker_InterSense::send_report(void)
{
#ifdef  VRPN_INCLUDE_INTERSENSE
	// Send the message on the connection
    if (d_connection) 
    {
	    char	msgbuf[1000];
	    int	len = encode_to(msgbuf);
	    if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf,
	    vrpn_CONNECTION_LOW_LATENCY)) {
	      fprintf(stderr,"InterSense: cannot write message: tossing\n");
	    }
	}
#endif
}



// This function should be called each time through the main loop
// of the server code. It polls for a report from the tracker and
// sends it if there is one. It will reset the tracker if there is
// no data from it for a few seconds.

void vrpn_Tracker_InterSense::mainloop()
{
  // Call the generic server mainloop, since we are a server
  server_mainloop();

  switch (status) {
    case vrpn_TRACKER_SYNCING:
    case vrpn_TRACKER_AWAITING_STATION:
    case vrpn_TRACKER_PARTIAL:
      {
	  // It turns out to be important to get the report before checking
	  // to see if it has been too long since the last report.  This is
	  // because there is the possibility that some other device running
	  // in the same server may have taken a long time on its last pass
	  // through mainloop().  Trackers that are resetting do this.  When
	  // this happens, you can get an infinite loop -- where one tracker
	  // resets and causes the other to timeout, and then it returns the
	  // favor.  By checking for the report here, we reset the timestamp
	  // if there is a report ready (ie, if THIS device is still operating).
	    
	  get_report();

	  // Ready for another report
	  status = vrpn_TRACKER_SYNCING;		
      }
      break; 

    case vrpn_TRACKER_RESETTING:
      reset();
      break;

    case vrpn_TRACKER_FAIL:
      VRPN_MSG_WARNING("Tracking failed, trying to reset (try power cycle if more than 4 attempts made)");
      status = vrpn_TRACKER_RESETTING;
      break;
  }
}


/** This function indicates to the driver that there is some sort of InterSense IS-900-
    compatible button device attached to the port (either a Wand or a Stylus).  The driver
    will configure the device to send reports when buttons are pressed and released.

    This routine returns 0 on success and -1 on failure (due to the sensor number being too
    large or errors writing to the device or can't create the button).
*/

int vrpn_Tracker_InterSense::add_is900_button(const char *button_device_name, int sensor, int numbuttons)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= ISD_MAX_STATIONS) ) {
    	return -1;
    }

    // Add a new button device and set the pointer to point at it.
    try { is900_buttons[sensor] = new vrpn_Button_Server(button_device_name, d_connection, numbuttons); }
    catch (...) {
    	VRPN_MSG_ERROR("Cannot open button device");
    	return -1;
    }

    // Send a new station-format command to the tracker so it will report the button states.
    return set_sensor_output_format(sensor);
}


/** This function indicates to the driver that there is an InterSense IS-900-
    compatible joystick device attached to the port (a Wand).  The driver
    will configure the device to send reports indicating the current status of the
    analogs when they change.  Note that a separate call to add_is900_button must
    be made in order to enable the buttons on the wand: this routine only handles
    the analog channels.

    The c0 and c1 parameters specify the clipping and scaling to take the reports
    from the two joystick axes into the range [-1..1].  The default is unscaled.

    This routine returns 0 on success and -1 on failure (due to the sensor number being too
    large or errors writing to the device or can't create the analog).
*/

int vrpn_Tracker_InterSense::add_is900_analog(const char *analog_device_name, int sensor,
	    double c0Min, double c0Low, double c0Hi, double c0Max,
	    double c1Min, double c1Low, double c1Hi, double c1Max)
{
    // Make sure this is a valid sensor
    if ( (sensor < 0) || (sensor >= ISD_MAX_STATIONS) ) {
    	return -1;
    }

    // Add a new analog device and set the pointer to point at it.
    try { is900_analogs[sensor] = new vrpn_Clipping_Analog_Server(analog_device_name, d_connection); }
    catch (...) {
	      VRPN_MSG_ERROR("Cannot open analog device");
	      return -1;
    }

    // Set the analog to have two channels, and set its channels to 0 to start with
    is900_analogs[sensor]->setNumChannels(2);
    is900_analogs[sensor]->setChannelValue(0, 0.0);
    is900_analogs[sensor]->setChannelValue(1, 0.0);

    // Set the scaling on the two channels.
    is900_analogs[sensor]->setClipValues(0, c0Min, c0Low, c0Hi, c0Max);
    is900_analogs[sensor]->setClipValues(1, c1Min, c1Low, c1Hi, c1Max);

    // Send a new station-format command to the tracker so it will report the analog status
    return set_sensor_output_format(sensor);
}
#endif
