#include <signal.h>                     // for signal, SIGINT
#include <stdio.h>                      // for printf, fprintf, stderr, etc
#include <stdlib.h>                     // for exit

#include "vrpn_Analog.h"                // for vrpn_Analog_Remote, etc
#include "vrpn_Button.h"                // for vrpn_Button_Remote, etc
#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Dial.h"                  // for vrpn_DIALCB, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs
#include "vrpn_Types.h"                 // for vrpn_float64

vrpn_Button_Remote *btn;
vrpn_Analog_Remote *ana;
vrpn_Dial_Remote *dial;

int done = 0;

const	int	MAX_DIALS = 128;
vrpn_float64	cur_dial_values[MAX_DIALS];

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_button (void *, const vrpn_BUTTONCB b)
{
	printf("B%d->%d\n", b.button, b.state);
}

void	VRPN_CALLBACK handle_analog (void *, const vrpn_ANALOGCB a)
{
	int i;
	printf("Analogs: ");
	for (i = 0; i < a.num_channel; i++) {
		printf("%4.2f ",a.channel[i]);
	}
	printf("\n");
}


void	VRPN_CALLBACK handle_dial (void *, const vrpn_DIALCB d)
{
	cur_dial_values[d.dial] += d.change;
	printf("Dial %d spun by %lf (currently at %lf)\n", d.dial, d.change,
		cur_dial_values[d.dial]);
}


/*****************************************************************************
 *
   init - initialize everything
 *
 *****************************************************************************/

void init (const char * devicename)
{
	int i;

	fprintf(stderr, "Button's name is %s.\n", devicename);
	btn = new vrpn_Button_Remote (devicename);
	
	fprintf(stderr, "Analog's name is %s.\n", devicename);
	ana = new vrpn_Analog_Remote (devicename);

	fprintf(stderr, "Dial's name is %s.\n", devicename);
	dial = new vrpn_Dial_Remote (devicename);

	// Zero all of the dial records
	for (i = 0; i < MAX_DIALS; i++) {
		cur_dial_values[i] = 0.0;
	}

	// Set up the callback handlers
	printf("Button update: B<number> is <newstate>\n");
	btn->register_change_handler(NULL, handle_button);
	printf("Analog update: Analogs: [new values listed]\n");
	ana->register_change_handler(NULL, handle_analog);
	printf("Dial update: Dial# spun by [amount]\n");
	dial->register_change_handler(NULL, handle_dial);

}	/* init */


void handle_cntl_c (int) {
    done = 1;
}

void shutdown (void) {

  fprintf(stderr, "\nIn control-c handler.\n");

  if (btn) delete btn;
  if (ana) delete ana;
  if (dial) delete dial;

  exit(0);
}

int main (int argc, char * argv [])
{

#ifdef hpux
  char default_station_name [100];
  strcpy(default_station_name, "CerealBox@ioglab");
#else
  char default_station_name [] = { "CerealBox@ioglab" };
#endif

  const char * station_name = default_station_name;

  if (argc < 2) {
    fprintf(stderr, "Usage:  %s  Device_name\n"
                    "  Device_name:  VRPN name of data source to contact\n"
                    "    example:  CerealBox@ioglab\n",
            argv[0]);
    exit(0);
  }

  // parse args

  station_name = argv[1];

  // initialize the PC/station
  init(station_name);

  // signal handler so logfiles get closed right
  signal(SIGINT, handle_cntl_c);

  /* 
   * main interactive loop
   */
  while ( ! done )
    {
	// Let the tracker and button do their things
	btn->mainloop();
	ana->mainloop();
	dial->mainloop();

	// Sleep for 1ms so we don't eat the CPU
	vrpn_SleepMsecs(1);
    }

  shutdown();
  return 0;
}   /* main */
