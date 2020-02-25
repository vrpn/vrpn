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

#ifdef VRPN_USE_JOYLIN

#define NAME_LENGTH 128

#include <fcntl.h>                      // for open, O_RDONLY
#include <stdio.h>                      // for NULL, fprintf, perror, etc
#include <stdlib.h>                     // for exit
#include <sys/select.h>                 // for select, FD_ISSET, FD_SET, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday
#include "vrpn_Types.h"                 // for vrpn_float64

#include <sys/ioctl.h>                  // for ioctl
#include <unistd.h>                     // for read
#include <string.h>

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Connection.h"            // for vrpn_Connection
#include <linux/joystick.h>             // for js_event, JSIOCGAXES, etc

vrpn_Joylin::vrpn_Joylin(const char * name, 
			 vrpn_Connection * c,
			 const char *portname):
  vrpn_Analog(name, c), vrpn_Button_Filter(name, c)
{ 
  namelen = 128;
  num_channel = 2;     // inherited : default for generic me-know-nothing PC joystick
  num_buttons = 2;      // inherited : this value is corrected by the ioctl call below.
  fd = -1;
  version = 0x000800;
  devname = (char *) calloc(namelen, sizeof(char));
  if (devname == NULL) {
    fprintf(stderr,"vrpn_Joylin::vrpn_Joylin(): Out of memory\n");
    return;
  }

  device = strdup(portname);
  init();
}

vrpn_Joylin::~vrpn_Joylin()
{
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
  free(device);
  device = NULL;

  free(devname);
  devname = NULL;
}

/****************************************************************************/
/* Initialize the device
*/
int vrpn_Joylin::init()
{
  if ((fd = open(device, O_RDONLY)) < 0) {  /* FIX LATER */
    fprintf(stderr, "vrpn_Joylin constructor could not open %s", device);
    perror(" joystick device");
    return -1;
  }

  ioctl(fd, JSIOCGVERSION, &version);
  ioctl(fd, JSIOCGAXES, &num_channel);
  ioctl(fd, JSIOCGBUTTONS, &num_buttons);
  ioctl(fd, JSIOCGNAME(namelen), devname);
  
#ifdef DEBUG
  printf("Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
	  devname, num_channel, num_buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);
#endif

  return 0;
}

void vrpn_Joylin::mainloop(void)
{
  struct timeval zerotime;
  fd_set fdset;
  struct js_event js;

  zerotime.tv_sec = 0;
  zerotime.tv_usec = 0;
  
  // Since we are a server, call the generic server mainloop()
  server_mainloop();

  if (fd < 0) { return; }

  bool got_response;
  do {
    got_response = false;
    FD_ZERO(&fdset);                      /* clear fdset              */
    FD_SET(fd, &fdset);                   /* include fd in fdset      */ 
    select(fd+1, &fdset, NULL, NULL, &zerotime);

    if (FD_ISSET(fd, &fdset)){            
      if (read(fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
	  send_text_message("Error reading from joystick", vrpn_Analog::timestamp, vrpn_TEXT_ERROR);
	  if (d_connection) { d_connection->send_pending_reports(); }

	  /* try to reopen the device, e.g. wireless joysticks 
	   * like to disconnect when not in use to save battery */
	  close(fd);
	  vrpn_SleepMsecs(5000);
	  init();
	  return;
      }
      got_response = true;

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
  } while (got_response);
}

#else 

vrpn_Joylin::vrpn_Joylin(const char * name, 
			 vrpn_Connection * c,
			 const char *):
  vrpn_Analog(name, c), vrpn_Button_Filter(name, c)
{ 
	  fprintf(stderr,"vrpn_Joylin::vrpn_Joylin: Can't open Linux joystick on non-Linux machine\n");
}

vrpn_Joylin::~vrpn_Joylin()
{
}

void vrpn_Joylin::mainloop(void)
{
}

#endif

