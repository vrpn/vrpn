#include <stdlib.h>
#include <stdio.h>
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"

vrpn_Button_Remote *btn;
vrpn_Tracker_Remote *tkr;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	handle_pos(void *userdata, const vrpn_TRACKERCB t)
{
	static	int	count = 0;

	fprintf(stderr, ".");
	if (++count >= 20) {
		fprintf(stderr, "\n");
		count = 0;
	}
}

void	handle_vel(void *userdata, const vrpn_TRACKERVELCB t)
{
	static	int	count = 0;

	fprintf(stderr, "/");
}

void	handle_acc(void *userdata, const vrpn_TRACKERACCCB t)
{
	static	int	count = 0;

	fprintf(stderr, "~");
}

void	handle_button(void *userdata, const vrpn_BUTTONCB b)
{
	printf("B%ld is %ld\n", b.button, b.state);
}

/*****************************************************************************
 *
   init - initialize everything
 *
 *****************************************************************************/

void init(void)
{
	//tkr = new vrpn_Tracker_Remote("Tracker0_hiball1");
	tkr = new vrpn_Tracker_Remote("Tracker0@iron");
	btn = new vrpn_Button_Remote("Button0@iron");

	// Set up the tracker callback handler
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	// Set up the button callback handler
	printf("Button update: B<number> is <newstate>\n");
	btn->register_change_handler(NULL, handle_button);

}	/* init */


int main(int argc, char *argv[])
{	int	done = 0;

#ifdef	_WIN32
  WSADATA wsaData; 
  int status;
  if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
    fprintf(stderr, "WSAStartup failed with %d\n", status);
    exit(1);
  }
#endif

	/* initialize the PC/station */
	init();

/* 
 * main interactive loop
 */
while ( ! done )
    {
	// Let the tracker and button do their things
	btn->mainloop();
	tkr->mainloop();

	// XXX Sleep a tiny bit to free up the other process
//	usleep(1000);
    }

}   /* main */


