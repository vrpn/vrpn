#include <unistd.h>
#include <stdio.h>
#include "vrpn_Sound.h"

char	*SERVER = "Sound0@ioph100";

int main()
{
	vrpn_Sound_Remote *remote;
	remote = new vrpn_Sound_Remote(SERVER);
	printf("Opened sound server %s\n",SERVER);

	printf("Playing at level 0.24:\n");
	remote->play_midi_sound(0.24);
	remote->mainloop();
	sleep (10);

	printf("Playing at level 0.45:\n");
	remote->play_midi_sound(0.45);
	remote->mainloop();
	sleep (10);

	printf("Playing at level 0.74:\n");
	remote->play_midi_sound(0.74);
	remote->mainloop();
	sleep (10);

	printf("Playing at level 1.00:\n");
	remote->play_midi_sound(1.00);
	remote->mainloop();
	sleep (10);

	printf("...Done\n");
	remote->play_stop(1);
	remote->mainloop();
	return 0;
}
