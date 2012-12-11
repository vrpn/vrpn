/*			clock_drift_estimator.C

	This is a VRPN client program that will connect to a VRPN
	server device use the built-in ping/poing messages from the
        vrpn_BaseClass to estimate both the round-trip time for
        VRPN messages and the clock drift between the client machine
        and the server machine.
*/

#include <math.h>                       // for floor
#include <stdio.h>                      // for NULL, printf, fprintf, etc
#include <stdlib.h>                     // for exit
#include <vrpn_Shared.h>                // for vrpn_gettimeofday, timeval, vrpn_TimevalSum, etc
#ifndef	_WIN32_WCE
#include <signal.h>                     // for signal, SIGINT
#endif
#include <string.h>                     // for strcmp
#include <vrpn_BaseClass.h>             // for vrpn_BaseClass
#include <vrpn_Connection.h>            // for vrpn_Connection, etc
#include <vrpn_Tracker.h>               // for vrpn_Tracker_NULL

#include "vrpn_Configure.h"             // for VRPN_CALLBACK

int done = 0;	    // Signals that the program should exit

//-------------------------------------
// This object is used to connect to whatever server object is
// specified.  The server object can be anything, a tracker, a button,
// an analog, etc.  It just needs to be derived from vrpn_BaseClass
// so that it implements the ping/pong messages.
// XXX Later, make an interface to this so that it can be more generally
// useful as an estimator and move the printing up to the application.

class vrpn_Clock_Drift_Estimator : public vrpn_BaseClass {
public:
  // The constructor adds another entry for the Pong message handler,
  // so that we can use these responses to estimate.
  vrpn_Clock_Drift_Estimator(const char * name, double min_repeat_wait_secs = 0, double estimation_interval_secs = 10, vrpn_Connection * c = NULL) :
    vrpn_BaseClass(name, c),
    d_doing_estimate(false),
    d_doing_ping(false),
    d_count(0)
  {
    vrpn_BaseClass::init();

    if (d_connection != NULL) {
      // Put in an additional handler for the pong message so that we can
      // tell when we get a response.
      register_autodeleted_handler(d_pong_message_id, handle_pong, this, d_sender_id);

      // Initialize the member variables used in estimating the time.
      // We want the estimation interval to start two seconds into the
      // future to give things time to settle in and get connected.
      vrpn_gettimeofday(&d_next_interval_time, NULL);
      d_next_interval_time.tv_sec += 2;
      d_next_ping_time = d_next_interval_time;

      // Initialize the estimation interval and minimum waits used to
      // tell when to do things.
      if ( (min_repeat_wait_secs < 0) || (estimation_interval_secs <= 0) || (min_repeat_wait_secs > estimation_interval_secs) ) {
        fprintf(stderr,"vrpn_Clock_Drift_Estimator::vrpn_Clock_Drift_Estimator(): Invalid time parameters (using 0, 10)\n");
        min_repeat_wait_secs = 0;
        estimation_interval_secs = 10;
      }
      d_min_repeat_wait.tv_sec = static_cast<long>(floor(min_repeat_wait_secs));
      d_min_repeat_wait.tv_usec = static_cast<long>(floor( (min_repeat_wait_secs - d_min_repeat_wait.tv_sec) * 1e6));
      d_estimation_interval.tv_sec = static_cast<long>(floor(estimation_interval_secs));
      d_estimation_interval.tv_usec = static_cast<long>(floor( (min_repeat_wait_secs - d_estimation_interval.tv_sec) * 1e6));

      d_last_ping_time.tv_sec = d_last_ping_time.tv_usec = 0;
    }
  };

  ~vrpn_Clock_Drift_Estimator() { return; };

  /// Mainloop the connection to send the message.
  void mainloop(void) {
    client_mainloop();
    if (d_connection) {
      // Send any pings, collect any pongs.
      d_connection->mainloop();

      // See if it is time for the next interval to start.  Start it if so.
      struct timeval now;
      vrpn_gettimeofday(&now, NULL);
      if (vrpn_TimevalGreater(now, d_next_interval_time) || vrpn_TimevalEqual(now, d_next_interval_time)) {

        // If we were doing an estimate, print the results.
        if (d_doing_estimate) {
          if (d_count == 0) {
            fprintf(stderr,"vrpn_Clock_Drift_Estimator::mainloop(): Zero count in ping response!\n");
          } else {
            printf("vrpn_Clock_Drift_Estimator::mainloop(): Clock statistics for %d responses:\n", d_count);
            printf("    Round-trip time:     mean = %lg, min = %lg, max = %lg\n", d_sum_rtt/d_count, d_min_rtt, d_max_rtt);
            printf("    Remote clock offset: mean = %lg, min = %lg, max = %lg\n", d_sum_skew/d_count, d_min_skew, d_max_skew);
          }
        }

        // Set up for the next estimate interval.
        d_doing_estimate = true;
        d_next_interval_time = vrpn_TimevalSum(now, d_estimation_interval);
        d_next_ping_time = now;
        d_count = 0;
        d_sum_rtt = 0;
        d_sum_skew = 0;
      }

      // See if it is time to send the next ping.  Send it if so.
      if (vrpn_TimevalGreater(now, d_next_ping_time) | vrpn_TimevalEqual(now, d_next_ping_time)) {
        vrpn_gettimeofday(&now, NULL);
        //printf("XXX ping\n");
        d_connection->pack_message(0, now, d_ping_message_id, d_sender_id,
                             NULL, vrpn_CONNECTION_RELIABLE);
        d_last_ping_time = now;

        // Tells the pong callback to listen to the response.
        d_doing_ping = true;

        // Don't do another until we hear a response.
        d_next_ping_time = now;
        d_next_ping_time.tv_sec += 10000;
      }

      // Send any pings, collect any pongs.
      d_connection->mainloop();
    }
  };

protected:
  // The interval over which to test before printing results,
  // and the minimum time to wait between sending pings
  struct timeval d_estimation_interval;
  struct timeval d_min_repeat_wait;

