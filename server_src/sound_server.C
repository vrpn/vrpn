#include "vrpn_Sound.h"
main()
{
	vrpn_Synchronized_Connection c;
	vrpn_Linux_Sound *mysound;
	mysound = new vrpn_Linux_Sound("sound@ioglab", &c);
	while (1) {
	mysound->mainloop();
	c.mainloop();
	}
}
