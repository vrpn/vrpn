/* file:	vrpn_Mouse.cpp
 * author:	Mike Weiblen mew@mew.cx 2004-01-14
 * copyright:	(C) 2003,2004 Michael Weiblen
 * license:	Released to the Public Domain.
 * depends:	gpm 1.19.6, VRPN 06_04
 * tested on:	Linux w/ gcc 2.95.4
 * references:  http://mew.cx/ http://vrpn.org/
 *              http://linux.schottelius.org/gpm/
*/

#include <stdio.h>                      // for NULL, fprintf, printf, etc
#include <string.h>

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Mouse.h"
#include "vrpn_Serial.h"                // for vrpn_open_commport, etc

#if defined(linux) && defined(VRPN_USE_GPM_MOUSE)
#include <gpm.h>                        // for Gpm_Event, Gpm_Connect, etc
#endif


#if !( defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS) )
#  include <sys/select.h>                 // for select, FD_ISSET, FD_SET, etc
#endif

#ifdef	_WIN32
#include <windows.h>

#pragma comment (lib, "user32.lib")

// Fix sent in by Andrei State to make this compile under Visual Studio 6.0.
// If you need this, you also have to copy multimon.h from the DirectX or
// another Windows SDK into a place where the compiler can find it.
#ifndef SM_XVIRTUALSCREEN
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#endif

#endif

///////////////////////////////////////////////////////////////////////////

vrpn_Mouse::vrpn_Mouse( const char* name, vrpn_Connection * cxn ) :
	vrpn_Analog( name, cxn ),
	vrpn_Button_Filter( name, cxn )
{
    int i;

    // initialize the vrpn_Analog
    vrpn_Analog::num_channel = 2;
    for( i = 0; i < vrpn_Analog::num_channel; i++) {
	vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }

    // initialize the vrpn_Button_Filter
    vrpn_Button_Filter::num_buttons = 3;
    for( i = 0; i < vrpn_Button_Filter::num_buttons; i++) {
	vrpn_Button_Filter::buttons[i] = vrpn_Button_Filter::lastbuttons[i] = 0;
    }

#if defined(linux) && defined(VRPN_USE_GPM_MOUSE)
    // attempt to connect to the GPM server
    gpm_zerobased = 1;
    gpm_visiblepointer = 1;

    Gpm_Connect gc;
    gc.eventMask   = ~0;
    gc.defaultMask = GPM_MOVE | GPM_HARD;
    gc.maxMod      = 0;
    gc.minMod      = 0;

    if( Gpm_Open( &gc, 0 ) < 0 )
    {
        // either GPM server is not running, or we're trying to run
        // on an xterm.
	throw GpmOpenFailure();
    }

    set_alerts( 1 );
#elif defined(_WIN32)
    // Nothing needs to be opened under Windows; we just make direct
    // calls below to find the values.
#else
    fprintf(stderr,"vrpn_Mouse::vrpn_Mouse() Not implement on this architecture\n");
#endif
}

///////////////////////////////////////////////////////////////////////////

vrpn_Mouse::~vrpn_Mouse()
{
#if defined(linux) && defined(VRPN_USE_GPM_MOUSE)
    Gpm_Close();
#endif
}

///////////////////////////////////////////////////////////////////////////

void vrpn_Mouse::mainloop()
{
    get_report();
    server_mainloop();
}

///////////////////////////////////////////////////////////////////////////

