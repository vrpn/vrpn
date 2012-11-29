#include <stdio.h>                      // for printf
#include "c_interface.h"

void	tracker_callback(unsigned sensor, const double pos[3], const double quat[4])
{
	printf("Tracker sensor %d at (%lf,%lf,%lf) with orientation (%lf,%lf,%lf,%lf)\n",
		sensor, pos[0], pos[1], pos[2], quat[0], quat[1], quat[2], quat[3]);
}

void	button_callback(unsigned button, vrpn_c_bool value)
{
	const char *state = (value?"pressed":"released");
	printf("Button %d %s\n", button, state);
}

int main()
{
	void *tkr;
	void *btn;
	const char *device_name = "Spaceball0@localhost";
	printf("This is just a simple dummy application to show how you could access VRPN from straight C with a C-style interface.\n");
	printf("It will try to connect to a local tracker and button device %s\n", device_name);
	tkr = vrpn_c_open_tracker(device_name, tracker_callback);
	btn = vrpn_c_open_button(device_name, button_callback);

	while (1) {
		vrpn_c_poll_tracker(tkr);
		vrpn_c_poll_button(btn);
	}
	return 0;
}

