// vrpn_Tracker_DTrack.C
//
// Advanced Realtime Tracking GmbH's (http://www.ar-tracking.de) DTrack/DTrack2 client

// developed by David Nahon for Virtools VR Pack (http://www.virtools.com)
// (07/20/2004) improved by Advanced Realtime Tracking GmbH (http://www.ar-tracking.de)
// (07/02/2007, 06/29/2009) upgraded by Advanced Realtime Tracking GmbH to support new devices
// (08/25/2010) a correction added by Advanced Realtime Tracking GmbH
// (12/01/2010) support of 3dof objects added by Advanced Realtime Tracking GmbH
//
// Recommended settings within DTrack's 'Settings / Network' or DTrack2's 'Settings / Output' dialog:
//   'ts', '6d', '6df' or '6df2', '6dcal', '3d' (optional)

/* Configuration file:

################################################################################
# Advanced Realtime Tracking GmbH (http://www.ar-tracking.de) DTrack/DTrack2 client 
#
# creates as many vrpn_Tracker as there are bodies or Flysticks, starting with the bodies
# creates 2 analogs per Flystick
# creates 8 buttons per Flystick
#
# NOTE: when using DTrack's older output format for Flystick data ('6df'), the numbering
#       of Flystick buttons differs from DTrack documentation (for compatibility with
#       older vrpn releases)
#
# Arguments:
#  char  name_of_this_device[]
#  int   udp_port                               (DTrack sends data to this UDP port)
#
# Optional arguments:
#  float time_to_reach_joy                      (in seconds; see below)
#  int   number_of_bodies, number_of_flysticks  (fixed numbers of bodies and Flysticks)
#  int   renumbered_ids[]                       (vrpn_Tracker IDs of bodies and Flysticks)
#  char  "3d"                                   (activates 3dof marker output if available;
#                                                always last argument if "-" is not present)
#  char  "-"                                    (activates tracing; always last argument)
#
# NOTE: time_to_reach_joy is the time needed to reach the maximum value (1.0 or -1.0) of the
#       joystick of older 'Flystick' devices when the corresponding button is pressed
#       (one of the last buttons amongst the 8); not necessary for newer 'Flystick2' devices
#       with its analog joystick
#
# NOTE: if fixed numbers of bodies and Flysticks should be used, both arguments
#       number_of_bodies and number_of_flysticks have to be set
#
# NOTE: renumbering of tracker IDs is only possible, if fixed numbers of bodies and
#       Flysticks are set; there has to be an argument present for each body/Flystick

#vrpn_Tracker_DTrack DTrack  5000
#vrpn_Tracker_DTrack DTrack  5000  -
#vrpn_Tracker_DTrack DTrack  5000  3d
#vrpn_Tracker_DTrack DTrack  5000  3d  -
#vrpn_Tracker_DTrack DTrack  5000  0.5
#vrpn_Tracker_DTrack DTrack  5000  0.5  2 2
#vrpn_Tracker_DTrack DTrack  5000  0.5  2 2  2 1 0 3
#vrpn_Tracker_DTrack DTrack  5000  0.5  2 2  2 1 0 3  3d  -

################################################################################
*/

// usually the following should work:

#ifndef _WIN32
	#define OS_UNIX  // for Unix (Linux, Irix, ...)
#else
	#define OS_WIN   // for MS Windows (2000, XP, ...)
#endif

#include <stdio.h>                      // for NULL, printf, fprintf, etc
#include <stdlib.h>                     // for strtod, exit, free, malloc, etc
#include <string.h>                     // for strncmp, memset, strcat, etc

#include "quat.h"                       // for Q_RAD_TO_DEG, etc
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Shared.h"                // for timeval, INVALID_SOCKET, etc
#include "vrpn_Tracker_DTrack.h"
#include "vrpn_Types.h"                 // for vrpn_float64
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#ifdef OS_WIN
  #ifdef VRPN_USE_WINSOCK2
    #include <winsock2.h>    // struct timeval is defined here
  #else
    #include <winsock.h>    // struct timeval is defined here
  #endif
#endif
#ifdef OS_UNIX
	#include <netinet/in.h>                 // for sockaddr_in, INADDR_ANY, etc
	#include <sys/socket.h>                 // for bind, recv, socket, AF_INET, etc
	#include <unistd.h>                     // for close
	#include <sys/select.h>                 // for select, FD_SET, FD_SETSIZE, etc
#endif


// There is a problem with linking on SGIs related to standard libraries.
#ifndef sgi

// --------------------------------------------------------------------------
// Globals:

#define DTRACK2VRPN_BUTTONS_PER_FLYSTICK  8  // number of vrpn buttons per Flystick (fixed)
#define DTRACK2VRPN_ANALOGS_PER_FLYSTICK  2  // number of vrpn analogs per Flystick (fixed)

