#include <quat.h>   // for q_mult
#include <stdio.h>  // for fprintf, stderr, NULL
#include <string.h> // for memcpy

// NOTE: a vrpn poser must accept poser data (pos and
//       ori info) which represent the transformation such
//       that the pos info is the position of the origin of
//       the poser coord sys in the source coord sys space, and the
//       quat represents the orientation of the poser relative to the
//       source space (ie, its value rotates the source's axes so that
//       they coincide with the poser's)

//      borrows heavily from the vrpn_Tracker code, as the poser is basically
//      the inverse of a tracker
#include "vrpn_Connection.h" // for vrpn_HANDLERPARAM, etc
// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h
// and unistd.h
#include "vrpn_Shared.h" // for timeval, vrpn_buffer, etc

#ifdef _WIN32
#ifndef _WIN32_WCE
#include <io.h>
#endif
#endif

#include "vrpn_Poser.h"

//#define VERBOSE
// #define READ_HISTOGRAM

vrpn_Poser::vrpn_Poser(const char* name, vrpn_Connection* c)
    : vrpn_BaseClass(name, c)
{
    vrpn_BaseClass::init();

    // Find out what time it is and put this into the timestamp
    vrpn_gettimeofday(&p_timestamp, NULL);

    // Set the position to the origin and the orientation to identity
    // just to have something there in case nobody fills them in later
    p_pos[0] = p_pos[1] = p_pos[2] = 0.0;
    p_quat[0] = p_quat[1] = p_quat[2] = 0.0;
    p_quat[3] = 1.0;

    // Set the velocity to zero and the orientation to identity
    // just to have something there in case nobody fills them in later
    p_vel[0] = p_vel[1] = p_vel[2] = 0.0;
    p_vel_quat[0] = p_vel_quat[1] = p_vel_quat[2] = 0.0;
    p_vel_quat[3] = 1.0;
    p_vel_quat_dt = 1;

    // Set the workspace max and min values just to have something there
    p_pos_max[0] = p_pos_max[1] = p_pos_max[2] = p_vel_max[0] = p_vel_max[1] =
        p_vel_max[2] = 1.0;
    p_pos_min[0] = p_pos_min[1] = p_pos_min[2] = p_vel_min[0] = p_vel_min[1] =
        p_vel_min[2] = -1.0;
    p_pos_rot_max[0] = p_pos_rot_max[1] = p_pos_rot_max[2] = p_vel_rot_max[0] =
        p_vel_rot_max[1] = p_vel_rot_max[2] = 1.0;
    p_pos_rot_min[0] = p_pos_rot_min[1] = p_pos_rot_min[2] = p_vel_rot_min[0] =
        p_vel_rot_min[1] = p_vel_rot_min[2] = -1.0;
}

void vrpn_Poser::p_print()
{
    fprintf(stderr, "Pos:  %lf, %lf, %lf\n", p_pos[0], p_pos[1], p_pos[2]);
    fprintf(stderr, "Quat: %lf, %lf, %lf, %lf\n", p_quat[0], p_quat[1],
            p_quat[2], p_quat[3]);
}

void vrpn_Poser::p_print_vel()
{
    fprintf(stderr, "Vel:     %lf, %lf, %lf\n", p_vel[0], p_vel[1], p_vel[2]);
    fprintf(stderr, "Quat:    %lf, %lf, %lf, %lf\n", p_vel_quat[0],
            p_vel_quat[1], p_vel_quat[2], p_vel_quat[3]);
    fprintf(stderr, "Quat_dt: %lf\n", p_vel_quat_dt);
}

int vrpn_Poser::register_types(void)
{
    // Register this poser device and the needed message types
    if (d_connection) {
        req_position_m_id =
            d_connection->register_message_type("vrpn_Poser Request Pos_Quat");
        req_position_relative_m_id = d_connection->register_message_type(
            "vrpn_Poser Request Relative Pos_Quat");
        req_velocity_m_id =
            d_connection->register_message_type("vrpn_Poser Request Velocity");
        req_velocity_relative_m_id = d_connection->register_message_type(
            "vrpn_Poser Request Relative Velocity");
    }
    return 0;
}

