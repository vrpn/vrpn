#ifdef	VRPN_USE_DTRACK

//#define TRACEDTRACK

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

#include "vrpn_Tracker_DTrack.h"
#include "vrpn_Shared.h"
#include "quat.h" 

///////////////////////////////////////////////////////
/* This code is based on the dtrack sample code example_without_remote_control.c

  # Advanced Realtime Tracking GmbH's (http://www.ar-tracking.de) DTrack client 
  # developed by David Nahon for Virtools VR Pack (http://www.virtools.com)
  # creates as many vrpn_Tracker as there are bodies or flysticks, starting with the bodies
  # creates 2 analogs per flystick
  # creates 8 buttons per flystick
  #
  #	char	name_of_this_device[]
  #	int	udp_port
  #	float	timeToReachJoy
  #
  # NOTE: timeToReachJoy is the time needed to reach the maximum value of the joystick 
  # (1.0 or -1.0) when the corresponding button is pressed ( one of the last buttons amongst the 8 )

  #vrpn_Tracker_DTrack DTrack 5000 0.5

  Note: Consider a system with a head tracker (a body) and a flystick. In case the body is not in the field 
  of view, this code will consider the flystick to be the first tracker, i.e the head tracker.
  To correct this problem, we should rely on the Ids dtrack maintains and have an extra set of parameters
  that would describe the mapping between dtrack's IDs and Vrpn's Ids
  
	D.N
 */
//////////////////////////////////////////////////////

#define UDPBUFSIZE  10000
//#define UDPTIMEOUT  1000000
#define DTRACK_UDPTIMEOUT  0


////////////////////////////////////////////////
////////////////////////////////////////////////

#define MAX_TIME_INTERVAL       (5000000) // max time between reports (usec)
#define	INCHES_TO_METERS	(2.54/100.0)
#define PI (3.14159265358979323846)
#define	FT_INFO(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_NORMAL) ; if (d_connection) d_connection->send_pending_reports(); }
#define	FT_WARNING(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_WARNING) ; if (d_connection) d_connection->send_pending_reports(); }
#define	FT_ERROR(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_ERROR) ; if (d_connection) d_connection->send_pending_reports(); }

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

static	double	secDuration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec)/ 1000000.+
	        (t1.tv_sec - t2.tv_sec);
}

vrpn_Tracker_DTrack::vrpn_Tracker_DTrack(const char *name, 
                                         vrpn_Connection *c, 
					 int UdpPort, 
					 float timeToReachJoy/*=1.0*/) :
	vrpn_Tracker(name, c),	  
	vrpn_Button(name, c),
	vrpn_Analog(name, c), m_first(true), m_x(0.), m_y(0.),m_cumulatedDt(0.),
	num_buttons(0), num_channel(0)
{
	if (timeToReachJoy != 0) {
		m_incPerSec = 1./timeToReachJoy;
	} else {
		m_incPerSec=100000000000000000;	// so it reaches immediately
	}

	char errStr[1024];

	dtracklib_init_type ini;
	ini.udpport = UdpPort;
	ini.udpbufsize = UDPBUFSIZE;
	ini.udptimeout_us = DTRACK_UDPTIMEOUT;

	strcpy(ini.remote_ip, "");
	ini.remote_port = 0;

	int err;
	if (err = dtracklib_init(&ini)) {
		sprintf(errStr,"Failed to initialize dtrack device '%s' on Udp port %d: dtracklib_init returned %d",name, UdpPort, err);
		// FT_ERROR(errStr);
		MessageBox(0,errStr,"vrpn_Tracker_DTrack",MB_ICONSTOP);
		exit(EXIT_FAILURE);
	}
}
 
vrpn_Tracker_DTrack::~vrpn_Tracker_DTrack()
{
}

// This function should be called each time through the main loop
// of the server code. It polls for a report from the tracker and
// sends it if there is one. It will reset the tracker if there is
// no data from it for a few seconds.

