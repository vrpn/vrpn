#ifndef _WIN32_WCE
#include <time.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef	_WIN32_WCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <ctype.h>


// NOTE: a vrpn tracker must call user callbacks with tracker data (pos and
//       ori info) which represent the transformation xfSourceFromSensor.
//       This means that the pos info is the position of the origin of
//       the sensor coord sys in the source coord sys space, and the
//       quat represents the orientation of the sensor relative to the
//       source space (ie, its value rotates the source's axes so that
//       they coincide with the sensor's)

#ifdef linux
#include <termios.h>
#endif

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"
#ifndef _WIN32
#include <netinet/in.h>
#endif

#ifdef	_WIN32
#ifndef _WIN32_WCE
#include <io.h>
#endif
#endif

#include "vrpn_Tracker.h"

#include "vrpn_RedundantTransmission.h"

#ifndef VRPN_CLIENT_ONLY
#include "vrpn_Serial.h"
#endif

static const char *default_tracker_cfg_file_name = "vrpn_Tracker.cfg";
//this is used unless we pass in a path & file name into the vrpn_TRACKER
//constructor

#define vrpn_ser_tkr_MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

//#define VERBOSE
// #define READ_HISTOGRAM

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


vrpn_Tracker::vrpn_Tracker (const char * name, vrpn_Connection * c,
			    const char * tracker_cfg_file_name) :
vrpn_BaseClass(name, c)
{
	FILE	*config_file;
	vrpn_BaseClass::init();

	// Set the current time to zero, just to have something there
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	// Set the sensor to 0 just to have something in there.
	d_sensor = 0;

	// Set the position to the origin and the orientation to identity
	// just to have something there in case nobody fills them in later
	pos[0] = pos[1] = pos[2] = 0.0;
	d_quat[0] = d_quat[1] = d_quat[2] = 0.0;
	d_quat[3] = 1.0;

	// Set the velocity to zero and the orientation to identity
	// just to have something there in case nobody fills them in later
	vel[0] = vel[1] = vel[2] = 0.0;
	vel_quat[0] = vel_quat[1] = vel_quat[2] = 0.0;
	vel_quat[3] = 1.0;
	vel_quat_dt = 1;

	// Set the acceleration to zero and the orientation to identity
	// just to have something there in case nobody fills them in later
	acc[0] = acc[1] = acc[2] = 0.0;
	acc_quat[0] = acc_quat[1] = acc_quat[2] = 0.0;
	acc_quat[3] = 1.0;
	acc_quat_dt = 1;

#ifdef DESKTOP_PHANTOM_DEFAULTS
        // Defaults for Desktop Phantom.
	tracker2room[0] = tracker2room[1] = 0.0;
        tracker2room[2] = -0.28;
#else
	// Set the room to tracker and sensor to unit transforms to identity
  	tracker2room[0] = tracker2room[1] = tracker2room[2] = 0.0;
#endif
	tracker2room_quat[0] = tracker2room_quat[1] = tracker2room_quat[2] = 0.0;
	tracker2room_quat[3] = 1.0;

	num_sensors = 1;
	for (vrpn_int32 sens = 0; sens < vrpn_TRACKER_MAX_SENSORS; sens++){
	    unit2sensor[sens][0] = unit2sensor[sens][1] = 
						unit2sensor[sens][2] = 0.0;
	    unit2sensor_quat[sens][0] = unit2sensor_quat[sens][1] = 0.0;
	    unit2sensor_quat[sens][2] = 0.0;
	    unit2sensor_quat[sens][3] = 1.0;
	}

#ifdef DESKTOP_PHANTOM_DEFAULTS
        // Defaults for Desktop Phantom. 
	workspace_min[0] = workspace_min[1] = -0.2;
        workspace_min[2] = -0.1;
	workspace_max[0] = workspace_max[1] = workspace_max[2] = 0.2;
#else
  	workspace_min[0] = workspace_min[1] = workspace_min[2] = -0.5;
  	workspace_max[0] = workspace_max[1] = workspace_max[2] = 0.5;
#endif
	// replace defaults with values from tracker config file
	// if it exists
	if (tracker_cfg_file_name==NULL) { //the default argument value
	  tracker_cfg_file_name=default_tracker_cfg_file_name;
	}
	if ((config_file = fopen(tracker_cfg_file_name, "r")) == NULL) {
	  // Can't find the config file
	  // Complain only if we are using the a non-default config file
	  // (Since most people don't use any config file at all,
	  // and would be confused to see an error about not being
	  // able to open the default config file)
	  if (tracker_cfg_file_name!=default_tracker_cfg_file_name) {
	      fprintf(stderr, "vrpn_Tracker: Can't find config file %s\n",
		      tracker_cfg_file_name);
	    }
	} else if (read_config_file(config_file, name)) {
		fprintf(stderr, "vrpn_Tracker: Found config file %s, but cannot read info for %s\n",
				tracker_cfg_file_name, name);
		fclose(config_file);
	} else {	// no problems
		fprintf(stderr,"vrpn_Tracker: Read room and sensor info from %s\n",
			tracker_cfg_file_name);
		fclose(config_file);
	}
}

int vrpn_Tracker::register_types(void)
{
	// Register this tracker device and the needed message types
	if (d_connection) {
	  position_m_id = d_connection->register_message_type("vrpn_Tracker Pos_Quat");
	  velocity_m_id = d_connection->register_message_type("vrpn_Tracker Velocity");
	  accel_m_id =d_connection->register_message_type("vrpn_Tracker Acceleration");
	  tracker2room_m_id = d_connection->register_message_type("vrpn_Tracker To_Room");
	  unit2sensor_m_id = d_connection->register_message_type("vrpn_Tracker Unit_To_Sensor");
	  request_t2r_m_id = d_connection->register_message_type("vrpn_Tracker Request_Tracker_To_Room");
	  request_u2s_m_id = d_connection->register_message_type("vrpn_Tracker Request_Unit_To_Sensor");
	  workspace_m_id = d_connection->register_message_type("vrpn_Tracker Workspace");
	  request_workspace_m_id = d_connection->register_message_type("vrpn_Tracker Request_Tracker_Workspace");
	  update_rate_id = d_connection->register_message_type("vrpn_Tracker set_update_rate");
	  reset_origin_m_id = d_connection->register_message_type("vrpn_Tracker Reset_Origin");
	}
	return 0;
}

