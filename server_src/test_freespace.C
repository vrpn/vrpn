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

char *TRACKER_NAME = "Freespace0";
char trackerName[512];
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
 * Console handlers for CTRL-C
 ****************************************************************************/
int quit = 0;
#ifdef _WIN32
static BOOL CtrlHandler(DWORD fdwCtrlType) {
    quit = 1;
    return TRUE;
}

void addControlHandler() {
    if (!SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE )) {
        printf("Could not install control handler\n");
    }
}

#else

static void sighandler(int num) {
    quit = 1;
}
void addControlHandler() {
    // Set up the signal handler to catch
    // CTRL-C and clean up gracefully.
    struct sigaction setmask;
    sigemptyset(&setmask.sa_mask);
    setmask.sa_handler = sighandler;
    setmask.sa_flags = 0;

    sigaction(SIGHUP, &setmask, NULL);
    sigaction(SIGINT, &setmask, NULL);
}
#endif

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

void	create_and_link_tracker_remote(const char * trackerName)
{
	// Open the tracker remote using this connection
	rtkr = new vrpn_Tracker_Remote (trackerName, connection);

	// Set up the tracker callback handlers
	rtkr->register_change_handler(NULL, handle_pos);
	rtkr->register_change_handler(NULL, handle_vel);
	rtkr->register_change_handler(NULL, handle_acc);
}
void create_and_link_button_remote(const char * trackerName)
{
	// Open the tracker remote using this connection
	rbutton = new vrpn_Button_Remote (trackerName, connection);
	// Set up the tracker callback handlers
	rbutton->register_change_handler(NULL, handle_buttons);
}
void create_and_link_dial_remote(const char * trackerName)
{
	// Open the tracker remote using this connection
	rdial = new vrpn_Dial_Remote (trackerName, connection);

	// Set up the tracker callback handlers
	rdial->register_change_handler(NULL, handle_dial);
}


int main (int argc, char * argv [])
{
  unsigned controller = 0, sendBody = 0, sendUser = 1;
  sprintf(trackerName, "%s", TRACKER_NAME);

  // The only command line argument is a string for the configuration
  if (argc > 1) {
      // Get the arguments (device_name, controller_index, bodyFrame, userFrame)
      if (sscanf(argv[1], "%511s%u%u%u", trackerName, &controller, &sendBody, &sendUser) != 4) {
        fprintf(stderr, "Bad vrpn_Freespace line: %s\n", argv[1]);
        fprintf(stderr, "Expect: deviceName controllerIndex bodyFrame userFrame\n");
        return -1;
      }
  }

  // Correctly handle command line input
  addControlHandler();

  //---------------------------------------------------------------------
  // explicitly open the connection
  connection = vrpn_create_server_connection(CONNECTION_PORT);

  //---------------------------------------------------------------------
  // create a freespace tracker for the first device.
  printf("Tracker's name is %s.\n", trackerName);
  freespace = vrpn_Freespace::create(trackerName, connection, 0, (sendBody != 0), (sendUser != 0));
  if (!freespace) {
      fprintf(stderr, "Error opening freespace device: %s\n", trackerName);
  	  return 1;
  }
  // the freespace device exposes 3 vrpn interfaces.  Tracker, buttons, 
  // and a Dial (scrollwheel)
  // libfreespace also reports values like a mouse (dx/dy), though the 
  // vrpn messages are not currently sent.  Considering the client side doesn't
  // have a notion of a mouse, but treats it as an analog device, I didn't feel
  // compelled to generate the messages, though the additional code is fairly 
  // straight forward

  create_and_link_tracker_remote(trackerName);
  create_and_link_button_remote(trackerName);
  create_and_link_dial_remote(trackerName);

  while ( !quit ) {

	// Let the servers, clients and connection do their things
  	freespace->mainloop();
  	rtkr->mainloop();
  	connection->mainloop();

  	// Sleep for 1ms each iteration so we don't eat the CPU
  	vrpn_SleepMsecs(1);
  }

  delete freespace;
  delete rtkr;
  delete connection;

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

