#include <ctype.h>  // for isspace
#include <stdio.h>  // for fprintf, stderr, NULL, etc
#include <string.h> // for memcpy, strlen, strncmp, etc

// NOTE: a vrpn tracker must call user callbacks with tracker data (pos and
//       ori info) which represent the transformation xfSourceFromSensor.
//       This means that the pos info is the position of the origin of
//       the sensor coord sys in the source coord sys space, and the
//       quat represents the orientation of the sensor relative to the
//       source space (ie, its value rotates the source's axes so that
//       they coincide with the sensor's)

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h
// and unistd.h
#include "vrpn_Shared.h" // for timeval, vrpn_buffer, etc

#ifdef _WIN32
#ifndef _WIN32_WCE
#include <io.h>
#endif
#endif

#include "vrpn_RedundantTransmission.h" // for vrpn_RedundantTransmission
#include "vrpn_Tracker.h"
#include "quat.h"

#if defined(VRPN_USE_LIBUSB_1_0)
#include <libusb.h> // for libusb_close, etc
#endif

#ifndef VRPN_CLIENT_ONLY
#include "vrpn_Serial.h" // for vrpn_close_commport, etc
#endif

static const char *default_tracker_cfg_file_name = "vrpn_Tracker.cfg";
// this is used unless we pass in a path & file name into the vrpn_TRACKER
// constructor

#define vrpn_ser_tkr_MAX_TIME_INTERVAL                                         \
    (2000000) // max time between reports (usec)

//#define VERBOSE
// #define READ_HISTOGRAM

vrpn_Tracker::vrpn_Tracker(const char *name, vrpn_Connection *c,
                           const char *tracker_cfg_file_name)
    : vrpn_BaseClass(name, c)
    , unit2sensor(NULL)
    , unit2sensor_quat(NULL)
    , num_unit2sensors(0)
{
    FILE *config_file;
    vrpn_BaseClass::init();

    // Set the current time to zero, just to have something there
    timestamp.tv_sec = 0;
    timestamp.tv_usec = 0;

    // Set the watchdog time to zero, which will disable it unless it is used
    watchdog_timestamp.tv_sec = 0;
    watchdog_timestamp.tv_usec = 0;

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
    if (tracker_cfg_file_name == NULL) { // the default argument value
        tracker_cfg_file_name = default_tracker_cfg_file_name;
    }
    if ((config_file = fopen(tracker_cfg_file_name, "r")) == NULL) {
        // Can't find the config file
        // Complain only if we are using the a non-default config file
        // (Since most people don't use any config file at all,
        // and would be confused to see an error about not being
        // able to open the default config file)
        if (tracker_cfg_file_name != default_tracker_cfg_file_name) {
            fprintf(stderr, "vrpn_Tracker: Can't find config file %s\n",
                    tracker_cfg_file_name);
        }
    }
    else if (read_config_file(config_file, name)) {
        fprintf(
            stderr,
            "vrpn_Tracker: Found config file %s, but cannot read info for %s\n",
            tracker_cfg_file_name, name);
        fclose(config_file);
    }
    else { // no problems
        fprintf(stderr, "vrpn_Tracker: Read room and sensor info from %s\n",
                tracker_cfg_file_name);
        fclose(config_file);
    }
}

int vrpn_Tracker::register_types(void)
{
    // Register this tracker device and the needed message types
    if (d_connection) {
        position_m_id =
            d_connection->register_message_type("vrpn_Tracker Pos_Quat");
        velocity_m_id =
            d_connection->register_message_type("vrpn_Tracker Velocity");
        accel_m_id =
            d_connection->register_message_type("vrpn_Tracker Acceleration");
        tracker2room_m_id =
            d_connection->register_message_type("vrpn_Tracker To_Room");
        unit2sensor_m_id =
            d_connection->register_message_type("vrpn_Tracker Unit_To_Sensor");
        request_t2r_m_id = d_connection->register_message_type(
            "vrpn_Tracker Request_Tracker_To_Room");
        request_u2s_m_id = d_connection->register_message_type(
            "vrpn_Tracker Request_Unit_To_Sensor");
        workspace_m_id =
            d_connection->register_message_type("vrpn_Tracker Workspace");
        request_workspace_m_id = d_connection->register_message_type(
            "vrpn_Tracker Request_Tracker_Workspace");
        update_rate_id =
            d_connection->register_message_type("vrpn_Tracker set_update_rate");
        reset_origin_m_id =
            d_connection->register_message_type("vrpn_Tracker Reset_Origin");
    }
    return 0;
}

// virtual
// Delete all of the unit2sensor position and quaternion entries.
vrpn_Tracker::~vrpn_Tracker(void)
{
    if (unit2sensor != NULL) {
      try {
        delete[] unit2sensor;
      } catch (...) {
        fprintf(stderr, "vrpn_Tracker::~vrpn_Tracker(): delete failed\n");
        return;
      }
    }
    if (unit2sensor_quat != NULL) {
      try {
        delete[] unit2sensor_quat;
      } catch (...) {
        fprintf(stderr, "vrpn_Tracker::~vrpn_Tracker(): delete failed\n");
        return;
      }
    }
    num_unit2sensors = 0;
}

// Make sure we have enough unit2sensor elements in the array.
// Returns false if we run out of memory, true otherwise.
bool vrpn_Tracker::ensure_enough_unit2sensors(unsigned num)
{
    unsigned i;
    ++num; // Just to make sure we don't fall prey to off-by-one indexing
           // errors.
    if (num > num_unit2sensors) {
        // Make sure we allocate in large chunks, rather than one at a time.
        if (num < 2 * num_unit2sensors) {
            num = 2 * num_unit2sensors;
        }

        // Allocate new space for the two lists.
        vrpn_Tracker_Pos *newlist;
        try { newlist = new vrpn_Tracker_Pos[num]; }
        catch (...) { return false; }
        vrpn_Tracker_Quat *newqlist;
        try { newqlist = new vrpn_Tracker_Quat[num]; }
        catch (...) { return false; }

        // Copy all of the existing elements.
        for (i = 0; i < num_unit2sensors; i++) {
            memcpy(newlist[i], unit2sensor[i], sizeof(vrpn_Tracker_Pos));
            memcpy(newqlist[i], unit2sensor_quat[i], sizeof(vrpn_Tracker_Quat));
        }

        // Initialize all of the new elements
        for (i = num_unit2sensors; i < num; i++) {
            newlist[i][0] = newlist[i][1] = newlist[i][2] = 0.0;
            newqlist[i][0] = 0.0;
            newqlist[i][1] = 0.0;
            newqlist[i][2] = 0.0;
            newqlist[i][3] = 1.0;
        }

        // Switch the new lists in for the old, and delete the old.
        if (unit2sensor != NULL) {
          try {
            delete[] unit2sensor;
          } catch (...) {
            fprintf(stderr, "vrpn_Tracker::ensure_enough_unit2sensors(): delete failed\n");
            return false;
          }
        }
        if (unit2sensor_quat != NULL) {
          try {
            delete[] unit2sensor_quat;
          } catch (...) {
            fprintf(stderr, "vrpn_Tracker::ensure_enough_unit2sensors(): delete failed\n");
            return false;
          }
        }
        unit2sensor = newlist;
        unit2sensor_quat = newqlist;
        num_unit2sensors = num;
    }
    return true;
}

