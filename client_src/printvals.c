#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_FileConnection.h"
#include "vrpn_FileController.h"

#ifndef _WIN32
#include <strings.h>
#endif

vrpn_Button_Remote *btn;
vrpn_Tracker_Remote *tkr;
vrpn_Connection * c;
vrpn_File_Controller * fc;

int   print_for_tracker = 1;	// Print tracker reports?

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	handle_pos (void *, const vrpn_TRACKERCB t)
{
	static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%ld.", t.sensor);
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("Pos, sensor %d = %f, %f, %f\n", t.sensor,
				t.pos[0], t.pos[1], t.pos[2]);
			count = 0;
		}
	}
}

void	handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	//static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%ld/", t.sensor);
}

void	handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	//static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%ld~", t.sensor);
}

void	handle_button (void *, const vrpn_BUTTONCB b)
{
	printf("B%ld is %ld\n", b.button, b.state);
}

int filter_pos (void * userdata, vrpn_HANDLERPARAM p) {

  vrpn_Connection * c = (vrpn_Connection *) userdata;
  int postype = c->register_message_type("Tracker Pos/Quat");

  if (p.type == postype)
    return 0;  // keep position messages

  return 1;  // discard all others
}

/*****************************************************************************
 *
   init - initialize everything
 *
 *****************************************************************************/

void init (const char * station_name, 
           const char * local_logfile, long local_logmode,
           const char * remote_logfile, long remote_logmode)
{
	char devicename [1000];
	//char * hn;
	int port;


	// explicitly open up connections with the proper logging parameters
	// these will be entered in the table and found by the
	// vrpn_get_connection_by_name() inside vrpn_Tracker and vrpn_Button

	sprintf(devicename, "Tracker0@%s", station_name);
	if (!strncmp(station_name, "file:", 5)) {
fprintf(stderr, "Opening file %s.\n", station_name);
	  c = new vrpn_File_Connection (station_name);  // now unnecessary!
          if (local_logfile || local_logmode ||
              remote_logfile || remote_logmode)
            fprintf(stderr, "Warning:  Reading from file, so not logging.\n");
	} else {
fprintf(stderr, "Connecting to host %s.\n", station_name);
	  port = vrpn_get_port_number(station_name);
	  c = new vrpn_Synchronized_Connection
                   (station_name, port,
		    local_logfile, local_logmode,
		    remote_logfile, remote_logmode);
	}

	fc = new vrpn_File_Controller (c);

fprintf(stderr, "Tracker's name is %s.\n", devicename);
	tkr = new vrpn_Tracker_Remote (devicename);


	sprintf(devicename, "Button0@%s", station_name);
fprintf(stderr, "Button's name is %s.\n", devicename);
	btn = new vrpn_Button_Remote (devicename);


	// Set up the tracker callback handler
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	// Set up the button callback handler
	printf("Button update: B<number> is <newstate>\n");
	btn->register_change_handler(NULL, handle_button);

}	/* init */


void handle_cntl_c (int) {

  static int invocations = 0;
  const char * n;
  long i;

  fprintf(stderr, "In control-c handler.\n");

  if (!invocations) {
    printf("(First press -- setting replay rate to 2.0 -- 2 more to exit)\n");
    fc->set_replay_rate(2.0f);
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
  if (invocations == 1) {
    printf("(Second press -- Starting replay over -- 1 more to exit)\n");
    fc->reset();
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }

  // print out sender names

  if (c)
    for (i = 0L; n = c->sender_name(i); i++)
      printf("Knew sender \"%s\".\n", n);

  // print out type names

  if (c)
    for (i = 0L; n = c->message_type_name(i); i++)
      printf("Knew type \"%s\".\n", n);


  if (btn)
    delete btn;
  if (tkr)
    delete tkr;
  if (c)
    delete c;

  exit(0);
}

void main (int argc, char * argv [])
{

#ifdef hpux
  char default_station_name [20];
  strcpy(default_station_name, "ioph100");
#else
  char default_station_name [] = { "ioph100" };
#endif

  const char * station_name = default_station_name;
  const char * local_logfile = NULL;
  const char * remote_logfile = NULL;
  long local_logmode = vrpn_LOG_NONE;
  long remote_logmode = vrpn_LOG_NONE;
  int	done = 0;
  int   filter = 0;
  int i;

#ifdef	_WIN32
  WSADATA wsaData; 
  int status;
  if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
    fprintf(stderr, "WSAStartup failed with %d\n", status);
    exit(1);
  }
#endif

  if (argc < 2) {
    fprintf(stderr, "Usage:  %s [-ll logfile mode] [-rl logfile mode]\n"
                    "           [-filterpos] station_name\n"
                    "  -notracker:  Don't print tracker reports\n" 
                    "  -ll:  log locally in <logfile>\n" 
                    "  -rl:  log remotely in <logfile>\n" 
                    "  <mode> is one of i, o, io\n" 
                    "  -filterpos:  log only Tracker Position messages\n"
                    "  station_name:  VRPN name of data source to contact\n"
                    "    one of:  <hostname>[:<portnum>]\n"
                    "             file:<filename>\n",
            argv[0]);
    exit(0);
  }

  // parse args

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-ll")) {
      i++;
      local_logfile = argv[i];
      i++;
      if (strchr(argv[i], 'i')) local_logmode |= vrpn_LOG_INCOMING;
      if (strchr(argv[i], 'o')) local_logmode |= vrpn_LOG_OUTGOING;
    } else if (!strcmp(argv[i], "-rl")) {
      i++;
      remote_logfile = argv[i];
      i++;
      if (strchr(argv[i], 'i')) remote_logmode |= vrpn_LOG_INCOMING;
      if (strchr(argv[i], 'o')) remote_logmode |= vrpn_LOG_OUTGOING;
    } else if (!strcmp(argv[i], "-notracker")) {
      print_for_tracker = 0;
    } else if (!strcmp(argv[i], "-filterpos")) {
      filter = 1;
    } else
      station_name = argv[i];
  }

  // initialize the PC/station
  init(station_name, 
       local_logfile, local_logmode,
       remote_logfile, remote_logmode);

  // signal handler so logfiles get closed right
  signal(SIGINT, handle_cntl_c);

  // filter the log if requested
  if (filter && c)
    c->register_log_filter(filter_pos, c);

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


