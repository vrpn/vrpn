#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#ifdef	_WIN32
#include <io.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Serial.h"

static const char *tracker_cfg_file_name = "vrpn_Tracker.cfg";

//#define VERBOSE
// #define READ_HISTOGRAM

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


vrpn_Tracker::vrpn_Tracker(char *name, vrpn_Connection *c) {
	FILE	*config_file;
  char * servicename;
  servicename = vrpn_copy_service_name(name);


	// Set our connection to the one passed in
	connection = c;

	// Register this tracker device and the needed message types
	if (connection) {
	  my_id = connection->register_sender(servicename);
	  position_m_id = connection->register_message_type("Tracker Pos/Quat");
	  velocity_m_id = connection->register_message_type("Tracker Velocity");
	  accel_m_id =connection->register_message_type("Tracker Acceleration");
	  tracker2room_m_id = connection->register_message_type(
							"Tracker To Room");
	  unit2sensor_m_id = connection->register_message_type(
							"Unit To Sensor");
	  request_t2r_m_id = connection->register_message_type(
						"Request Tracker To Room");
	  request_u2s_m_id = connection->register_message_type(
						"Request Unit To Sensor");
	  workspace_m_id = connection->register_message_type(
							"Tracker Workspace");
	  request_workspace_m_id = connection->register_message_type(
						"Request Tracker Workspace");
	  update_rate_id = connection->register_message_type
				("vrpn Tracker set update rate");
	  reset_origin_m_id = connection->register_message_type(
						"Reset Origin");
	}

	// Set the current time to zero, just to have something there
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	// Set the sensor to 0 just to have something in there.
	sensor = 0;

	// Set the position to the origin and the orientation to identity
	// just to have something there in case nobody fills them in later
	pos[0] = pos[1] = pos[2] = 0.0;
	quat[0] = quat[1] = quat[2] = 0.0;
	quat[3] = 1.0;

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

	// Set the room to tracker and sensor to unit transforms to identity
	tracker2room[0] = tracker2room[1] = tracker2room[2] = 0.0;
	tracker2room_quat[0] = tracker2room_quat[1] = 
						tracker2room_quat[2] = 0.0;
	tracker2room_quat[3] = 1.0;

	num_sensors = 1;
	for (int sens = 0; sens < TRACKER_MAX_SENSORS; sens++){
	    unit2sensor[sens][0] = unit2sensor[sens][1] = 
						unit2sensor[sens][2] = 0.0;
	    unit2sensor_quat[sens][0] = unit2sensor_quat[sens][1] = 0.0;
	    unit2sensor_quat[sens][2] = 0.0;
	    unit2sensor_quat[sens][3] = 1.0;
	}

	workspace_min[0] = workspace_min[1] = workspace_min[2] = -0.5;
	workspace_max[0] = workspace_max[1] = workspace_max[2] = 0.5;

	// replace defaults with values from "vrpn_Tracker.cfg" file
	// if it exists
	if ((config_file = fopen(tracker_cfg_file_name, "r")) == NULL) {
		fprintf(stderr, "Cannot open config file %s\n",
				tracker_cfg_file_name);
	}
	else if (read_config_file(config_file, name)){
		fprintf(stderr, "Found but cannot read config file %s\n",
				tracker_cfg_file_name);
		fclose(config_file);
	}
	else	// no problems
		fclose(config_file);

  if (servicename)
    delete [] servicename;
}

int vrpn_Tracker::read_config_file(FILE *config_file, char *tracker_name)
{
    char	line[512];	// line read from input file
    int		num_sens;
    int 	which_sensor;
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
		(num_sens > TRACKER_MAX_SENSORS)) break;
	    for (i = 0; i < num_sens; i++){
		// get which sensor this xform is for
		if (fgets(line, sizeof(line), config_file) == NULL) break;
		if ((sscanf(line, "%d", &which_sensor) != 1) ||
		    (which_sensor > TRACKER_MAX_SENSORS)) break;
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
   printf("Sensor   :%d\n", sensor);
   printf("Timestamp:%ld:%ld\n", timestamp.tv_sec, timestamp.tv_usec);
   printf("Pos      :%lf, %lf, %lf\n", pos[0],pos[1],pos[2]);
   printf("Quat     :%lf, %lf, %lf, %lf\n", quat[0],quat[1],quat[2],quat[3]);
}

