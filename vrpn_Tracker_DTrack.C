// vrpn_Tracker_DTrack.C
//
// Advanced Realtime Tracking GmbH's (http://www.ar-tracking.de) DTrack client

// developped by David Nahon for Virtools VR Pack (http://www.virtools.com)
// improved by Advanced Realtime Tracking GmbH (http://www.ar-tracking.de)
//
// Recommended settings within DTrack's 'Settings / Network' dialog:
//   '6d', '6df', '6dcal'

/* Configuration file:

################################################################################
# Advanced Realtime Tracking GmbH (http://www.ar-tracking.de) DTrack client 
#
# creates as many vrpn_Tracker as there are bodies or flysticks, starting with the bodies
# creates 2 analogs per flystick
# creates 8 buttons per flystick
#
# NOTE: numbering of flystick buttons differs from DTrack documentation
#
# Arguments:
#  char  name_of_this_device[]
#  int   udp_port                               (DTrack sends data to this udp port)
#  float time_to_reach_joy                      (see below)
#
# Optional arguments:
#  int   number_of_bodies, number_of_flysticks  (fixed numbers of bodies and flysticks)
#  int   renumbered_ids[]                       (vrpn_Tracker IDs of bodies and flysticks)
#  char  "-"                                    (activates tracing; always last argument)
#
# NOTE: time_to_reach_joy is the time needed to reach the maximum value of the joystick 
#       (1.0 or -1.0) when the corresponding button is pressed (one of the last buttons
#       amongst the 8)
#
# NOTE: if fixed numbers of bodies and flysticks should be used, both arguments
#       number_of_bodies and number_of_flysticks have to be set
#
# NOTE: renumbering of tracker IDs is only possible, if fixed numbers of bodies and
#       flysticks are set; there has to be an argument present for each body/flystick

#vrpn_Tracker_DTrack DTrack  5000 0.5
#vrpn_Tracker_DTrack DTrack  5000 0.5  2 2
#vrpn_Tracker_DTrack DTrack  5000 0.5  2 2  2 1 0 3

################################################################################
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <unistd.h>
#endif

#include "vrpn_Tracker_DTrack.h"
#include "vrpn_Shared.h"
#include "quat.h" 

// --------------------------------------------------------------------------

#define UDPRECEIVE_BUFSIZE       10000  // size of udp buffer for DTrack data (one frame; in bytes)

// --------------------------------------------------------------------------

#define	DT_INFO(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_NORMAL) ; if (d_connection) d_connection->send_pending_reports(); }
#define	DT_WARNING(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_WARNING) ; if (d_connection) d_connection->send_pending_reports(); }
#define	DT_ERROR(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_ERROR) ; if (d_connection) d_connection->send_pending_reports(); }

// Local prototypes:

static double duration_sec(struct timeval t1, struct timeval t2);

static char* string_nextline(char *str, char *start, unsigned long len);
static char* string_get_ul(char *str, unsigned long *ul);
static char* string_get_f(char* str, float* f);
static char* string_get_block(char *str, const char* fmt, unsigned long* uldat, float* fdat);

static int udp_init(unsigned short port);
static int udp_exit(int sock);
static int udp_receive(int sock, char *buffer, int maxlen);


// --------------------------------------------------------------------------
// Constructor:
// name (i): device name
// c (i): vrpn_Connection
// dtrack_port (i): DTrack udp port
// timeToReachJoy (i): time needed to reach the maximum value of the joystick
// fixNbody, fixNflystick (i): fixed numbers of DTrack bodies and flysticks (-1 if not wanted)
// fixId (i): renumbering of targets; must have exactly (fixNbody + fixNflystick) elements (NULL if not wanted)
// actTracing (i): activate trace output

vrpn_Tracker_DTrack::vrpn_Tracker_DTrack(const char *name, vrpn_Connection *c, 
	                                      unsigned short dtrackPort, float timeToReachJoy,
	                                      int fixNbody, int fixNflystick, unsigned long* fixId,
	                                      bool actTracing) :
	vrpn_Tracker(name, c),	  
	vrpn_Button(name, c),
	vrpn_Analog(name, c)
{

	int i;
	
	// Dunno how many of these there are yet...
	
	num_sensors = 0;
	num_channel = 0;
	num_buttons = 0;
	
	// init variables: general

	tracing = actTracing;
	tracing_frames = 0;

	tim_first.tv_sec = tim_first.tv_usec = 0;  // also used to recognize the first frame
	tim_last.tv_sec = tim_last.tv_usec = 0;

	// init variables: DTrack data

	if(fixNbody >= 0 && fixNflystick >= 0){    // fixed numbers of bodies and flysticks should be used
		fix_nbody = fixNbody;
		fix_nflystick = fixNflystick;
	}else{
		fix_nbody = fix_nflystick = -1;
	}

	for(i=0; i<vrpn_DTRACK_MAX_NBODY + vrpn_DTRACK_MAX_NFLYSTICK; i++){
		fix_idbody[i] = i;
	}
	for(i=0; i<vrpn_DTRACK_MAX_NFLYSTICK; i++){
		fix_idflystick[i] = i;
	}
		
	if(fix_nbody >= 0 && fixId){
		for(i=0; i<fix_nbody + fix_nflystick; i++){  // take the renumbering information for targets
			fix_idbody[i] = fixId[i];
		}
		
		for(i=0; i<fix_nflystick; i++){  // take the renumbering information for flysticks (vrpn button data)
			fix_idflystick[i] = fixId[i+fix_nbody];
		}

		for(i=0; i<fix_nbody; i++){      // remove all bodies from the flystick renumbering data
			for(int j=0; j<fix_nflystick; j++){
				if(fixId[i] < fixId[j+fix_nbody] && fix_idflystick[j] > 0){  // be sure to avoid crazy numbers...
					fix_idflystick[j]--;
				}
			}
		}
	}
	
	warning_nbodycal = 0;
	max_nbody = max_nflystick = 0;
	
	// init variables: preparing data for VRPN
	
	for(i=0; i<vrpn_DTRACK_MAX_NFLYSTICK; i++){  // current value of 'joystick' channels
		joy_x[i] = joy_y[i] = 0;
	}
	
	if(timeToReachJoy >= 0){
		joy_incPerSec = 1. / timeToReachJoy;          // increase of 'joystick' channel
	}else{
		joy_incPerSec = 1e20;                         // so it reaches immediately
	}

	// init: communicating with DTrack

	if(dtrack_init(dtrackPort)){
		exit(EXIT_FAILURE);
	}
}
 
// Destructor:

vrpn_Tracker_DTrack::~vrpn_Tracker_DTrack()
{

	dtrack_exit();
}


// --------------------------------------------------------------------------
// Main loop:

// This function should be called each time through the main loop
// of the server code. It checks for a report from the tracker and
// sends it if there is one.

void vrpn_Tracker_DTrack::mainloop()
{
	struct timeval timestamp;
	double dt;
	int nbody, nflystick, i;
	
	// call the generic server mainloop, since we are a server:
	server_mainloop();

	// get data from DTrack:

	unsigned long dtr_frames;
	int dtr_nbody, dtr_nbodycal;
	int dtr_nflystick;

	int err = dtrack_receive(&dtr_frames,
				&dtr_nbodycal, &dtr_nbody, dtr_body, vrpn_DTRACK_MAX_NBODY,
				&dtr_nflystick, dtr_flystick, vrpn_DTRACK_MAX_NFLYSTICK);

	if(err <= 0){
		if(err < 0){
			fprintf(stderr, "vrpn_Tracker_DTrack: Receive Error from DTrack (%d).\n", err);
		}
		return;
	}

	tracing_frames++;

	// get timestamp:

	gettimeofday(&timestamp, NULL);
	
	if(tim_first.tv_sec == 0 && tim_first.tv_usec == 0){
		tim_first = tim_last = timestamp;
	}

	dt = duration_sec(timestamp, tim_last);
	tim_last = timestamp;
	
	if(tracing && ((tracing_frames % 10) == 0)){
		printf("framenr %lu  time %.3f\n", dtr_frames, duration_sec(timestamp, tim_first));
	}

	// find number of targets visible for vrpn to choose the correct vrpn ID numbers:
	// (1) takes fixed number of bodies and flysticks, if defined in the configuration file
	// (2) otherwise uses the '6dcal' line in DTrack's output, that gives the total number of
	//     calibrated targets, if available
	// (3) otherwise tracks the maximum number of appeared targets

	if(fix_nbody >= 0){            // fixed numbers should be used
		nbody = fix_nbody;          // number of bodies visible for vrpn
		nflystick = fix_nflystick;  // number of flysticks visible for vrpn
	}else{
		if(dtr_nbodycal >= 0){      // DTracks sends information about the number of calibrated targets
			max_nbody = dtr_nbodycal - dtr_nflystick;
		}else{                      // track the maximum number of appeared targets (at least)
			if(!warning_nbodycal){   // mention warning (once)
				fprintf(stderr, "vrpn_Tracker_DTrack warning: no DTrack '6dcal' data available.\n");
				warning_nbodycal = 1;
			}
			
			if(dtr_nbody > 0){
				if((int)dtr_body[dtr_nbody-1].id >= max_nbody){
					max_nbody = dtr_body[dtr_nbody-1].id + 1;
				}
			}
		}
		
		if(dtr_nflystick > max_nflystick){  // DTrack always sends information about the total number of flysticks
			max_nflystick = dtr_nflystick;
		}
		
		nbody = max_nbody;          // number of bodies visible for vrpn
		nflystick = max_nflystick;  // number of flysticks visible for vrpn
	}

	// report tracker data to vrpn:

	num_sensors = nbody + nflystick;     // total number of targets visible for vrpn
	num_buttons = nflystick * 8;         // 8 buttons per Flystick
	num_channel = nflystick * 2;         // 2 channels per joystick/Flystick

	for(i=0; i<dtr_nbody; i++){      // DTrack bodies
		if((int)dtr_body[i].id < nbody){       // there might be more DTrack bodies than wanted
			unsigned long newid = fix_idbody[dtr_body[i].id];  // renumbered ID
			
			dtrack2vrpn_body(newid, "", dtr_body[i].id, dtr_body[i].loc, dtr_body[i].rot, timestamp);
		}
	}

	for(i=0; i<dtr_nflystick; i++){      // DTrack Flysticks
		if((int)dtr_flystick[i].id < nflystick){   // there might be more DTrack Flysticks than wanted
			unsigned long newid = fix_idbody[dtr_flystick[i].id + nbody];  // renumbered ID for body data
			
			if(dtr_flystick[i].quality >= 0){  // otherwise the Flystick target is not tracked at the moment
				dtrack2vrpn_body(newid, "f", dtr_flystick[i].id, dtr_flystick[i].loc, dtr_flystick[i].rot, timestamp);
			}
			
			newid = fix_idflystick[dtr_flystick[i].id];  // renumbered ID for button data
			
			dtrack2vrpn_flystickbuttons(newid, dtr_flystick[i].id, dtr_flystick[i].bt, dt, timestamp);
		}
	}
	
	// finish main loop:

	vrpn_Analog::report_changes();       // report any analog event;
	vrpn_Button::report_changes();       // report any button event;
}


// -----------------------------------------------------------------------------------------
// Helpers:

// Get duration between two timestamps:
// t1, t2 (i): timestamps
// return value (o): duration in sec

static double duration_sec(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec)/ 1000000.+ (t1.tv_sec - t2.tv_sec);
}


// -----------------------------------------------------------------------------------------
// Preparing data for VRPN:
// these functions convert DTrack data to vrpn data

// Preparing body data:
// id (i): VRPN Body ID
// str_dtrack (i): DTrack body name ('body' or 'flystick'; just used for trace output)
// id_dtrack (i): DTrack Body ID (just used for trace output)
// loc (i): position
// rot (i): orientation (3x3 rotation matrix)
// timestamp (i): timestamp for body data
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack2vrpn_body(unsigned long id, char* str_dtrack, unsigned long id_dtrack,
                                          float* loc, float* rot, struct timeval timestamp)
{

	d_sensor = id;
	
	// position (plus converting to unit meter):
	  
	pos[0] = loc[0] / 1000.;
	pos[1] = loc[1] / 1000.;
	pos[2] = loc[2] / 1000.;

	// orientation:

	q_matrix_type destMatrix;  // if this is not good, just build the matrix and do a matrixToQuat
	
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
	
	q_from_row_matrix(d_quat, destMatrix);
	
	// pack and deliver tracker report:
	
	if(d_connection){
		char msgbuf[1000];
		int len = vrpn_Tracker::encode_to(msgbuf);
		
		if(d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf,
		                              vrpn_CONNECTION_LOW_LATENCY))
		{
			fprintf(stderr, "vrpn_Tracker_DTrack: cannot write message: tossing.\n");
		}
	}

	// tracing:

	if(tracing && ((tracing_frames % 10) == 0)){
		q_vec_type yawPitchRoll;
			
		q_to_euler(yawPitchRoll, d_quat);
			
		printf("body id (DTrack vrpn): %s%d %d  pos (x y z): %.4f %.4f %.4f  euler (y p r): %.3f %.3f %.3f\n",
			str_dtrack, id_dtrack, id, pos[0], pos[1], pos[2],
			Q_RAD_TO_DEG(yawPitchRoll[0]), Q_RAD_TO_DEG(yawPitchRoll[1]), Q_RAD_TO_DEG(yawPitchRoll[2]));
	}

	return 0;
}


// Preparing flystick button data (incl. transforming HAT switch buttons):
// id (i): VRPN Flystick ID
// id_dtrack (i): DTrack Body ID (just used for trace output)
// but (i): buttons (binary)
// dt (i): time since last change
// timestamp (i): timestamp for button data
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack2vrpn_flystickbuttons(unsigned long id,  unsigned long id_dtrack, 
                                                     unsigned long but, double dt, struct timeval timestamp)
{
	unsigned char B1, B2, B3, B4, B5, B6, B7, B8;

	// buttons:
	// NOTE: numbering of two buttons (B2 and B4) differs from DTrack documentation!
	
	B1 = (unsigned char)(but & 1);
	B2 = (unsigned char)((but & 8) >> 3);    // numbering changed in respect to DTrack documentation
	B3 = (unsigned char)((but & 4) >> 2);
	B4 = (unsigned char)((but & 2) >> 1);    // numbering changed in respect to DTrack documentation
	
	B5 = (unsigned char)((but & 16) >> 4);   // HAT switch down
	B6 = (unsigned char)((but & 32) >> 5);   // HAT switch left
	B7 = (unsigned char)((but & 64) >> 6);   // HAT switch up
	B8 = (unsigned char)((but & 128) >> 7);  // HAT switch right
	
	buttons[id * 8] = B1;
	buttons[id * 8 + 1] = B2;
	buttons[id * 8 + 2] = B3;
	buttons[id * 8 + 3] = B4;
	
	buttons[id * 8 + 4] = B5;
	buttons[id * 8 + 5] = B6;
	buttons[id * 8 + 6] = B7;
	buttons[id * 8 + 7] = B8;
	
	vrpn_Button::timestamp = timestamp;  // timestamp for button event (explicitly necessary)

	// transform HAT switch buttons into time varying floating values (channel):
	
	joy_x[id] = dtrack2vrpn_butToChannel(joy_x[id], B8, B6, dt);  // horizontal
	joy_y[id] = dtrack2vrpn_butToChannel(joy_y[id], B5, B7, dt);  // vertical

	channel[id * 2] = (float )joy_x[id];
	channel[id * 2 + 1] = (float )joy_y[id];

	// tracing:

	if(tracing && ((tracing_frames % 10) == 0)){
		printf("flystick id (DTrack vrpn): f%d %d  but (B1 - B8): %d %d %d %d  %d %d %d %d  joy (x y): %.3f %.3f\n",
			id_dtrack, id, B1, B2, B3, B4, B5, B6, B7, B8, joy_x[id], joy_y[id]);
	}

	return 0;
}

// Transforming HAT switch buttons into time varying floating values:
// curVal (i): current floating value
// incBut, decBut (i): button to increase/decrease the floating value
// dt (i): time since last change
// return value (o): new floating value

double vrpn_Tracker_DTrack::dtrack2vrpn_butToChannel(double curVal,
                          unsigned char incBut, unsigned char decBut, double dt)
{
	double ret;	// return value

	if (incBut) {
		ret = curVal + dt * joy_incPerSec;
		if (ret > 1) ret = 1.;
		return ret;
	}
	
	if (decBut) {
		ret = curVal - dt * joy_incPerSec;
		if (ret < -1) ret = -1.;
		return ret;
	} else {
		return 0.f;  // no button
	}
}


// --------------------------------------------------------------------------
// Communication with DTrack:
// these functions receive and parse data packets from DTrack

// Initializing communication with DTrack:
// dtrack_port (i): DTrack udp port number
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack_init(unsigned short dtrack_port)
{

	if(!(udpbuf = (char *)malloc(UDPRECEIVE_BUFSIZE))){  // init udp buffer
		fprintf(stderr, "vrpn_Tracker_DTrack: Cannot Allocate Memory for UDP Buffer.\n");
		return -1;
	}

	if((udpsock = udp_init(dtrack_port)) < 0){           // init udp socket
		fprintf(stderr, "vrpn_Tracker_DTrack: Cannot Initialize UDP Socket.\n");
		return -1;
	}

	return 0;
}


// Deinitializing communication with DTrack:
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack_exit(void)
{

	if(udpsock > 0){
		udp_exit(udpsock);   // exit udp socket
	}
	
	if(udpbuf){             // exit udp buffer
		free(udpbuf);
	}
	
	return 0;
}


// Receive and process DTrack udp packet (ASCII protocol):
// framenr (o): frame counter
// nbodycal (o): number of calibrated bodies (-1, if information not available in packet)
// nbody (o): number of tracked bodies
// body (o): array containing 6d data
// max_nbody (i): maximum number of bodies in array body (no processing is done, if 0)
// nflystick (o): number of calibrated flysticks
// flystick (o): array containing 6df data
// max_nflystick (i): maximum number of flysticks in array flystick (no processing is done, if 0)
// return value (o): data received (0/1), <0 if error occured

int vrpn_Tracker_DTrack::dtrack_receive(unsigned long* framenr,
	int* nbodycal, int* nbody, dtrack_body_type* body, int max_nbody,
	int* nflystick, dtrack_flystick_type* flystick, int max_nflystick)
{
	char* s;
	int i, j, len, n;
	unsigned long ul, ularr[2];
	float farr[6];

	// defaults:

	*framenr = 0;
	*nbodycal = -1;    // i.e. not available
	*nbody = 0;
	*nflystick = 0;

	// receive udp packet:

	len = udp_receive(udpsock, udpbuf, UDPRECEIVE_BUFSIZE-1);
	if(len <= 0){
		return len;
	}

	s = udpbuf;
	s[len] = '\0';

	do{
		// line for frame counter:

		if(!strncmp(s, "fr ", 3)){
			s += 3;
			
			if(!(s = string_get_ul(s, framenr))){  // get frame counter
				*framenr = 0;
				return -10;
			}
			continue;
		}

		// line for additional information about number of calibrated bodies:

		if(!strncmp(s, "6dcal ", 6)){
			if(max_nbody <= 0){
				continue;
			}
			
			s += 6;
			
			if(!(s = string_get_ul(s, &ul))){    // get number of bodies
				return -10;
			}

			*nbodycal = (int )ul;
			continue;
		}

		// line for 6d data:

		if(!strncmp(s, "6d ", 3)){
			if(max_nbody <= 0){
				continue;
			}
			
			s += 3;
			
			if(!(s = string_get_ul(s, &ul))){    // get number of bodies
				return -10;
			}

			*nbody = n = (int )ul;
			if(n > max_nbody){
				n = max_nbody;
			}

			for(i=0; i<n; i++){                  // get data of body
				if(!(s = string_get_block(s, "uf", &body[i].id, farr))){
					return -10;
				}
				
				if(!(s = string_get_block(s, "ffffff", NULL, farr))){
					return -10;
				}
				for(j=0; j<3; j++){
					body[i].loc[j] = farr[j];
				}
				
				if(!(s = string_get_block(s, "fffffffff", NULL, body[i].rot))){
					return -10;
				}
			}
			
			continue;
		}
		
		// line for flystick data:

		if(!strncmp(s, "6df ", 4)){
			if(max_nflystick <= 0){
				continue;
			}
			
			s += 4;
			
			if(!(s = string_get_ul(s, &ul))){    // get number of flysticks
				return -10;
			}

			*nflystick = n = (int )ul;
			if(n > max_nflystick){
				n = max_nflystick;
			}

			for(i=0; i<n; i++){                  // get data of body
				if(!(s = string_get_block(s, "ufu", ularr, &flystick[i].quality))){
					return -10;
				}

				flystick[i].id = ularr[0];
				flystick[i].bt = ularr[1];
				
				if(!(s = string_get_block(s, "ffffff", NULL, farr))){
					return -10;
				}
				for(j=0; j<3; j++){
					flystick[i].loc[j] = farr[j];
				}
				
				if(!(s = string_get_block(s, "fffffffff", NULL, flystick[i].rot))){
					return -10;
				}
			}
			
			continue;
		}
		
		// ignore invalid line identifier
		
	}while((s = string_nextline(udpbuf, s, UDPRECEIVE_BUFSIZE)));

	return 1;
}


// -----------------------------------------------------------------------------------------
// Parsing DTrack data:

// Search next line in buffer:
// str (i): buffer (total)
// start (i): start position within buffer
// len (i): buffer length in bytes
// return (i): begin of line, NULL if no new line in buffer

static char* string_nextline(char* str, char* start, unsigned long len)
{
	char* s = start;
	char* se = str + len;
	bool crlffound = false;

	while(s < se){
		if(*s == '\r' || *s == '\n'){  // crlf
			crlffound = true;
		}else{
			if(crlffound){              // begin of new line found
				return (*s) ? s : NULL;  // first character is '\0': end of buffer
			}
		}

		s++;
	}

	return NULL;                      // no new line found in buffer
}


// Read next 'unsigned long' value from string:
// str (i): string
// ul (o): read value
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_ul(char* str, unsigned long* ul)
{
	char* s;
	
	*ul = strtoul(str, &s, 0);
	return (s == str) ? NULL : s;
}


// Read next 'float' value from string:
// str (i): string
// f (o): read value
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_f(char* str, float* f)
{
	char* s;
	
	*f = (float )strtod(str, &s);   // strtof() only available in GNU-C
	return (s == str) ? NULL : s;
}


// Process next block '[...]' in string:
// str (i): string
// fmt (i): format string ('u' for 'unsigned long', 'f' for 'float')
// uldat (o): array for 'unsigned long' values (long enough due to fmt)
// fdat (o): array for 'float' values (long enough due to fmt)
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_block(char* str, const char* fmt, unsigned long* uldat, float* fdat)
{
	char* strend;
	int index_ul, index_f;

	if(!(str = strchr(str, '['))){       // search begin of block
		return NULL;
	}
	if(!(strend = strchr(str, ']'))){    // search end of block
		return NULL;
	}
	str++;
	*strend = '\0';

	index_ul = index_f = 0;

	while(*fmt){
		switch(*fmt++){
			case 'u':
				if(!(str = string_get_ul(str, &uldat[index_ul++]))){
					return NULL;
				}
				break;
				
			case 'f':
				if(!(str = string_get_f(str, &fdat[index_f++]))){
					return NULL;
				}
				break;
				
			default:    // ignore unknown format character
				break;
		}
	}

	return strend + 1;
}


// -----------------------------------------------------------------------------------------
// Receiving udp data:

// Initialize udp socket:
// port (i): port number
// return value (o): socket number, <0 if error

static int udp_init(unsigned short port)
{
   int sock;
   struct sockaddr_in name;

	// init socket dll (only Windows):

	#ifdef WIN32
	{
		WORD vreq;
		WSADATA wsa;

		vreq = MAKEWORD(2, 0);
		if(WSAStartup(vreq, &wsa) != 0){
			return -1;
		}
	}
	#endif
        
   // create the socket:

	sock = socket(PF_INET, SOCK_DGRAM, 0);
   if(sock < 0){
      return -2;
   }

   // name the socket:

	name.sin_family = AF_INET;
   name.sin_port = htons(port);
   name.sin_addr.s_addr = htonl(INADDR_ANY);
	
   if(bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0){
      return -3;
   }
        
   return sock;
}


// Deinitialize udp socket:
// sock (i): socket number
// return value (o): 0 ok, -1 error

static int udp_exit(int sock)
{
	int err;

	#ifdef WIN32
		err = closesocket(sock);
		WSACleanup();
	#else
		err = close(sock);
	#endif

	if(err < 0){
		return -1;
	}

	return 0;
}


// Receiving udp data:
//   - tries to receive packets, as long as data are available
//   - no waiting for timeout
// sock (i): socket number
// buffer (o): buffer for udp data
// maxlen (i): length of buffer
// return value (o): number of received bytes, <0 if error occured

static int udp_receive(int sock, char *buffer, int maxlen)
{
	int nbytes;
	fd_set set;
	struct timeval tout;

	// waiting for data:

	FD_ZERO(&set);
	FD_SET(sock, &set);

	tout.tv_sec = 0;    // no timeout
	tout.tv_usec = 0;

	switch(select(FD_SETSIZE, &set, NULL, NULL, &tout)){
		case 1:
			break;        // data available
		case 0:
			return 0;     // no data available
		default:
	      return -1;    // error
	}

	while(1){

		// receive one packet:

		nbytes = recv(sock, buffer, maxlen, 0);

		if(nbytes < 0){  // receive error
			return -2;
		}

		// check, if more data available: if so, receive another packet
		
		FD_ZERO(&set);
		FD_SET(sock, &set);

		tout.tv_sec = 0;   // no timeout
		tout.tv_usec = 0;

		if(select(FD_SETSIZE, &set, NULL, NULL, &tout) != 1){
			
			// no more data available: check length of received packet and return
			
			if(nbytes >= maxlen){   // buffer overflow
      		return -3;
		   }

			return nbytes;
		}
	}
}


