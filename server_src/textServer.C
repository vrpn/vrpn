#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <vrpn_Connection.h>
#include "vrpn_Text.h"

#define MAX 1024

int main (int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Must pass a device name as the sole argument\n");
		return 1;
	}
	char msg[MAX];
	vrpn_Connection *sc = vrpn_create_server_connection();
	vrpn_Text_Sender *s = new vrpn_Text_Sender(argv[1], sc);
	
	while (1) {
		while (!sc->connected()) {  // wait until we've got a connection
		  sc->mainloop();
                }
                while (sc->connected()) {
		  printf("Please enter the message:\n");
		  scanf("%s", msg);
	          s->send_message(msg, vrpn_TEXT_NORMAL);
		  s->mainloop();
		  sc->mainloop();
		}
	}
}