// virtual
vrpn_Poser::~vrpn_Poser(void) {}

/*
int vrpn_Poser::register_server_handlers(void)
{
    if (d_connection){


    }
    else {
        return -1;
    }
    return 0;
}
*/

// NOTE:    you need to be sure that if you are sending vrpn_float64 then
//          the entire array needs to remain aligned to 8 byte boundaries
//	        (malloced data and static arrays are automatically alloced in
//          this way).  Assumes that there is enough room to store the
//          entire message.  Returns the number of characters sent.
int vrpn_Poser::encode_to(char* buf)
{
    char* bufptr = buf;
    int buflen = 1000;

    // Message includes: vrpn_float64 p_pos[3], vrpn_float64 p_quat[4]
    // Byte order of each needs to be reversed to match network standard

    vrpn_buffer(&bufptr, &buflen, p_pos[0]);
    vrpn_buffer(&bufptr, &buflen, p_pos[1]);
    vrpn_buffer(&bufptr, &buflen, p_pos[2]);

    vrpn_buffer(&bufptr, &buflen, p_quat[0]);
    vrpn_buffer(&bufptr, &buflen, p_quat[1]);
    vrpn_buffer(&bufptr, &buflen, p_quat[2]);
    vrpn_buffer(&bufptr, &buflen, p_quat[3]);

    return 1000 - buflen;
}

int vrpn_Poser::encode_vel_to(char* buf)
{
    char* bufptr = buf;
    int buflen = 1000;

    // Message includes: vrpn_float64 p_vel[3], vrpn_float64 p_vel_quat[4],
    // vrpn_float64 p_vel_quat_dt
    // Byte order of each needs to be reversed to match network standard

    vrpn_buffer(&bufptr, &buflen, p_vel[0]);
    vrpn_buffer(&bufptr, &buflen, p_vel[1]);
    vrpn_buffer(&bufptr, &buflen, p_vel[2]);

    vrpn_buffer(&bufptr, &buflen, p_vel_quat[0]);
    vrpn_buffer(&bufptr, &buflen, p_vel_quat[1]);
    vrpn_buffer(&bufptr, &buflen, p_vel_quat[2]);
    vrpn_buffer(&bufptr, &buflen, p_vel_quat[3]);

    vrpn_buffer(&bufptr, &buflen, p_vel_quat_dt);

    return 1000 - buflen;
}

void vrpn_Poser::set_pose(const timeval t, const vrpn_float64 position[3],
                          const vrpn_float64 quaternion[4])
{
    // Update the time
    p_timestamp.tv_sec = t.tv_sec;
    p_timestamp.tv_usec = t.tv_usec;

    // Update the position and quaternion
    memcpy(p_pos, position, sizeof(p_pos));
    memcpy(p_quat, quaternion, sizeof(p_quat));
}

void vrpn_Poser::set_pose_relative(const timeval t,
                                   const vrpn_float64 position_delta[3],
                                   const vrpn_float64 quaternion[4])
{
    // Update the time
    p_timestamp.tv_sec = t.tv_sec;
    p_timestamp.tv_usec = t.tv_usec;

    // Update the position and quaternion
    p_pos[0] += position_delta[0];
    p_pos[1] += position_delta[1];
    p_pos[2] += position_delta[2];
    q_mult(p_quat, quaternion, p_quat);
}

void vrpn_Poser::set_pose_velocity(const timeval t,
                                   const vrpn_float64 velocity[3],
                                   const vrpn_float64 quaternion[4],
                                   const vrpn_float64 interval)
{
    // Update the time
    p_timestamp.tv_sec = t.tv_sec;
    p_timestamp.tv_usec = t.tv_usec;

    // Update the position and quaternion
    memcpy(p_vel, velocity, sizeof(p_vel));
    memcpy(p_vel_quat, quaternion, sizeof(p_vel_quat));

    // Update the interval
    p_vel_quat_dt = interval;
}