int vrpn_Tracker::read_config_file(FILE *config_file, const char *tracker_name)
{

    char line[512]; // line read from input file
    vrpn_int32 num_sens;
    vrpn_int32 which_sensor;
    float f[14];
    int i, j;

    // Read lines from the file until we run out
    while (fgets(line, sizeof(line), config_file) != NULL) {
        // Make sure the line wasn't too long
        if (strlen(line) >= sizeof(line) - 1) {
            fprintf(stderr, "Line too long in config file: %s\n", line);
            return -1;
        }
        // find tracker name in file
        if ((!(strncmp(line, tracker_name, strlen(tracker_name)))) &&
            (isspace(line[strlen(tracker_name)]))) {
            // get the tracker2room xform and the workspace volume
            if (fgets(line, sizeof(line), config_file) == NULL) break;
            if (sscanf(line, "%f%f%f", &f[0], &f[1], &f[2]) != 3) break;
            if (fgets(line, sizeof(line), config_file) == NULL) break;
            if (sscanf(line, "%f%f%f%f", &f[3], &f[4], &f[5], &f[6]) != 4)
                break;
            if (fgets(line, sizeof(line), config_file) == NULL) break;
            if (sscanf(line, "%f%f%f%f%f%f", &f[7], &f[8], &f[9], &f[10],
                       &f[11], &f[12]) != 6)
                break;

            for (i = 0; i < 3; i++) {
                tracker2room[i] = f[i];
                workspace_min[i] = f[i + 7];
                workspace_max[i] = f[i + 10];
            }
            for (i = 0; i < 4; i++)
                tracker2room_quat[i] = f[i + 3];
            // get the number of sensors
            if (fgets(line, sizeof(line), config_file) == NULL) break;
            if (sscanf(line, "%d", &num_sens) != 1) break;
            if (!ensure_enough_unit2sensors(num_sens + 1)) {
                fprintf(stderr, "Out of memory\n");
                return -1;
            }
            for (i = 0; i < num_sens; i++) {
                // get which sensor this xform is for
                if (fgets(line, sizeof(line), config_file) == NULL) break;
                if (sscanf(line, "%d", &which_sensor) != 1) break;
                if (!ensure_enough_unit2sensors(which_sensor + 1)) {
                    fprintf(stderr, "Out of memory\n");
                    return -1;
                }
                // get the sensor to unit xform
                if (fgets(line, sizeof(line), config_file) == NULL) break;
                if (sscanf(line, "%f%f%f", &f[0], &f[1], &f[2]) != 3) break;
                if (fgets(line, sizeof(line), config_file) == NULL) break;
                if (sscanf(line, "%f%f%f%f", &f[3], &f[4], &f[5], &f[6]) != 4)
                    break;
                for (j = 0; j < 3; j++) {
                    unit2sensor[which_sensor][j] = f[j];
                }
                for (j = 0; j < 4; j++) {
                    unit2sensor_quat[which_sensor][j] = f[j + 3];
                }
            }
            num_sensors = num_sens;
            return 0; // success
        }
    }
    fprintf(stderr, "Error reading or %s not found in config file\n",
            tracker_name);
    return -1;
}

void vrpn_Tracker::print_latest_report(void)
{
    printf("----------------------------------------------------\n");
    printf("Sensor    :%d\n", d_sensor);
    printf("Timestamp :%ld:%ld\n", timestamp.tv_sec,
           static_cast<long>(timestamp.tv_usec));
    printf("Framecount:%d\n", frame_count);
    printf("Pos       :%lf, %lf, %lf\n", pos[0], pos[1], pos[2]);
    printf("Quat      :%lf, %lf, %lf, %lf\n", d_quat[0], d_quat[1], d_quat[2],
           d_quat[3]);
}