// virtual
vrpn_Tracker::~vrpn_Tracker (void) {

}

int vrpn_Tracker::read_config_file (FILE * config_file,
                                    const char * tracker_name) {

    char	line[512];	// line read from input file
    vrpn_int32	num_sens;
    vrpn_int32 	which_sensor;
    float	f[14];
    int 	i,j;

    // Read lines from the file until we run out
    while ( fgets(line, sizeof(line), config_file) != NULL ) {
	// Make sure the line wasn't too long
	if (strlen(line) >= sizeof(line)-1) {
		 fprintf(stderr,"Line too long in config file: %s\n",line);
		 return -1;
	}
	// find tracker name in file 
	if ((!(strncmp(line,tracker_name,strlen(tracker_name)))) && 
		(isspace(line[strlen(tracker_name)]) )) {
	    // get the tracker2room xform and the workspace volume
	    if (fgets(line, sizeof(line), config_file) == NULL) break;
	    if (sscanf(line, "%f%f%f", &f[0], &f[1], &f[2]) != 3) break;
	    if (fgets(line, sizeof(line), config_file) == NULL) break;
	    if (sscanf(line, "%f%f%f%f",&f[3],&f[4], &f[5], &f[6]) != 4) break;
	    if (fgets(line, sizeof(line), config_file) == NULL) break;
	    if (sscanf(line, "%f%f%f%f%f%f",&f[7],&f[8],&f[9],&f[10],&f[11],
							&f[12]) != 6) break;

	    for (i = 0; i < 3; i++){
		tracker2room[i] = f[i];
		workspace_min[i] = f[i+7];
		workspace_max[i] = f[i+10];
	    }
	    for (i = 0; i < 4; i++)
		tracker2room_quat[i] = f[i+3];
	    // get the number of sensors
	    if (fgets(line, sizeof(line), config_file) == NULL) break;
	    if ((sscanf(line, "%d", &num_sens) != 1) ||
		(num_sens > vrpn_TRACKER_MAX_SENSORS)) break;
	    for (i = 0; i < num_sens; i++){
		// get which sensor this xform is for
		if (fgets(line, sizeof(line), config_file) == NULL) break;
		if ((sscanf(line, "%d", &which_sensor) != 1) ||
		    (which_sensor > vrpn_TRACKER_MAX_SENSORS)) break;
		// get the sensor to unit xform
		if (fgets(line, sizeof(line), config_file) == NULL) break;
		if (sscanf(line, "%f%f%f", &f[0], &f[1], &f[2]) != 3) break;
		if (fgets(line, sizeof(line), config_file) == NULL) break;
		if (sscanf(line,"%f%f%f%f",&f[3],&f[4],&f[5],&f[6]) != 4) break;
		for (j = 0; j < 3; j++)
		    unit2sensor[which_sensor][j] = f[j];
		for (j = 0; j < 4; j++)
		    unit2sensor_quat[which_sensor][j] = f[j+3];
	    }
	    num_sensors = num_sens;
	    return 0;	// success
	}
    }
    fprintf(stderr, "Error reading or %s not found in config file\n", 
		tracker_name);
    return -1;
}

void vrpn_Tracker::print_latest_report(void)
{
   printf("----------------------------------------------------\n");
   printf("Sensor   :%d\n", d_sensor);
   printf("Timestamp:%ld:%ld\n", timestamp.tv_sec, timestamp.tv_usec);
   printf("Pos      :%lf, %lf, %lf\n", pos[0],pos[1],pos[2]);
   printf("Quat     :%lf, %lf, %lf, %lf\n",
          d_quat[0],d_quat[1],d_quat[2],d_quat[3]);
}

int vrpn_Tracker::register_server_handlers(void)
{
    if (d_connection){
 		if (register_autodeleted_handler(request_t2r_m_id,
				handle_t2r_request, this, d_sender_id))
		{
			fprintf(stderr,"vrpn_Tracker:can't register t2r handler\n");
			return -1;
		}
		if (register_autodeleted_handler(request_u2s_m_id,
				handle_u2s_request, this, d_sender_id))
		{
			fprintf(stderr,"vrpn_Tracker:can't register u2s handler\n");
			return -1;
		}
		if (register_autodeleted_handler(request_workspace_m_id,
			handle_workspace_request, this, d_sender_id))
		{
			fprintf(stderr,"vrpn_Tracker:  "
								   "Can't register workspace handler\n");
			return -1;
		}
    } else {
		return -1;
	}
    return 0;
}

// put copies of vector and quat into arrays passed in
void vrpn_Tracker::get_local_t2r(vrpn_float64 *vec, vrpn_float64 *quat)
{
    int i;
    for (i = 0; i < 3; i++)
	vec[i] = tracker2room[i];
    for (i = 0; i < 4; i++)
	quat[i] = tracker2room_quat[i];
}

// put copies of vector and quat into arrays passed in
void vrpn_Tracker::get_local_u2s(vrpn_int32 sensor, vrpn_float64 *vec, vrpn_float64 *quat)
{
    int i;
    for (i = 0; i < 3; i++)
	vec[i] = unit2sensor[sensor][i];
    for (i = 0; i < 4; i++)
	quat[i] = unit2sensor_quat[sensor][i];
}

