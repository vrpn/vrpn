/*
# Linux Joystick. Interface to the Linux Joystick driver by Vojtech Pavlik
# included in several Linux distributions. The server code has been tested 
# with Linux Joystick driver version 1.2.14. Yet, there is no way how to
# map a typical joystick's zillion buttons and axes on few buttons and axes
# really used. Unfortunately, even joysticks of the same kind can have 
# different button mappings from one to another.  Driver written by Harald
# Barth (haba@pdc.kth.se).
*/

#include "vrpn_Joylin.h"

#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <string.h>

#define NAME_LENGTH 128

#ifdef linux
vrpn_Joylin::vrpn_Joylin(char * name, 
			 vrpn_Connection * c,
			 char *portname):
  vrpn_Analog(name, c), vrpn_Button(name, c)
{ 
  namelen = 128;
  num_channel = 2;     // inherited : default for generic me-know-nothing PC joystick
  num_buttons = 2;      // inherited : this value is corrected by the ioctl call below.
  fd = -1;
  version = 0x000800;
  name = new char[namelen];

  strncpy(name, "Unknown", namelen); // paranoia for future changes of namelen
  name[namelen-1] = 0;
  
  if ((fd = open(portname, O_RDONLY)) < 0) {  /* FIX LATER */
    fprintf(stderr, "vrpn_Joylin constructor could not open %s", portname);
    perror(" joystick device");
    exit(1);
  }

  ioctl(fd, JSIOCGVERSION, &version);
  ioctl(fd, JSIOCGAXES, &num_channel);
  ioctl(fd, JSIOCGBUTTONS, &num_buttons);
  ioctl(fd, JSIOCGNAME(namelen), name);
  
  fprintf(stderr, "Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
	  name, num_channel, num_buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);
}


void vrpn_Joylin::mainloop(void) {
  struct timeval zerotime;
  fd_set fdset;
  struct js_event js;
  int i;

  zerotime.tv_sec = 0;
  zerotime.tv_usec = 0;
  
  // Since we are a server, call the generic server mainloop()
  server_mainloop();
 

  FD_ZERO(&fdset);                      /* clear fdset              */
  FD_SET(fd, &fdset);                   /* include fd in fdset      */ 
  select(fd+1, &fdset, NULL, NULL, &zerotime);
    if (FD_ISSET(fd, &fdset)){            
      if (read(fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
		  send_text_message("Error reading from joystick", vrpn_Analog::timestamp, vrpn_TEXT_ERROR);
		  if (d_connection) { d_connection->send_pending_reports(); }
		  return;
      }
      switch(js.type & ~JS_EVENT_INIT) {
      case JS_EVENT_BUTTON:
                  vrpn_gettimeofday((timeval *)&this->vrpn_Button::timestamp, NULL);
		  buttons[js.number] = js.value;
		  break;
      case JS_EVENT_AXIS:
		  vrpn_gettimeofday((timeval *)&this->vrpn_Analog::timestamp, NULL);
		  channel[js.number] = js.value / 32767.0;           /* FIX LATER */
		  break;
      }

#ifdef DEBUG
    if (num_channel) {
	printf("Axes: ");
	 for (i = 0; i < num_channel; i++) {
	  printf("%2d:%.3f ", i, channel[i]);
	 }
    }
    if (num_buttons) {
	 printf("Buttons: ");
	 for (i = 0; i < num_buttons; i++) {
	  printf("%2d:%s ", i, buttons[i] ? "on " : "off");
	 }
    }
    printf("\n");
    fflush(stdout);
#endif	
      
      vrpn_Analog::report_changes(); // report any analog event;
      vrpn_Button::report_changes(); // report any button event;
    }
}

#else 

vrpn_Joylin::vrpn_Joylin(char * name, 
			 vrpn_Connection * c,
			 char *):
  vrpn_Analog(name, c), vrpn_Button(name, c)
{ 
	  fprintf(stderr,"vrpn_Joylin::vrpn_Joylin: Can't open Linux joystick on non-Linux machine\n");
}
void vrpn_Joylin::mainloop(void) {
}

#endif

/****************************************************************************/
/* Send request for status to joysticks and reads response.
*/
int vrpn_Joylin::init() {
	return 0;
}