int vrpn_Tracker::register_server_handlers(void)
{
    if (d_connection) {
        if (register_autodeleted_handler(request_t2r_m_id, handle_t2r_request,
                                         this, d_sender_id)) {
            fprintf(stderr, "vrpn_Tracker:can't register t2r handler\n");
            return -1;
        }
        if (register_autodeleted_handler(request_u2s_m_id, handle_u2s_request,
                                         this, d_sender_id)) {
            fprintf(stderr, "vrpn_Tracker:can't register u2s handler\n");
            return -1;
        }
        if (register_autodeleted_handler(request_workspace_m_id,
                                         handle_workspace_request, this,
                                         d_sender_id)) {
            fprintf(stderr, "vrpn_Tracker:  "
                            "Can't register workspace handler\n");
            return -1;
        }
    }
    else {
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
void vrpn_Tracker::get_local_u2s(vrpn_int32 sensor, vrpn_float64 *vec,
                                 vrpn_float64 *quat)
{
    ensure_enough_unit2sensors(sensor + 1);
    int i;
    for (i = 0; i < 3; i++)
        vec[i] = unit2sensor[sensor][i];
    for (i = 0; i < 4; i++)
        quat[i] = unit2sensor_quat[sensor][i];
}

int vrpn_Tracker::handle_t2r_request(void *userdata, vrpn_HANDLERPARAM p)
{
    struct timeval current_time;
    char msgbuf[1000];
    vrpn_int32 len;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata; // == this normally

    p = p; // Keep the compiler from complaining

    vrpn_gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our t2r transform was read in by the constructor

    // send t2r transform
    if (me->d_connection) {
        len = me->encode_tracker2room_to(msgbuf);
        if (me->d_connection->pack_message(
                len, me->timestamp, me->tracker2room_m_id, me->d_sender_id,
                msgbuf, vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker: cannot write t2r message\n");
        }
    }
    return 0;
}

int vrpn_Tracker::handle_u2s_request(void *userdata, vrpn_HANDLERPARAM p)
{
    struct timeval current_time;
    char msgbuf[1000];
    vrpn_int32 len;
    vrpn_int32 i;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata;

    p = p; // Keep the compiler from complaining

    vrpn_gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our u2s transforms were read in by the constructor

    if (me->d_connection) {
        me->ensure_enough_unit2sensors(me->num_sensors + 1);
        for (i = 0; i < me->num_sensors; i++) {
            me->d_sensor = i;
            // send u2s transform
            len = me->encode_unit2sensor_to(msgbuf);
            if (me->d_connection->pack_message(
                    len, me->timestamp, me->unit2sensor_m_id, me->d_sender_id,
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
    char msgbuf[1000];
    vrpn_int32 len;
    vrpn_Tracker *me = (vrpn_Tracker *)userdata;

    p = p; // Keep the compiler from complaining

    vrpn_gettimeofday(&current_time, NULL);
    me->timestamp.tv_sec = current_time.tv_sec;
    me->timestamp.tv_usec = current_time.tv_usec;

    // our workspace was read in by the constructor

    if (me->d_connection) {
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

int vrpn_Tracker::encode_tracker2room_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;
    int i;

    // Encode the position part of the transformation.
    for (i = 0; i < 3; i++) {
        vrpn_buffer(&bufptr, &buflen, tracker2room[i]);
    }

    // Encode the quaternion part of the transformation.
    for (i = 0; i < 4; i++) {
        vrpn_buffer(&bufptr, &buflen, tracker2room_quat[i]);
    }

    // Return the number of characters sent.
    return 1000 - buflen;
}

/** Encodes the "Unit to Sensor" transformation into the buffer
    specified. Returns the number of bytes encoded into the
    buffer.  Assumes that the buffer can hold all of the data.
    Encodes the position, then the quaternion.

    WARNING: make sure sensor is set to desired value before encoding
    this message.
*/

int vrpn_Tracker::encode_unit2sensor_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;
    int i;

    // Encode the sensor number, then put a filler in32 to re-align
    // to the 64-bit boundary.
    vrpn_buffer(&bufptr, &buflen, d_sensor);
    vrpn_buffer(&bufptr, &buflen, (vrpn_int32)(0));

    // Encode the position part of the transformation.
    for (i = 0; i < 3; i++) {
        vrpn_buffer(&bufptr, &buflen, unit2sensor[d_sensor][i]);
    }

    // Encode the quaternion part of the transformation.
    for (i = 0; i < 4; i++) {
        vrpn_buffer(&bufptr, &buflen, unit2sensor_quat[d_sensor][i]);
    }

    // Return the number of characters sent.
    return 1000 - buflen;
}

int vrpn_Tracker::encode_workspace_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;

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
int vrpn_Tracker::encode_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;

    // Message includes: long sensor, long scrap, vrpn_float64 pos[3],
    // vrpn_float64 quat[4]
    // Byte order of each needs to be reversed to match network standard

    vrpn_buffer(&bufptr, &buflen, d_sensor);
    vrpn_buffer(&bufptr, &buflen,
                d_sensor); // This is just to take up space to align

    vrpn_buffer(&bufptr, &buflen, pos[0]);
    vrpn_buffer(&bufptr, &buflen, pos[1]);
    vrpn_buffer(&bufptr, &buflen, pos[2]);

    vrpn_buffer(&bufptr, &buflen, d_quat[0]);
    vrpn_buffer(&bufptr, &buflen, d_quat[1]);
    vrpn_buffer(&bufptr, &buflen, d_quat[2]);
    vrpn_buffer(&bufptr, &buflen, d_quat[3]);

    return 1000 - buflen;
}

int vrpn_Tracker::encode_vel_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;

    // Message includes: long unitNum, vrpn_float64 vel[3], vrpn_float64
    // vel_quat[4]
    // Byte order of each needs to be reversed to match network standard

    vrpn_buffer(&bufptr, &buflen, d_sensor);
    vrpn_buffer(&bufptr, &buflen,
                d_sensor); // This is just to take up space to align

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

int vrpn_Tracker::encode_acc_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;

    // Message includes: long unitNum, vrpn_float64 acc[3], vrpn_float64
    // acc_quat[4]
    // Byte order of each needs to be reversed to match network standard

    vrpn_buffer(&bufptr, &buflen, d_sensor);
    vrpn_buffer(&bufptr, &buflen,
                d_sensor); // This is just to take up space to align

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

vrpn_Tracker_NULL::vrpn_Tracker_NULL(const char *name, vrpn_Connection *c,
                                     vrpn_int32 sensors, vrpn_float64 Hz)
    : vrpn_Tracker(name, c)
    , update_rate(Hz)
    , d_redundancy(NULL)
{
    num_sensors = sensors;
    register_server_handlers();
    // Nothing left to do
}

void vrpn_Tracker_NULL::mainloop()
{
    struct timeval current_time;
    char msgbuf[1000];
    vrpn_int32 i, len;

    // Call the generic server mainloop routine, since this is a server
    server_mainloop();

    // See if its time to generate a new report
    vrpn_gettimeofday(&current_time, NULL);
    if (vrpn_TimevalDuration(current_time, timestamp) >=
        1000000.0 / update_rate) {

        // Update the time
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

        // Send messages for all sensors if we have a connection
        if (d_redundancy) {
            for (i = 0; i < num_sensors; i++) {
                d_sensor = i;

                // Pack position report
                len = encode_to(msgbuf);
                if (d_redundancy->pack_message(len, timestamp, position_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,
                            "NULL tracker: can't write message: tossing\n");
                }

                // Pack velocity report
                len = encode_vel_to(msgbuf);
                if (d_redundancy->pack_message(len, timestamp, velocity_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,
                            "NULL tracker: can't write message: tossing\n");
                }

                // Pack acceleration report
                len = encode_acc_to(msgbuf);
                if (d_redundancy->pack_message(len, timestamp, accel_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,
                            "NULL tracker: can't write message: tossing\n");
                }
            }
        }
        else if (d_connection) {
            for (i = 0; i < num_sensors; i++) {
                d_sensor = i;

                // Pack position report
                len = encode_to(msgbuf);
                if (d_connection->pack_message(len, timestamp, position_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,
                            "NULL tracker: can't write message: tossing\n");
                }

                // Pack velocity report
                len = encode_vel_to(msgbuf);
                if (d_connection->pack_message(len, timestamp, velocity_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,
                            "NULL tracker: can't write message: tossing\n");
                }

                // Pack acceleration report
                len = encode_acc_to(msgbuf);
                if (d_connection->pack_message(len, timestamp, accel_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,
                            "NULL tracker: can't write message: tossing\n");
                }
            }
        }
    }
}

void vrpn_Tracker_NULL::setRedundantTransmission(vrpn_RedundantTransmission *t)
{
    d_redundancy = t;
}

vrpn_Tracker_Spin::vrpn_Tracker_Spin(const char *name, vrpn_Connection *c,
  vrpn_int32 sensors, vrpn_float64 Hz,
  vrpn_float64 axisX, vrpn_float64 axisY,
  vrpn_float64 axisZ, vrpn_float64 spinRateHz)
  : vrpn_Tracker(name, c)
  , update_rate(Hz)
  , x(axisX)
  , y(axisY)
  , z(axisZ)
  , spin_rate_Hz(spinRateHz)
{
  num_sensors = sensors;
  register_server_handlers();

  // Get the time we started
  vrpn_gettimeofday(&start, NULL);

  // If the spin rate is set to be negative, then invert the
  // axis and set it to positive.
  if (spin_rate_Hz < 0) {
    spin_rate_Hz *= -1;
    x *= -1;
    y *= -1;
    z *= -1;
  }

  // Set the pose velocity to match the axis we're spinning
  // about and our rate of spin.  It will remain the same
  // throughout the run.  Make the dt such that we spin by
  // less than half a rotation each delta, so we don't alias
  // our speed.  Scale rotation by this dt.
  double dt;
  if (spin_rate_Hz == 0) {
    dt = 1.0;
  } else {
    dt = 0.9 * (0.5/spin_rate_Hz);
  }
  q_from_axis_angle(vel_quat, x, y, z, dt * spin_rate_Hz * 2*Q_PI);
  vel_quat_dt = dt;
}

void vrpn_Tracker_Spin::mainloop()
{
  struct timeval current_time;
  char msgbuf[1000];
  vrpn_int32 i, len;

  // Call the generic server mainloop routine, since this is a server
  server_mainloop();

  // See if its time to generate a new report
  vrpn_gettimeofday(&current_time, NULL);
  if (vrpn_TimevalDurationSeconds(current_time, timestamp) >=
    1.0 / update_rate) {

    // Update the time
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    // Figure out our new pose.
    double duration = vrpn_TimevalDurationSeconds(current_time, start);
    q_from_axis_angle(d_quat, x, y, z, duration * spin_rate_Hz * 2*Q_PI);

    // Send messages for all sensors if we have a connection
    if (d_connection) {
      for (i = 0; i < num_sensors; i++) {
        d_sensor = i;

        // Pack position report
        len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id,
          d_sender_id, msgbuf,
          vrpn_CONNECTION_LOW_LATENCY)) {
          fprintf(stderr,
            "NULL tracker: can't write message: tossing\n");
        }

        // Pack velocity report
        len = encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, velocity_m_id,
          d_sender_id, msgbuf,
          vrpn_CONNECTION_LOW_LATENCY)) {
          fprintf(stderr,
            "NULL tracker: can't write message: tossing\n");
        }

        // Pack acceleration report
        len = encode_acc_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, accel_m_id,
          d_sender_id, msgbuf,
          vrpn_CONNECTION_LOW_LATENCY)) {
          fprintf(stderr,
            "NULL tracker: can't write message: tossing\n");
        }
      }
    }
  }
}