int vrpn_Tracker::handle_t2r_request(void *userdata, vrpn_HANDLERPARAM p)
{
    struct timeval current_time;
    char    msgbuf[1000];
    vrpn_int32     len;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata; // == this normally

    p = p; // Keep the compiler from complaining

    gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our t2r transform was read in by the constructor

    // send t2r transform
    if (me->d_connection) {
        len = me->encode_tracker2room_to(msgbuf);
        if (me->d_connection->pack_message(len, me->timestamp,
            me->tracker2room_m_id, me->d_sender_id,
            msgbuf, vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker: cannot write t2r message\n");
        }
    }
    return 0;
}

int vrpn_Tracker::handle_u2s_request(void *userdata, vrpn_HANDLERPARAM p)
{
    struct timeval current_time;
    char    msgbuf[1000];
    vrpn_int32     len;
    vrpn_int32 i;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata;

    p = p; // Keep the compiler from complaining

    gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our u2s transforms were read in by the constructor

    if (me->d_connection){
	for (i = 0; i < me->num_sensors; i++){
	    me->d_sensor = i;
            // send u2s transform
            len = me->encode_unit2sensor_to(msgbuf);
            if (me->d_connection->pack_message(len, me->timestamp,
                me->unit2sensor_m_id, me->d_sender_id,
                msgbuf, vrpn_CONNECTION_RELIABLE)) {
                fprintf(stderr, "vrpn_Tracker: cannot write u2s message\n");
	    }
        }
    }
    return 0;
}

int vrpn_Tracker::handle_workspace_request(void *userdata, vrpn_HANDLERPARAM p)
{
    struct timeval current_time;
    char    msgbuf[1000];
    vrpn_int32     len;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata;

    p = p; // Keep the compiler from complaining

    gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our workspace was read in by the constructor

    if (me->d_connection){
        len = me->encode_workspace_to(msgbuf);
        if (me->d_connection->pack_message(len, me->timestamp,
            me->workspace_m_id, me->d_sender_id,
            msgbuf, vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker: cannot write workspace message\n");
        }
    }
    return 0;
}

/** Encodes the "Tracker to Room" transformation into the buffer
    specified. Returns the number of bytes encoded into the
    buffer.  Assumes that the buffer can hold all of the data.
    Encodes the position, then the quaternion.
*/

int     vrpn_Tracker::encode_tracker2room_to(char *buf)
{
	char	*bufptr = buf;
	int	buflen = 1000;
	int	i;

	// Encode the position part of the transformation.
	for (i = 0; i < 3; i++) {
		vrpn_buffer( &bufptr, &buflen, tracker2room[i] );
	}

	// Encode the quaternion part of the transformation.
	for (i = 0; i < 4; i++) {
		vrpn_buffer( &bufptr, &buflen, tracker2room_quat[i] );
	}

	// Return the number of characters sent.
	return 1000-buflen;
}

/** Encodes the "Unit to Sensor" transformation into the buffer
    specified. Returns the number of bytes encoded into the
    buffer.  Assumes that the buffer can hold all of the data.
    Encodes the position, then the quaternion.

    WARNING: make sure sensor is set to desired value before encoding
    this message.
*/

int	vrpn_Tracker::encode_unit2sensor_to(char *buf)
{
	char	*bufptr = buf;
	int	buflen = 1000;
	int	i;

	// Encode the sensor number, then put a filler in32 to re-align
	// to the 64-bit boundary.
	vrpn_buffer( &bufptr, &buflen, d_sensor );
	vrpn_buffer( &bufptr, &buflen, (vrpn_int32)(0) );

	// Encode the position part of the transformation.
	for (i = 0; i < 3; i++) {
		vrpn_buffer( &bufptr, &buflen, unit2sensor[d_sensor][i] );
	}

	// Encode the quaternion part of the transformation.
	for (i = 0; i < 4; i++) {
		vrpn_buffer( &bufptr, &buflen, unit2sensor_quat[d_sensor][i] );
	}

	// Return the number of characters sent.
	return 1000-buflen;
}

int	vrpn_Tracker::encode_workspace_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   vrpn_buffer(&bufptr, &buflen, workspace_min[0]);
   vrpn_buffer(&bufptr, &buflen, workspace_min[1]);
   vrpn_buffer(&bufptr, &buflen, workspace_min[2]);

   vrpn_buffer(&bufptr, &buflen, workspace_max[0]);
   vrpn_buffer(&bufptr, &buflen, workspace_max[1]);
   vrpn_buffer(&bufptr, &buflen, workspace_max[2]);

   return 1000 - buflen;
}

// NOTE: you need to be sure that if you are sending vrpn_float64 then 
//       the entire array needs to remain aligned to 8 byte boundaries
//	 (malloced data and static arrays are automatically alloced in
//	  this way).  Assumes that there is enough room to store the
//	 entire message.  Returns the number of characters sent.
int	vrpn_Tracker::encode_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   // Message includes: long sensor, long scrap, vrpn_float64 pos[3], vrpn_float64 quat[4]
   // Byte order of each needs to be reversed to match network standard

   vrpn_buffer(&bufptr, &buflen, d_sensor);
   vrpn_buffer(&bufptr, &buflen, d_sensor); // This is just to take up space to align

   vrpn_buffer(&bufptr, &buflen, pos[0]);
   vrpn_buffer(&bufptr, &buflen, pos[1]);
   vrpn_buffer(&bufptr, &buflen, pos[2]);

   vrpn_buffer(&bufptr, &buflen, d_quat[0]);
   vrpn_buffer(&bufptr, &buflen, d_quat[1]);
   vrpn_buffer(&bufptr, &buflen, d_quat[2]);
   vrpn_buffer(&bufptr, &buflen, d_quat[3]);

   return 1000 - buflen;
}

int	vrpn_Tracker::encode_vel_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   // Message includes: long unitNum, vrpn_float64 vel[3], vrpn_float64 vel_quat[4]
   // Byte order of each needs to be reversed to match network standard

   vrpn_buffer(&bufptr, &buflen, d_sensor);
   vrpn_buffer(&bufptr, &buflen, d_sensor); // This is just to take up space to align

   vrpn_buffer(&bufptr, &buflen, vel[0]);
   vrpn_buffer(&bufptr, &buflen, vel[1]);
   vrpn_buffer(&bufptr, &buflen, vel[2]);

   vrpn_buffer(&bufptr, &buflen, vel_quat[0]);
   vrpn_buffer(&bufptr, &buflen, vel_quat[1]);
   vrpn_buffer(&bufptr, &buflen, vel_quat[2]);
   vrpn_buffer(&bufptr, &buflen, vel_quat[3]);

   vrpn_buffer(&bufptr, &buflen, vel_quat_dt);

   return 1000 - buflen;
}