int vrpn_Mouse::get_report()
{
#if defined(linux) && defined(VRPN_USE_GPM_MOUSE)
    fd_set readset;

    FD_ZERO( &readset );
    FD_SET( gpm_fd, &readset );
    struct timeval timeout = { 0, 0 };
    select( gpm_fd+1, &readset, NULL, NULL, &timeout );
    if( ! FD_ISSET( gpm_fd, &readset ) )
	return 0;

    Gpm_Event evt;
    if( Gpm_GetEvent( &evt ) <= 0 )
	return 0;

    if( evt.type & GPM_UP )
    {
	if( evt.buttons & GPM_B_LEFT )   buttons[0] = 0;
	if( evt.buttons & GPM_B_MIDDLE ) buttons[1] = 0;
	if( evt.buttons & GPM_B_RIGHT )  buttons[2] = 0;
    }
    else
    {
	buttons[0] = (evt.buttons & GPM_B_LEFT)   ? 1 : 0;
	buttons[1] = (evt.buttons & GPM_B_MIDDLE) ? 1 : 0;
	buttons[2] = (evt.buttons & GPM_B_RIGHT)  ? 1 : 0;
    }

    channel[0] = (vrpn_float64) evt.dx / gpm_mx;
    channel[1] = (vrpn_float64) evt.dy / gpm_my;
    return 1;
#elif defined(_WIN32)
    const unsigned LEFT_MOUSE_BUTTON = 0x01;
    const unsigned RIGHT_MOUSE_BUTTON = 0x02;
    const unsigned MIDDLE_MOUSE_BUTTON = 0x04;

    // Find out if the mouse buttons are pressed.
    if (0x80000 & GetKeyState(LEFT_MOUSE_BUTTON)) {
      vrpn_Button::buttons[0] = 1;
    } else {
      vrpn_Button::buttons[0] = 0;
    }
    if (0x80000 & GetKeyState(MIDDLE_MOUSE_BUTTON)) {
      vrpn_Button::buttons[1] = 1;
    } else {
      vrpn_Button::buttons[1] = 0;
    }
    if (0x80000 & GetKeyState(RIGHT_MOUSE_BUTTON)) {
      vrpn_Button::buttons[2] = 1;
    } else {
      vrpn_Button::buttons[2] = 0;
    }

    // Find the position of the cursor in X,Y with range 0..1 across the screen
    POINT curPos;
    GetCursorPos(&curPos);
    vrpn_Analog::channel[0] = (vrpn_float64)(curPos.x - GetSystemMetrics(SM_XVIRTUALSCREEN)) / GetSystemMetrics(SM_CXVIRTUALSCREEN);
    vrpn_Analog::channel[1] = (vrpn_float64)(curPos.y - GetSystemMetrics(SM_YVIRTUALSCREEN)) / GetSystemMetrics(SM_CYVIRTUALSCREEN);

    vrpn_gettimeofday( &timestamp, NULL );
    report_changes();
    return 1;
#else
    return 0;
#endif
}

///////////////////////////////////////////////////////////////////////////

void vrpn_Mouse::report_changes( vrpn_uint32 class_of_service )
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button_Filter::timestamp = timestamp;

    vrpn_Analog::report_changes( class_of_service );
    vrpn_Button_Filter::report_changes();
}

///////////////////////////////////////////////////////////////////////////

void vrpn_Mouse::report( vrpn_uint32 class_of_service )
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button_Filter::timestamp = timestamp;

    vrpn_Analog::report( class_of_service );
    vrpn_Button_Filter::report_changes();
}

///////////////////////////////////////////////////////////////////////////
#define BUTTON_READY 	  (1)
#define BUTTON_FAIL	  (-1)

// (RDK) serial mouse wired up as button device
vrpn_Button_SerialMouse::vrpn_Button_SerialMouse(const char *name,vrpn_Connection *c,
						 const char *port, int baud, vrpn_MOUSETYPE type)
    : vrpn_Button_Filter(name, c)
{
    status = BUTTON_FAIL;
    printed_error = false;
    // Find out the port name and baud rate;
    if (port == NULL) {
		fprintf(stderr,"vrpn_Button_SerialMouse: NULL port name\n");
		return;
    } else {
		vrpn_strcpy(portname, port);
		portname[sizeof(portname)-1] = '\0';
    }
    num_buttons = 3;
    baudrate = baud;
    
    // Open the serial port we are going to use
    if ( (serial_fd=vrpn_open_commport(portname, baudrate)) == -1) {
		fprintf(stderr,"vrpn_Button_SerialMouse: Cannot open serial port\n");
		return;
    }
    
    for (vrpn_int32 i = 0; i < num_buttons; i++) {
		buttons[i] = lastbuttons[i] = 0;
		buttonstate[i] = vrpn_BUTTON_MOMENTARY;
    }

    mousetype = type;
    lastL = lastR = 0;
    // first time in read(), this will get set to 0
    lastM = (mousetype == THREEBUTTON_EMULATION)?1:0;  

    // Say we are ready and find out what time it is
    status = BUTTON_READY;
    vrpn_gettimeofday(&timestamp, NULL);      
}

