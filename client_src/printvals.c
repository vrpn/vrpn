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

void	handle_tracker(void *userdata, vrpn_TRACKERCB t)
{
	static	int	count = 0;

	fprintf(stderr, ".");
	if (++count >= 60) {
		fprintf(stderr, "\n");
		count = 0;
	}
}

void	handle_button(void *userdata, vrpn_BUTTONCB b)
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
	//btn = new vrpn_Button_Remote("Button0_silver");
	//tkr = new vrpn_Tracker_Remote("Tracker0_hiball1");
	tkr = new vrpn_Tracker_Remote("Tracker0_ioglab");

	// Set up the tracker callback handler
	tkr->register_change_handler(NULL, handle_tracker);

	// Set up the button callback handler
	// btn->register_change_handler(NULL, handle_button);

}	/* init */


int main(int argc, char *argv[])
{	int	done = 0;

	/* initialize the PC/station */
	init();

/* 
 * main interactive loop
 */
while ( ! done )
    {
	// Let the tracker and button do their things
	// btn->mainloop();
	tkr->mainloop();

	// XXX Sleep a tiny bit to free up the other process
//	usleep(1000);
    }

}   /* main */