  // When we're going to next try a ping and when the next estimation interval starts
  struct timeval  d_next_ping_time;
  struct timeval  d_next_interval_time;

  // When we last sent a ping.
  struct timeval d_last_ping_time;

  // Have we started doing an estimate or a ping?
  bool  d_doing_estimate;
  bool  d_doing_ping;

  // Ping statistics
  double  d_min_rtt;    // Round-trip time
  double  d_max_rtt;
  double  d_sum_rtt;
  double  d_min_skew;   // Skew is from remote time to local time
  double  d_max_skew;
  double  d_sum_skew;
  unsigned  d_count;
 
  /// No types to register beyond the ping/pong, which are done in BaseClass.
  virtual int register_types(void) { return 0; };

  // Report the elapsed time in seconds between the first and second time, where
  // the first time is sooner.  It will return a negative number if the first
  // is later than the second.
  static double elapsed_secs(const struct timeval &t1, const struct timeval &t2)
  {
    return 1e-3 * vrpn_TimevalMsecs( vrpn_TimevalDiff(t2,t1) );
  };

  static int VRPN_CALLBACK handle_pong(void *userdata, vrpn_HANDLERPARAM p)
  {
    //printf("XXX PONG\n");
    vrpn_Clock_Drift_Estimator  *me = static_cast<vrpn_Clock_Drift_Estimator *>(userdata);

    // If we're currently estimating, then update the statistics based on
    // the time of the response and the time we asked for a response.
    if (me->d_doing_ping) {
      struct timeval now;
      vrpn_gettimeofday(&now, NULL);

      // Find the round trip was by subtracting the time the last
      // ping was sent from the current time.
      double rtt = elapsed_secs(me->d_last_ping_time, now);

      // Estimate the clock skew by assuming that the response was generated
      // halfway between the sending time and now.  We want to compute
      // the number to add to the remote clock value to produce a time in
      // the local clock value.
      struct timeval interval = vrpn_TimevalDiff(now, me->d_last_ping_time);
      struct timeval half_interval = vrpn_TimevalScale(interval, 0.5);
      struct timeval expected = vrpn_TimevalSum(me->d_last_ping_time, half_interval);
      double remote_to_local = elapsed_secs(p.msg_time, expected);

      // If this is the first return, set the min and max values to the values
      if (me->d_count == 0) {
        me->d_min_rtt = me->d_max_rtt = rtt;
        me->d_min_skew = me->d_max_skew = remote_to_local;
      }

      // Add these into the ongoing statistics and increment the count of
      // samples.
      me->d_sum_rtt += rtt;
      if (rtt < me->d_min_rtt) { me->d_min_rtt = rtt; }
      if (rtt > me->d_max_rtt) { me->d_max_rtt = rtt; }
      me->d_sum_skew += remote_to_local;
      if (remote_to_local < me->d_min_skew) { me->d_min_skew = remote_to_local; }
      if (remote_to_local > me->d_max_skew) { me->d_max_skew = remote_to_local; }
      me->d_count++;

      // Set the time for the next ping.  Mark us down as not doing an
      // estimate, so we'll ignore misaligned pong responses.
      me->d_next_ping_time = vrpn_TimevalSum(now, me->d_min_repeat_wait);
      me->d_doing_ping = false;
    }

    return 0;
  };
};

static vrpn_Clock_Drift_Estimator *g_clock = NULL;
static vrpn_Connection    *g_connection = NULL;
static vrpn_Tracker_NULL          *g_tracker = NULL;

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
"Usage:  %s server_to_use | LOCAL\n"
"  server_to_use:  VRPN name of device to connect to (eg: Tracker0@ioglab)\n"
"                  'LOCAL' means to launch a local server on a dedicated connection\n",
  arg0);

  exit(0);
}

int main (int argc, char * argv [])
{
  if (argc != 2) {
    Usage(argv[0]);
  } else {
    if (strcmp(argv[1], "LOCAL") == 0) {
      printf("Opening local server on dedicated connection\n");
      g_connection = vrpn_create_server_connection();
      g_tracker = new vrpn_Tracker_NULL("Tracker0", g_connection);
      g_clock = new vrpn_Clock_Drift_Estimator("Tracker0@localhost", 0.01, 5);
    } else {
      printf("Connecting to server %s\n", argv[1]);
      g_clock = new vrpn_Clock_Drift_Estimator(argv[1], 0.01, 5);
    }
  }

#ifndef	_WIN32_WCE
  // signal handler so logfiles get closed right
  signal(SIGINT, handle_cntl_c);
#endif

/* 
 * main interactive loop
 */
  printf("Press ^C to exit.\n");
  while ( ! done ) {
    g_clock->mainloop();
    if (g_tracker) { g_tracker->mainloop(); }
    if (g_connection) { g_connection->mainloop(); }
  }

  if (g_clock) { delete g_clock; g_clock = NULL; }
  if (g_tracker) { delete g_tracker; g_tracker = NULL; }
  if (g_connection) { delete g_connection; g_connection = NULL; }
  return 0;
}   /* main */