vrpn_Tracker_Server::vrpn_Tracker_Server(const char *name, vrpn_Connection *c,
                                         vrpn_int32 sensors)
    : vrpn_Tracker(name, c)
{
    num_sensors = sensors;
    register_server_handlers();
    // Nothing left to do
}

void vrpn_Tracker_Server::mainloop()
{
    // Call the generic server mainloop routine, since this is a server
    server_mainloop();
}

int vrpn_Tracker_Server::report_pose(const int sensor, const struct timeval t,
                                     const vrpn_float64 position[3],
                                     const vrpn_float64 quaternion[4],
                                     const vrpn_uint32 class_of_service)
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Update the time
    timestamp.tv_sec = t.tv_sec;
    timestamp.tv_usec = t.tv_usec;

    // Send messages for all sensors if we have a connection
    if (sensor >= num_sensors) {
        send_text_message("Sensor number too high", timestamp, vrpn_TEXT_ERROR);
        return -1;
    }
    else if (!d_connection) {
        send_text_message("No connection", timestamp, vrpn_TEXT_ERROR);
        return -1;
    }
    else {
        d_sensor = sensor;

        // Pack position report
        memcpy(pos, position, sizeof(pos));
        memcpy(d_quat, quaternion, sizeof(d_quat));
        len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id,
                                       d_sender_id, msgbuf, class_of_service)) {
            fprintf(stderr,
                    "vrpn_Tracker_Server: can't write message: tossing\n");
            return -1;
        }
    }
    return 0;
}

int vrpn_Tracker_Server::report_pose_velocity(
    const int sensor, const struct timeval t, const vrpn_float64 position[3],
    const vrpn_float64 quaternion[4], const vrpn_float64 interval,
    const vrpn_uint32 class_of_service)
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Update the time
    timestamp.tv_sec = t.tv_sec;
    timestamp.tv_usec = t.tv_usec;

    // Send messages for all sensors if we have a connection
    if (sensor >= num_sensors) {
        send_text_message("Sensor number too high", timestamp, vrpn_TEXT_ERROR);
        return -1;
    }
    else if (!d_connection) {
        send_text_message("No connection", timestamp, vrpn_TEXT_ERROR);
        return -1;
    }
    else {
        d_sensor = sensor;

        // Pack velocity report
        memcpy(vel, position, sizeof(pos));
        memcpy(vel_quat, quaternion, sizeof(d_quat));
        vel_quat_dt = interval;
        len = encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, velocity_m_id,
                                       d_sender_id, msgbuf, class_of_service)) {
            fprintf(stderr,
                    "vrpn_Tracker_Server: can't write message: tossing\n");
            return -1;
        }
    }

    return 0;
}