#define UDPRECEIVE_BUFSIZE       20000  // size of udp buffer for DTrack data (one frame; in bytes)

// --------------------------------------------------------------------------

// Local error codes:

#define DTRACK_ERR_NONE       0  // no error
#define DTRACK_ERR_TIMEOUT    1  // timeout while receiving data
#define DTRACK_ERR_UDP        2  // UDP receive error
#define DTRACK_ERR_PARSE      3  // error in UDP packet

// Local prototypes:

static char* string_nextline(char* str, char* start, int len);
static char* string_get_i(char* str, int* i);
static char* string_get_ui(char* str, unsigned int* ui);
static char* string_get_d(char* str, double* d);
static char* string_get_f(char* str, float* f);
static char* string_get_block(char* str, const char* fmt, int* idat, float* fdat);

static vrpn_Tracker_DTrack::socket_type udp_init(unsigned short port);
static int udp_exit(vrpn_Tracker_DTrack::socket_type sock);
static int udp_receive(vrpn_Tracker_DTrack::socket_type sock, void *buffer, int maxlen, int tout_us);


// --------------------------------------------------------------------------
// Constructor:
// name (i): device name
// c (i): vrpn_Connection
// dtrackPort (i): DTrack UDP port
// timeToReachJoy (i): time needed to reach the maximum value of the joystick
// fixNbody, fixNflystick (i): fixed numbers of DTrack bodies and Flysticks (-1 if not wanted)
// fixId (i): renumbering of targets; must have exactly (fixNbody + fixNflystick) elements (NULL if not wanted)
// actTracing (i): activate trace output

