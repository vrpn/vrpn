#ifndef vrpn_POSER_ANALOG_H
#define vrpn_POSER_ANALOG_H

#ifndef _WIN32_WCE
#include <time.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#ifndef _WIN32_WCE
#include <sys/time.h>
#endif
#endif

#include "vrpn_Poser.h"
#include "vrpn_Analog.h"


// This code is for a Poser server that uses a vrpn_Analog to drive a device.  
// It is basically the inverse of a vrpn_Tracker_AnalogFly.
// We are assuming that one Analog device will be used to drive all axes.  This could be
// Changed by storing an Analog device per axis.


// Class for holding data used in transforming pose data to analog values for each axis
class vrpn_PA_axis {
    public:
        vrpn_PA_axis(int c = -1, double offset = 0.0, double s = 1.0) : channel(c), offset(0), scale(s) {}  

        int channel;        // Which channel to use from the Analog device.  Default value of -1 means 
                            // no motion for this axis
        double offset;      // Offset to apply to pose values for this channel to reach 0
        double scale;       // Scale applied to pose values to get the correct analog value
};

// Class for passing in config data.  Usually read from the vrpn.cfg file in the server code.
class vrpn_Poser_AnalogParam {
    public:
        vrpn_Poser_AnalogParam();

		// port to use for the connection
		int port_num;	

		// number of channels to use
		int num_channels;
                                
        // Translation for the three axes
        vrpn_PA_axis x, y, z;

        // Rotation for the three axes
        vrpn_PA_axis rx, ry, rz;

        // Workspace max and min info
        vrpn_float64    pos_min[3], pos_max[3], pos_rot_min[3], pos_rot_max[3],
                        vel_min[3], vel_max[3], vel_rot_min[3], vel_rot_max[3];
};

class vrpn_Poser_Analog : public vrpn_Poser_Server {
    public:
        vrpn_Poser_Analog(const char* name, vrpn_Connection* c, vrpn_Poser_AnalogParam* p);

        virtual ~vrpn_Poser_Analog();

        virtual void mainloop();

    protected:
        // The Analog device
        vrpn_Analog_Server* ana;

		// The connection for the analog device
		vrpn_Connection* ana_c;

        // Axes for translation and rotation
        vrpn_PA_axis x, y, z, rx, ry, rz;

        // Routine to update the analog values from the current pose
        void update_Analog_values();
};

#endif