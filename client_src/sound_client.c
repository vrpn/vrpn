#include <unistd.h>
#include "vrpn_Sound.h"
main()
{
	vrpn_Sound_Remote *remote;
	remote = new vrpn_Sound_Remote("sound@carbon");

	remote->preload_sampled_sound("/net/nano/nano3/sounds/sfx-4/4-3.au");
	remote->mainloop();
	sleep(5);
	remote->play_sampled_sound("/net/nano/nano3/sounds/sfx-4/4-3.au", 100,
	                    vrpn_SND_SINGLE, vrpn_SND_BOTH, 1);
	remote->mainloop();
	remote->play_sampled_sound("/net/nano/nano3/sounds/sfx-3/3-2.au", 60,
	                    vrpn_SND_LOOPED, vrpn_SND_BOTH, 2);
	remote->mainloop();
	sleep(3);
	remote->play_sampled_sound("/net/nano/nano3/sounds/sfx-4/4-3.au", 100,
	                    vrpn_SND_LOOPED, vrpn_SND_BOTH, 3);
	remote->mainloop();
	sleep(20);

        remote->play_stop(2);
	while(1)
	remote->mainloop();
}
