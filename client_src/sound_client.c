#include "vrpn_Sound.h"
main()
{
	vrpn_Sound_Remote *remote;
	remote = new vrpn_Sound_Remote("sound@ionanowb");
	remote->play_sampled_sound("/usr/lib/games/xboing/sounds/ddloo.au", 60,
	                    vrpn_SND_LOOPED, vrpn_SND_BOTH);
	while(1)
	remote->mainloop();
}