int	vrpn_Tracker::encode_acc_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   // Message includes: long unitNum, vrpn_float64 acc[3], vrpn_float64 acc_quat[4]
   // Byte order of each needs to be reversed to match network standard

   vrpn_buffer(&bufptr, &buflen, d_sensor);
   vrpn_buffer(&bufptr, &buflen, d_sensor); // This is just to take up space to align

   vrpn_buffer(&bufptr, &buflen, acc[0]);
   vrpn_buffer(&bufptr, &buflen, acc[1]);
   vrpn_buffer(&bufptr, &buflen, acc[2]);

   vrpn_buffer(&bufptr, &buflen, acc_quat[0]);
   vrpn_buffer(&bufptr, &buflen, acc_quat[1]);
   vrpn_buffer(&bufptr, &buflen, acc_quat[2]);
   vrpn_buffer(&bufptr, &buflen, acc_quat[3]);

   vrpn_buffer(&bufptr, &buflen, acc_quat_dt);

   return 1000 - buflen;
}


vrpn_Tracker_NULL::vrpn_Tracker_NULL
                  (const char * name, vrpn_Connection * c,
	           vrpn_int32 sensors, vrpn_float64 Hz) :
    vrpn_Tracker(name, c), update_rate(Hz),
    num_sensors(sensors),
    d_redundancy (NULL)
{
	register_server_handlers();
	// Nothing left to do
}

void	vrpn_Tracker_NULL::mainloop()
{
	struct timeval current_time;
	char	msgbuf[1000];
	vrpn_int32	i,len;

	// Call the generic server mainloop routine, since this is a server
	server_mainloop();

	// See if its time to generate a new report
	gettimeofday(&current_time, NULL);
	if ( duration(current_time,timestamp) >= 1000000.0/update_rate) {

	  // Update the time
	  timestamp.tv_sec = current_time.tv_sec;
	  timestamp.tv_usec = current_time.tv_usec;

	  // Send messages for all sensors if we have a connection
	  if (d_redundancy) {
	    for (i = 0; i < num_sensors; i++) {
		d_sensor = i;

		// Pack position report
		len = encode_to(msgbuf);
		if (d_redundancy->pack_message(len, timestamp,
			position_m_id, d_sender_id, msgbuf,
                        vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}

		// Pack velocity report
		len = encode_vel_to(msgbuf);
		if (d_redundancy->pack_message(len, timestamp,
			velocity_m_id, d_sender_id, msgbuf,
                        vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}

		// Pack acceleration report
		len = encode_acc_to(msgbuf);
		if (d_redundancy->pack_message(len, timestamp,
			accel_m_id, d_sender_id, msgbuf,
                        vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}
	    }
	  } else if (d_connection) {
	    for (i = 0; i < num_sensors; i++) {
		d_sensor = i;

		// Pack position report
		len = encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			position_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}

		// Pack velocity report
		len = encode_vel_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			velocity_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}

		// Pack acceleration report
		len = encode_acc_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			accel_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}
	    }
	  }
	}
}

void vrpn_Tracker_NULL::setRedundantTransmission
                              (vrpn_RedundantTransmission * t) {
  d_redundancy = t;
}


vrpn_Tracker_Server::vrpn_Tracker_Server
                  (const char * name, vrpn_Connection * c,
	           vrpn_int32 sensors) :
    vrpn_Tracker(name, c),
    num_sensors(sensors)
{
	register_server_handlers();
	// Nothing left to do
}

void	vrpn_Tracker_Server::mainloop()
{
	// Call the generic server mainloop routine, since this is a server
	server_mainloop();
}

int	vrpn_Tracker_Server::report_pose(int sensor, struct timeval t,
									 vrpn_float64 position[3], vrpn_float64 quaternion[4])
{
	char	msgbuf[1000];
	vrpn_int32	len;

	  // Update the time
	  timestamp.tv_sec = t.tv_sec;
	  timestamp.tv_usec = t.tv_usec;

	  // Send messages for all sensors if we have a connection
	  if (sensor >= num_sensors) {
		  send_text_message("Sensor number too high", timestamp, vrpn_TEXT_ERROR);
		  return -1;
	  } else if (!d_connection) {
		  send_text_message("No connection", timestamp, vrpn_TEXT_ERROR);
		  return -1;
	  } else {
		d_sensor = sensor;

		// Pack position report
		memcpy(pos, position, sizeof(pos));
		memcpy(d_quat, quaternion, sizeof(d_quat));
		len = encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			position_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"vrpn_Tracker_Server: can't write message: tossing\n");
		 return -1;
		}
	  }
	  return 0;
}

int	vrpn_Tracker_Server::report_pose_velocity(int sensor, struct timeval t,
									 vrpn_float64 position[3], vrpn_float64 quaternion[4],
									 vrpn_float64 interval)
{
	char	msgbuf[1000];
	vrpn_int32	len;

	  // Update the time
	  timestamp.tv_sec = t.tv_sec;
	  timestamp.tv_usec = t.tv_usec;

	  // Send messages for all sensors if we have a connection
	  if (sensor >= num_sensors) {
		  send_text_message("Sensor number too high", timestamp, vrpn_TEXT_ERROR);
		  return -1;
	  } else if (!d_connection) {
		  send_text_message("No connection", timestamp, vrpn_TEXT_ERROR);
		  return -1;
	  } else {
		d_sensor = sensor;

		// Pack velocity report
		memcpy(vel, position, sizeof(pos));
		memcpy(vel_quat, quaternion, sizeof(d_quat));
		vel_quat_dt = interval;
		len = encode_vel_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			velocity_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"vrpn_Tracker_Server: can't write message: tossing\n");
		 return -1;
		}
	  }

	return 0;
}

