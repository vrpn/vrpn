#include "vrpn_Analog.h"        // for vrpn_CHANNEL_MAX
#include "vrpn_Analog_Output.h" // for vrpn_Analog_Output_Remote
#include "vrpn_Connection.h"    // for vrpn_HANDLERPARAM, etc
#include "vrpn_Poser_Analog.h"
#include "vrpn_Shared.h" // for vrpn_unbuffer, timeval, etc

vrpn_Poser_AnalogParam::vrpn_Poser_AnalogParam()
{
    // set workspace values to defaults
    for (int i = 0; i < 3; i++) {
        pos_min[i] = vel_min[i] = -1;
        pos_max[i] = vel_max[i] = 1;
        pos_rot_min[i] = vel_rot_min[i] = -45;
        pos_rot_max[i] = vel_rot_max[i] = 45;
    }
}

bool vrpn_Poser_Analog::setup_channel(vrpn_PA_fullaxis* full)
{
    // If the name is NULL, we're done.
    if (full->axis.ana_name == NULL) {
        return 0;
    }

    // Create the Analog Output Remote
    // If the name starts with the '*' character, use the server
    // connection rather than making a new one.
    if (full->axis.ana_name != NULL) {
        if (full->axis.ana_name[0] == '*') {
          try {
            full->ana = new vrpn_Analog_Output_Remote(&(full->axis.ana_name[1]),
              d_connection);
          } catch (...) {
            fprintf(stderr, "vrpn_Poser_Analog: Out of memory\n");
            return false;
          }
        } else {
          try {
            full->ana = new vrpn_Analog_Output_Remote(full->axis.ana_name);
          } catch (...) {
            fprintf(stderr, "vrpn_Poser_Analog: Out of memory\n");
            return false;
          }
        }
    }
    else {
        full->ana = NULL;
        fprintf(stderr,
                "vrpn_Poser_Analog: Can't open Analog: No name given\n");
        return false;
    }
    return true;
}

vrpn_Poser_Analog::vrpn_Poser_Analog(const char* name, vrpn_Connection* c,
                                     vrpn_Poser_AnalogParam* p,
                                     bool act_as_tracker)
    : vrpn_Poser(name, c)
    , vrpn_Tracker(name, c)
    , d_act_as_tracker(act_as_tracker)
{
    int i;

    //	register_server_handlers();

    // Make sure that we have a valid connection
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_Poser_Analog: No connection\n");
        return;
    }

    // Register a handler for the position change callback for this device
    if (register_autodeleted_handler(req_position_m_id, handle_change_message,
                                     this, d_sender_id)) {
        fprintf(stderr, "vrpn_Poser_Analog: can't register position handler\n");
        d_connection = NULL;
    }

    // Register a handler for the velocity change callback for this device
    if (register_autodeleted_handler(
            req_velocity_m_id, handle_vel_change_message, this, d_sender_id)) {
        fprintf(stderr, "vrpn_Poser_Analog: can't register velocity handler\n");
        d_connection = NULL;
    }

    // Set up the axes
    x.axis = p->x;
    y.axis = p->y;
    z.axis = p->z;
    rx.axis = p->rx;
    ry.axis = p->ry;
    rz.axis = p->rz;

    x.ana = y.ana = z.ana = NULL;
    rx.ana = ry.ana = rz.ana = NULL;

    x.value = y.value = z.value = 0.0;
    rx.value = ry.value = rz.value = 0.0;

    x.pa = this;
    y.pa = this;
    z.pa = this;
    rx.pa = this;
    ry.pa = this;
    rz.pa = this;

    //--------------------------------------------------------------------
    // Open analog remotes for any channels that have non-NULL names.
    // If the name starts with the "*" character, use tracker
    //      connection rather than getting a new connection for it.
    setup_channel(&x);
    setup_channel(&y);
    setup_channel(&z);
    setup_channel(&rx);
    setup_channel(&ry);
    setup_channel(&rz);

    // Set up the workspace max and min values
    for (i = 0; i < 3; i++) {
        p_pos_min[i] = p->pos_min[i];
        p_pos_max[i] = p->pos_max[i];
        p_vel_min[i] = p->vel_min[i];
        p_vel_max[i] = p->vel_max[i];
        p_pos_rot_min[i] = p->pos_rot_min[i];
        p_pos_rot_max[i] = p->pos_rot_max[i];
        p_vel_rot_min[i] = p->vel_rot_min[i];
        p_vel_rot_max[i] = p->vel_rot_max[i];
    }

    // Check the pose for each channel against the max and min values of the
    // workspace
    // and set it to the location closest to the origin.
    p_pos[0] = p_pos[1] = p_pos[2] = 0.0;
    p_quat[0] = p_quat[1] = p_quat[2] = 0.0;
    p_quat[3] = 1.0;
    for (i = 0; i < 3; i++) {
        if (p_pos[i] < p_pos_min[i]) {
            p_pos[i] = p_pos_min[i];
        }
        else if (p_pos[i] > p_pos_max[i]) {
            p_pos[i] = p_pos_max[i];
        }
    }
}

vrpn_Poser_Analog::~vrpn_Poser_Analog() {}

