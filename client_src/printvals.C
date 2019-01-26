#include <signal.h>                     // for signal, SIGINT
#include <stdio.h>                      // for fprintf, stderr, NULL, etc
#include <stdlib.h>                     // for atof, exit, atoi
#include <string.h>                     // for strcmp, strncmp
#include <vector>
#include <vrpn_Configure.h>             // for VRPN_CALLBACK
#include <vrpn_Button.h>                // for vrpn_BUTTONCB
#include <vrpn_Shared.h>                // for vrpn_SleepMsecs
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include <vrpn_Connection.h>            // for vrpn_HANDLERPARAM
#include <vrpn_Types.h>                 // for vrpn_float64, vrpn_int32
#include <vrpn_BaseClass.h>             // for vrpn_System_TextPrinter, etc
#include <vrpn_FileConnection.h>
#include <vrpn_FileController.h>
#include <vrpn_RedundantTransmission.h>
//#include <vrpn_DelayedConnection.h>

vrpn_Button_Remote *btn,*btn2;
vrpn_Tracker_Remote *tkr;
vrpn_Connection * c;
vrpn_File_Controller * fc;

vrpn_RedundantRemote * rc;

int   print_for_tracker = 1;	// Print tracker reports?
int beQuiet = 0;
int beRedundant = 0;
int redNum = 0;
double redTime = 0.0;
double delayTime = 0.0;

int done = 0;	    // Signals that the program should exit

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos (void *, const vrpn_TRACKERCB t)
{
	static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%d.", t.sensor);
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("Pos, sensor %d = %5.3f, %5.3f, %5.3f", t.sensor,
				t.pos[0], t.pos[1], t.pos[2]);
			printf("  Quat = %5.2f, %5.2f, %5.2f, %5.2f\n",
				t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
			count = 0;
		}
	}
}

void	VRPN_CALLBACK handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	//static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%d/", t.sensor);
}

void	VRPN_CALLBACK handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	//static	int	count = 0;

	if (!print_for_tracker) { return; };
	fprintf(stderr, "%d~", t.sensor);
}

void	VRPN_CALLBACK handle_button (void *, const vrpn_BUTTONCB b)
{
	printf("B%d is %d\n", b.button, b.state);
}

int VRPN_CALLBACK handle_gotConnection (void *, vrpn_HANDLERPARAM) {

  if (beRedundant) {
    fprintf(stderr, "printvals got connection;  "
            "initializing redundant xmission.\n");
    rc->set(redNum, vrpn_MsecsTimeval(redTime * 1000.0));
  }

  return 0;
}

int VRPN_CALLBACK filter_pos (void * userdata, vrpn_HANDLERPARAM p) {

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
           const char * local_in_logfile, const char * local_out_logfile,
           const char * remote_in_logfile, const char * remote_out_logfile,
           const char * NIC)
{
	std::vector<char> devicename(strlen(station_name)+12);
	//char * hn;

        vrpn_int32 gotConn_type;

	// explicitly open up connections with the proper logging parameters
	// these will be entered in the table and found by the
	// vrpn_get_connection_by_name() inside vrpn_Tracker and vrpn_Button

	sprintf(devicename.data(), "Tracker0@%s", station_name);
	if (!strncmp(station_name, "file:", 5)) {
        fprintf(stderr, "Opening file %s.\n", station_name);
	  c = new vrpn_File_Connection (station_name);  // now unnecessary!
          if (local_in_logfile || local_out_logfile ||
              remote_in_logfile || remote_out_logfile) {
            fprintf(stderr, "Warning:  Reading from file, so not logging.\n");
          }
	} else {
        fprintf(stderr, "Connecting to host %s.\n", station_name);
	  c = vrpn_get_connection_by_name
                    (station_name,
		    local_in_logfile, local_out_logfile,
		    remote_in_logfile, remote_out_logfile,
                    NIC);
          if (delayTime > 0.0) {
            //((vrpn_DelayedConnection *) c)->setDelay
                              //(vrpn_MsecsTimeval(delayTime * 1000.0));
            //((vrpn_DelayedConnection *) c)->delayAllTypes(vrpn_TRUE);
          }
	}

	fc = new vrpn_File_Controller (c);

	if (beRedundant) {
	    rc = new vrpn_RedundantRemote (c);
	}

        fprintf(stderr, "Tracker's name is %s.\n", devicename.data());
	tkr = new vrpn_Tracker_Remote (devicename.data());

	sprintf(devicename.data(), "Button0@%s", station_name);
        fprintf(stderr, "Button's name is %s.\n", devicename.data());
	btn = new vrpn_Button_Remote (devicename.data());
	sprintf(devicename.data(), "Button1@%s", station_name);
        fprintf(stderr, "Button 2's name is %s.\n", devicename.data());
	btn2 = new vrpn_Button_Remote (devicename.data());


	// Set up the tracker callback handler
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	// Set up the button callback handler
	printf("Button update: B<number> is <newstate>\n");
	btn->register_change_handler(NULL, handle_button);
	btn2->register_change_handler(NULL, handle_button);

        gotConn_type = c->register_message_type(vrpn_got_connection);

        c->register_handler(gotConn_type, handle_gotConnection, NULL);

}	/* init */


