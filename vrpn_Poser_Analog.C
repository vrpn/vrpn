#include "vrpn_Poser_Analog.h"

vrpn_Poser_AnalogParam::vrpn_Poser_AnalogParam() {
	// default port to use
	port_num = vrpn_DEFAULT_LISTEN_PORT_NO;

	// default number of channels
	num_channels = 8;

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

    // Create the Analog Server 
	ana_c = new vrpn_Synchronized_Connection(p->port_num);

	if (ana_c == NULL) {
		fprintf(stderr, "vrpn_Poser_Analog: Can't open Connection\n");
	}

	ana = new vrpn_Analog_Server("Analog", ana_c);

    if (ana == NULL) {
        fprintf(stderr, "vrpn_Poser_Analog: Can't open Analog\n");
    }

	ana->setNumChannels(p->num_channels);

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

	// Call the Analog's connection mainloop
	if (ana_c) {
		ana_c->mainloop();
	}

    // Call the Analog's mainloop
    if (ana) {
		
		// set the analog values
		update_Analog_values();

		ana->report_changes();
		ana->mainloop();
	}
}

void vrpn_Poser_Analog::update_Analog_values() {
    // ONLY DOING TRANS FOR NOW...ADD ROT LATER
    if (x.channel != -1 && x.channel < ana->numChannels()) {
		ana->channels()[x.channel] = (pos[0] - x.offset) * x.scale;
    }
    if (y.channel != -1 && y.channel < ana->numChannels()) {
        ana->channels()[y.channel] = (pos[1] - y.offset) * y.scale;
    }
    if (z.channel != -1 && z.channel < ana->numChannels()) {
        ana->channels()[z.channel] = (pos[2] - z.offset) * z.scale;
    }
}