void vrpn_Tracker_DTrack::mainloop()
{
	struct timeval timestamp;
	struct timezone zone;
	double dt;

	gettimeofday(&timestamp, &zone);
	if (m_first)
	{
		m_first=false;
		m_firstTv = timestamp;
		m_lastTime = m_now = 0.;
	}
	m_lastTime = m_now;
	m_now = secDuration(timestamp, m_firstTv);
	dt = m_now-m_lastTime;
	m_cumulatedDt += dt;

	// Call the generic server mainloop, since we are a server
	server_mainloop();

//	gettimeofday((timeval *)&vrpn_Tracker::timestamp, &zone);
//	gettimeofday((timeval *)&timestamp, &zone);
//	gettimeofday((timeval *)&timestamp, &zone);
//	gettimeofday((timeval *)&timestamp, &zone);
//	gettimeofday((timeval *)&vrpn_Tracker::timestamp, &zone);

	int err = dtracklib_receive_udp_ascii(
				&framenr, &m_timestamp,
				&nbodycal, &nbody, body, vrpn_DT_MAX_NBODY,
				&nflystick, flystick, vrpn_DT_MAX_NFLYSTICK,
				&nmarker, marker, vrpn_DT_MAX_NMARKER);

	if(err == DTRACKLIB_ERR_TIMEOUT){
		//printf("--- timeout receiving\n");
		return;
	}

	if(err){
		printf("--- error receiving %d\n", err);
		return;
	}

#ifdef TRACEDTRACK
	printf("framenr:%d - t=%f\n", framenr, m_now);
#endif

/*	frame 740 ts -1.000 nbodcal -1 nbod 1 nfly 1 nmar 0
bod 0 qu 1.000 loc 684.21 155.46 818.86 ang 39.59 11.42 118.80 rot -0.472 0.615
0.632 -0.859 -0.482 -0.173 0.198 -0.625 0.755
fly 0 qu 1.000 bt 0 loc -238.24 95.79 1029.46 ang -97.56 -54.11 -78.24 rot 0.119
 0.292 0.949 0.574 0.759 -0.306 -0.810 0.581 -0.077 */

	num_sensors = nbody + nflystick;
	num_channel=2*nflystick;	// 2 channels joystick
	num_buttons=8*nflystick;	// 8 buttons ??

    // start with Bodies
	for(int i=0;i<nbody;i++) {
		d_sensor=i;
		pos[0]=body[i].loc[0]/1000.;
		pos[1]=body[i].loc[1]/1000.;
		pos[2]=body[i].loc[2]/1000.;
		
		dtrackMatrix2vrpnquat(body[i].rot,d_quat);


#ifdef TRACEDTRACK // DEBUG
		//printf("DTrack Euler head: %f %f %f\n", -Q_RAD_TO_DEG(body[i].rot[2]), -Q_RAD_TO_DEG(body[i].rot[1]), -Q_RAD_TO_DEG(body[i].rot[0]));

		
		q_vec_type yawPitchRoll;
		q_to_euler(yawPitchRoll, d_quat);
		printf("Matrix method body[%d] euler (yaw, pitch, roll): %f %f %f\n", i, Q_RAD_TO_DEG(yawPitchRoll[0]), Q_RAD_TO_DEG(yawPitchRoll[1]),
			Q_RAD_TO_DEG(yawPitchRoll[2]));
#endif

		// pack and deliver tracker report;
		if (d_connection) {
			char	msgbuf[1000];
			int	len = vrpn_Tracker::encode_to(msgbuf);
			if (d_connection->pack_message(len, *(timeval *)&timestamp,
				position_m_id, d_sender_id, msgbuf,
				vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Tracker vrpn_Tracker_DTrack: "
								"cannot write message: tossing\n");
			}
		}
	}

	// now the flysticks
	int curChannelId=0;
	int curButtonId=0;
	for(int j=0;j<nflystick;j++) {
		d_sensor=i+j;
		pos[0]=flystick[j].loc[0]/1000.;
		pos[1]=flystick[j].loc[1]/1000.;
		pos[2]=flystick[j].loc[2]/1000.;

		//q_from_euler(d_quat, -(flystick[j].rot[2]), -(flystick[j].rot[1]), -(flystick[j].rot[0]));
		dtrackMatrix2vrpnquat(flystick[j].rot,d_quat);

#ifdef TRACEDTRACK
		q_vec_type yawPitchRoll;
		q_to_euler(yawPitchRoll, d_quat);
		printf("Matrix method flystic[%d] euler (yaw, pitch, roll): %f %f %f\n", j,Q_RAD_TO_DEG(yawPitchRoll[0]), Q_RAD_TO_DEG(yawPitchRoll[1]),
			Q_RAD_TO_DEG(yawPitchRoll[2]));
#endif
		// pack and deliver tracker report;
		if (d_connection) {
			char	msgbuf[1000];
			int	len = vrpn_Tracker::encode_to(msgbuf);
			if (d_connection->pack_message(len, *(timeval *)&timestamp,
				position_m_id, d_sender_id, msgbuf,
				vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Tracker vrpn_Tracker_DTrack: "
								"cannot write message: tossing\n");
			}
		}
		

#ifdef TRACEDTRACK
		printf("buttons flystick[%d]: (1 - 8): ", j);
#endif
		// buttons
		buttons[curButtonId++]=(unsigned char)(flystick[j].bt & 1);
#ifdef TRACEDTRACK
		printf("%d ", buttons[curButtonId-1]);
#endif
		buttons[curButtonId++]=(unsigned char)((flystick[j].bt & 8) >> 3);
#ifdef TRACEDTRACK
		printf("%d ", buttons[curButtonId-1]);
#endif
		buttons[curButtonId++]=(unsigned char)((flystick[j].bt & 4) >> 2);
#ifdef TRACEDTRACK
		printf("%d ", buttons[curButtonId-1]);
#endif
		buttons[curButtonId++]=(unsigned char)((flystick[j].bt & 2) >> 1);

#ifdef TRACEDTRACK
		printf("%d ", buttons[curButtonId-1]);
#endif		
#ifdef TRACEDTRACK
			printf("\n");

#endif

		// transform buttons 5 (down), 7 (up), 6 (left), 8 (right)
		// into time varying floating values
		unsigned char B5, B6, B7, B8;
		B5 = (unsigned char)((flystick[j].bt & 16) >> 4);
		B6 = (unsigned char)((flystick[j].bt & 32) >> 5);
		B7 = (unsigned char)((flystick[j].bt & 64) >> 6);
		B8 = (unsigned char)((flystick[j].bt & 128) >> 7);

		buttons[curButtonId++]=B5;
		buttons[curButtonId++]=B6;
		buttons[curButtonId++]=B7;
		buttons[curButtonId++]=B8;


		if (m_cumulatedDt > 0.01) {
			m_x = butToChannel(m_x, B8, B6, m_cumulatedDt );
			m_y = butToChannel(m_y, B5, B7, m_cumulatedDt );
			m_cumulatedDt =0.;
		}

#ifdef TRACEDTRACK
		printf("\njoystick (x,y - B5 B6 B7 B8): (%f,%f - %d %d %d %d)\n", m_x, m_y, B5, B6, B7, B8);
#endif
		// set buttons and pseudo-joystick
		// flystick[j].bt
		
		channel[curChannelId++]=(float)m_x;
		channel[curChannelId++]=(float)m_y;
	}

  ///////////////////////////

//  vrpn_Tracker::report_changes(); // report any analog event;
  vrpn_Analog::report_changes(); // report any analog event;
  vrpn_Button::report_changes(); // report any button event;

}