vrpn_Tracker_DTrack::vrpn_Tracker_DTrack(const char *name, vrpn_Connection *c, 
	                                      int dtrackPort, float timeToReachJoy,
	                                      int fixNbody, int fixNflystick, int* fixId,
	                                      bool act3DOFout, bool actTracing) :
	vrpn_Tracker(name, c),	  
	vrpn_Button_Filter(name, c),
	vrpn_Analog(name, c)
{

	int i;
	
	// Dunno how many of these there are yet...
	
	num_sensors = 0;
	num_channel = 0;
	num_buttons = 0;
	
	// init variables: general
	output_3dof_marker = act3DOFout;
	tracing = actTracing;
	tracing_frames = 0;

	tim_first.tv_sec = tim_first.tv_usec = 0;  // also used to recognize the first frame
	tim_last.tv_sec = tim_last.tv_usec = 0;

	// init variables: DTrack data

	if(fixNbody >= 0 && fixNflystick >= 0){    // fixed numbers of bodies and Flysticks should be used
		use_fix_numbering = true;
		
		fix_nbody = fixNbody;
		fix_nflystick = fixNflystick;

		fix_idbody.resize(fix_nbody + fix_nflystick);
		fix_idflystick.resize(fix_nflystick);

		if(fixId){  // take the renumbering information for bodies and Flysticks
			for(i=0; i<fix_nbody + fix_nflystick; i++){  // take the renumbering information for bodies
				fix_idbody[i] = fixId[i];
			}
		
			for(i=0; i<fix_nflystick; i++){  // take the renumbering information for Flysticks (vrpn button data)
				fix_idflystick[i] = fixId[i + fix_nbody];
			}

			for(i=0; i<fix_nbody; i++){      // remove all bodies from the Flystick renumbering data
				for(int j=0; j<fix_nflystick; j++){
					if(fixId[i] < fixId[j + fix_nbody] && fix_idflystick[j] > 0){  // be sure to avoid crazy numbers...
						fix_idflystick[j]--;
					}
				}
			}
		}else{  // take identity numbering for bodies and Flysticks
			for(i=0; i<fix_nbody + fix_nflystick; i++){
				fix_idbody[i] = i;
			}
			for(i=0; i<fix_nflystick; i++){
				fix_idflystick[i] = i;
			}
		}
	}else{  // no fixed numbers
		use_fix_numbering = false;
	}

	warning_nbodycal = false;
	
	// init variables: preparing data for VRPN
	
	if(timeToReachJoy > 1e-20){
		joy_incPerSec = 1.f / timeToReachJoy;          // increase of 'joystick' channel
	}else{
		joy_incPerSec = 1e20f;                         // so it reaches immediately
	}

	// init: communicating with DTrack

	if(!dtrack_init(dtrackPort)){
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
	long tts, ttu;
	float dt;
	int nbody, nflystick, i;
	int newid;
	
	// call the generic server mainloop, since we are a server:

	server_mainloop();

	// get data from DTrack:

	if(!dtrack_receive()){
		if(d_lasterror != DTRACK_ERR_TIMEOUT){
			fprintf(stderr, "vrpn_Tracker_DTrack: Receive Error from DTrack.\n");
		}
		return;
	}

	tracing_frames++;

	// get time stamp:

	vrpn_gettimeofday(&timestamp, NULL);
	
	if(act_timestamp >= 0){  // use DTrack time stamp if available
		tts = (long )act_timestamp;
		ttu = (long )((act_timestamp - tts) * 1000000);
		tts += timestamp.tv_sec - timestamp.tv_sec % 86400;  // add day part of vrpn time stamp

		if(tts >= timestamp.tv_sec + 43200 - 1800){  // shift closer to vrpn time stamp
			tts -= 86400; 
		}else if(tts <= timestamp.tv_sec - 43200 - 1800){
			tts += 86400;
		}

		timestamp.tv_sec = tts;
		timestamp.tv_usec = ttu;
	}

	if(tim_first.tv_sec == 0 && tim_first.tv_usec == 0){
		tim_first = tim_last = timestamp;
	}

	dt = (float )vrpn_TimevalDurationSeconds(timestamp, tim_last);
	tim_last = timestamp;
	
	if(tracing && ((tracing_frames % 10) == 0)){
		printf("framenr %u  time %.3lf\n", act_framecounter, vrpn_TimevalDurationSeconds(timestamp, tim_first));
	}

	// find number of targets visible for vrpn to choose the correct vrpn ID numbers:
	// (1) takes fixed number of bodies and Flysticks, if defined in the configuration file
	// (2) otherwise uses the '6dcal' line in DTrack's output, that gives the total number of
	//     calibrated targets, if available
	// (3) otherwise tracks the maximum number of appeared targets

	if(use_fix_numbering){             // fixed numbers should be used
		nbody = fix_nbody;                // number of bodies visible for vrpn
		nflystick = fix_nflystick;        // number of Flysticks visible for vrpn
	}else if(act_has_bodycal_format){  // DTrack/DTrack2 sent information about the number of calibrated targets
		nbody = act_num_bodycal;          // number of bodies visible for vrpn
		nflystick = act_num_flystick;     // number of Flysticks visible for vrpn
	}else{                             // else track the maximum number of appeared targets (at least)
		if(!warning_nbodycal){            // mention warning (once)
			fprintf(stderr, "vrpn_Tracker_DTrack warning: no DTrack '6dcal' data available.\n");
			warning_nbodycal = true;
		}

		nbody = act_num_body;             // number of bodies visible for vrpn
		nflystick = act_num_flystick;     // number of Flysticks visible for vrpn
	}

	// report tracker data to vrpn:

	num_sensors = nbody + nflystick;     // total number of targets visible for vrpn
	num_buttons = nflystick * DTRACK2VRPN_BUTTONS_PER_FLYSTICK;  // 8 buttons per Flystick
	num_channel = nflystick * DTRACK2VRPN_ANALOGS_PER_FLYSTICK;  // 2 channels per joystick/Flystick

	for(i=0; i<act_num_body; i++){       // DTrack standard bodies
		if(act_body[i].id < nbody){       // there might be more DTrack standard bodies than wanted
			if(act_body[i].quality >= 0){     // report position only if body is tracked
				if(use_fix_numbering){
					newid = fix_idbody[act_body[i].id];  // renumbered ID
				}else{
					newid = act_body[i].id;
				}
			
				dtrack2vrpn_body(newid, "", act_body[i].id, act_body[i].loc, act_body[i].rot, timestamp);
			}
		}
	}

	if(num_channel >= static_cast<int>(joy_last.size())){  // adjust length of vector for current joystick value
		size_t j0 = joy_last.size();
		
		joy_simulate.resize(num_channel);
		joy_last.resize(num_channel);

		for(size_t j=j0; j< static_cast<size_t>(num_channel); j++){
			joy_simulate[j] = false;
			joy_last[j] = 0;
		}
	}

	for(i=0; i<(int)act_num_flystick; i++){   // DTrack Flysticks
		if(act_flystick[i].id < nflystick){   // there might be more DTrack Flysticks than wanted
			if(act_flystick[i].quality >= 0){     // report position only if Flystick is tracked
				if(use_fix_numbering){
					newid = fix_idbody[act_flystick[i].id + nbody];  // renumbered ID for position
				}else{
					newid = act_flystick[i].id + nbody;
				}
			
				dtrack2vrpn_body(newid, "f", act_flystick[i].id, act_flystick[i].loc, act_flystick[i].rot,
				                 timestamp);
			}
			
			if(use_fix_numbering){
				newid = fix_idflystick[act_flystick[i].id];  // renumbered ID for buttons and analogs
			}else{
				newid = act_flystick[i].id;
			}
			
			dtrack2vrpn_flystickbuttons(newid, act_flystick[i].id,
			                            act_flystick[i].num_button, act_flystick[i].button, timestamp);
			
			dtrack2vrpn_flystickanalogs(newid, act_flystick[i].id,
			                            act_flystick[i].num_joystick, act_flystick[i].joystick, dt, timestamp);
		}
	}
	
	if (output_3dof_marker) {
		int offset = num_sensors;
		num_sensors += act_num_marker;
		for(i=0; i<act_num_marker; i++){       // DTrack 3dof marker
			dtrack2vrpn_marker(offset + i, "m", act_marker[i].id, act_marker[i].loc, timestamp);
		}
	}

	// finish main loop:

	vrpn_Analog::report_changes();       // report any analog event;
	vrpn_Button::report_changes();       // report any button event;
}


// -----------------------------------------------------------------------------------------
// Helpers:


// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------
// Preparing data for VRPN:
// these functions convert DTrack data to vrpn data

// Preparing marker data:
// id (i): VRPN body ID
// str_dtrack (i): DTrack marker name (just used for trace output)
// id_dtrack (i): DTrack marker ID (just used for trace output)
// loc (i): position
// timestamp (i): timestamp for body data
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack2vrpn_marker(int id, const char* str_dtrack, int id_dtrack,
                                          const float* loc, struct timeval timestamp)
{

	d_sensor = id;
	
	// position (plus converting to unit meter):
	  
	pos[0] = loc[0] / 1000.;
	pos[1] = loc[1] / 1000.;
	pos[2] = loc[2] / 1000.;

	// orientation: none

	q_make(d_quat, 1, 0, 0, 0);
	
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
			
		printf("marker id (DTrack vrpn): %s%d %d  pos (x y z): %.4f %.4f %.4f\n",
			str_dtrack, id_dtrack, id, pos[0], pos[1], pos[2]);
	}

	return 0;
}


