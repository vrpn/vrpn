#include <unistd.h>
#include <stdio.h>
#include "vrpn_Sound.h"

char	*SERVER = "Sound0@ioph100";

int main()
{
	vrpn_Sound_Remote *remote;
	remote = new vrpn_Sound_Remote(SERVER);
	printf("Opened sound server %s\n",SERVER);

	printf("Playing at level 0.25:\n");
	remote->play_midi_sound(0.25);
	remote->mainloop();
	sleep (10);

	printf("Playing at level 0.5:\n");
	remote->play_midi_sound(0.5);
	remote->mainloop();
	sleep (10);

	printf("Playing at level 0.75:\n");
	remote->play_midi_sound(0.75);
	remote->mainloop();
	sleep (10);

	printf("Playing at level 1.00:\n");
	remote->play_midi_sound(1.00);
	remote->mainloop();
	sleep (10);

	printf("...Done\n");
	remote->play_midi_sound(0.00);
	remote->mainloop();
	return 0;
}
