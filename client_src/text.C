#include <signal.h>                     // for signal, SIGINT
#include <stdio.h>                      // for printf, NULL
#include <stdlib.h>                     // for exit
#include <vrpn_Connection.h>            // for vrpn_Connection
#include <vrpn_Text.h>                  // for vrpn_Text_Receiver, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK

vrpn_Connection * c;

void handle_cntl_c (int) {

  const char * n;
  long i;

  if (c)
    for (i = 0L; (n = c->sender_name(i)); i++)
      printf("Knew sender \"%s\".\n", n);

  // print out type names

  if (c)
    for (i = 0L; (n = c->message_type_name(i)); i++)
      printf("Knew type \"%s\".\n", n);

  exit(0);
}

void  VRPN_CALLBACK my_handler(void *, const vrpn_TEXTCB info){
	printf("%d %d %s\n", info.type, info.level, info.message);
}

int main(int, char* argv[])
{
	vrpn_Text_Receiver * r = new vrpn_Text_Receiver (argv[1]);

	r->register_message_handler(NULL, my_handler);

	// DEBUGGING - TCH 8 Sept 98
	c = r->connectionPtr();
	signal(SIGINT, handle_cntl_c);

	while (1)
		r->mainloop();
}
	