// Preparing body data:
// id (i): VRPN body ID
// str_dtrack (i): DTrack body name ('body' or 'Flystick'; just used for trace output)
// id_dtrack (i): DTrack body ID (just used for trace output)
// loc (i): position
// rot (i): orientation (3x3 rotation matrix)
// timestamp (i): timestamp for body data
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack2vrpn_body(int id, const char* str_dtrack, int id_dtrack,
                                          const float* loc, const float* rot, struct timeval timestamp)
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


// Preparing Flystick button data:
// id (i): VRPN Flystick ID
// id_dtrack (i): DTrack Flystick ID (just used for trace output)
// num_but (i): number of buttons
// but (i): button state (1 pressed, 0 not pressed)
// timestamp (i): timestamp for button data
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack2vrpn_flystickbuttons(int id, int id_dtrack, 
                                                     int num_but, const int* but, struct timeval timestamp)
{
	int n, i, ind;

	n = (num_but > DTRACK2VRPN_BUTTONS_PER_FLYSTICK) ? DTRACK2VRPN_BUTTONS_PER_FLYSTICK : num_but;

	// buttons:
	// NOTE: numbering of two buttons (B2 and B4) differs from DTrack documentation, as long as the 'older'
	//       output format '6df' is used by DTrack!

	ind = id * DTRACK2VRPN_BUTTONS_PER_FLYSTICK;
	i = 0;
	while(i < n){
		buttons[ind++] = but[i];
		i++;
	}
	while(i < DTRACK2VRPN_BUTTONS_PER_FLYSTICK){  // fill remaining buttons
		buttons[ind++] = 0;
		i++;
	}
	
	if(act_has_old_flystick_format){  // for backward compatibility!
		// NOTE: numbering of two buttons (button 2 and button 4) differs from DTrack documentation!
		buttons[id * DTRACK2VRPN_BUTTONS_PER_FLYSTICK + 1] = but[3];
		buttons[id * DTRACK2VRPN_BUTTONS_PER_FLYSTICK + 3] = but[1];
	}

	vrpn_Button::timestamp = timestamp;  // timestamp for button event (explicitly necessary)

	// tracing:

	if(tracing && ((tracing_frames % 10) == 0)){
		printf("flystick id (DTrack vrpn): f%d %d  but ", id_dtrack, id);
		for(i=0; i<n; i++){
			printf(" %d", but[i]);
		}
		printf("\n");
	}

	return 0;
}


// Preparing Flystick analog data:
// id (i): VRPN Flystick ID
// id_dtrack (i): DTrack Flystick ID (just used for trace output)
// num_ana (i): number of analogs
// ana (i): analog state (-1 <= ana <= 1)
// dt (i): time since last change
// timestamp (i): timestamp for analog data
// return value (o): 0 ok, -1 error

