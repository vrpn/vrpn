#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <vrpn_Connection.h>
#include "vrpn_Text.h"

#define MAX 1024

int main (int argc, char* argv[])
{
	char msg[MAX];
	vrpn_Connection *sc = new vrpn_Synchronized_Connection();
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