int vrpn_Tracker_Server::report_pose_acceleration(
    const int sensor, const struct timeval t, const vrpn_float64 position[3],
    const vrpn_float64 quaternion[4], const vrpn_float64 interval,
    const vrpn_uint32 class_of_service)
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Update the time
    timestamp.tv_sec = t.tv_sec;
    timestamp.tv_usec = t.tv_usec;

    // Send messages for all sensors if we have a connection
    if (sensor >= num_sensors) {
        send_text_message("Sensor number too high", timestamp, vrpn_TEXT_ERROR);
        return -1;
    }
    else if (!d_connection) {
        send_text_message("No connection", timestamp, vrpn_TEXT_ERROR);
        return -1;
    }
    else {
        d_sensor = sensor;

        // Pack acceleration report
        memcpy(acc, position, sizeof(pos));
        memcpy(acc_quat, quaternion, sizeof(d_quat));
        acc_quat_dt = interval;
        len = encode_acc_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, accel_m_id, d_sender_id,
                                       msgbuf, class_of_service)) {
            fprintf(stderr,
                    "vrpn_Tracker_Server: can't write message: tossing\n");
            return -1;
        }
    }

    return 0;
}

#ifndef VRPN_CLIENT_ONLY
vrpn_Tracker_Serial::vrpn_Tracker_Serial(const char *name, vrpn_Connection *c,
                                         const char *port, long baud)
    : vrpn_Tracker(name, c)
    , serial_fd(-1)
{
    register_server_handlers();
    // Find out the port name and baud rate
    if (port == NULL) {
        fprintf(stderr, "vrpn_Tracker_Serial: NULL port name\n");
        status = vrpn_TRACKER_FAIL;
        return;
    } else {
        vrpn_strcpy(portname, port);
    }
    baudrate = baud;

    // Open the serial port we're going to use
    if ((serial_fd = vrpn_open_commport(portname, baudrate)) == -1) {
        fprintf(stderr, "vrpn_Tracker_Serial: Cannot Open serial port\n");
        status = vrpn_TRACKER_FAIL;
    }

    // Reset the tracker and find out what time it is
    status = vrpn_TRACKER_RESETTING;
    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Tracker_Serial::~vrpn_Tracker_Serial()
{
    if (serial_fd >= 0) {
        vrpn_close_commport(serial_fd);
        serial_fd = -1;
    }
}

void vrpn_Tracker_Serial::send_report(void)
{
    // Send the message on the connection
    if (d_connection) {
        char msgbuf[1000];
        int len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "Tracker: cannot write message: tossing\n");
        }
    }
    else {
        fprintf(stderr, "Tracker: No valid connection\n");
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
    case vrpn_TRACKER_PARTIAL: {
        // It turns out to be important to get the report before checking
        // to see if it has been too long since the last report.  This is
        // because there is the possibility that some other device running
        // in the same server may have taken a long time on its last pass
        // through mainloop().  Trackers that are resetting do this.  When
        // this happens, you can get an infinite loop -- where one tracker
        // resets and causes the other to timeout, and then it returns the
        // favor.  By checking for the report here, we reset the timestamp
        // if there is a report ready (ie, if THIS device is still operating).

        while (get_report()) { // While we get reports, continue to send them.
            send_report();
        };

        struct timeval current_time;
        vrpn_gettimeofday(&current_time, NULL);
        int time_lapsed; /* The time since the last report */

        // Watchdog timestamp is implemented by Polhemus Liberty driver.
        // XXX All trackers should be modified to use this, or it to not.
        // If the watchdog timestamp is zero, use the last timestamp to check.
        if (watchdog_timestamp.tv_sec == 0) {
            time_lapsed = vrpn_TimevalDuration(current_time, timestamp);
        }
        else { // The watchdog_timestamp is being used
            time_lapsed =
                vrpn_TimevalDuration(current_time, watchdog_timestamp);
        }

        if (time_lapsed > vrpn_ser_tkr_MAX_TIME_INTERVAL) {
            char errmsg[1024];
            sprintf(errmsg, "Tracker failed to read... current_time=%ld:%ld, "
                            "timestamp=%ld:%ld\n",
                    current_time.tv_sec,
                    static_cast<long>(current_time.tv_usec), timestamp.tv_sec,
                    static_cast<long>(timestamp.tv_usec));
            send_text_message(errmsg, current_time, vrpn_TEXT_ERROR);
            status = vrpn_TRACKER_FAIL;
        }
    } break;

    case vrpn_TRACKER_RESETTING:
        reset();
        break;

    case vrpn_TRACKER_FAIL:
        send_text_message("Tracker failed, trying to reset (Try power cycle if "
                          "more than 4 attempts made)",
                          timestamp, vrpn_TEXT_ERROR);
        if (serial_fd >= 0) {
            vrpn_close_commport(serial_fd);
            serial_fd = -1;
        }
        if ((serial_fd = vrpn_open_commport(portname, baudrate)) == -1) {
            fprintf(
                stderr,
                "vrpn_Tracker_Serial::mainloop(): Cannot Open serial port\n");
            status = vrpn_TRACKER_FAIL;
        }
        status = vrpn_TRACKER_RESETTING;
        break;
    }
}

#if defined(VRPN_USE_LIBUSB_1_0)