int vrpn_Tracker_DTrack::dtrack2vrpn_flystickanalogs(int id, int id_dtrack,
	                                int num_ana, const float* ana, float dt, struct timeval timestamp)
{
	int n, i, ind;
	float f;

	n = (num_ana > DTRACK2VRPN_ANALOGS_PER_FLYSTICK) ? DTRACK2VRPN_ANALOGS_PER_FLYSTICK : num_ana;

	// analogs:
	// NOTE: simulation of time varying floating values, if a joystick action of an older
	//       'Flystick' device is recognized!

	ind = id * DTRACK2VRPN_ANALOGS_PER_FLYSTICK;
	i = 0;
	while(i < n){
		f = ana[i];

		// simulation of time varying floating values (actually just necessary for older
		// 'Flystick' devices and for backward compatibility):

		if(f == 0){  // zero position: reset everything
			joy_simulate[ind] = false;
		}else if((f > 0.99 || f < -0.99) && joy_last[ind] == 0){  // extreme change: start simulation
			joy_simulate[ind] = true;
		}

		if(joy_simulate[ind]){  // simulation of time varying floating values
			if(f > 0){
				f = joy_last[ind] + joy_incPerSec * dt;

				if(f >= 1){
					f = 1;
					joy_simulate[ind] = false;
				}
			}else{
				f = joy_last[ind] - joy_incPerSec * dt;

				if(f <= -1){
					f = -1;
					joy_simulate[ind] = false;
				}
			}
		} 

		joy_last[ind] = f;
		channel[ind++] = f;
		i++;
	}
	while(i < DTRACK2VRPN_ANALOGS_PER_FLYSTICK){  // fill remaining analogs
		channel[ind++] = 0;
		i++;
	}
	
	vrpn_Analog::timestamp = timestamp;  // timestamp for analog event (explicitly necessary)

	// tracing:

	if(tracing && ((tracing_frames % 10) == 0)){
		printf("flystick id (DTrack vrpn): f%d %d  ana ", id_dtrack, id);
		for(i=0; i<n; i++){
			printf(" %.2f", ana[i]);
		}
		printf("\n");
	}

	return 0;
}


// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------
// Communication with DTrack:
// these functions receive and parse data packets from DTrack

// Initializing communication with DTrack:
//
// udpport (i): UDP port number to receive data from DTrack
//
// return value (o): initialization was successful (boolean)

bool vrpn_Tracker_DTrack::dtrack_init(int udpport)
{

	d_udpbuf = NULL;
	d_lasterror = DTRACK_ERR_NONE;

	// create UDP socket:

	if(udpport <= 0 || udpport > 65535){
		fprintf(stderr, "vrpn_Tracker_DTrack: Illegal UDP port %d.\n", udpport);
		return false;
	}

	d_udpsock = udp_init((unsigned short )udpport);
	
	if(d_udpsock == INVALID_SOCKET){
		fprintf(stderr, "vrpn_Tracker_DTrack: Cannot Initialize UDP Socket.\n");
		return false;
	}

	d_udptimeout_us = 0;

	// create UDP buffer:

	d_udpbufsize = UDPRECEIVE_BUFSIZE;
	
	d_udpbuf = (char *)malloc(d_udpbufsize);
	
	if(d_udpbuf == NULL){
		udp_exit(d_udpsock);
		d_udpsock = INVALID_SOCKET;
		fprintf(stderr, "vrpn_Tracker_DTrack: Cannot Allocate Memory for UDP Buffer.\n");
		return false;
	}

	// reset actual DTrack data:

	act_framecounter = 0;
	act_timestamp = -1;

	act_num_marker = act_num_body = act_num_flystick = 0;
	act_has_bodycal_format = false;
	act_has_old_flystick_format = false;

	return true;
}


// Deinitializing communication with DTrack:
//
// return value (o): deinitialization was successful (boolean)

bool vrpn_Tracker_DTrack::dtrack_exit(void)
{

	// release buffer:

	if(d_udpbuf != NULL){
		free(d_udpbuf);
	}

	// release UDP socket:

	if(d_udpsock != INVALID_SOCKET){
		udp_exit(d_udpsock);
	}
	
	return true;
}


// ---------------------------------------------------------------------------------------------------
// Receive and process one DTrack data packet (UDP; ASCII protocol):
//
// return value (o): receiving was successful (boolean)