int	vrpn_Tracker_Server::report_pose_acceleration(int sensor, struct timeval t,
									 vrpn_float64 position[3], vrpn_float64 quaternion[4],
									 vrpn_float64 interval)
{
	char	msgbuf[1000];
	vrpn_int32	len;

	  // Update the time
	  timestamp.tv_sec = t.tv_sec;
	  timestamp.tv_usec = t.tv_usec;

	  // Send messages for all sensors if we have a connection
	  if (sensor >= num_sensors) {
		  send_text_message("Sensor number too high", timestamp, vrpn_TEXT_ERROR);
		  return -1;
	  } else if (!d_connection) {
		  send_text_message("No connection", timestamp, vrpn_TEXT_ERROR);
		  return -1;
	  } else {
		d_sensor = sensor;

		// Pack acceleration report
		memcpy(acc, position, sizeof(pos));
		memcpy(acc_quat, quaternion, sizeof(d_quat));
		acc_quat_dt = interval;
		len = encode_acc_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			accel_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"vrpn_Tracker_Server: can't write message: tossing\n");
		 return -1;
		}
	  }

	return 0;
}


#ifndef VRPN_CLIENT_ONLY
vrpn_Tracker_Serial::vrpn_Tracker_Serial
                    (const char * name, vrpn_Connection * c,
	             const char * port, long baud) :
    vrpn_Tracker(name, c)
{
   register_server_handlers();
   // Find out the port name and baud rate
   if (port == NULL) {
	fprintf(stderr,"vrpn_Tracker_Serial: NULL port name\n");
	status = vrpn_TRACKER_FAIL;
	return;
   } else {
	strncpy(portname, port, sizeof(portname));
	portname[sizeof(portname)-1] = '\0';
   }
   baudrate = baud;

   // Open the serial port we're going to use
   if ( (serial_fd=vrpn_open_commport(portname, baudrate)) == -1) {
	fprintf(stderr,"vrpn_Tracker_Serial: Cannot Open serial port\n");
	status = vrpn_TRACKER_FAIL;
   }

   // Reset the tracker and find out what time it is
   status = vrpn_TRACKER_RESETTING;
   gettimeofday(&timestamp, NULL);
}

void vrpn_Tracker_Serial::send_report(void)
{
    // Send the message on the connection
    if (d_connection) {
	    char	msgbuf[1000];
	    int	len = encode_to(msgbuf);
	    if (d_connection->pack_message(len, timestamp,
		    position_m_id, d_sender_id, msgbuf,
		    vrpn_CONNECTION_LOW_LATENCY)) {
	      fprintf(stderr,"Tracker: cannot write message: tossing\n");
	    }
    } else {
	    fprintf(stderr,"Tracker: No valid connection\n");
    }
}

/** This function should be called each time through the main loop
    of the server code. It polls for a report from the tracker and
    sends them if there are one or more. It will reset the tracker
    if there is no data from it for a few seconds.
**/
void vrpn_Tracker_Serial::mainloop()
{
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

	    while (get_report()) {	// While we get reports, continue to send them.
		send_report();
	    };

	    struct timeval current_time;
	    gettimeofday(&current_time, NULL);
	    if ( duration(current_time,timestamp) > vrpn_ser_tkr_MAX_TIME_INTERVAL) {
		    fprintf(stderr,"Tracker failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",current_time.tv_sec, current_time.tv_usec, timestamp.tv_sec, timestamp.tv_usec);
		    send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
		    status = vrpn_TRACKER_FAIL;
	    }
      }
      break;

    case vrpn_TRACKER_RESETTING:
	reset();
	break;

    case vrpn_TRACKER_FAIL:
	send_text_message("Tracker failed, trying to reset (Try power cycle if more than 4 attempts made)", timestamp, vrpn_TEXT_ERROR);
	vrpn_close_commport(serial_fd);
	serial_fd = vrpn_open_commport(portname, baudrate);
	status = vrpn_TRACKER_RESETTING;
	break;
   }
}
#endif  // VRPN_CLIENT_ONLY

vrpn_Tracker_Remote::vrpn_Tracker_Remote (const char * name, vrpn_Connection *cn) :
	vrpn_Tracker (name, cn)
{
	tracker2roomchange_list = NULL;
	for (vrpn_int32 i = 0; i < vrpn_TRACKER_MAX_SENSOR_LIST; i++){
		change_list[i] = NULL;
		velchange_list[i] = NULL;
		accchange_list[i] = NULL;
		unit2sensorchange_list[i] = NULL;
	}
	workspacechange_list = NULL;

	// Make sure that we have a valid connection
	if (d_connection == NULL) {
		fprintf(stderr,"vrpn_Tracker_Remote: No connection\n");
		return;
	}

	// Register a handler for the position change callback from this device.
	if (register_autodeleted_handler(position_m_id, handle_change_message,
	    this, d_sender_id)) {
		fprintf(stderr,
		    "vrpn_Tracker_Remote: can't register position handler\n");
		d_connection = NULL;
	}

	// Register a handler for the velocity change callback from this device.
	if (register_autodeleted_handler(velocity_m_id,
	    handle_vel_change_message, this, d_sender_id)) {
		fprintf(stderr,
		    "vrpn_Tracker_Remote: can't register velocity handler\n");
		d_connection = NULL;
	}

	// Register a handler for the acceleration change callback.
	if (register_autodeleted_handler(accel_m_id,
	    handle_acc_change_message, this, d_sender_id)) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote: can't register acceleration handler\n");
		d_connection = NULL;
	}
	
	// Register a handler for the room to tracker xform change callback
        if (register_autodeleted_handler(tracker2room_m_id,
            handle_tracker2room_change_message, this, d_sender_id)) {
                fprintf(stderr,
                  "vrpn_Tracker_Remote: can't register tracker2room handler\n");
                d_connection = NULL;
        }

	// Register a handler for the sensor to unit xform change callback
        if (register_autodeleted_handler(unit2sensor_m_id,
            handle_unit2sensor_change_message, this, d_sender_id)) {
                fprintf(stderr,
                  "vrpn_Tracker_Remote: can't register unit2sensor handler\n");
                d_connection = NULL;
        }

        // Register a handler for the workspace change callback
        if (register_autodeleted_handler(workspace_m_id,
            handle_workspace_change_message, this, d_sender_id)) {
                fprintf(stderr,
                  "vrpn_Tracker_Remote: can't register workspace handler\n");
                d_connection = NULL;
        }


	// Find out what time it is and put this into the timestamp
	gettimeofday(&timestamp, NULL);
}