void vrpn_Button_SerialMouse::mainloop()
{
	// Call the generic server mainloop, since we are a server
	server_mainloop();
 
    switch (status) {
	case BUTTON_READY:
	    read();
	    report_changes();
	    break;
	case BUTTON_FAIL:
      	{	
	    if (printed_error)	break;
	    printed_error = true;
	    send_text_message("vrpn_Button_SerialMouse failure!", timestamp, vrpn_TEXT_ERROR);
        }
      	break;
    }
}
    
// Fill in the buttons[] array with the current value of each of the
// buttons  For a description of the protocols for a Microsoft 3button
// mouse and a MouseSystems mouse, see http://www.hut.fi/~then/mytexts/mouse.html
void vrpn_Button_SerialMouse::read(void)
{ 
    // Make sure we're ready to read
    if (status != BUTTON_READY) {
	    return;
    }

    unsigned char buffer;

    // process as long as we can get characters
    int num = 1;
    int debounce = 0;
    while (num) 
	{
		num = vrpn_read_available_characters(serial_fd, &buffer, 1);

		if (num <= 0) {
			if (debounce) {
#ifdef VERBOSE		
				fprintf (stderr,"state: %d %d %d  last: %d %d %d\n", 
				buttons[0],buttons[1],buttons[2],
				lastL, lastM, lastR);
#endif
			lastL = buttons[0];
			lastM = buttons[1];
			lastR = buttons[2];
			}
			return;  // nothing there or error, so return
		}

		switch (mousetype) {
		  case THREEBUTTON_EMULATION:
			// a mouse capable of 3 button emulation
			// this mouse encodes its buttons in a byte that is one of
			// 0xc0 0xd0 0xe0 0xf0.

		        // Throw away all bytes that are not one of C0, D0, E0 or F0.
		        if ( (buffer != 0xc0) && (buffer != 0xd0) &&
			  (buffer != 0xe0) && (buffer != 0xf0) ) {
			  continue;
			}

			buttons[0] = (unsigned char)( (buffer & 0x20)?1:0 );
			buttons[2] = (unsigned char)( (buffer & 0x10)?1:0 );
			// middle button check:: we get here without a change in left or right
			// This means that we toggle the middle button by moving the mouse
			// around while not pressing or releasing the other buttons!
			if ((buttons[0] == lastL) && (buttons[2] == lastR) && !debounce) {
				buttons[1] = (unsigned char)( lastM?0:1 );
			}
			debounce = 1;
			break;

		  case MOUSESYSTEMS:

			// mousesystems (real PC 3 button mouse) protocol
			// The pc three button mouse encodes its buttons in a byte 
			// that looks like 1 0 0 0 0 lb mb rb

		        if ((buffer  & 0xf8) != 0x80) {	// Ignore all bytes but first in record
				continue;
			}
			debounce = 1;
			buttons[0] = (unsigned char)( (buffer & 4)?0:1 );
			buttons[1] = (unsigned char)( (buffer & 2)?0:1 );
			buttons[2] = (unsigned char)( (buffer & 1)?0:1 );
			break;
				
		default:
			printf("vrpn_Button_SerialMouse::read(): Unknown mouse type\n");
			break;
		} // switch
    } // while (num) 
}


/*EOF*/