void vrpn_Poser::set_pose_velocity_relative(
    const timeval t, const vrpn_float64 velocity_delta[3],
    const vrpn_float64 quaternion[4], const vrpn_float64 interval_delta)
{
    // Update the time
    p_timestamp.tv_sec = t.tv_sec;
    p_timestamp.tv_usec = t.tv_usec;

    // Update the position and quaternion
    p_vel[0] += velocity_delta[0];
    p_vel[1] += velocity_delta[1];
    p_vel[2] += velocity_delta[2];
    q_mult(p_vel_quat, quaternion, p_vel_quat);

    // Update the interval
    p_vel_quat_dt += interval_delta;
}

//////////////////////////////////////////////////////////////////////////////////////
// Server Code

vrpn_Poser_Server::vrpn_Poser_Server(const char* name, vrpn_Connection* c)
    : vrpn_Poser(name, c)
{
    //	register_server_handlers();

    // Make sure that we have a valid connection
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_Poser_Server: No connection\n");
        return;
    }

    // Register a handler for the position change callback for this device
    if (register_autodeleted_handler(req_position_m_id, handle_change_message,
                                     this, d_sender_id)) {
        fprintf(stderr, "vrpn_Poser_Server: can't register position handler\n");
        d_connection = NULL;
    }

    // Register a handler for the relative position change callback for this
    // device
    if (register_autodeleted_handler(req_position_relative_m_id,
                                     handle_relative_change_message, this,
                                     d_sender_id)) {
        fprintf(
            stderr,
            "vrpn_Poser_Server: can't register relative position handler\n");
        d_connection = NULL;
    }

    // Register a handler for the velocity change callback for this device
    if (register_autodeleted_handler(
            req_velocity_m_id, handle_vel_change_message, this, d_sender_id)) {
        fprintf(stderr, "vrpn_Poser_Server: can't register velocity handler\n");
        d_connection = NULL;
    }

    // Register a handler for the relative velocity change callback for this
    // device
    if (register_autodeleted_handler(req_velocity_relative_m_id,
                                     handle_relative_vel_change_message, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Poser_Server: can't register velocity handler\n");
        d_connection = NULL;
    }
}

void vrpn_Poser_Server::mainloop()
{
    // Call the generic server mainloop routine, since this is a server
    server_mainloop();
}

int vrpn_Poser_Server::handle_change_message(void* userdata,
                                             vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Server* me = (vrpn_Poser_Server*)userdata;
    const char* params = (p.buffer);
    int i;

    vrpn_POSERCB cp;
    // Fill in the parameters to the poser from the message
    if (p.payload_len != (7 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Poser_Server: change message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(7 * sizeof(vrpn_float64)));
        return -1;
    }
    me->p_timestamp = p.msg_time;

    for (i = 0; i < 3; i++) {
        vrpn_unbuffer(&params, &me->p_pos[i]);
    }
    for (i = 0; i < 4; i++) {
        vrpn_unbuffer(&params, &me->p_quat[i]);
    }

    // Check the pose against the max and min values of the workspace
    for (i = 0; i < 3; i++) {
        if (me->p_pos[i] < me->p_pos_min[i]) {
            me->p_pos[i] = me->p_pos_min[i];
        }
        else if (me->p_pos[i] > me->p_pos_max[i]) {
            me->p_pos[i] = me->p_pos_max[i];
        }
    }

    /// Now pack the information in a way that user-routine will understand
    cp.msg_time = me->p_timestamp;
    memcpy(cp.pos, me->p_pos, sizeof(cp.pos));
    memcpy(cp.quat, me->p_quat, sizeof(cp.quat));
    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_callback_list.call_handlers(cp);

    return 0;
}