vrpn_Tracker_USB::vrpn_Tracker_USB(const char *name, vrpn_Connection *c,
                                   vrpn_uint16 vendor, vrpn_uint16 product,
                                   long baud)
    : vrpn_Tracker(name, c)
    , _device_handle(NULL)
    , _vendor(vendor)
    , _product(product)
    , _baudrate(baud)
{
    // Register handlers
    register_server_handlers();

    // Initialize libusb
    if (libusb_init(&_context) != 0) {
        fprintf(stderr, "vrpn_Tracker_USB: can't init LibUSB\n");
        status = vrpn_TRACKER_FAIL;
        return;
    }
    // libusb_set_debug (_context, 3);

    // Open and claim an usb device with the expected vendor and product ID.
    if ((_device_handle = libusb_open_device_with_vid_pid(_context, _vendor,
                                                          _product)) == NULL) {
        fprintf(stderr, "vrpn_Tracker_USB: can't find any Polhemus High Speed "
                        "Liberty Latus devices\n");
        fprintf(stderr,
                "                      (Did you remember to run as root?)\n");
        status = vrpn_TRACKER_FAIL;
        return;
    }

    if (libusb_claim_interface(_device_handle, 0) != 0) {
        fprintf(stderr,
                "vrpn_Tracker_USB: can't claim interface for this device\n");
        fprintf(stderr,
                "                      (Did you remember to run as root?)\n");
        libusb_close(_device_handle);
        _device_handle = NULL;
        libusb_exit(_context);
        _context = NULL;
        status = vrpn_TRACKER_FAIL;
        return;
    }

    // Reset the tracker and find out what time it is
    status = vrpn_TRACKER_RESETTING;
    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Tracker_USB::~vrpn_Tracker_USB()
{
    if (_device_handle) {
        libusb_close(_device_handle);
        _device_handle = NULL;
    }
    if (_context) {
        libusb_exit(_context);
        _context = NULL;
    }
}

void vrpn_Tracker_USB::send_report(void)
{
    // Send the message on the connection
    if (d_connection) {
        char msgbuf[1000];
        int len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "Tracker: cannot write message: tossing\n");
        }
    }
    else {
        fprintf(stderr, "Tracker: No valid connection\n");
    }
}

/** This function should be called each time through the main loop
    of the server code. It polls for reports from the tracker and
    sends them if there are one or more. It will reset the tracker
    if there is no data from it for a few seconds.
**/
void vrpn_Tracker_USB::mainloop()
{
    server_mainloop();

    switch (status) {
    case vrpn_TRACKER_SYNCING:
    case vrpn_TRACKER_PARTIAL: {
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

        // Save reception time
        struct timeval current_time;
        vrpn_gettimeofday(&current_time, NULL);
        int time_lapsed; // The time since the last report

        // Watchdog timestamp is implemented by Polhemus Liberty driver.
        // XXX All trackers should be modified to use this, or it to not.
        // If the watchdog timestamp is zero, use the last timestamp to check.
        if (watchdog_timestamp.tv_sec == 0) {
            time_lapsed = vrpn_TimevalDuration(current_time, timestamp);
        }
        else { // The watchdog_timestamp is being used
            time_lapsed =
                vrpn_TimevalDuration(current_time, watchdog_timestamp);
        }

        if (time_lapsed > vrpn_ser_tkr_MAX_TIME_INTERVAL) {
            char errmsg[1024];
            sprintf(errmsg, "Tracker failed to read... current_time=%ld:%ld, "
                            "timestamp=%ld:%ld\n",
                    current_time.tv_sec,
                    static_cast<long>(current_time.tv_usec), timestamp.tv_sec,
                    static_cast<long>(timestamp.tv_usec));
            send_text_message(errmsg, current_time, vrpn_TEXT_ERROR);
            status = vrpn_TRACKER_FAIL;
        }

    } break;

    case vrpn_TRACKER_RESETTING:
        reset();
        break;

    case vrpn_TRACKER_FAIL:
        send_text_message("Tracker failed, trying to reset (Try power cycle if "
                          "more than 4 attempts made)",
                          timestamp, vrpn_TEXT_ERROR);
        // Reset the device handle and then attempt to connect to a device.
        if (_device_handle) {
            libusb_close(_device_handle);
            _device_handle = NULL;
        }
        if ((_device_handle = libusb_open_device_with_vid_pid(
                 _context, _vendor, _product)) == NULL) {
            fprintf(stderr, "vrpn_Tracker_USB::mainloop(): can't find any "
                            "Polhemus High Speed Liberty Latus devices\n");
            status = vrpn_TRACKER_FAIL;
            break;
        }

        if (libusb_claim_interface(_device_handle, 0) != 0) {
            fprintf(stderr, "vrpn_Tracker_USB::mainloop(): can't claim "
                            "interface for this device\n");
            libusb_close(_device_handle);
            _device_handle = NULL;
            status = vrpn_TRACKER_FAIL;
            break;
        }

        status = vrpn_TRACKER_RESETTING;
        break;
    }
}

// End of LIBUSB
#endif

#endif // VRPN_CLIENT_ONLY

vrpn_Tracker_Remote::vrpn_Tracker_Remote(const char *name, vrpn_Connection *cn)
    : vrpn_Tracker(name, cn)
    , sensor_callbacks(NULL)
    , num_sensor_callbacks(0)
{
    // Make sure that we have a valid connection
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_Tracker_Remote: No connection\n");
        return;
    }

    // Register a handler for the position change callback from this device.
    if (register_autodeleted_handler(position_m_id, handle_change_message, this,
                                     d_sender_id)) {
        fprintf(stderr,
                "vrpn_Tracker_Remote: can't register position handler\n");
        d_connection = NULL;
    }

    // Register a handler for the velocity change callback from this device.
    if (register_autodeleted_handler(velocity_m_id, handle_vel_change_message,
                                     this, d_sender_id)) {
        fprintf(stderr,
                "vrpn_Tracker_Remote: can't register velocity handler\n");
        d_connection = NULL;
    }

    // Register a handler for the acceleration change callback.
    if (register_autodeleted_handler(accel_m_id, handle_acc_change_message,
                                     this, d_sender_id)) {
        fprintf(stderr,
                "vrpn_Tracker_Remote: can't register acceleration handler\n");
        d_connection = NULL;
    }

    // Register a handler for the room to tracker xform change callback
    if (register_autodeleted_handler(tracker2room_m_id,
                                     handle_tracker2room_change_message, this,
                                     d_sender_id)) {
        fprintf(stderr,
                "vrpn_Tracker_Remote: can't register tracker2room handler\n");
        d_connection = NULL;
    }

    // Register a handler for the sensor to unit xform change callback
    if (register_autodeleted_handler(unit2sensor_m_id,
                                     handle_unit2sensor_change_message, this,
                                     d_sender_id)) {
        fprintf(stderr,
                "vrpn_Tracker_Remote: can't register unit2sensor handler\n");
        d_connection = NULL;
    }

    // Register a handler for the workspace change callback
    if (register_autodeleted_handler(workspace_m_id,
                                     handle_workspace_change_message, this,
                                     d_sender_id)) {
        fprintf(stderr,
                "vrpn_Tracker_Remote: can't register workspace handler\n");
        d_connection = NULL;
    }

    // Find out what time it is and put this into the timestamp
    vrpn_gettimeofday(&timestamp, NULL);
}