bool vrpn_Tracker_DTrack::dtrack_receive(void)
{
	char* s;
	int i, j, k, l, n, len, id;
	char sfmt[20];
	int iarr[3];
	float f;
	int loc_num_bodycal, loc_num_flystick1, loc_num_meatool;

	if(d_udpsock == INVALID_SOCKET){
		d_lasterror = DTRACK_ERR_UDP;
		return false;
	}

	// defaults:
	
	act_framecounter = 0;
	act_timestamp = -1;

	loc_num_bodycal = -1;  // i.e. not available
	loc_num_flystick1 = loc_num_meatool = 0;
	
	act_has_bodycal_format = false;
	
	// receive UDP packet:

	len = udp_receive(d_udpsock, d_udpbuf, d_udpbufsize-1, d_udptimeout_us);

	if(len == -1){
		d_lasterror = DTRACK_ERR_TIMEOUT;
		return false;
	}
	if(len <= 0){
		d_lasterror = DTRACK_ERR_UDP;
		return false;
	}

	s = d_udpbuf;
	s[len] = '\0';

	// process lines:

	d_lasterror = DTRACK_ERR_PARSE;

	do{
		// line for frame counter:

		if(!strncmp(s, "fr ", 3)){
			s += 3;
			
			if(!(s = string_get_ui(s, &act_framecounter))){  // get frame counter
				act_framecounter = 0;
				return false;
			}

			continue;
		}

		// line for timestamp:

		if(!strncmp(s, "ts ", 3)){
			s += 3;
			
			if(!(s = string_get_d(s, &act_timestamp))){   // get time stamp
				act_timestamp = -1;
				return false;
			}

			continue;
		}
		
		// line for additional information about number of calibrated bodies:

		if(!strncmp(s, "6dcal ", 6)){
			s += 6;

			act_has_bodycal_format = true;

			if(!(s = string_get_i(s, &loc_num_bodycal))){  // get number of calibrated bodies
				return false;
			}

			continue;
		}

		// line for 3dof marker data:

		if(!strncmp(s, "3d ", 3)){
			s += 3;
			act_num_marker = 0;

			if(!(s = string_get_i(s, &n))){               // get number of standard bodies (in line)
				return false;
			}

			if (static_cast<unsigned>(n) > act_marker.size()) {
				act_marker.resize(n);
			}

			for(i=0; i<n; i++){                           // get data of standard bodies
				if(!(s = string_get_block(s, "if", &id, &f))){
					return false;
				}

				act_marker[act_num_marker].id = id;

				if(!(s = string_get_block(s, "fff", NULL, act_marker[act_num_marker].loc))){
					return false;
				}

				act_num_marker++;
			}

			continue;
		}

		// line for standard body data:

		if(!strncmp(s, "6d ", 3)){
			s += 3;
			
			for(i=0; i<act_num_body; i++){  // disable all existing data
				memset(&act_body[i], 0, sizeof(vrpn_dtrack_body_type));
				act_body[i].id = i;
				act_body[i].quality = -1;
			}

			if(!(s = string_get_i(s, &n))){               // get number of standard bodies (in line)
				return false;
			}

			for(i=0; i<n; i++){                           // get data of standard bodies
				if(!(s = string_get_block(s, "if", &id, &f))){
					return false;
				}

				if(id >= act_num_body){  // adjust length of vector
					act_body.resize(id + 1);

					for(j=act_num_body; j<=id; j++){
						memset(&act_body[j], 0, sizeof(vrpn_dtrack_body_type));
						act_body[j].id = j;
						act_body[j].quality = -1;
					}

					act_num_body = id + 1;
				}
				
				act_body[id].id = id;
				act_body[id].quality = f;
				
				if(!(s = string_get_block(s, "fff", NULL, act_body[id].loc))){
					return false;
				}
			
				if(!(s = string_get_block(s, "fffffffff", NULL, act_body[id].rot))){
					return false;
				}
			}
			
			continue;
		}
		
		// line for Flystick data (older format):

		if(!strncmp(s, "6df ", 4)){
			s += 4;

			act_has_old_flystick_format = true;
			
			if(!(s = string_get_i(s, &n))){               // get number of calibrated Flysticks
				return false;
			}

			loc_num_flystick1 = n;
			
			if(n > act_num_flystick){  // adjust length of vector
				act_flystick.resize(n);
				
				act_num_flystick = n;
			}
			
			for(i=0; i<n; i++){                           // get data of Flysticks
				if(!(s = string_get_block(s, "ifi", iarr, &f))){
					return false;
				}
					
				if(iarr[0] != i){  // not expected
					return false;
				}

				act_flystick[i].id = iarr[0];
				act_flystick[i].quality = f;

				act_flystick[i].num_button = 8;
				k = iarr[1];
				for(j=0; j<8; j++){
					act_flystick[i].button[j] = k & 0x01;
					k >>= 1;
				}
				
				act_flystick[i].num_joystick = 2;  // additionally to buttons 5-8
				if(iarr[1] & 0x20){
					act_flystick[i].joystick[0] = -1;
				}else if(iarr[1] & 0x80){
					act_flystick[i].joystick[0] = 1;
				}else{
					act_flystick[i].joystick[0] = 0;
				}
				if(iarr[1] & 0x10){
					act_flystick[i].joystick[1] = -1;
				}else if(iarr[1] & 0x40){
					act_flystick[i].joystick[1] = 1;
				}else{
					act_flystick[i].joystick[1] = 0;
				}
				
				if(!(s = string_get_block(s, "fff", NULL, act_flystick[i].loc))){
					return false;
				}
				
				if(!(s = string_get_block(s, "fffffffff", NULL, act_flystick[i].rot))){
					return false;
				}
			}
			
			continue;
		}
		
		// line for Flystick data (newer format):

		if(!strncmp(s, "6df2 ", 5)){
			s += 5;
			
			act_has_old_flystick_format = false;
			
			if(!(s = string_get_i(s, &n))){               // get number of calibrated Flysticks
				return false;
			}

			if(n > act_num_flystick){  // adjust length of vector
				act_flystick.resize(n);
				
				act_num_flystick = n;
			}
			
			if(!(s = string_get_i(s, &n))){               // get number of Flysticks
				return false;
			}

			for(i=0; i<n; i++){                           // get data of Flysticks
				if(!(s = string_get_block(s, "ifii", iarr, &f))){
					return false;
				}
					
				if(iarr[0] != i){  // not expected
					return false;
				}

				act_flystick[i].id = iarr[0];
				act_flystick[i].quality = f;

				if(iarr[1] > vrpn_DTRACK_FLYSTICK_MAX_BUTTON){
					return false;
				}
				if(iarr[2] > vrpn_DTRACK_FLYSTICK_MAX_JOYSTICK){
					return false;
				}
				act_flystick[i].num_button = iarr[1];
				act_flystick[i].num_joystick = iarr[2];
				
				if(!(s = string_get_block(s, "fff", NULL, act_flystick[i].loc))){
					return false;
				}
				
				if(!(s = string_get_block(s, "fffffffff", NULL, act_flystick[i].rot))){
					return false;
				}

				strcpy(sfmt, "");
				j = 0;
				while(j < act_flystick[i].num_button){
					strcat(sfmt, "i");
					j += 32;
				}
				j = 0;
				while(j < act_flystick[i].num_joystick){
					strcat(sfmt, "f");
					j++;
				}
				
				if(!(s = string_get_block(s, sfmt, iarr, act_flystick[i].joystick))){
					return false;
				}

				k = l = 0;
				for(j=0; j<act_flystick[i].num_button; j++){
					act_flystick[i].button[j] = iarr[k] & 0x01;
					iarr[k] >>= 1;
					
					l++;
					if(l == 32){
						k++;
						l = 0;
					}
				}
			}
			
			continue;
		}

		// line for measurement tool data:

		if(!strncmp(s, "6dmt ", 5)){
			s += 5;
			
			if(!(s = string_get_i(s, &n))){               // get number of calibrated measurement tools
				return false;
			}

			loc_num_meatool = n;

			continue;
		}
		
		// ignore unknown line identifiers (could be valid in future DTracks)
		
	}while((s = string_nextline(d_udpbuf, s, d_udpbufsize)) != NULL);

	// set number of calibrated standard bodies, if necessary:

	if(loc_num_bodycal >= 0){  // '6dcal' information was available
		act_num_bodycal = loc_num_bodycal - loc_num_flystick1 - loc_num_meatool;
	}

	d_lasterror = DTRACK_ERR_NONE;
	return true;
}


// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------
// Parsing DTrack data:

// Search next line in buffer:
// str (i): buffer (total)
// start (i): start position within buffer
// len (i): buffer length in bytes
// return (i): begin of line, NULL if no new line in buffer

static char* string_nextline(char* str, char* start, int len)
{
	char* s = start;
	char* se = str + len;
	int crlffound = 0;

	while(s < se){
		if(*s == '\r' || *s == '\n'){  // crlf
			crlffound = 1;
		}else{
			if(crlffound){              // begin of new line found
				return (*s) ? s : NULL;  // first character is '\0': end of buffer
			}
		}

		s++;
	}

	return NULL;                      // no new line found in buffer
}


// Read next 'int' value from string:
// str (i): string
// i (o): read value
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_i(char* str, int* i)
{
	char* s;
	
	*i = (int )strtol(str, &s, 0);
	return (s == str) ? NULL : s;
}


// Read next 'unsigned int' value from string:
// str (i): string
// ui (o): read value
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_ui(char* str, unsigned int* ui)
{
	char* s;
	
	*ui = (unsigned int )strtoul(str, &s, 0);
	return (s == str) ? NULL : s;
}


// Read next 'double' value from string:
// str (i): string
// d (o): read value
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_d(char* str, double* d)
{
	char* s;
	
	*d = strtod(str, &s);
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
// fmt (i): format string ('i' for 'int', 'f' for 'float')
// idat (o): array for 'int' values (long enough due to fmt)
// fdat (o): array for 'float' values (long enough due to fmt)
// return value (o): pointer behind read value in str; NULL in case of error

static char* string_get_block(char* str, const char* fmt, int* idat, float* fdat)
{
	char* strend;
	int index_i, index_f;

	if((str = strchr(str, '[')) == NULL){       // search begin of block
		return NULL;
	}
	if((strend = strchr(str, ']')) == NULL){    // search end of block
		return NULL;
	}
	
	str++;                               // (temporarily) remove delimiters
	*strend = '\0';

	index_i = index_f = 0;

	while(*fmt){
		switch(*fmt++){
			case 'i':
				if((str = string_get_i(str, &idat[index_i++])) == NULL){
					*strend = ']';
					return NULL;
				}
				break;
				
			case 'f':
				if((str = string_get_f(str, &fdat[index_f++])) == NULL){
					*strend = ']';
					return NULL;
				}
				break;
				
			default:    // unknown format character
				*strend = ']';
				return NULL;
		}
	}

	// ignore additional data inside the block
	
	*strend = ']';
	return strend + 1;
}


// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------
// Handling UDP data:

#ifdef OS_UNIX
#define INVALID_SOCKET -1
#endif

// Initialize UDP socket:
// port (i): port number
// return value (o): socket number, NULL if error

static vrpn_Tracker_DTrack::socket_type udp_init(unsigned short port)
{
	vrpn_Tracker_DTrack::socket_type sock;
	struct sockaddr_in name;

	// initialize socket dll (only Windows):

#ifdef OS_WIN
	{
		WORD vreq;
		WSADATA wsa;

		vreq = MAKEWORD(2, 0);
		
		if(WSAStartup(vreq, &wsa) != 0){
			return NULL;
		}
	}
#endif
        
	// create socket:

#ifdef OS_UNIX
	{
		int usock;
		
		usock = socket(PF_INET, SOCK_DGRAM, 0);
	
		if(usock < 0){
			return INVALID_SOCKET;
		}
		sock = usock;
	}
#endif
#ifdef OS_WIN
	{
		SOCKET wsock;
		
		wsock = socket(PF_INET, SOCK_DGRAM, 0);

		if(wsock == INVALID_SOCKET){
			WSACleanup();
			return INVALID_SOCKET;
		}

		sock = wsock;
	}
#endif

   // name socket:

	name.sin_family = AF_INET;
   name.sin_port = htons(port);
   name.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0){
		udp_exit(sock);
		return INVALID_SOCKET;
	}
        
   return sock;
}