int vrpn_Tracker::register_server_handlers(void)
{
    if (connection){
 	if (connection->register_handler(request_t2r_m_id,
            handle_t2r_request, this, my_id)){
                fprintf(stderr,"vrpn_Tracker:can't register t2r handler\n");
		return -1;
	}
        if (connection->register_handler(request_u2s_m_id,
            handle_u2s_request, this, my_id)){
                fprintf(stderr,"vrpn_Tracker:can't register u2s handler\n");
		return -1;
	}
	if (connection->register_handler(request_workspace_m_id,
	    handle_workspace_request, this, my_id)){
		fprintf(stderr,"vrpn_Tracker:  "
                               "Can't register workspace handler\n");
		return -1;
	}
	if (connection->register_handler
		(update_rate_id, handle_update_rate_request, this, my_id)) {
		fprintf(stderr, "vrpn_Tracker:  "
                                "Can't register update rate handler.\n");
                return -1;
        }
    }
    else
	return -1;
    return 0;
}

// put copies of vector and quat into arrays passed in
void vrpn_Tracker::get_local_t2r(double *vec, double *quat)
{
    int i;
    for (i = 0; i < 3; i++)
	vec[i] = tracker2room[i];
    for (i = 0; i < 4; i++)
	quat[i] = tracker2room_quat[i];

}

