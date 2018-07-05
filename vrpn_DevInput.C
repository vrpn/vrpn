/* file:	vrpn_DevInput.cpp
 * author:	Mike Weiblen mew@mew.cx 2004-01-14
 * copyright:	(C) 2003,2004 Michael Weiblen
 * license:	Released to the Public Domain.
 * depends:	gpm 1.19.6, VRPN 06_04
 * tested on:	Linux w/ gcc 2.95.4
 * references:  http://mew.cx/ http://vrpn.org/
 *              http://linux.schottelius.org/gpm/
*/

#include "vrpn_DevInput.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#ifdef VRPN_USE_DEV_INPUT
#include <sys/select.h>                 // for select, FD_ISSET, FD_SET, etc
#include <vrpn_Shared.h>                // for vrpn_gettimeofday
#include <unistd.h>                     // for close, read
#include <utility>                      // for pair
#include <fcntl.h>                      // for open, O_RDONLY
#include <linux/input.h>                // for input_event, ABS_MAX, etc
#include <errno.h>                      // for errno, EACCES, ENOENT
#include <string.h>                     // for strcmp, NULL, strerror
#include <sys/ioctl.h>                  // for ioctl
#include <iostream>                     // for operator<<, ostringstream, etc
#include <map>                          // for map, _Rb_tree_iterator, etc
#include <string>                       // for string, operator+, etc
#include <sstream>

#define	REPORT_ERROR(msg) { send_text_message(msg, timestamp, vrpn_TEXT_ERROR); }

static const std::string EMPTY_STRING("");

static std::string getDeviceNodes(const std::string &device_name)
{
  std::map<std::string, std::string> s_devicesNodes;

  bool permission_missing = false;
  unsigned int id = 0;
  while (1) {
    std::ostringstream oss;
    oss << "/dev/input/event" << id;

    int fd = open(oss.str().c_str(), O_RDONLY);
    if(fd >= 0){
      char name[512];
      if((ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0)
          && (s_devicesNodes.find(name) == s_devicesNodes.end())) {
        s_devicesNodes[name] = oss.str();
      }

      close(fd);
    } else {
      if (errno == ENOENT) break;
      if (errno == EACCES) permission_missing = true;
    }
    errno = 0;
    id++;
  }
  if (permission_missing) {
    std::cout << "vrpn_DevInput device scan warning : permission denied for some nodes !" << std::endl;
  }

  std::map<std::string, std::string>::iterator node_name = s_devicesNodes.find(device_name);
  if (node_name != s_devicesNodes.end()) {
    return node_name->second;
  }

  return EMPTY_STRING;
}

///////////////////////////////////////////////////////////////////////////

vrpn_DevInput::vrpn_DevInput( const char* name, vrpn_Connection * cxn, const char *device_name, const char * type, int int_param )
  : vrpn_Analog( name, cxn )
  , vrpn_Button_Filter( name, cxn )
  , d_fileDescriptor(-1)	// None found yet, device broken.
{
  int i;

  if (strcmp(type, "keyboard") == 0) {
    d_type = DEVICE_KEYBOARD;
  } else if (strcmp(type, "absolute") == 0) {
    d_type = DEVICE_MOUSE_ABSOLUTE;
  } else if (strcmp(type, "relative") == 0) {
    d_type = DEVICE_MOUSE_RELATIVE;
  } else {
    REPORT_ERROR("Third parameter must be keyboard, absolute or relative");
    return;
  }

  vrpn_Button_Filter::num_buttons = 0;
  vrpn_Analog::num_channel        = 0;

  switch (d_type) {
  case DEVICE_KEYBOARD:
    if ((int_param < 1) || (int_param >= vrpn_BUTTON_MAX_BUTTONS)) {
      REPORT_ERROR("In case of keyboard, the value must be between 1 and 256");
      return;
    }
    vrpn_Button_Filter::num_buttons = int_param;
    break;
  case DEVICE_MOUSE_ABSOLUTE:
    vrpn_Analog::num_channel = REL_MAX;
    vrpn_Button_Filter::num_buttons = 0x50;
    d_absolute_min = 0;
    d_absolute_range = int_param;
    break;
  case DEVICE_MOUSE_RELATIVE:
    vrpn_Analog::num_channel = ABS_MAX;
    vrpn_Button_Filter::num_buttons = 0x50;
    break;
  };

  // initialize the vrpn_Analog
  for( i = 0; i < vrpn_Analog::num_channel; i++) {
    vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
  }

  // initialize the vrpn_Button_Filter
  for( i = 0; i < vrpn_Button_Filter::num_buttons; i++) {
    vrpn_Button_Filter::buttons[i] = vrpn_Button_Filter::lastbuttons[i] = 0;
  }

  std::string node = getDeviceNodes(device_name);
  if (node.length() == 0) {
	char msg[4096];
	sprintf(msg, "vrpn_DevInput::vrpn_DevInput(): Could not get device %s",
		device_name);
	REPORT_ERROR(msg);
	return;
  }

  d_fileDescriptor = open(node.c_str(), O_RDONLY);
  if(d_fileDescriptor < 0){
	char msg[4096];
	sprintf(msg, "vrpn_DevInput::vrpn_DevInput(): Could not open device %s (%s)",
		device_name, strerror(errno));
	REPORT_ERROR(msg);
	return;
  }
}