// Deinitialize UDP socket:
// sock (i): socket number
// return value (o): 0 ok, -1 error

static int udp_exit(vrpn_Tracker_DTrack::socket_type sock)
{
	int err;

#ifdef OS_UNIX
	err = close(sock);
#endif
#ifdef OS_WIN
	err = closesocket(sock);
	WSACleanup();
#endif

	if(err < 0){
		return -1;
	}

	return 0;
}


// Receive UDP data:
//   - tries to receive packets, as long as data are available
// sock (i): socket number
// buffer (o): buffer for UDP data
// maxlen (i): length of buffer
// tout_us (i): timeout in us (micro sec)
// return value (o): number of received bytes, <0 if error/timeout occurred

// Don't tell us about the FD_SET causing a conditional expression to be constant
#ifdef	_WIN32
#pragma warning ( disable : 4127 )
#endif

static int udp_receive(vrpn_Tracker_DTrack::socket_type sock, void *buffer, int maxlen, int tout_us)
{
	int nbytes, err;
	fd_set set;
	struct timeval tout;

	// waiting for data:

	FD_ZERO(&set);
	FD_SET(sock, &set);

	tout.tv_sec = tout_us / 1000000;
	tout.tv_usec = tout_us % 1000000;

	switch((err = select(FD_SETSIZE, &set, NULL, NULL, &tout))){
		case 1:
			break;        // data available
		case 0:
			return -1;    // timeout
		default:
	      return -2;    // error
	}

	// receiving packet:

	while(1){

		// receive one packet:

		nbytes = recv(sock, (char *)buffer, maxlen, 0);

		if(nbytes < 0){  // receive error
			return -3;
		}

		// check, if more data available: if so, receive another packet
		
		FD_ZERO(&set);
		FD_SET(sock, &set);

		tout.tv_sec = 0;   // timeout with value of zero, thus no waiting
		tout.tv_usec = 0;

		if(select(FD_SETSIZE, &set, NULL, NULL, &tout) != 1){
			
			// no more data available: check length of received packet and return
			
			if(nbytes >= maxlen){   // buffer overflow
      		return -4;
		   }

			return nbytes;
		}
	}
}

#endif