// The remote tracker has to un-register its handlers when it
// is destroyed to avoid seg faults (this is taken care of by
// using autodeleted handlers above). It should also remove all
// remaining user-registered callbacks to free up memory.

vrpn_Tracker_Remote::~vrpn_Tracker_Remote()
{
	// Delete all of the callback handlers that other code had registered
	// with this object. This will free up the memory taken by the lists
	//XXX do this once there is only one list, not one/sensor and one for all
}

int vrpn_Tracker_Remote::request_t2r_xform(void)
{
	char *msgbuf = NULL;
	vrpn_int32 len = 0; // no payload
	struct timeval current_time;

	gettimeofday(&current_time, NULL);
	timestamp.tv_sec = current_time.tv_sec;
	timestamp.tv_usec = current_time.tv_usec;

	if (d_connection) {
		if (d_connection->pack_message(len, timestamp, request_t2r_m_id,
		    d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
			fprintf(stderr, 
			    "vrpn_Tracker_Remote: cannot request t2r xform\n");
			return -1;
		}
	}
	return 0;
}

int vrpn_Tracker_Remote::request_u2s_xform(void)
{
	char *msgbuf = NULL;
        vrpn_int32 len = 0; // no payload
        struct timeval current_time;

        gettimeofday(&current_time, NULL);
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

        if (d_connection) {
                if (d_connection->pack_message(len, timestamp, request_u2s_m_id,
                    d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
                        fprintf(stderr, 
                            "vrpn_Tracker_Remote: cannot request u2s xform\n");
		    	return -1;
                }
        }
	return 0;
}

int vrpn_Tracker_Remote::set_update_rate (vrpn_float64 samplesPerSecond)
{
  char * msgbuf;
  vrpn_int32 len;
  struct timeval now;

  len = sizeof(vrpn_float64);
  msgbuf = new char [len];
  if (!msgbuf) {
    fprintf(stderr, "vrpn_Tracker_Remote::set_update_rate:  "
                    "Out of memory!\n");
    return -1;
  }

  ((vrpn_float64 *) msgbuf)[0] = htond(samplesPerSecond);

  gettimeofday(&now, NULL);
  timestamp.tv_sec = now.tv_sec;
  timestamp.tv_usec = now.tv_usec;

  if (d_connection) {
    if (d_connection->pack_message(len, timestamp, update_rate_id,
                    d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr, "vrpn_Tracker_Remote::set_update_rate:  "
                      "Cannot send message.\n");
      return -1;
    }
  }
  return 0;
}

int vrpn_Tracker_Remote::reset_origin()
{
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    if (d_connection->pack_message(0, timestamp, reset_origin_m_id,
                                d_sender_id, NULL, vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr,"vrpn_Tracker_Remote: cannot write message: tossing\n");
    }
  }
  return 0;
}

void	vrpn_Tracker_Remote::mainloop()
{
	if (d_connection) { d_connection->mainloop(); }
	client_mainloop();
}


int vrpn_Tracker_Remote::register_change_handler(void *userdata,
		vrpn_TRACKERCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
        vrpn_TRACKERCHANGELIST  *new_entry;

	if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
	    fprintf(stderr, 
		"vrpn_Tracker_Remote::register_handler: bad sensor index\n");
	    return -1;
	}
        // Ensure that the handler is non-NULL
        if (handler == NULL) {
                fprintf(stderr,
                    "vrpn_Tracker_Remote::register_handler: NULL handler\n");
                return -1;
        }

        // Allocate and initialize the new entry
        if ( (new_entry = new vrpn_TRACKERCHANGELIST) == NULL) {
                fprintf(stderr,
                    "vrpn_Tracker_Remote::register_handler: Out of memory\n");
                return -1;
        }
        new_entry->handler = handler;
        new_entry->userdata = userdata;

        // Add this handler to the chain at the beginning (don't check to see
        // if it is already there, since duplication is okay).
        new_entry->next = change_list[whichSensor];
        change_list[whichSensor] = new_entry;

        return 0;
}


int vrpn_Tracker_Remote::register_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
	vrpn_TRACKERVELCHANGELIST	*new_entry;

        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
                "vrpn_Tracker_Remote::register_handler: bad sensor index\n");
            return -1;
        }
	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
		   "vrpn_Tracker_Remote::register_vel_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_TRACKERVELCHANGELIST) == NULL) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote::register_vel_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = velchange_list[whichSensor];
	velchange_list[whichSensor] = new_entry;

	return 0;
}


int vrpn_Tracker_Remote::register_change_handler(void *userdata,
		vrpn_TRACKERACCCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
	vrpn_TRACKERACCCHANGELIST	*new_entry;
        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
                "vrpn_Tracker_Remote::register_handler: bad sensor index\n");
            return -1;
        }

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
		   "vrpn_Tracker_Remote::register_acc_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_TRACKERACCCHANGELIST) == NULL) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote::register_acc_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = accchange_list[whichSensor];
	accchange_list[whichSensor] = new_entry;

	return 0;
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
			vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler)
{
	vrpn_TRACKERTRACKER2ROOMCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
        if (handler == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                   	":register_tracker2room_handler: NULL handler\n");
                return -1;
        }

        // Allocate and initialize the new entry
        if ( (new_entry = new vrpn_TRACKERTRACKER2ROOMCHANGELIST) == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                  ":register_tracker2room_handler: Out of memory\n");
                return -1;
        }
        new_entry->handler = handler;
        new_entry->userdata = userdata;

        // Add this handler to the chain at the beginning (don't check to see
        // if it is already there, since duplication is okay).
        new_entry->next = tracker2roomchange_list;
        tracker2roomchange_list = new_entry;

        return 0;
}