void shutdown () {

  const char * n;
  long i;

  fprintf(stderr, "\nIn control-c handler.\n");
/* Commented out this test code for the common case
  static int invocations = 0;
  if (!invocations) {
    printf("(First press -- setting replay rate to 2.0 -- 3 more to exit)\n");
    fc->set_replay_rate(2.0f);
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
  if (invocations == 1) {
    printf("(Second press -- Starting replay over -- 2 more to exit)\n");
    fc->reset();
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
  if (invocations == 2) {
    struct timeval t;
    printf("(Third press -- Jumping replay to t+30 seconds -- "
           "1 more to exit)\n");
    t.tv_sec = 30L;
    t.tv_usec = 0L;
    fc->play_to_time(t);
    invocations++;
    signal(SIGINT, handle_cntl_c);
    return;
  }
*/

  // print out sender names

  if (c)
    for (i = 0L; (n = c->sender_name(i)); i++)
      printf("Knew local sender \"%s\".\n", n);

  // print out type names

  if (c)
    for (i = 0L; (n = c->message_type_name(i)); i++)
      printf("Knew local type \"%s\".\n", n);

  if (btn)
    delete btn;
  if (btn2)
    delete btn2;
  if (tkr)
    delete tkr;
  if (c)
    delete c;

  exit(0);
}

// WARNING: On Windows systems, this handler is called in a separate
// thread from the main program (this differs from Unix).  To avoid all
// sorts of chaos as the main program continues to handle packets, we
// set a done flag here and let the main program shut down in its own
// thread by calling shutdown() to do all of the stuff we used to do in
// this handler.

void handle_cntl_c(int) {
    done = 1;
}

void Usage (const char * arg0) {
  fprintf(stderr,
"Usage:  %s [-lli logfile] [-llo logfile] [-rli logfile ] [-rlo logfile]\n"
"           [-NIC ip] [-filterpos] [-quiet]\n"
"           [-red num time] [-delay time] station_name\n"
"  -notracker:  Don't print tracker reports\n" 
"  -lli:  log incoming messages locally in <logfile>\n" 
"  -llo:  log outgoing messages locally in <logfile>\n" 
"  -rli:  log incoming messages remotely in <logfile>\n" 
"  -rlo:  log outgoing messages remotely in <logfile>\n" 
"  -NIC:  use network interface with address <ip>\n"
"  -filterpos:  log only Tracker Position messages\n"
"  -quiet:  ignore VRPN warnings\n"
"  -red <num> <time>:  send every message <num>\n"
"    times <time> seconds apart\n"
"  -delay <time:  delay all messages received by <time>\n"
"  station_name:  VRPN name of data source to contact\n"
"    one of:  <hostname>[:<portnum>]\n"
"             file:<filename>\n",
  arg0);

  exit(0);
}

int main (int argc, char * argv [])
{

#ifdef hpux
  char default_station_name [20];
  strcpy(default_station_name, "ioph100");
#else
  char default_station_name [] = { "ioph100" };
#endif

  const char * station_name = default_station_name;
  const char * local_in_logfile = NULL;
  const char * local_out_logfile = NULL;
  const char * remote_in_logfile = NULL;
  const char * remote_out_logfile = NULL;
  const char * NIC = NULL;
  //long local_logmode = vrpn_LOG_NONE;
  //long remote_logmode = vrpn_LOG_NONE;
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
    Usage(argv[0]);
  }

  // parse args

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-lli")) {
      i++;
      local_in_logfile = argv[i];
    } else if (!strcmp(argv[i], "-llo")) {
      i++;
      local_out_logfile = argv[i];
    } else if (!strcmp(argv[i], "-rli")) {
      i++;
      remote_in_logfile = argv[i];
    } else if (!strcmp(argv[i], "-rlo")) {
      i++;
      remote_out_logfile = argv[i];
    } else if (!strcmp(argv[i], "-notracker")) {
      print_for_tracker = 0;
    } else if (!strcmp(argv[i], "-filterpos")) {
      filter = 1;
    } else if (!strcmp(argv[i], "-NIC")) {
      i++;
      NIC = argv[i];
    } else if (!strcmp(argv[i], "-quiet")) {
      beQuiet = 1;
    } else if (!strcmp(argv[i], "-red")) {
      beRedundant = 1;
      i++;
      redNum = atoi(argv[i]);
      i++;
      redTime = atof(argv[i]);
    } else if (!strcmp(argv[i], "-delay")) {
      i++;
      delayTime = atof(argv[i]);
    } else
      station_name = argv[i];
  }

  // initialize the PC/station
  init(station_name, 
       local_in_logfile, local_out_logfile,
       remote_in_logfile, remote_out_logfile,
       NIC);

  // signal handler so logfiles get closed right
  signal(SIGINT, handle_cntl_c);

  // filter the log if requested
  if (filter && c) {
    c->register_log_filter(filter_pos, c);
  }

  if (beQuiet) {
    vrpn_System_TextPrinter.remove_object(btn);
    vrpn_System_TextPrinter.remove_object(btn2);
    vrpn_System_TextPrinter.remove_object(tkr);
    if (beRedundant) {
	vrpn_System_TextPrinter.remove_object(rc);
    }
  }

  if (beRedundant) {
    rc->set(redNum, vrpn_MsecsTimeval(redTime * 1000.0));
  }

/* 
 * main interactive loop
 */
  while ( ! done ) {
        
	// Run this so control happens!
	c->mainloop();

	// Let the tracker and button do their things
	btn->mainloop();
	btn2->mainloop();
	tkr->mainloop();

        if (beRedundant) {
	    rc->mainloop();
	}

	// Sleep for 1ms so we don't eat the CPU
	vrpn_SleepMsecs(1);
  }

  shutdown();
  return 0;

}   /* main */