///////////////////////////////////////////////////////////////////////////

vrpn_DevInput::~vrpn_DevInput()
{
  if (d_fileDescriptor >= 0) {
    close(d_fileDescriptor);
  }
  d_fileDescriptor = -1;
}

///////////////////////////////////////////////////////////////////////////

void vrpn_DevInput::mainloop()
{
  get_report();

  server_mainloop();

  report_changes();
}

///////////////////////////////////////////////////////////////////////////

int vrpn_DevInput::get_report()
{
  fd_set readset;

  if (d_fileDescriptor < 0) {
    return 0;
  }

  FD_ZERO( &readset );
  FD_SET( d_fileDescriptor, &readset );
  struct timeval timeout = { 0, 0 };
  select( d_fileDescriptor+1, &readset, NULL, NULL, &timeout );

  vrpn_gettimeofday( &timestamp, NULL );

  if( ! FD_ISSET( d_fileDescriptor, &readset ) )
    return 0;

  struct input_event event;
  if (read(d_fileDescriptor, &event, sizeof(event)) < sizeof(event)) {
    return 0;
  }

  switch (event.type) {
  case EV_KEY: {
    int button_number = event.code;
    if ((d_type == DEVICE_MOUSE_RELATIVE) || (d_type == DEVICE_MOUSE_ABSOLUTE)) {
      button_number -= BTN_MOUSE;
    }
    if ((button_number >= 0) && (button_number < vrpn_Button_Filter::num_buttons)) {
      buttons[button_number] = event.value;
    }
  } break;
  case EV_REL: {
    int channel_number = event.code;
    if ((channel_number >= 0) && (channel_number < vrpn_Analog::num_channel)) {
      for (unsigned int i = 0 ; i < vrpn_Analog::num_channel ; i++) {
	vrpn_Analog::last[i] = 0;
      }
      vrpn_Analog::channel[channel_number] = (vrpn_float64)event.value;
    }
  } break;
  case EV_ABS:
    int channel_number = event.code;
    if ((channel_number >= 0) && (channel_number < vrpn_Analog::num_channel)) {
      vrpn_float64 value = ((vrpn_float64)event.value - d_absolute_min) / d_absolute_range;
      vrpn_Analog::channel[channel_number] = value;
    }
    break;
  };

  return 1;
}

///////////////////////////////////////////////////////////////////////////

void vrpn_DevInput::report_changes( vrpn_uint32 class_of_service )
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Button_Filter::timestamp = timestamp;

  vrpn_Analog::report_changes( class_of_service );
  vrpn_Button_Filter::report_changes();
}

///////////////////////////////////////////////////////////////////////////

void vrpn_DevInput::report( vrpn_uint32 class_of_service )
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Button_Filter::timestamp = timestamp;

  vrpn_Analog::report( class_of_service );
  vrpn_Button_Filter::report_changes();
}

#endif

/*EOF*/
