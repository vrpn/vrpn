#include <unistd.h>
#include <stdio.h>
#include "vrpn_Sound.h"

char	*SERVER = "Sound0@ioglab";
char	*SOUND1 = "/net/nano/nano3/sounds/sfx-4/4-3.au";
char	*SOUND2 = "/net/nano/nano3/sounds/sfx-3/3-2.au";

int main()
{
	vrpn_Sound_Remote *remote;
	remote = new vrpn_Sound_Remote(SERVER);
	printf("Opened sound server %s\n",SERVER);

	printf("Preloading sound %s\n",SOUND1);
	remote->preload_sampled_sound(SOUND1);
	remote->mainloop();
	sleep(5);

	printf("Playing sound %s\n",SOUND1);
	remote->play_sampled_sound(SOUND1, 100,
	                    vrpn_SND_SINGLE, vrpn_SND_BOTH, 1);
	remote->mainloop();
	sleep(3);

	printf("Playing looped sound %s\n",SOUND2);
	remote->play_sampled_sound(SOUND2, 60,
	                    vrpn_SND_LOOPED, vrpn_SND_BOTH, 2);
	remote->mainloop();
	sleep(3);

	printf("Playing looped sound %s\n",SOUND1);
	remote->play_sampled_sound(SOUND1, 100,
	                    vrpn_SND_LOOPED, vrpn_SND_BOTH, 3);
	remote->mainloop();
	sleep(20); 

	printf("...stopping looped sound on channel 2...\n");
        remote->play_stop(2);
	printf("...stopping looped sound on channel 3...\n");
        remote->play_stop(3);
	remote->mainloop();

	sleep (3);
	remote->mainloop();

	printf("...Done\n");
	return 0;
}
