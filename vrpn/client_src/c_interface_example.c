#include <stdio.h>
#include <math.h>
#include "c_interface.h"

void	tracker_callback(unsigned sensor, const double pos[3], const double quat[4])
{
	printf("Tracker sensor %d at (%lf,%lf,%lf) with orientation (%lf,%lf,%lf,%lf)\n",
		sensor, pos[0], pos[1], pos[2], quat[0], quat[1], quat[2], quat[3]);
}

void	button_callback(unsigned button, bool value)
{
	const char *state = (value?"pressed":"released");
	printf("Button %d %s\n", button, state);
}

int main()
{
	const char *device_name = "Spaceball0@localhost";
	void *tkr = vrpn_c_open_tracker(device_name, tracker_callback);
	void *btn = vrpn_c_open_button(device_name, button_callback);

	while (true) {
		vrpn_c_poll_tracker(tkr);
		vrpn_c_poll_button(btn);
	}
}

