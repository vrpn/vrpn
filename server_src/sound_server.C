#include "vrpn_Sound.h"
main()
{
	vrpn_Connection c;
	vrpn_Linux_Sound *mysound;
	mysound = new vrpn_Linux_Sound("sound@daytona", &c);
	while (1) {
	mysound->mainloop();
	c.mainloop();
	}
}