// The remote tracker has to un-register its handlers when it
// is destroyed to avoid seg faults (this is taken care of by
// using autodeleted handlers above). It should also remove all
// remaining user-registered callbacks to free up memory.

vrpn_Tracker_Remote::~vrpn_Tracker_Remote()
{
    if (sensor_callbacks != NULL) {
      try {
        delete[] sensor_callbacks;
      } catch (...) {
        fprintf(stderr, "vrpn_Tracker_Remote::~vrpn_Tracker_Remote(): delete failed\n");
        return;
      }
    }
    num_sensor_callbacks = 0;
}

// Make sure we have enough sensor_callback elements in the array.
// Returns false if we run out of memory, true otherwise.
bool vrpn_Tracker_Remote::ensure_enough_sensor_callbacks(unsigned num)
{
    unsigned i;
    ++num; // Just to make sure we don't fall prey to off-by-one indexing
           // errors.
    if (num > num_sensor_callbacks) {
        // Make sure we allocate in large chunks, rather than one at a time.
        if (num < 2 * num_sensor_callbacks) {
            num = 2 * num_sensor_callbacks;
        }

        // Allocate new space for the list.
        vrpn_Tracker_Sensor_Callbacks *newlist;
        try { newlist = new vrpn_Tracker_Sensor_Callbacks[num]; }
        catch (...) {
            return false;
        }

        // Copy all of the existing elements.
        for (i = 0; i < num_sensor_callbacks; i++) {
            newlist[i] = sensor_callbacks[i];
        }

        // The new elements will be empty by default, nothing to do here.

        // Switch the new list in for the old, and delete the old.
        if (sensor_callbacks != NULL) {
          try {
            delete[] sensor_callbacks;
          } catch (...) {
            fprintf(stderr, "vrpn_Tracker_Remote::ensure_enough_sensor_callbacks(): delete failed\n");
            return false;
          }
        }
        sensor_callbacks = newlist;
        num_sensor_callbacks = num;
    }
    return true;
}