int vrpn_Poser_Server::handle_relative_change_message(void* userdata,
                                                      vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Server* me = (vrpn_Poser_Server*)userdata;
    const char* params = (p.buffer);
    int i;

    // Fill in the parameters to the poser from the message
    if (p.payload_len != (7 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Poser_Server: change message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(7 * sizeof(vrpn_float64)));
        return -1;
    }
    me->p_timestamp = p.msg_time;

    vrpn_float64 dp[3], dq[4];
    for (i = 0; i < 3; i++) {
        vrpn_unbuffer(&params, &(dp[i]));
    }
    for (i = 0; i < 4; i++) {
        vrpn_unbuffer(&params, &(dq[i]));
    }

    // apply the requested changes
    for (i = 0; i <= 2; i++)
        me->p_pos[i] += dp[i];
    q_mult(me->p_quat, dq, me->p_quat);

    // Check the pose against the max and min values of the workspace
    for (i = 0; i < 3; i++) {
        if (me->p_pos[i] < me->p_pos_min[i]) {
            me->p_pos[i] = me->p_pos_min[i];
        }
        else if (me->p_pos[i] > me->p_pos_max[i]) {
            me->p_pos[i] = me->p_pos_max[i];
        }
    }

    /// Now pack the information in a way that user-routine will understand
    vrpn_POSERCB cp;
    cp.msg_time = me->p_timestamp;
    memcpy(cp.pos, dp, sizeof(cp.pos));
    memcpy(cp.quat, dq, sizeof(cp.quat));
    // Go down the list of callbacks that have been registered.
    me->d_relative_callback_list.call_handlers(cp);

    return 0;
}

int vrpn_Poser_Server::handle_vel_change_message(void* userdata,
                                                 vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Server* me = (vrpn_Poser_Server*)userdata;
    const char* params = (p.buffer);
    int i;

    // Fill in the parameters to the poser from the message
    if (p.payload_len != (8 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Poser_Server: velocity message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(8 * sizeof(vrpn_float64)));
        return -1;
    }
    me->p_timestamp = p.msg_time;

    for (i = 0; i < 3; i++) {
        vrpn_unbuffer(&params, &me->p_vel[i]);
    }
    for (i = 0; i < 4; i++) {
        vrpn_unbuffer(&params, &me->p_vel_quat[i]);
    }
    vrpn_unbuffer(&params, &me->p_vel_quat_dt);

    // Check the velocity against the max and min values of the workspace
    for (i = 0; i < 3; i++) {
        if (me->p_vel[i] < me->p_vel_min[i]) {
            me->p_vel[i] = me->p_vel_min[i];
        }
        else if (me->p_vel[i] > me->p_vel_max[i]) {
            me->p_vel[i] = me->p_vel_max[i];
        }
    }
    return 0;
}

