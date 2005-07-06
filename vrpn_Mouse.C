/* file:	vrpn_Mouse.cpp
 * author:	Mike Weiblen mew@mew.cx 2004-01-14
 * copyright:	(C) 2003,2004 Michael Weiblen
 * license:	Released to the Public Domain.
 * depends:	gpm 1.19.6, VRPN 06_04
 * tested on:	Linux w/ gcc 2.95.4
 * references:  http://mew.cx/ http://vrpn.org/
 *              http://linux.schottelius.org/gpm/
*/

#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

#ifdef	linux
#include <gpm.h>
#endif

#include "vrpn_Mouse.h"

///////////////////////////////////////////////////////////////////////////

vrpn_Mouse::vrpn_Mouse( const char* name, vrpn_Connection* cxn ) :
	vrpn_Analog( name, cxn ),
	vrpn_Button_Filter( name, cxn )
{
#ifdef linux
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

    // initialize the vrpn_Analog
    vrpn_Analog::num_channel = 2;
    for( int i = 0; i < vrpn_Analog::num_channel; i++)
    {
	vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }

    // initialize the vrpn_Button_Filter
    vrpn_Button_Filter::num_buttons = 3;
    for( int i = 0; i < vrpn_Button_Filter::num_buttons; i++)
    {
	vrpn_Button_Filter::buttons[i] = vrpn_Button_Filter::lastbuttons[i] = 0;
    }
    set_alerts( 1 );
#else
    fprintf(stderr,"vrpn_Mouse::vrpn_Mouse() Not implement on this architecture\n");
#endif
}

///////////////////////////////////////////////////////////////////////////

vrpn_Mouse::~vrpn_Mouse()
{
#ifdef linux
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
#ifdef linux
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

    gettimeofday( &timestamp, NULL );
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

/*EOF*/