void vrpn_Poser_Analog::mainloop()
{
    // Call generic server mainloop, since we are a server
    server_mainloop();

    // Call the Analog outputs' mainloops
    if (x.ana != NULL) {
        x.ana->mainloop();
    };
    if (y.ana != NULL) {
        y.ana->mainloop();
    };
    if (z.ana != NULL) {
        z.ana->mainloop();
    };
    if (rx.ana != NULL) {
        rx.ana->mainloop();
    };
    if (ry.ana != NULL) {
        ry.ana->mainloop();
    };
    if (rz.ana != NULL) {
        rz.ana->mainloop();
    };
}

int vrpn_Poser_Analog::handle_change_message(void* userdata,
                                             vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Analog* me = (vrpn_Poser_Analog*)userdata;
    const char* params = (p.buffer);
    int i;
    bool outside_bounds = false;

    // Fill in the parameters to the poser from the message
    if (p.payload_len != (7 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Poser_Server: change message payload error\n");
        fprintf(stderr, "             (got %d, expected %d)\n", p.payload_len,
                static_cast<int>(7 * sizeof(vrpn_float64)));
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
            outside_bounds = true;
        }
        else if (me->p_pos[i] > me->p_pos_max[i]) {
            me->p_pos[i] = me->p_pos_max[i];
            outside_bounds = true;
        }
    }

    // Update the analog values based on the request we just got.
    if (!me->update_Analog_values()) {
        fprintf(stderr, "vrpn_Poser_Analog: Error updating Analog values\n");
    }

    if (me->d_act_as_tracker) {
        // Tell the client where we actually went (clipped position and
        // orientation).
        // using sensor 0 as the one to use to report.
        me->d_sensor = 0;
        me->pos[0] = me->p_pos[0];
        me->pos[1] = me->p_pos[1];
        me->pos[2] = me->p_pos[2];
        me->d_quat[0] = me->p_quat[0];
        me->d_quat[1] = me->p_quat[1];
        me->d_quat[2] = me->p_quat[2];
        me->d_quat[3] = me->p_quat[3];
        vrpn_gettimeofday(&me->vrpn_Tracker::timestamp, NULL);
        char msgbuf[1000];
        vrpn_int32 len;
        len = me->vrpn_Tracker::encode_to(msgbuf);
        if (me->d_connection->pack_message(
                len, me->vrpn_Tracker::timestamp, me->position_m_id,
                me->d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "vrpn_Poser_Analog::handle_change_message(): can't "
                            "write message: tossing\n");
            return -1;
        }
    }

    return 0;
}

int vrpn_Poser_Analog::handle_vel_change_message(void* userdata,
                                                 vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Analog* me = (vrpn_Poser_Analog*)userdata;
    const char* params = (p.buffer);
    int i;
    bool outside_bounds = false;

    // Fill in the parameters to the poser from the message
    if (p.payload_len != (8 * sizeof(vrpn_float64))) {
        fprintf(stderr, "vrpn_Poser_Server: velocity message payload error\n");
        fprintf(stderr, "             (got %d, expected %d)\n", p.payload_len,
                static_cast<int>(8 * sizeof(vrpn_float64)));
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
            outside_bounds = true;
        }
        else if (me->p_vel[i] > me->p_vel_max[i]) {
            me->p_vel[i] = me->p_vel_max[i];
            outside_bounds = true;
        }
    }

    // XXX Update the values now.

    if (me->d_act_as_tracker) {
        // Tell the client where we actually went (clipped position and
        // orientation).
        // using sensor 0 as the one to use to report.
        me->d_sensor = 0;
        me->vel[0] = me->p_vel[0];
        me->vel[1] = me->p_vel[1];
        me->vel[2] = me->p_vel[2];
        me->vel_quat[0] = me->p_vel_quat[0];
        me->vel_quat[1] = me->p_vel_quat[1];
        me->vel_quat[2] = me->p_vel_quat[2];
        me->vel_quat[3] = me->p_vel_quat[3];
        vrpn_gettimeofday(&me->vrpn_Tracker::timestamp, NULL);
        char msgbuf[1000];
        vrpn_int32 len;
        len = me->vrpn_Tracker::encode_vel_to(msgbuf);
        if (me->d_connection->pack_message(
                len, me->vrpn_Tracker::timestamp, me->velocity_m_id,
                me->d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "vrpn_Poser_Analog::handle_vel_change_message(): "
                            "can't write message: tossing\n");
            return -1;
        }
    }

    return 0;
}

bool vrpn_Poser_Analog::update_Analog_values()
{
    vrpn_float64 value;
    bool ret = true;

    // XXX ONLY DOING TRANS FOR NOW...ADD ROT LATER
    if (x.axis.channel != -1 && x.axis.channel < vrpn_CHANNEL_MAX) {
        value = (p_pos[0] - x.axis.offset) * x.axis.scale;
        if (x.ana != NULL) {
            ret &= x.ana->request_change_channel_value(x.axis.channel, value);
        }
    }
    if (y.axis.channel != -1 && y.axis.channel < vrpn_CHANNEL_MAX) {
        value = (p_pos[1] - y.axis.offset) * y.axis.scale;
        if (y.ana != NULL) {
            ret &= y.ana->request_change_channel_value(y.axis.channel, value);
        }
    }
    if (z.axis.channel != -1 && z.axis.channel < vrpn_CHANNEL_MAX) {
        value = (p_pos[2] - z.axis.offset) * z.axis.scale;
        if (z.ana != NULL) {
            ret &= z.ana->request_change_channel_value(z.axis.channel, value);
        }
    }

    return ret;
}