int vrpn_Poser_Server::handle_relative_vel_change_message(void* userdata,
                                                          vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Server* me = (vrpn_Poser_Server*)userdata;
    const char* params = (p.buffer);
    int i;

    // Fill in the parameters to the poser from the message
    if (p.payload_len != (8 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Poser_Server: velocity message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(8 * sizeof(vrpn_float64)));
        return -1;
    }
    me->p_timestamp = p.msg_time;

    vrpn_float64 dv[3], dq[4], di;
    for (i = 0; i < 3; i++) {
        vrpn_unbuffer(&params, &(dv[i]));
    }
    for (i = 0; i < 4; i++) {
        vrpn_unbuffer(&params, &(dq[i]));
    }
    vrpn_unbuffer(&params, &di);

    // apply the requested changes
    for (i = 0; i < 2; i++)
        me->p_vel[i] += dv[i];
    q_mult(me->p_quat, dq, me->p_quat);
    me->p_vel_quat_dt += di;

    // Check the velocity against the max and min values of the workspace
    for (i = 0; i < 3; i++) {
        if (me->p_vel[i] < me->p_vel_min[i]) {
            me->p_vel[i] = me->p_vel_min[i];
        }
        else if (me->p_vel[i] > me->p_vel_max[i]) {
            me->p_vel[i] = me->p_vel_max[i];
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
// Client Code
// Note that the Remote class uses p_pos and p_quat for the absolute
// position/orientation as well as position/orientation deltas when
// requesting relative changes.  Externally, these are write-only variables,
// so no user code will need to change.  If these are ever made readable
// from the Remote class, additional data members will need to be added
// to hold the delta values, as well as addition methods for encoding.

vrpn_Poser_Remote::vrpn_Poser_Remote(const char* name, vrpn_Connection* c)
    : vrpn_Poser(name, c)
{
    // Make sure that we have a valid connection
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_Poser_Remote: No connection\n");
        return;
    }
}

// The remote poser has to un-register its handlers when it
// is destroyed to avoid seg faults (this is taken care of by
// using autodeleted handlers above). It should also remove all
// remaining user-registered callbacks to free up memory.

vrpn_Poser_Remote::~vrpn_Poser_Remote()
{
    // Delete all of the callback handlers that other code had registered
    // with this object. This will free up the memory taken by the lists
}

void vrpn_Poser_Remote::mainloop()
{
    if (d_connection) {
        d_connection->mainloop();
    }
    client_mainloop();
}

int vrpn_Poser_Remote::request_pose(const struct timeval t,
                                    const vrpn_float64 position[3],
                                    const vrpn_float64 quaternion[4])
{
    // Set the requested pose
    set_pose(t, position, quaternion);

    // Send position request
    if (client_send_pose() != 0) {
        fprintf(stderr, "vrpn_Poser_Remote: request_pose failed\n");
        return 0;
    }

    return 1;
}

int vrpn_Poser_Remote::request_pose_relative(
    const struct timeval t, const vrpn_float64 position_delta[3],
    const vrpn_float64 quaternion[4])
{
    // Set the requested pose
    set_pose_relative(t, position_delta, quaternion);

    // Send position request
    if (client_send_pose_relative() != 0) {
        fprintf(stderr, "vrpn_Poser_Remote: request_pose_relative failed\n");
        return 0;
    }

    return 1;
}

int vrpn_Poser_Remote::request_pose_velocity(const struct timeval t,
                                             const vrpn_float64 velocity[3],
                                             const vrpn_float64 quaternion[4],
                                             const vrpn_float64 interval)
{
    // Set the requested velocity
    set_pose_velocity(t, velocity, quaternion, interval);

    // Send position request
    if (client_send_pose_velocity() != 0) {
        fprintf(stderr, "vrpn_Poser_Remote: request_pose_velocity failed\n");
        return 0;
    }

    return 1;
}

int vrpn_Poser_Remote::request_pose_velocity_relative(
    const struct timeval t, const vrpn_float64 velocity_delta[3],
    const vrpn_float64 quaternion[4], const vrpn_float64 interval_delta)
{
    // Set the requested velocity
    set_pose_velocity(t, velocity_delta, quaternion, interval_delta);

    // Send position request
    if (client_send_pose_velocity_relative() != 0) {
        fprintf(stderr,
                "vrpn_Poser_Remote: request_pose_velocity_relative failed\n");
        return 0;
    }

    return 1;
}

int vrpn_Poser_Remote::client_send_pose()
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Pack pose
    len = encode_to(msgbuf);
    if (d_connection->pack_message(len, p_timestamp, req_position_m_id,
                                   d_sender_id, msgbuf,
                                   vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Poser_Remote: can't write a message: tossing\n");
        return -1;
    }

    return 0;
}

int vrpn_Poser_Remote::client_send_pose_relative()
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Pack pose delta.
    len = encode_to(msgbuf);
    if (d_connection->pack_message(len, p_timestamp, req_position_relative_m_id,
                                   d_sender_id, msgbuf,
                                   vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Poser_Remote: can't write a message: tossing\n");
        return -1;
    }

    return 0;
}

int vrpn_Poser_Remote::client_send_pose_velocity()
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Pack velocity
    len = encode_vel_to(msgbuf);
    if (d_connection->pack_message(len, p_timestamp, req_velocity_m_id,
                                   d_sender_id, msgbuf,
                                   vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Poser_Remote: can't write a message: tossing\n");
        return -1;
    }

    return 0;
}

int vrpn_Poser_Remote::client_send_pose_velocity_relative()
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Pack velocity delta
    len = encode_vel_to(msgbuf);
    if (d_connection->pack_message(len, p_timestamp, req_velocity_relative_m_id,
                                   d_sender_id, msgbuf,
                                   vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Poser_Remote: can't write a message: tossing\n");
        return -1;
    }

    return 0;
}
