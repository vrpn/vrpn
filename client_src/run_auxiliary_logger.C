#include <stdio.h>                      // for fprintf, NULL, printf, etc
#include <stdlib.h>                     // for atoi, exit
#include <string.h>                     // for strcmp

#include "vrpn_Auxiliary_Logger.h"
#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday, etc

vrpn_Auxiliary_Logger_Remote *g_logger;


/*****************************************************************************
 *
   Callback handlers and the data structures they use to pass information
   to the rest of the program.
 *
 *****************************************************************************/

// Caller sets to false before mainloop(); handle_log_report sets to true
// when a report arrives.
bool  g_got_report = false;

// Names returned to the log report handler.  Filled in by the report handler.
const unsigned NAMELEN = 2048;
char g_local_in[NAMELEN];
char g_local_out[NAMELEN];
char g_remote_in[NAMELEN];
char g_remote_out[NAMELEN];

void	VRPN_CALLBACK handle_log_report (void *, const vrpn_AUXLOGGERCB info)
{
  vrpn_strcpy(g_local_in, info.local_in_logfile_name);
  vrpn_strcpy(g_local_out, info.local_out_logfile_name);
  vrpn_strcpy(g_remote_in, info.remote_in_logfile_name);
  vrpn_strcpy(g_remote_out, info.remote_out_logfile_name);
  printf( "log report:  \'%s\'  \'%s\'  \'%s\'  \'%s\'\n", 
	  g_local_in, g_local_out, g_remote_in, g_remote_out );
  g_got_report = true;
}

/*****************************************************************************
 *
 *****************************************************************************/

// Request some log files and wait up to a second for the report of these
// files.  Return true if we got a report and the names match.
bool test_logfile_names(const char *local_in, const char *local_out,
                        const char *remote_in, const char *remote_out)
{
  struct timeval  start;
  struct timeval now;

  // Mark no report and the request logging with the specified
  // parameters.
  g_got_report = false;
  if (!g_logger->send_logging_request(local_in, local_out, remote_in, remote_out)) {
    fprintf(stderr, "test_logfile_names: Logging request send failed\n");
    return false;
  }

  // Mainloop the logger for up to five seconds waiting for a response.
  // If we don't get a response, this is a failure.
  vrpn_gettimeofday(&start, NULL);
  do {
    g_logger->mainloop();
    vrpn_gettimeofday(&now, NULL);
    vrpn_SleepMsecs(1);
  } while ( !g_got_report && (vrpn_TimevalDurationSeconds(now, start) < 5.0));
  if (!g_got_report) {
    fprintf(stderr, "test_logfile_names: Timeout waiting for report of logging from server\n");
    return false;
  }

  // Check to see if the names are the same.  Return true if they all are.
  if ( (strcmp(g_local_in, local_in) == 0) &&
       (strcmp(g_local_out, local_out) == 0) &&
       (strcmp(g_remote_in, remote_in) == 0) &&
       (strcmp(g_remote_out, remote_out) == 0) ) {
    return true;
  } else {
    return false;
  }
}


/*****************************************************************************
 *
   init - initialize everything
 *
 *****************************************************************************/

int main (int argc, char * argv [])
{
  const char * devicename = "Logger0@localhost";
  const char * logfilename = "deleteme.vrpn";
  int log_duration_seconds = 120;
  struct timeval  start;
  struct timeval now;
  int ret = 0;

  // parse args
  if (argc != 4) {
    fprintf(stderr, "Usage: %s Device_name remote_logfile_name time_seconds\n"
                    "  Device_name:  VRPN name of data source to contact\n"
                    "    example:  Logger0@localhost\n"
                    "  remote_logfile_name: The name of the file to log remotely\n"
                    "  time_seconds: How long to log before closing and exiting\n",
            argv[0]);
    exit(0);
  } else {
    devicename = argv[1];
    logfilename = argv[2];
    log_duration_seconds = atoi(argv[3]);
  }

  // Open the logger and set up its callback handler.
  fprintf(stderr, "Logger's name is %s.\n", devicename);
  g_logger = new vrpn_Auxiliary_Logger_Remote (devicename);
  g_logger->register_report_handler(NULL, handle_log_report);

  // Main loop for half a second to get things started on the
  // connection.
  vrpn_gettimeofday(&start, NULL);
  do {
    g_logger->mainloop();
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalDurationSeconds(now, start) < 0.5);

  // Try to create the named log file as the remote outgoing log
  // on the server.  Wait for the specified duration to give it
  // time to log.
  if (!test_logfile_names("", "", "", logfilename) ) {
    fprintf(stderr,"Error creating remote outgoing log file %s\n",logfilename);
    ret = -1;
  }
  vrpn_gettimeofday(&start, NULL);
  do {
    g_logger->mainloop();
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalDurationSeconds(now, start) <= log_duration_seconds);

  // Try to create blank log files (no log should be made).  Wait for a while after
  // creation to give time to stop logging.
  if (!test_logfile_names("", "", "", "") ) {
    fprintf(stderr,"Error turning off logs\n");
    ret = -1;
  }
  vrpn_gettimeofday(&start, NULL);
  do {
    g_logger->mainloop();
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalDurationSeconds(now, start) < 5.0);

  // Done.
  if (ret == 0) {
    printf("Success!\n");
  } else {
    printf("Make sure that files with the requested names don't already exist.\n");
  }
  delete g_logger;
  return ret;

}   /* main */
