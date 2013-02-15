#include <stdio.h>                      // for fprintf, stderr

#include "vrpn_Keyboard.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday

class VRPN_API vrpn_Connection;
#ifdef	_WIN32
#pragma comment (lib, "user32.lib")
#endif

vrpn_Keyboard::vrpn_Keyboard (const char * name, vrpn_Connection * c) :
    vrpn_Button_Filter(name, c)
{
    int i;
    // Set the parameters in the parent classes
    num_buttons = 256;
	   
    for( i = 0; i < num_buttons; i++) {
      buttons[i] = lastbuttons[i] = 0;
    }
	
#ifndef _WIN32
    fprintf(stderr,"vrpn_Keyboard:: Not implement on this architecture\n");
#endif
}

vrpn_Keyboard::~vrpn_Keyboard()
{
}
   
int vrpn_Keyboard::get_report(void)
{
    struct timeval time;
    vrpn_gettimeofday(&time, NULL); // set timestamp of this event
    timestamp = time;
#ifdef	_WIN32
    int i;
    // Read one key state, which will read all of the events
    // and make it possible to read the state of all the keys;
    // We're ignoring the return for this particular key; it
    // will be read again as part of the 256-key read below.
    GetKeyState(1);

    // Read all 256 keys from the keyboard, then translate the
    // "virtual key" value from each into a scanline code and fill
    // in the appropriate entry with each value.
    BYTE  virtual_keys[256];
    if (GetKeyboardState(virtual_keys) == 0) {
      fprintf(stderr,"vrpn_Keyboard::get_report(): Could not read keyboard state\n");
      return 0;
    }

    // Clear all 256 key values, then fill in the ones that are
    // nonzero.  This is done because some of the keys from the
    // keyboard (control and shift) map into the same scan code;
    // if we just set all of the codes, then the right ones
    // overwrite the left ones.
    for (i = 0; i < 256; i++) {
      buttons[i] = 0;
    }
    for (i = 0; i < 256; i++) {
      unsigned scancode = MapVirtualKey(i, 0);
      if ( (scancode != 0) && ((0x80 & virtual_keys[i]) != 0) ) {
        buttons[scancode] = 1;
      }
    }
#endif
    report_changes();        // Report updates to VRPN
    return 0;
}

// This routine is called each time through the server's main loop. It will
// get a reading and also handle the server_mainloop requirement.
void vrpn_Keyboard::mainloop()
{
    server_mainloop();
    get_report();
}
