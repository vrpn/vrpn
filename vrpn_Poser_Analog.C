#include "vrpn_Poser_Analog.h"


vrpn_Poser_AnalogParam::vrpn_Poser_AnalogParam() {
    // Name of the Analog device
    ana_name = NULL;

    // set workspace values to defaults
    for (int i = 0; i < 3; i ++) {
        pos_min[i] = vel_min[i] = -1;
        pos_max[i] = vel_max[i] = 1;
        pos_rot_min[i] = vel_rot_min[i] = -45;
        pos_rot_max[i] = vel_rot_max[i] = 45;
    }
}

vrpn_Poser_Analog::vrpn_Poser_Analog(const char* name, vrpn_Connection* c, vrpn_Poser_AnalogParam* p) :
    vrpn_Poser_Server(name, c) 
{
    int i;

    // Set up the axes
    x = p->x;
    y = p->y;
    z = p->z;
    rx = p->rx;
    ry = p->ry;
    rz = p->rz;

    // Create the Analog Remote 
    // If the name starts with the '*' character, use the server
    // connection rather than making a new one.
    if (p->ana_name != NULL) {
        if (p->ana_name[0] == '*') {
            ana = new vrpn_Analog_Output_Remote(&(p->ana_name[1]), d_connection);
        }
        else {
            ana = new vrpn_Analog_Output_Remote(p->ana_name);
        }

        if (ana == NULL) {
            fprintf(stderr, "vrpn_Poser_Analog: Can't open Analog %s\n", p->ana_name);
        }
    }
	else {
		ana = NULL;
		fprintf(stderr, "vrpn_Poser_Analog: Can't open Analog: No name given\n");
	}

    // Set up the workspace max and min values
    for (i = 0; i < 3; i++) {
        pos_min[i] = p->pos_min[i];
        pos_max[i] = p->pos_max[i];
        vel_min[i] = p->vel_min[i];
        vel_max[i] = p->vel_max[i];
        pos_rot_min[i] = p->pos_rot_min[i];
        pos_rot_max[i] = p->pos_rot_max[i];
        vel_rot_min[i] = p->vel_rot_min[i];
        vel_rot_max[i] = p->vel_rot_max[i];
    }
}

vrpn_Poser_Analog::~vrpn_Poser_Analog() {}

void vrpn_Poser_Analog::mainloop() {
    // Call generic server mainloop, since we are a server
    server_mainloop();

    // Call the Analog's mainloop
    if (ana) {
		ana->mainloop();
	
		if (ana->connectionPtr()->connected()) {
			// Update the Analog's values
			if (!update_Analog_values()) {
				fprintf(stderr, "vrpn_Poser_Analog: Error updating Analog values\n");
			}
		}
	}
}

bool vrpn_Poser_Analog::update_Analog_values() {
    vrpn_float64 values[vrpn_CHANNEL_MAX];

	int i;
	int max_channel = -1;	// sets the range of values we are changing

	for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
		values[i] = 0.0;
	}

    // ONLY DOING TRANS FOR NOW...ADD ROT LATER
    if (x.channel != -1 && x.channel < vrpn_CHANNEL_MAX) {
		if (max_channel <= x.channel) max_channel = x.channel + 1;
		values[x.channel] = (pos[0] - x.offset) * x.scale;
    }
    if (y.channel != -1 && y.channel < vrpn_CHANNEL_MAX) {
		if (max_channel <= y.channel) max_channel = y.channel + 1;
        values[y.channel] = (pos[1] - y.offset) * y.scale;
    }
    if (z.channel != -1 && z.channel < vrpn_CHANNEL_MAX) {
		if (max_channel <= z.channel) max_channel = z.channel + 1;
        values[z.channel] = (pos[2] - z.offset) * z.scale;
    }

    return ana->request_change_channels(max_channel, values);
}