// test_freespace.C
//	This is a VRPN test program that has both clients and servers
// running within the same thread. It is intended to test whether a
// Hillcrest Labs Freespace powered device is working properly.
// Most of this code was copied from the test_radamec_spi.C file
//

#ifdef VRPN_USE_FREESPACE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Dial.h"
#include "vrpn_Button.h"
#include "vrpn_Freespace.h"

char	*TRACKER_NAME = "Freespace0";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

// The connection that is used by all of the servers and remotes
vrpn_Connection		*connection;

// The tracker remote
vrpn_Tracker_Remote	*rtkr;
vrpn_Dial_Remote	*rdial;
vrpn_Button_Remote	*rbutton;
// The freespace device
vrpn_Freespace		*freespace;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos (void *, const vrpn_TRACKERCB t)
{
	printf(" + pos, sensor: %+08.08lf %+08.08lf %+08.08lf %+08.08lf\n", t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
}

void	VRPN_CALLBACK handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	printf(" + vel, sensor: %+08.08lf %+08.08lf %+08.08lf\n", t.vel[0], t.vel[1], t.vel[2]);
}

void	VRPN_CALLBACK handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	printf(" + acc, sensor: %+08.08lf %+08.08lf %+08.08lf\n", t.acc[0], t.acc[1], t.acc[2]);
}
void	VRPN_CALLBACK handle_dial (void *, const vrpn_DIALCB d)
{
	printf(" + dial %d %lf: \n", d.dial, d.change);
}
void	VRPN_CALLBACK handle_buttons (void *, const vrpn_BUTTONCB b)
{
	printf(" + button %d %d: \n", b.button, b.state);
}

/*****************************************************************************
 *
   Routines to create remotes and link their callback handlers.
 *
 *****************************************************************************/

void	create_and_link_tracker_remote(void)
{
	// Open the tracker remote using this connection
	rtkr = new vrpn_Tracker_Remote (TRACKER_NAME, connection);

	// Set up the tracker callback handlers
	rtkr->register_change_handler(NULL, handle_pos);
	rtkr->register_change_handler(NULL, handle_vel);
	rtkr->register_change_handler(NULL, handle_acc);
}
void create_and_link_button_remote(void)
{
	// Open the tracker remote using this connection
	rbutton = new vrpn_Button_Remote (TRACKER_NAME, connection);
	// Set up the tracker callback handlers
	rbutton->register_change_handler(NULL, handle_buttons);
}
void create_and_link_dial_remote(void)
{
	// Open the tracker remote using this connection
	rdial = new vrpn_Dial_Remote (TRACKER_NAME, connection);

	// Set up the tracker callback handlers
	rdial->register_change_handler(NULL, handle_dial);
}


int main (int argc, char * argv [])
{
bool sendBody = 0, sendUser = 1;
// painfully simple CL options to turn on/off body/user frame reports
  if (argc > 1) {
     sendBody = atoi(argv[1]);
     if (argc > 2) { 
       sendUser = atoi(argv[2]);
    }
  }
  //---------------------------------------------------------------------
  // explicitly open the connection
  connection = vrpn_create_server_connection(CONNECTION_PORT);

  //---------------------------------------------------------------------
  // Open the tracker server, using this connection, 2 sensors, update 1 times/sec
  printf("Tracker's name is %s.\n", TRACKER_NAME);

  // create a freespace tracker for the first device.
  freespace = vrpn_Freespace::create(TRACKER_NAME, connection, 0, sendBody, sendUser);
  if (!freespace) {
  	fprintf(stderr, "Error opening freespace device\n");
  	return 1;
  }
  // the freespace device exposes 3 vrpn interfaces.  Tracker, buttons, 
  // and a Dial (scrollwheel)
  // libfreespace also reports values like a mouse (dx/dy), though the 
  // vrpn messages are not currently sent.  Considering the client side doesn't
  // have a notion of a mouse, but treats it as an analog device, I didn't feel
  // compelled to generate the messages, though the additional code is fairly 
  // straight forward

  create_and_link_tracker_remote();
  create_and_link_button_remote();
  create_and_link_dial_remote();

  /* 



 
   * main interactive loop
   */
  while ( 1 ) {

	// Let the servers, clients and connection do their things
  	freespace->mainloop();
  	rtkr->mainloop();
  	connection->mainloop();

  	// Sleep for 1ms each iteration so we don't eat the CPU
  	vrpn_SleepMsecs(1);
  }
  return 0;
}   /* main */

#else
#include <stdio.h>
int main (void)
{
  printf("vrpn_FreeSpace not compiled in, set this in Cmake config or vrpn_Configuration.h and recompile\n");
  return -1;
}
#endif