double vrpn_Tracker_DTrack::butToChannel(double curVal, unsigned char incBut, unsigned char decBut, double dt)
{
	double ret;	// return value

	if (incBut) {
		ret = curVal + dt*m_incPerSec;
		if (ret > 1) ret = 1.;
		return ret;
	}
	if (decBut) {
		ret = curVal - dt*m_incPerSec;
		if (ret < -1) ret = -1.;
		return ret;
	} else {
		return 0.f;
	}
}

void vrpn_Tracker_DTrack::dtrackMatrix2vrpnquat(const float* rot, vrpn_float64* quat)
{
		// if this is not good, just build the matrix and do a matrixToQuat
		q_matrix_type destMatrix;
		destMatrix[0][0] = rot[0];
		destMatrix[0][1] = rot[1];
		destMatrix[0][2] = rot[2];
		destMatrix[0][3] = 0.0;

		destMatrix[1][0] = rot[3];
		destMatrix[1][1] = rot[4];
		destMatrix[1][2] = rot[5];
		destMatrix[1][3] = 0.0;

		destMatrix[2][0] = rot[6];
		destMatrix[2][1] = rot[7];
		destMatrix[2][2] = rot[8];
		destMatrix[2][3] = 0.0;

		destMatrix[3][0] = 0.0;
		destMatrix[3][1] = 0.0;
		destMatrix[3][2] = 0.0;
		destMatrix[3][3] = 1.0;

		q_from_row_matrix(quat, destMatrix);
}

#endif

