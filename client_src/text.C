#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
  #include <sys/time.h>
#endif
#include <signal.h>
#include <vrpn_Connection.h>
#include <vrpn_Text.h>

vrpn_Connection * c;

void handle_cntl_c (int) {

  const char * n;
  long i;

  if (c)
    for (i = 0L; n = c->sender_name(i); i++)
      printf("Knew sender \"%s\".\n", n);

  // print out type names

  if (c)
    for (i = 0L; n = c->message_type_name(i); i++)
      printf("Knew type \"%s\".\n", n);

  exit(0);
}

void  VRPN_CALLBACK my_handler(void * userdata, const vrpn_TEXTCB info){
	printf("%d %d %s\n", info.type, info.level, info.message);
}

int main(int argc, char* argv[])
{
	vrpn_Text_Receiver * r = new vrpn_Text_Receiver (argv[1]);

	r->register_message_handler(NULL, my_handler);

	// DEBUGGING - TCH 8 Sept 98
	c = r->connectionPtr();
	signal(SIGINT, handle_cntl_c);

	while (1)
		r->mainloop();
}
	