// put copies of vector and quat into arrays passed in
void vrpn_Tracker::get_local_u2s(int sensor, double *vec, double *quat)
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
    int     len;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata; // == this normally

    p = p; // Keep the compiler from complaining

    gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our t2r transform was read in by the constructor

    // send t2r transform
    if (me->connection) {
        len = me->encode_tracker2room_to(msgbuf);
        if (me->connection->pack_message(len, me->timestamp,
            me->tracker2room_m_id, me->my_id,
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
    int     len;
    int i;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata;

    p = p; // Keep the compiler from complaining

    gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our u2s transforms were read in by the constructor

    if (me->connection){
	for (i = 0; i < me->num_sensors; i++){
	    me->sensor = i;
            // send u2s transform
            len = me->encode_unit2sensor_to(msgbuf);
            if (me->connection->pack_message(len, me->timestamp,
                me->unit2sensor_m_id, me->my_id,
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
    int     len;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata;

    p = p; // Keep the compiler from complaining

    gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our workspace was read in by the constructor

    if (me->connection){
        len = me->encode_workspace_to(msgbuf);
        if (me->connection->pack_message(len, me->timestamp,
            me->workspace_m_id, me->my_id,
            msgbuf, vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker: cannot write workspace message\n");
        }
    }
    return 0;
}

int vrpn_Tracker::handle_update_rate_request (void *, vrpn_HANDLERPARAM) {
  fprintf(stderr, "vrpn_Tracker::handle_update_rate_request:  "
                  "Don't know how to do that!\n");
  return 0;
}

vrpn_Connection *vrpn_Tracker::connectionPtr() {
  return connection;
}

int     vrpn_Tracker::encode_tracker2room_to(char *buf)
{
    int i;
    double *dBuf = (double *)buf;
    int index = 0;
    for (i = 0; i < 3; i++) {
        dBuf[index++] = *(double *)(&tracker2room[i]);
    }
    for (i = 0; i < 4; i++) {
        dBuf[index++] = *(double *)(&tracker2room_quat[i]);
    }

    // convert the doubles
    for (i = 0; i < index; i++) {    
        dBuf[i] = htond(dBuf[i]);
    }
    return index*sizeof(double);
}

// WARNING: make sure sensor is set to desired value before sending this message
int	vrpn_Tracker::encode_unit2sensor_to(char *buf)
{
    int i;
    double *dBuf = (double *)buf;
    int index = 0;

    *(long *)dBuf = htonl(sensor);
    
    // re-align to doubles
    index++;

    for (i = 0; i < 3; i++) {
    	dBuf[index++] = *(double *)(&unit2sensor[sensor][i]);
    }
    for (i = 0; i < 4; i++) {
	dBuf[index++] = *(double *)(&unit2sensor_quat[sensor][i]);
    }

    // convert the doubles
    for (i = 1; i < index; i++) {
	dBuf[i] = htond(dBuf[i]);
    }
    return index*sizeof(double);
}

int	vrpn_Tracker::encode_workspace_to(char *buf)
{
    int i;
    double *dBuf = (double *)buf;
    int index = 0;

    for (i = 0; i < 3; i++) {
        dBuf[index++] = *(double *)(&workspace_min[i]);
    }
    for (i = 0; i < 3; i++) {
        dBuf[index++] = *(double *)(&workspace_max[i]);
    }

    // convert the doubles
    for (i = 1; i < index; i++) {
        dBuf[i] = htond(dBuf[i]);
    }
    return index*sizeof(double);
}

// NOTE: you need to be sure that if you are sending doubles then 
//       the entire array needs to remain aligned to 8 byte boundaries
//	 (malloced data and static arrays are automatically alloced in
//	  this way)
int	vrpn_Tracker::encode_to(char *buf)
{
   // Message includes: long unitNum, double pos[3], double quat[4]
   // Byte order of each needs to be reversed to match network standard
   // This moving is done by horrible typecast hacks.  Please forgive.

   int i;
   double *dBuf = (double *)buf;
   int	index = 0;

   // Move the sensor, position, quaternion there
   *(long *)dBuf = htonl(sensor);

   // re-align to doubles
   index++;

   for (i = 0; i < 3; i++) {
   	dBuf[index++] = *(double *)(&pos[i]);
   }
   for (i = 0; i < 4; i++) {
   	dBuf[index++] = *(double *)(&quat[i]);
   }

   // convert the doubles
   for (i = 1; i < index; i++) {
   	dBuf[i] = htond(dBuf[i]);
   }

   return index*sizeof(double);
}

int	vrpn_Tracker::encode_vel_to(char *buf)
{
   // Message includes: long unitNum, double vel[3], double vel_quat[4]
   // Byte order of each needs to be reversed to match network standard
   // This moving is done by horrible typecast hacks.  Please forgive.

   int i;
   double *dBuf = (double *)buf;
   int	index = 0;

   // Move the sensor, velocity, quaternion there
   *(long *)dBuf = htonl(sensor);

   // re-align to doubles
   index++;

   for (i = 0; i < 3; i++) {
   	dBuf[index++] = *(double*)(&vel[i]);
   }
   for (i = 0; i < 4; i++) {
   	dBuf[index++] = *(double*)(&vel_quat[i]);
   }
   dBuf[index++] = vel_quat_dt;

   // convert the doubles
   for (i = 1; i < index; i++) {
   	dBuf[i] = htond(dBuf[i]);
   }
   
   return index*sizeof(double);
}

int	vrpn_Tracker::encode_acc_to(char *buf)
{
   // Message includes: long unitNum, double acc[3], double acc_quat[4]
   // Byte order of each needs to be reversed to match network standard
   // This moving is done by horrible typecast hacks.  Please forgive.

   int i;
   double *dBuf = (double*)(buf);
   int	index = 0;

   // Move the sensor, acceleration, quaternion there
   *(long *)dBuf = htonl(sensor);
   // re-align to doubles
   index++;

   for (i = 0; i < 3; i++) {
   	dBuf[index++] = *(double *)(&acc[i]);
   }
   for (i = 0; i < 4; i++) {
   	dBuf[index++] = *(double *)(&acc_quat[i]);
   }

   dBuf[index++] = acc_quat_dt;

   // convert the doubles
   for (i = 1; i < index; i++) {
   	dBuf[i] = htond(dBuf[i]);
   }

   return index*sizeof(double);
}

#ifndef VRPN_CLIENT_ONLY


vrpn_Tracker_Canned::vrpn_Tracker_Canned (char * name, vrpn_Connection * c,
                                          char * datafile) 
  : vrpn_Tracker(name, c) {
    register_server_handlers();
    fp =fopen(datafile,"r");
    if (fp == NULL) {
      perror("can't open datafile:");
      return;
    } else {
      fread(&totalRecord, sizeof(int), 1, fp);
      fread(&t, sizeof(vrpn_TRACKERCB), 1, fp);
      printf("pos[0]=%.4f, time = %ld usec\n", t.pos[0],
	     t.msg_time.tv_usec);
      printf("sizeof trackcb = %d, total = %d\n", sizeof(t), totalRecord);
      current =1;
      copy();
    }
}


vrpn_Tracker_Canned::~vrpn_Tracker_Canned (void) {
  if (fp != NULL) fclose(fp);
}


void vrpn_Tracker_Canned::mainloop (void) {
  // Send the message on the connection;
  if (connection) {
    char	msgbuf[1000];
    int	len = encode_to(msgbuf);
    if (connection->pack_message(len, timestamp,
				 position_m_id, my_id, msgbuf,
				 vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"Tracker: cannot write message: tossing\n");
    }
  } else {
    fprintf(stderr,"Tracker canned: No valid connection\n");
    return;
  }

  if (fread(&t, sizeof(vrpn_TRACKERCB), 1, fp) < 1) {
    reset();
    return;
  }
  struct timeval timeout;
  timeout.tv_sec =0;
  timeout.tv_usec = (t.msg_time.tv_sec * 1000000 + t.msg_time.tv_usec)
    - (timestamp.tv_sec *  1000000 + timestamp.tv_usec);
  if (timeout.tv_usec > 1000000) timeout.tv_usec = 0;
  
  //printf("pos[0]=%.4f, time = %ld usec\n", pos[0], timeout.tv_usec);
  select(0, 0, 0, 0, & timeout);  // wait for that long;
  current ++;
  copy();

}

void vrpn_Tracker_Canned::copy (void) {
  memcpy(&(timestamp), &(t.msg_time), sizeof(struct timeval));
  sensor = t.sensor;
  pos[0] = t.pos[0];  pos[1] = t.pos[1];  pos[2] = t.pos[2];
  quat[0] = t.quat[0];  quat[1] = t.quat[1];  
  quat[2] = t.quat[2];  quat[3] = t.quat[3];
}

void vrpn_Tracker_Canned::reset() {
  fprintf(stderr, "Resetting!");
  if (fp == NULL) return;
  fseek(fp, sizeof(int), SEEK_SET);
  fread(&t, sizeof(vrpn_TRACKERCB), 1, fp);
  copy();
  current = 1;
}








#endif  // VRPN_CLIENT_ONLY

vrpn_Tracker_NULL::vrpn_Tracker_NULL(char *name, vrpn_Connection *c,
	int sensors, float Hz) : vrpn_Tracker(name, c), update_rate(Hz),
	num_sensors(sensors)
{
	register_server_handlers();
	// Nothing left to do
}

void	vrpn_Tracker_NULL::mainloop(void)
{
	struct timeval current_time;
	char	msgbuf[1000];
	int	i,len;

	// See if its time to generate a new report
	gettimeofday(&current_time, NULL);
	if ( duration(current_time,timestamp) >= 1000000.0/update_rate) {

	  // Update the time
	  timestamp.tv_sec = current_time.tv_sec;
	  timestamp.tv_usec = current_time.tv_usec;

	  // Send messages for all sensors if we have a connection
	  if (connection) {
	    for (i = 0; i < num_sensors; i++) {
		sensor = i;

		// Pack position report
		len = encode_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			position_m_id, my_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}

		// Pack velocity report
		len = encode_vel_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			velocity_m_id, my_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}

		// Pack acceleration report
		len = encode_acc_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			accel_m_id, my_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		 fprintf(stderr,"NULL tracker: can't write message: tossing\n");
		}
	    }
	  }
	}
}

#ifndef VRPN_CLIENT_ONLY
vrpn_Tracker_Serial::vrpn_Tracker_Serial(char *name, vrpn_Connection *c,
	char *port, long baud): vrpn_Tracker(name, c)
{
   register_server_handlers();
   // Find out the port name and baud rate
   if (port == NULL) {
	fprintf(stderr,"vrpn_Tracker_Serial: NULL port name\n");
	status = TRACKER_FAIL;
	return;
   } else {
	strncpy(portname, port, sizeof(portname));
	portname[sizeof(portname)-1] = '\0';
   }
   baudrate = baud;

   // Open the serial port we're going to use
   if ( (serial_fd=vrpn_open_commport(portname, baudrate)) == -1) {
	fprintf(stderr,"vrpn_Tracker_Serial: Cannot Open serial port\n");
	status = TRACKER_FAIL;
   }

   // Reset the tracker and find out what time it is
   status = TRACKER_RESETTING;
   gettimeofday(&timestamp, NULL);
}
#endif  // VRPN_CLIENT_ONLY

vrpn_Tracker_Remote::vrpn_Tracker_Remote(char *name ) :
	vrpn_Tracker(name, vrpn_get_connection_by_name(name))
{
	tracker2roomchange_list = NULL;
	for (int i = 0; i < TRACKER_MAX_SENSOR_LIST; i++){
		change_list[i] = NULL;
		velchange_list[i] = NULL;
		accchange_list[i] = NULL;
		unit2sensorchange_list[i] = NULL;
	}
	workspacechange_list = NULL;

	// Make sure that we have a valid connection
	if (connection == NULL) {
		fprintf(stderr,"vrpn_Tracker_Remote: No connection\n");
		return;
	}

	// Register a handler for the position change callback from this device.
	if (connection->register_handler(position_m_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,
		    "vrpn_Tracker_Remote: can't register position handler\n");
		connection = NULL;
	}

	// Register a handler for the velocity change callback from this device.
	if (connection->register_handler(velocity_m_id,
	    handle_vel_change_message, this, my_id)) {
		fprintf(stderr,
		    "vrpn_Tracker_Remote: can't register velocity handler\n");
		connection = NULL;
	}

	// Register a handler for the acceleration change callback.
	if (connection->register_handler(accel_m_id,
	    handle_acc_change_message, this, my_id)) {
		fprintf(stderr,
		  "vrpn_Tracker_Remote: can't register acceleration handler\n");
		connection = NULL;
	}
	
	// Register a handler for the room to tracker xform change callback
        if (connection->register_handler(tracker2room_m_id,
            handle_tracker2room_change_message, this, my_id)) {
                fprintf(stderr,
                  "vrpn_Tracker_Remote: can't register tracker2room handler\n");
                connection = NULL;
        }

	// Register a handler for the sensor to unit xform change callback
        if (connection->register_handler(unit2sensor_m_id,
            handle_unit2sensor_change_message, this, my_id)) {
                fprintf(stderr,
                  "vrpn_Tracker_Remote: can't register unit2sensor handler\n");
                connection = NULL;
        }

        // Register a handler for the workspace change callback
        if (connection->register_handler(workspace_m_id,
            handle_workspace_change_message, this, my_id)) {
                fprintf(stderr,
                  "vrpn_Tracker_Remote: can't register workspace handler\n");
                connection = NULL;
        }


	// Find out what time it is and put this into the timestamp
	gettimeofday(&timestamp, NULL);
}

// the remote tracker has to un-register it's handlers when it
// is destroyed
vrpn_Tracker_Remote::~vrpn_Tracker_Remote() {

  // Unregister a handler for the position change callback from this device.
  if (connection)
  if (connection->unregister_handler(position_m_id, handle_change_message,
				   this, my_id)) {
    fprintf(stderr,
	    "vrpn_Tracker_Remote: can't unregister position handler\n");
    connection = NULL;
  }
  
  // Unregister a handler for the velocity change callback from this device.
  if (connection)
  if (connection->unregister_handler(velocity_m_id,
				   handle_vel_change_message, this, my_id)) {
    fprintf(stderr,
	    "vrpn_Tracker_Remote: can't unregister velocity handler\n");
    connection = NULL;
  }
  
  // Unregister a handler for the acceleration change callback.
  if (connection)
  if (connection->unregister_handler(accel_m_id,
				   handle_acc_change_message, this, my_id)) {
    fprintf(stderr,
	    "vrpn_Tracker_Remote: can't unregister acceleration handler\n");
    connection = NULL;
  }
  
  // Unregister a handler for the room to tracker xform change callback
  if (connection)
  if (connection->unregister_handler(tracker2room_m_id,
				   handle_tracker2room_change_message, this, my_id)) {
	  fprintf(stderr,
                  "vrpn_Tracker_Remote: can't unregister tracker2room handler\n");
	  connection = NULL;
  }
  
  // Unregister a handler for the sensor to unit xform change callback
  if (connection)
  if (connection->unregister_handler(unit2sensor_m_id,
				   handle_unit2sensor_change_message, this, my_id)) {
    fprintf(stderr,
	    "vrpn_Tracker_Remote: can't unregister unit2sensor handler\n");
    connection = NULL;
  }
}

int vrpn_Tracker_Remote::request_t2r_xform(void)
{
	char *msgbuf = NULL;
	int len = 0; // no payload
	struct timeval current_time;

	gettimeofday(&current_time, NULL);
	timestamp.tv_sec = current_time.tv_sec;
	timestamp.tv_usec = current_time.tv_usec;

	if (connection) {
		if (connection->pack_message(len, timestamp, request_t2r_m_id,
		    my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
			fprintf(stderr, 
			    "vrpn_Tracker_Remote: cannot request t2r xform\n");
			return -1;
		}
		connection->mainloop();
	}
	return 0;
}

int vrpn_Tracker_Remote::request_u2s_xform(void)
{
	char *msgbuf = NULL;
        int len = 0; // no payload
        struct timeval current_time;

        gettimeofday(&current_time, NULL);
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

        if (connection) {
                if (connection->pack_message(len, timestamp, request_u2s_m_id,
                    my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
                        fprintf(stderr, 
                            "vrpn_Tracker_Remote: cannot request u2s xform\n");
		    	return -1;
                }
                connection->mainloop();
        }
	return 0;
}

int vrpn_Tracker_Remote::set_update_rate (double samplesPerSecond) {
  char * msgbuf;
  int len;
  struct timeval now;

  len = sizeof(double);
  msgbuf = new char [len];
  if (!msgbuf) {
    fprintf(stderr, "vrpn_Tracker_Remote::set_update_rate:  "
                    "Out of memory!\n");
    return -1;
  }

  ((double *) msgbuf)[0] = htond(samplesPerSecond);

  gettimeofday(&now, NULL);
  timestamp.tv_sec = now.tv_sec;
  timestamp.tv_usec = now.tv_usec;

  if (connection) {
    if (connection->pack_message(len, timestamp, update_rate_id,
                    my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr, "vrpn_Tracker_Remote::set_update_rate:  "
                      "Cannot send message.\n");
      return -1;
    }
    connection->mainloop();
  }
  return 0;


}

int vrpn_Tracker_Remote::reset_origin() {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    if (connection->pack_message(0, timestamp, reset_origin_m_id,
                                my_id, NULL, vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr,"vrpn_Tracker_Remote: cannot write message: tossing\n");
    }
    connection->mainloop();
  }
  return 0;
}

void	vrpn_Tracker_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); }
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
			vrpn_TRACKERCHANGEHANDLER handler)
{
	return register_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
                        vrpn_TRACKERCHANGEHANDLER handler, int whichSensor)
{
        vrpn_TRACKERCHANGELIST  *new_entry;

	if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
                        vrpn_TRACKERVELCHANGEHANDLER handler)
{
	return register_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
			vrpn_TRACKERVELCHANGEHANDLER handler, int whichSensor)
{
	vrpn_TRACKERVELCHANGELIST	*new_entry;

        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
                        vrpn_TRACKERACCCHANGEHANDLER handler)
{
	return register_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
			vrpn_TRACKERACCCHANGEHANDLER handler, int whichSensor)
{
	vrpn_TRACKERACCCHANGELIST	*new_entry;
        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:"
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
                        vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler)
{
	return register_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::register_change_handler(void *userdata,
		vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, int whichSensor)
{
        vrpn_TRACKERUNIT2SENSORCHANGELIST      *new_entry;

        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
            fprintf(stderr,
                "vrpn_Tracker_Remote::register_handler: bad sensor index\n");
            return -1;
        }

        // Ensure that the handler is non-NULL
        if (handler == NULL) {
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:"
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
                fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:"
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
                        vrpn_TRACKERCHANGEHANDLER handler)
{
	return unregister_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
			vrpn_TRACKERCHANGEHANDLER handler, int whichSensor)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERCHANGELIST	*victim, **snitch;

        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
                        vrpn_TRACKERVELCHANGEHANDLER handler)
{
	return unregister_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler, int whichSensor)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERVELCHANGELIST	*victim, **snitch;

        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
                        vrpn_TRACKERACCCHANGEHANDLER handler)
{
	return unregister_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
	vrpn_TRACKERACCCHANGEHANDLER handler, int whichSensor)
{
	// The pointer at *snitch points to victim
	vrpn_TRACKERACCCHANGELIST	*victim, **snitch;

        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
                        vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler)
{
	return unregister_change_handler(userdata, handler, ALL_SENSORS);
}

int vrpn_Tracker_Remote::unregister_change_handler(void *userdata,
              vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, int whichSensor)
{
        // The pointer at *snitch points to victim
        vrpn_TRACKERUNIT2SENSORCHANGELIST       *victim, **snitch;

        if ((whichSensor < 0) || (whichSensor > TRACKER_MAX_SENSOR_LIST)) {
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
	double *params = (double*)(p.buffer);
	vrpn_TRACKERCB	tp;
	vrpn_TRACKERCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (8*sizeof(double)) ) {
		fprintf(stderr,"vrpn_Tracker: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 8*sizeof(double) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	tp.sensor = ntohl(*((long *)params));
	params++;

	// Typecasting used to get the byte order correct on the doubles
	// that are coming from the other side.
	for (i = 0; i < 3; i++) {
	 	tp.pos[i] = ntohd(*params++);
	}
	for (i = 0; i < 4; i++) {
		tp.quat[i] = ntohd(*params++);
	}

	handler = me->change_list[ALL_SENSORS];
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}
	if (tp.sensor < TRACKER_MAX_SENSORS)
		handler = me->change_list[tp.sensor];
	else{
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
	double *params = (double*)(p.buffer);
	vrpn_TRACKERVELCB tp;
	vrpn_TRACKERVELCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (9*sizeof(double)) ) {
		fprintf(stderr,"vrpn_Tracker: vel message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 9*sizeof(double) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	tp.sensor = ntohl(*(long *)params);
	params++;

	// Typecasting used to get the byte order correct on the doubles
	// that are coming from the other side.
	for (i = 0; i < 3; i++) {
		tp.vel[i] = ntohd(*params++);
	}
	for (i = 0; i < 4; i++) {
		tp.vel_quat[i] = ntohd(*params++);
	}

	tp.vel_quat_dt = ntohd(*params++);

	handler = me->velchange_list[ALL_SENSORS];
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

        if (tp.sensor < TRACKER_MAX_SENSORS)
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
	double *params = (double*)(p.buffer);
	vrpn_TRACKERACCCB tp;
	vrpn_TRACKERACCCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (9*sizeof(double)) ) {
		fprintf(stderr, "vrpn_Tracker: acc message payload error\n");
		fprintf(stderr, "(got %d, expected %d)\n",
			p.payload_len, 9*sizeof(double) );
		return -1;
	}
	tp.msg_time = p.msg_time;
	tp.sensor = ntohl(*(long *)params);
	params++;

	// Typecasting used to get the byte order correct on the doubles
	// that are coming from the other side.
	for (i = 0; i < 3; i++) {
		tp.acc[i] = ntohd(*params++);
	}
	for (i = 0; i < 4; i++) {
		tp.acc_quat[i] =  ntohd(*params++);
	}

	tp.acc_quat_dt = ntohd(*params++);

	handler = me->accchange_list[ALL_SENSORS];
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

        if (tp.sensor < TRACKER_MAX_SENSORS)
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
	double *params = (double*)(p.buffer);
	vrpn_TRACKERTRACKER2ROOMCB tp;
	vrpn_TRACKERTRACKER2ROOMCHANGELIST *handler = 
						me->tracker2roomchange_list;
	int i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (7*sizeof(double))) {
		fprintf(stderr, "vrpn_Tracker: tracker2room message payload");
		fprintf(stderr, " error\n(got %d, expected %d)\n",
			p.payload_len, 7*sizeof(double));
		return -1;
	}
	tp.msg_time = p.msg_time;
	// Typecasting used to get the byte order correct on the doubles
	// that are coming from the other side.
        for (i = 0; i < 3; i++) {
                tp.tracker2room[i] = ntohd(*params++);
        }
        for (i = 0; i < 4; i++) {
                tp.tracker2room_quat[i] =  ntohd(*params++);
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
        double *params = (double*)(p.buffer);
        vrpn_TRACKERUNIT2SENSORCB tp;
        vrpn_TRACKERUNIT2SENSORCHANGELIST *handler;
        int i;

        // Fill in the parameters to the tracker from the message
        if (p.payload_len != (8*sizeof(double))) {
                fprintf(stderr, "vrpn_Tracker: unit2sensor message payload");
                fprintf(stderr, " error\n(got %d, expected %d)\n",
                        p.payload_len, 8*sizeof(double));
                return -1;
        }
        tp.msg_time = p.msg_time;
	tp.sensor = ntohl(*((long *)params));
        params++;
	
        // Typecasting used to get the byte order correct on the doubles
        // that are coming from the other side.
        for (i = 0; i < 3; i++) {
                tp.unit2sensor[i] = ntohd(*params++);
        }
        for (i = 0; i < 4; i++) {
                tp.unit2sensor_quat[i] =  ntohd(*params++);
        }
	
	handler = me->unit2sensorchange_list[ALL_SENSORS];
        // Go down the list of callbacks that have been registered.
        // Fill in the parameter and call each.
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

        if (tp.sensor < TRACKER_MAX_SENSORS)
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
	double *params = (double*)(p.buffer);
	vrpn_TRACKERWORKSPACECB tp;
	vrpn_TRACKERWORKSPACECHANGELIST *handler = 
						me->workspacechange_list;
	int i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len != (6*sizeof(double))) {
		fprintf(stderr, "vrpn_Tracker: tracker2room message payload");
		fprintf(stderr, " error\n(got %d, expected %d)\n",
			p.payload_len, 6*sizeof(double));
		return -1;
	}
	tp.msg_time = p.msg_time;
	// Typecasting used to get the byte order correct on the doubles
	// that are coming from the other side.
        for (i = 0; i < 3; i++) {
                tp.workspace_min[i] = ntohd(*params++);
        }
        for (i = 0; i < 3; i++) {
                tp.workspace_max[i] =  ntohd(*params++);
        }

        // Go down the list of callbacks that have been registered.
        // Fill in the parameter and call each.
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

	return 0;
}