int vrpn_Tracker_Remote::register_change_handler(void *userdata,
	vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
        vrpn_TRACKERUNIT2SENSORCHANGELIST      *new_entry;

        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
                "vrpn_Tracker_Remote::register_handler: bad sensor index\n");
            return -1;
        }

        // Ensure that the handler is non-NULL
        if (handler == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                        ":register_unit2sensor_handler: NULL handler\n");
                return -1;
        }

        // Allocate and initialize the new entry
        if ( (new_entry = new vrpn_TRACKERUNIT2SENSORCHANGELIST) == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                  ":register_unit2sensor_handler: Out of memory\n");
                return -1;
        }
        new_entry->handler = handler;
        new_entry->userdata = userdata;

        // Add this handler to the chain at the beginning (don't check to see
        // if it is already there, since duplication is okay).
        new_entry->next = unit2sensorchange_list[whichSensor];
        unit2sensorchange_list[whichSensor] = new_entry;

        return 0;
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler)
{
	vrpn_TRACKERWORKSPACECHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
        if (handler == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                   	":register_workspace_handler: NULL handler\n");
                return -1;
        }

        // Allocate and initialize the new entry
        if ( (new_entry = new vrpn_TRACKERWORKSPACECHANGELIST) == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                  ":register_workspace_handler: Out of memory\n");
                return -1;
        }
        new_entry->handler = handler;
        new_entry->userdata = userdata;

        // Add this handler to the chain at the beginning (don't check to see
        // if it is already there, since duplication is okay).
        new_entry->next = workspacechange_list;
        workspacechange_list = new_entry;

        return 0;
}


int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
		vrpn_TRACKERCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERCHANGELIST	*victim, **snitch;

        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
                "vrpn_Tracker_Remote::unregister_handler: bad sensor index\n");
            return -1;
        }

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(change_list[whichSensor]);
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERVELCHANGELIST	*victim, **snitch;

        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
             "vrpn_Tracker_Remote::unregister_vel_handler: bad sensor index\n");
            return -1;
        }

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(velchange_list[whichSensor]);
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
	    fprintf(stderr,
	      "vrpn_Tracker_Remote::unregister_vel_handler: No such handler\n");
	    return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
	vrpn_TRACKERACCCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERACCCHANGELIST	*victim, **snitch;

        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
             "vrpn_Tracker_Remote::unregister_acc_handler: bad sensor index\n");
            return -1;
        }

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(accchange_list[whichSensor]);
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
	    fprintf(stderr,
	      "vrpn_Tracker_Remote::unregister_acc_handler: No such handler\n");
	    return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
                        vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler)
{
        // The pointer at *snitch points to victim
        vrpn_TRACKERTRACKER2ROOMCHANGELIST       *victim, **snitch;

        // Find a handler with this registry in the list (any one will do,
        // since all duplicates are the same).
        snitch = &tracker2roomchange_list;
        victim = *snitch;
        while ( (victim != NULL) &&
                ( (victim->handler != handler) ||
                  (victim->userdata != userdata) )) {
            snitch = &( (*snitch)->next );
            victim = victim->next;
        }

        // Make sure we found one
        if (victim == NULL) {
            fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
              ":unregister_tracker2room_handler: No such handler\n");
            return -1;
        }

        // Remove the entry from the list
        *snitch = victim->next;
        delete victim;

        return 0;
}


int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
	vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
        // The pointer at *snitch points to victim
        vrpn_TRACKERUNIT2SENSORCHANGELIST       *victim, **snitch;

        if ((whichSensor < 0) || (whichSensor >= vrpn_TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
             "vrpn_Tracker_Remote::unregister_u2s_handler: bad sensor index\n");
            return -1;
        }

        // Find a handler with this registry in the list (any one will do,
        // since all duplicates are the same).
        snitch = &(unit2sensorchange_list[whichSensor]);
        victim = *snitch;
        while ( (victim != NULL) &&
                ( (victim->handler != handler) ||
                  (victim->userdata != userdata) )) {
            snitch = &( (*snitch)->next );
            victim = victim->next;
        }

        // Make sure we found one
        if (victim == NULL) {
            fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
              ":unregister_unit2sensor_handler: No such handler\n");
            return -1;
        }

        // Remove the entry from the list
        *snitch = victim->next;
        delete victim;

        return 0;
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERWORKSPACECHANGELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(workspacechange_list);
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote::unregister_workspace_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Tracker_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	const char *params = (p.buffer);
	vrpn_int32  padding;
	vrpn_TRACKERCB	tp;
	vrpn_TRACKERCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (8*sizeof(vrpn_float64)) ) {
		fprintf(stderr,"vrpn_Tracker: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 8*sizeof(vrpn_float64) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	vrpn_unbuffer(&params, &tp.sensor);
	vrpn_unbuffer(&params, &padding);

	for (i = 0; i < 3; i++) {
	 	vrpn_unbuffer(&params, &tp.pos[i]);
	}
	for (i = 0; i < 4; i++) {
		vrpn_unbuffer(&params, &tp.quat[i]);
	}

	handler = me->change_list[vrpn_ALL_SENSORS];
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

	if (tp.sensor < 0) {
	    fprintf(stderr,"vrpn_Tracker_Rem:pos sensor index is negative!\n");
	    return -1;
	} else if (tp.sensor < vrpn_TRACKER_MAX_SENSORS) {
		handler = me->change_list[tp.sensor];
	} else {
	    fprintf(stderr,"vrpn_Tracker_Rem:pos sensor index too large\n");
	    return -1;
	}
	// Go down the list of callbacks that have been registered for this
	// particular sensor
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}
	return 0;
}