int vrpn_Tracker_Remote::request_t2r_xform(void)
{
    char *msgbuf = NULL;
    vrpn_int32 len = 0; // no payload
    struct timeval current_time;

    vrpn_gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    if (d_connection) {
        if (d_connection->pack_message(len, timestamp, request_t2r_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker_Remote: cannot request t2r xform\n");
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

    vrpn_gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    if (d_connection) {
        if (d_connection->pack_message(len, timestamp, request_u2s_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker_Remote: cannot request u2s xform\n");
            return -1;
        }
    }
    return 0;
}

int vrpn_Tracker_Remote::request_workspace(void)
{
    char *msgbuf = NULL;
    vrpn_int32 len = 0; // no payload
    struct timeval current_time;

    vrpn_gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    if (d_connection) {
        if (d_connection->pack_message(len, timestamp, request_workspace_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr, "vrpn_Tracker_Remote: cannot request workspace\n");
            return -1;
        }
    }

    return 0;
}

int vrpn_Tracker_Remote::set_update_rate(vrpn_float64 samplesPerSecond)
{
    char msgbuf[sizeof(vrpn_float64)];
    char *bufptr = msgbuf;
    vrpn_int32 len = sizeof(vrpn_float64);
    struct timeval now;

    vrpn_int32 buflen = len;
    vrpn_buffer(&bufptr, &buflen, samplesPerSecond);

    vrpn_gettimeofday(&now, NULL);
    timestamp.tv_sec = now.tv_sec;
    timestamp.tv_usec = now.tv_usec;

    if (d_connection) {
        if (d_connection->pack_message(len, timestamp, update_rate_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_RELIABLE)) {
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
    vrpn_gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    if (d_connection) {
        if (d_connection->pack_message(0, timestamp, reset_origin_m_id,
                                       d_sender_id, NULL,
                                       vrpn_CONNECTION_RELIABLE)) {
            fprintf(stderr,
                    "vrpn_Tracker_Remote: cannot write message: tossing\n");
        }
    }
    return 0;
}

void vrpn_Tracker_Remote::mainloop()
{
    if (d_connection) {
        d_connection->mainloop();
    }
    client_mainloop();
}

int vrpn_Tracker_Remote::register_change_handler(
    void *userdata, vrpn_TRACKERCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: bad sensor index\n");
        return -1;
    }
    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        fprintf(stderr,
                "vrpn_Tracker_Remote::register_change_handler: NULL handler\n");
        return -1;
    }

    // If this is the ALL_SENSORS value, put it on the all list; otherwise,
    // put it into the normal list.
    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_change.register_handler(userdata,
                                                              handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor].d_change.register_handler(userdata,
                                                                       handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::register_change_handler(
    void *userdata, vrpn_TRACKERVELCHANGEHANDLER handler,
    vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: bad sensor index\n");
        return -1;
    }
    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        fprintf(stderr,
                "vrpn_Tracker_Remote::register_change_handler: NULL handler\n");
        return -1;
    }

    // If this is the ALL_SENSORS value, put it on the all list; otherwise,
    // put it into the normal list.
    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_velchange.register_handler(userdata,
                                                                 handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor].d_velchange.register_handler(
            userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::register_change_handler(
    void *userdata, vrpn_TRACKERACCCHANGEHANDLER handler,
    vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: bad sensor index\n");
        return -1;
    }

    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        fprintf(stderr,
                "vrpn_Tracker_Remote::register_change_handler: NULL handler\n");
        return -1;
    }

    // If this is the ALL_SENSORS value, put it on the all list; otherwise,
    // put it into the normal list.
    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_accchange.register_handler(userdata,
                                                                 handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor].d_accchange.register_handler(
            userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::register_change_handler(
    void *userdata, vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler,
    vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: bad sensor index\n");
        return -1;
    }

    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        fprintf(stderr, "%s%s", "vrpn_Tracker_Remote:",
                ":register_change_handler: NULL handler\n");
        return -1;
    }

    // If this is the ALL_SENSORS value, put it on the all list; otherwise,
    // put it into the normal list.
    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_unit2sensorchange.register_handler(
            userdata, handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor]
            .d_unit2sensorchange.register_handler(userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::register_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::unregister_change_handler(
    void *userdata, vrpn_TRACKERCHANGEHANDLER handler, vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(stderr, "vrpn_Tracker_Remote::unregister_change_handler: bad "
                        "sensor index\n");
        return -1;
    }

    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_change.unregister_handler(userdata,
                                                                handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor].d_change.unregister_handler(
            userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::unregister_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::unregister_change_handler(
    void *userdata, vrpn_TRACKERVELCHANGEHANDLER handler,
    vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(stderr, "vrpn_Tracker_Remote::unregister_change_handler: bad "
                        "sensor index\n");
        return -1;
    }

    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_velchange.unregister_handler(userdata,
                                                                   handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor].d_velchange.unregister_handler(
            userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::unregister_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::unregister_change_handler(
    void *userdata, vrpn_TRACKERACCCHANGEHANDLER handler,
    vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(stderr, "vrpn_Tracker_Remote::unregister_change_handler: bad "
                        "sensor index\n");
        return -1;
    }

    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_accchange.unregister_handler(userdata,
                                                                   handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor].d_accchange.unregister_handler(
            userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::unregister_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::unregister_change_handler(
    void *userdata, vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler,
    vrpn_int32 whichSensor)
{
    if (whichSensor < vrpn_ALL_SENSORS) {
        fprintf(stderr, "vrpn_Tracker_Remote::unregister_change_handler: bad "
                        "sensor index\n");
        return -1;
    }

    if (whichSensor == vrpn_ALL_SENSORS) {
        return all_sensor_callbacks.d_unit2sensorchange.unregister_handler(
            userdata, handler);
    }
    else if (ensure_enough_sensor_callbacks(whichSensor)) {
        return sensor_callbacks[whichSensor]
            .d_unit2sensorchange.unregister_handler(userdata, handler);
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_Remote::unregister_change_handler: Out of memory\n");
        return -1;
    }
}

int vrpn_Tracker_Remote::handle_change_message(void *userdata,
                                               vrpn_HANDLERPARAM p)
{
    vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
    const char *params = (p.buffer);
    vrpn_int32 padding;
    vrpn_TRACKERCB tp;
    int i;

    // Fill in the parameters to the tracker from the message
    if (p.payload_len != (8 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Tracker: change message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(8 * sizeof(vrpn_float64)));
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

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->all_sensor_callbacks.d_change.call_handlers(tp);

    // Go down the list of callbacks that have been registered for this
    // particular sensor
    if (tp.sensor < 0) {
        fprintf(stderr, "vrpn_Tracker_Rem:pos sensor index is negative!\n");
        return -1;
    }
    else if (me->ensure_enough_sensor_callbacks(tp.sensor)) {
        me->sensor_callbacks[tp.sensor].d_change.call_handlers(tp);
    }
    else {
        fprintf(stderr, "vrpn_Tracker_Rem:pos sensor index too large\n");
        return -1;
    }
    return 0;
}

int vrpn_Tracker_Remote::handle_vel_change_message(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
    const char *params = p.buffer;
    vrpn_int32 padding;
    vrpn_TRACKERVELCB tp;
    int i;

    // Fill in the parameters to the tracker from the message
    if (p.payload_len != (9 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Tracker: vel message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(9 * sizeof(vrpn_float64)));
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

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->all_sensor_callbacks.d_velchange.call_handlers(tp);

    // Go down the list of callbacks that have been registered for this
    // particular sensor
    if (me->ensure_enough_sensor_callbacks(tp.sensor)) {
        me->sensor_callbacks[tp.sensor].d_velchange.call_handlers(tp);
    }
    else {
        fprintf(stderr, "vrpn_Tracker_Rem:vel sensor index too large\n");
        return -1;
    }
    return 0;
}

int vrpn_Tracker_Remote::handle_acc_change_message(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
    const char *params = p.buffer;
    vrpn_int32 padding;
    vrpn_TRACKERACCCB tp;
    int i;

    // Fill in the parameters to the tracker from the message
    if (p.payload_len != (9 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Tracker: acc message payload error\n");
        fprintf(stderr, "(got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(9 * sizeof(vrpn_float64)));
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

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->all_sensor_callbacks.d_accchange.call_handlers(tp);

    // Go down the list of callbacks that have been registered for this
    // particular sensor
    if (me->ensure_enough_sensor_callbacks(tp.sensor)) {
        me->sensor_callbacks[tp.sensor].d_accchange.call_handlers(tp);
    }
    else {
        fprintf(stderr, "vrpn_Tracker_Rem:acc sensor index too large\n");
        return -1;
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
    int i;

    // Fill in the parameters to the tracker from the message
    if (p.payload_len != (8 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Tracker: unit2sensor message payload");
        fprintf(stderr, " error\n(got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(8 * sizeof(vrpn_float64)));
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

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->all_sensor_callbacks.d_unit2sensorchange.call_handlers(tp);

    // Go down the list of callbacks that have been registered for this
    // particular sensor
    if (me->ensure_enough_sensor_callbacks(tp.sensor)) {
        me->sensor_callbacks[tp.sensor].d_unit2sensorchange.call_handlers(tp);
    }
    else {
        fprintf(stderr, "vrpn_Tracker_Rem:u2s sensor index too large\n");
        return -1;
    }

    return 0;
}

int vrpn_Tracker_Remote::handle_tracker2room_change_message(void *userdata,
                                                            vrpn_HANDLERPARAM p)
{
    vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
    const char *params = p.buffer;
    vrpn_TRACKERTRACKER2ROOMCB tp;
    int i;

    // Fill in the parameters to the tracker from the message
    if (p.payload_len != (7 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Tracker: tracker2room message payload");
        fprintf(stderr, " error\n(got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(7 * sizeof(vrpn_float64)));
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
    me->d_tracker2roomchange_list.call_handlers(tp);

    return 0;
}

int vrpn_Tracker_Remote::handle_workspace_change_message(void *userdata,
                                                         vrpn_HANDLERPARAM p)
{
    vrpn_Tracker_Remote *me = (vrpn_Tracker_Remote *)userdata;
    const char *params = p.buffer;
    vrpn_TRACKERWORKSPACECB tp;
    int i;

    // Fill in the parameters to the tracker from the message
    if (p.payload_len != (6 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Tracker: tracker2room message payload");
        fprintf(stderr, " error\n(got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(6 * sizeof(vrpn_float64)));
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
    me->d_workspacechange_list.call_handlers(tp);

    return 0;
}
