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
#include "vrpn_Tracker_isense.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"
#include "quat.h" 

#ifdef  VRPN_INCLUDE_INTERSENSE
#include "isense.c"
#endif

#define MAX_TIME_INTERVAL       (5000000) // max time between reports (usec)
#define	INCHES_TO_METERS	(2.54/100.0)
#define PI (3.14159265358979323846)
#define	FT_INFO(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_NORMAL) ; if (d_connection) d_connection->send_pending_reports(); }
#define	FT_WARNING(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_WARNING) ; if (d_connection) d_connection->send_pending_reports(); }
#define	FT_ERROR(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_ERROR) ; if (d_connection) d_connection->send_pending_reports(); }

#define LOCAL_ADDR "130.207.103.85"

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

static int i;

vrpn_Tracker_InterSense::vrpn_Tracker_InterSense(const char *name, 
                                         vrpn_Connection *c,
                                         int commPort) :
vrpn_Tracker(name,c)
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  char errStr[1024];

  register_server_handlers();

  m_CommPort = commPort;
//  m_Handle = ISLIB_OpenTracker(NULL, commPort, FALSE, FALSE);
  m_Handle = ISD_OpenTracker(NULL, commPort, FALSE, FALSE);

  
  if(m_Handle == -1)
  {
    //    sprintf(errStr,"Failed to open tracker '%s' on COM%d: ISLIB_OpenTracker returned -1",name,commPort);
    //    MessageBox(0,errStr,"vrpn_Tracker_InterSense",MB_ICONSTOP);
    status = vrpn_TRACKER_FAIL;
    return;
  }

  ISD_TRACKER_INFO_TYPE trackerInfo;

  ISD_GetTrackerConfig(m_Handle,&trackerInfo,FALSE);
  //ISD_ResetAngles(m_Handle,0.0,0.0,0.0);

  for(int station=0;station<ISD_MAX_STATIONS;station++)
  {
    //Get the info about all its stations
    ISD_GetStationConfig(m_Handle,&m_StationInfo[station],station+1,0);

	ISD_ResetHeading(m_Handle,station+1); //Not sure if this is needed

	//some models of intersense trackers (like the intertrax series) will only report
	//in euler angles

    // Try to set the tracker to report in quaternion format
	// (to avoid gimbal lock)
    if(m_StationInfo[station].State == TRUE && m_StationInfo[station].AngleFormat == ISD_EULER)
    {
      m_StationInfo[station].AngleFormat = ISD_QUATERNION;
      if(!ISD_SetStationConfig(m_Handle,&m_StationInfo[station],station+1,FALSE))
      {
		//FIXME this should be a VRPN warning (of low priority)	
        sprintf(errStr,"Warning: Your tracker doesn't seem to support the quaternion format - couldn't set station config for Station%d. ",station+1);
		gettimeofday(&timestamp, NULL);
		FT_WARNING(errStr);

        m_StationInfo[station].AngleFormat = ISD_EULER;
      }
    }

	//what is the update rate of this tracker?
	//we might want to set the update rate of the mainloop to based on this value.
	//for now we just print it out.

	//it would also be nice if we could set the update rate of the tracker here as well
	//(and let the user specifiy it in the config file)
	sprintf(errStr,"sync state=%d update rate=%f\n",trackerInfo.SyncState, trackerInfo.SyncRate);
	gettimeofday(&timestamp, NULL);
	FT_INFO(errStr);
	fprintf(stderr,errStr);

	
  }
  status =   vrpn_TRACKER_SYNCING;
#else
  fprintf(stderr,"Intersense library not compiled into this version.  Use Fastrak driver for IS-600/900 or recompile with VRPN_INCLUDE_INTERSENSE defined\n");
  status = vrpn_TRACKER_FAIL;
#endif
}
 
vrpn_Tracker_InterSense::~vrpn_Tracker_InterSense()
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  ISD_CloseTracker(m_Handle);
#endif
}

void vrpn_Tracker_InterSense::reset()
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  char errStr[1024]; 
  
  m_Handle = ISD_OpenTracker(NULL,m_CommPort,FALSE,FALSE);

  if(m_Handle == -1)
  {
    sprintf(errStr,"InterSense: Failed to open tracker '%s' on COM%d: ISD_OpenTracker returned -1",d_servicename,m_CommPort);
    fprintf(stderr,errStr);
    gettimeofday(&timestamp, NULL);
	FT_ERROR(errStr);

    status = vrpn_TRACKER_FAIL;
  }
  else
    status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
#else
  fprintf(stderr,"Intersense library not compiled into this version.  Use Fastrak driver for IS-600/900 or recompile with VRPN_INCLUDE_INTERSENSE defined\n");
  status = vrpn_TRACKER_FAIL;
#endif
}

// This function will read characters until it has a full report, then
// put that report into the time, sensor, pos and quat fields so that it can
// be sent the next time through the loop.
void vrpn_Tracker_InterSense::get_report(void)
{
#ifdef  VRPN_INCLUDE_INTERSENSE
  q_vec_type angles;
  ISD_TRACKER_DATA_TYPE data;

  if(ISD_GetData(m_Handle,&data)) {
    for(int station=0;station<ISD_MAX_STATIONS;station++) {
      if(m_StationInfo[station].State == TRUE) {
	gettimeofday(&timestamp, NULL);	// Set watchdog now
	// we need a better clock!


	d_sensor = station+1;

	// Copy the tracker data into our internal storage before sending
	pos[0] = data.Station[station].Position[0];
	pos[1] = data.Station[station].Position[1];
	pos[2] = data.Station[station].Position[2];

	if(m_StationInfo[station].AngleFormat == ISD_QUATERNION) {	
	  d_quat[0] = data.Station[station].Orientation[0];
	  d_quat[1] = data.Station[station].Orientation[1];
	  d_quat[2] = data.Station[station].Orientation[2];
	  d_quat[3] = data.Station[station].Orientation[3];
        } else {
	  // Just return Euler for now...
  
	  angles[0] = data.Station[station].Orientation[0];
	  angles[1] = data.Station[station].Orientation[1];
	  angles[2] = data.Station[station].Orientation[2];

	  q_from_euler(d_quat, angles[0], angles[1], angles[2]);	
	}

	//printf("Isense %f, %f, %f\n",pos[0],pos[1],pos[2]);
	//printf("Isense a:%f, %f, %f : ",angles[0],angles[1],angles[2]); //if the tracker reports a quat, these will be garbage
	//printf("q: %f, %f, %f, %f\n",d_quat[0],d_quat[1],d_quat[2],d_quat[3]);	
      }
    }
  }
  status = vrpn_TRACKER_REPORT_READY;
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
	  
	  // Ready for another report
	  status = vrpn_TRACKER_SYNCING;		
      }
      break; 

    case vrpn_TRACKER_RESETTING:
      reset();
      break;

    case vrpn_TRACKER_FAIL:
      FT_WARNING("Tracking failed, trying to reset (try power cycle if more than 4 attempts made)");
      status = vrpn_TRACKER_RESETTING;
      break;
  }
}