int vrpn_Tracker_Remote::handle_vel_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	const char *params = p.buffer;
	vrpn_int32  padding;
	vrpn_TRACKERVELCB tp;
	vrpn_TRACKERVELCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (9*sizeof(vrpn_float64)) ) {
		fprintf(stderr,"vrpn_Tracker: vel message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 9*sizeof(vrpn_float64) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	vrpn_unbuffer(&params, &tp.sensor);
	vrpn_unbuffer(&params, &padding);

	for (i = 0; i < 3; i++) {
	    vrpn_unbuffer(&params, &tp.vel[i]);
	}
	for (i = 0; i < 4; i++) {
	    vrpn_unbuffer(&params, &tp.vel_quat[i]);
	}

	vrpn_unbuffer(&params, &tp.vel_quat_dt);

	handler = me->velchange_list[vrpn_ALL_SENSORS];
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

        if (tp.sensor < vrpn_TRACKER_MAX_SENSORS)
                handler = me->velchange_list[tp.sensor];
        else{
                fprintf(stderr,"vrpn_Tracker_Rem:vel sensor index too large\n");
		return -1;
	}
        // Go down the list of callbacks that have been registered for this
	// particular sensor
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }
	return 0;
}

int vrpn_Tracker_Remote::handle_acc_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	const char *params = p.buffer;
	vrpn_int32  padding;
	vrpn_TRACKERACCCB tp;
	vrpn_TRACKERACCCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (9*sizeof(vrpn_float64)) ) {
		fprintf(stderr, "vrpn_Tracker: acc message payload error\n");
		fprintf(stderr, "(got %d, expected %d)\n",
			p.payload_len, 9*sizeof(vrpn_float64) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	vrpn_unbuffer(&params, &tp.sensor);
	vrpn_unbuffer(&params, &padding);

	for (i = 0; i < 3; i++) {
		vrpn_unbuffer(&params, &tp.acc[i]);
	}
	for (i = 0; i < 4; i++) {
		vrpn_unbuffer(&params, &tp.acc_quat[i]);
	}

	vrpn_unbuffer(&params, &tp.acc_quat_dt);

	handler = me->accchange_list[vrpn_ALL_SENSORS];
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

        if (tp.sensor < vrpn_TRACKER_MAX_SENSORS)
                handler = me->accchange_list[tp.sensor];
        else{
                fprintf(stderr,"vrpn_Tracker_Rem:acc sensor index too large\n");
                return -1;
        }
        // Go down the list of callbacks that have been registered for this
	// particular sensor
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }
	return 0;
}

int vrpn_Tracker_Remote::handle_tracker2room_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	const char *params = p.buffer;
	vrpn_TRACKERTRACKER2ROOMCB tp;
	vrpn_TRACKERTRACKER2ROOMCHANGELIST *handler = 
						me->tracker2roomchange_list;
	int i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (7*sizeof(vrpn_float64))) {
		fprintf(stderr, "vrpn_Tracker: tracker2room message payload");
		fprintf(stderr, " error\n(got %d, expected %d)\n",
			p.payload_len, 7*sizeof(vrpn_float64));
		return -1;
	}
	tp.msg_time = p.msg_time;

        for (i = 0; i < 3; i++) {
                vrpn_unbuffer(&params, &tp.tracker2room[i]);
        }
        for (i = 0; i < 4; i++) {
                vrpn_unbuffer(&params, &tp.tracker2room_quat[i]);
        }

        // Go down the list of callbacks that have been registered.
        // Fill in the parameter and call each.
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

        return 0;
}

int vrpn_Tracker_Remote::handle_unit2sensor_change_message(void *userdata,
        vrpn_HANDLERPARAM p)
{
        vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	const char *params = p.buffer;
	vrpn_int32 padding;
        vrpn_TRACKERUNIT2SENSORCB tp;
        vrpn_TRACKERUNIT2SENSORCHANGELIST *handler;
        int i;

        // Fill in the parameters to the tracker from the message
        if (p.payload_len != (8*sizeof(vrpn_float64))) {
                fprintf(stderr, "vrpn_Tracker: unit2sensor message payload");
                fprintf(stderr, " error\n(got %d, expected %d)\n",
                        p.payload_len, 8*sizeof(vrpn_float64));
                return -1;
        }
        tp.msg_time = p.msg_time;
	vrpn_unbuffer(&params, &tp.sensor);
	vrpn_unbuffer(&params, &padding);
	
        // Typecasting used to get the byte order correct on the vrpn_float64
        // that are coming from the other side.
        for (i = 0; i < 3; i++) {
                vrpn_unbuffer(&params, &tp.unit2sensor[i]);
        }
        for (i = 0; i < 4; i++) {
                vrpn_unbuffer(&params, &tp.unit2sensor_quat[i]);
        }
	
	handler = me->unit2sensorchange_list[vrpn_ALL_SENSORS];
        // Go down the list of callbacks that have been registered.
        // Fill in the parameter and call each.
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

        if (tp.sensor < vrpn_TRACKER_MAX_SENSORS)
                handler = me->unit2sensorchange_list[tp.sensor];
        else{
                fprintf(stderr,"vrpn_Tracker_Rem:u2s sensor index too large\n");
                return -1;
        }
        // Go down the list of callbacks that have been registered for this
	// particular sensor
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

        return 0;
}

int vrpn_Tracker_Remote::handle_workspace_change_message(void *userdata, 
	vrpn_HANDLERPARAM p)
{
	vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
	const char *params = p.buffer;
	vrpn_TRACKERWORKSPACECB tp;
	vrpn_TRACKERWORKSPACECHANGELIST *handler = 
						me->workspacechange_list;
	int i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (6*sizeof(vrpn_float64))) {
		fprintf(stderr, "vrpn_Tracker: tracker2room message payload");
		fprintf(stderr, " error\n(got %d, expected %d)\n",
			p.payload_len, 6*sizeof(vrpn_float64));
		return -1;
	}
	tp.msg_time = p.msg_time;

        for (i = 0; i < 3; i++) {
                vrpn_unbuffer(&params, &tp.workspace_min[i]);
        }
        for (i = 0; i < 3; i++) {
                vrpn_unbuffer(&params, &tp.workspace_max[i]);
        }

        // Go down the list of callbacks that have been registered.
        // Fill in the parameter and call each.
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

	return 0;
}
