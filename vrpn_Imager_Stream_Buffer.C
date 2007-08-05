#include  <math.h>
#include  <stdio.h>
#include  "vrpn_Imager_Stream_Buffer.h"

vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer(const char * name, const char * imager_server_name, vrpn_Connection * c) :
vrpn_Auxilliary_Logger_Server(name, c)
, vrpn_Imager_Server(name, c, 0, 0) // Default number of rows and columns for the device.
, d_logging_thread(NULL)
, d_imager_server_name(NULL)
{
  // Copy the name of the server we are to connect to when we are logging.
  d_imager_server_name = new char[strlen(imager_server_name)+1];
  if (d_imager_server_name == NULL) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: Out of memory\n");
    d_connection = NULL;
    return;
  }
  strcpy(d_imager_server_name, imager_server_name);

  // Create the logging thread but do not run it yet.
  vrpn_ThreadData td;
  td.pvUD = this;
  d_logging_thread = new vrpn_Thread(static_logging_thread_func, td);
  if (d_logging_thread == NULL) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: can't create logging thread\n");
    d_connection = NULL;
    return;
  }

  // Register a handler for the got connection message.
  got_first_connection_m_id = d_connection->register_message_type( vrpn_got_first_connection );
  if (got_first_connection_m_id == -1) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: can't register got first connection type\n");
    d_connection = NULL;
    return;
  }
  if (register_autodeleted_handler(got_first_connection_m_id,
    static_handle_got_first_connection, this, vrpn_ANY_SENDER)) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: can't register got first connection handler\n");
    d_connection = NULL;
  }
}

vrpn_Imager_Stream_Buffer::~vrpn_Imager_Stream_Buffer()
{
  stop_logging_thread();
  if (d_imager_server_name) {
    delete [] d_imager_server_name;
    d_imager_server_name = NULL;
  }
}

void vrpn_Imager_Stream_Buffer::mainloop(void)
{
  // Required from all servers
  server_mainloop();

  // See if we have a new image description from a logging thread.  If so,
  // fill in our values and send a description to any attached clients.
  if (d_shared_state.get_imager_description(d_nRows, d_nCols, d_nDepth, d_nChannels)) {
    send_description();
  }
}

/* static */
// This method just passes the call on to the virtual function.
int vrpn_Imager_Stream_Buffer::static_handle_got_first_connection(void *userdata,
	vrpn_HANDLERPARAM p)
{
  vrpn_Imager_Stream_Buffer *me = static_cast<vrpn_Imager_Stream_Buffer *>(userdata);
  me->handle_got_first_connection();
  return 0;
}

// Handle got first connection request by (having the second thread) create
// a connection to the server and waiting until we get a description message
// from the imager server we're listening to.  Timeout after a while if the
// connection cannot be made or the server does not respond.

void vrpn_Imager_Stream_Buffer::handle_got_first_connection(void)
{
  // There should be no thread in existence when this call is made.
  // If there is, kill it and complain.
  if (d_logging_thread->running()) {
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    send_text_message("handle_got_first_connection: Thread running when it should not be", now, vrpn_TEXT_ERROR);
    d_logging_thread->kill();
    return;
  }

  // Reset the shared state before starting the thread running.
  d_shared_state.init();

  // Create a thread whose userdata points at the object that
  // created it.  Then call the start function on that thread and wait
  // for its vrpn_Imager_Remote to receive the info from the remote server
  // it has connected to.  We time out after a few seconds if we don't
  // get the response, leaving us with a presumably broken connection
  // to the server.
  if (!d_logging_thread->go()) {
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    send_text_message("handle_got_first_connection: Failed to start logging thread", now, vrpn_TEXT_ERROR);
    delete d_logging_thread;
    d_logging_thread = NULL;
    return;
  }
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  do {
    if (d_shared_state.get_imager_description(d_nRows, d_nCols, d_nDepth, d_nChannels)) {
      return;
    }

    vrpn_SleepMsecs(1);
    vrpn_gettimeofday(&now, NULL);
  } while ( vrpn_TimevalDiff(now, start).tv_sec < 3 );

  // Timed out, so we won't be hearing from the server!
  vrpn_gettimeofday(&now, NULL);
  send_text_message("handle_got_first_connection: Didn't hear from server.", now, vrpn_TEXT_WARNING);
}

// Handle dropped last connection on our primary connection by shutting down the
// connection to the imager server (killing the logging thread).
void vrpn_Imager_Stream_Buffer::handle_dropped_last_connection(void)
{
  if (!stop_logging_thread()) {
    fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_dropped_last_connection(): Had to kill logging thread\n");
  }
}

// The function that is called to become the logging thread.  It is passed
// a pointer to "this" so that it can acces the object that created it.
// The static function basically just unpacks the "this" pointer and
// then calls the non-static function.
void vrpn_Imager_Stream_Buffer::static_logging_thread_func(vrpn_ThreadData &threadData)
{
    vrpn_Imager_Stream_Buffer *me = static_cast<vrpn_Imager_Stream_Buffer *>(threadData.pvUD);
    me->logging_thread_func();
}

// Note that it must use semaphores to get at the data that will be shared
// between the main thread and itself.
// This function does all the work of the logging thread, including all
// interactions with the vrpn_Imager_Server connection(s) and the client
// object; it forwards information both ways to the main thread that is
// communicating with the end-user client connection.
// DO NOT CALL VRPN message sends from this function or those it calls,
// because we're not the thread that is connected to the client object's
// connection and such calls are not thread-safe.
void vrpn_Imager_Stream_Buffer::logging_thread_func(void)
{
  // Initialize everything to a clean state.
  d_log_connection = NULL;
  d_imager_remote = NULL;

  // Open a connection to the server object, not asking it to log anything.
  // (Logging will be started later if we receive a message from our client.)
  // Create a vrpn_Imager_Remote object and set its callbacks to fill things
  // into the shared data structures.
  d_log_connection = open_new_log_connection("","","","");
  if (d_log_connection == NULL) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::logging_thread_func(): Cannot open connection\n");
    return;
  }
  if (!setup_handlers_for_logging_connection(d_log_connection)) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::logging_thread_func(): Cannot set up handlers\n");
    return;
  }

  //XXX;

  // Keep doing mainloop() on the client object(s) and checking
  // for commands that we're supposed to issue until we're
  // told that we're supposed to die.  Sleep a little between iterations
  // to avoid eating CPU time.
  while (!d_shared_state.time_to_exit()) {
    if (d_imager_remote) {
      d_imager_remote->mainloop();
    }
    if (d_log_connection) {
      d_log_connection->mainloop();
      d_log_connection->save_log_so_far();
    }

    // Check to see if we've been asked to create new log files.  If we have,
    // then attempt to do so.  If that works, pass back the names of the files
    // created to the initial thread.  If it did not work, return empty log-file
    // names.
    char *lil, *lol, *ril, *rol;
    if ( d_shared_state.get_logfile_request(&lil, &lol, &ril, &rol) ) {
      if (make_new_logging_connection(lil, lol, ril, rol)) {
        d_shared_state.set_logfile_result(lil, lol, ril, rol);
      } else {
        d_shared_state.set_logfile_result("","","","");
      }
      // Delete the allocated space only if there were return values.
      delete [] lil;
      delete [] lol;
      delete [] ril;
      delete [] rol;
    }

    //XXX;

    vrpn_SleepMsecs(1);
  }

  // Now that we've been told to die, clean up everything and return.
  if (d_imager_remote) {
    delete d_imager_remote;
    d_imager_remote = NULL;
  }
  if (d_log_connection) {
    d_log_connection->removeReference();
    d_log_connection = NULL;
  }
}

vrpn_Connection *vrpn_Imager_Stream_Buffer::open_new_log_connection(
                  const char *local_in_logfile_name,
                  const char *local_out_logfile_name,
                  const char *remote_in_logfile_name,
                  const char *remote_out_logfile_name)
{
  vrpn_Connection *ret = NULL;

  // Find the relevant part of the name (skip past last '@'
  // if there is one); also find the port number.
  const char *cname = d_imager_server_name;
  const char *where_at;	// Part of name past last '@'
  if ( (where_at = strrchr(cname, '@')) != NULL) {
	  cname = where_at+1;	// Chop off the front of the name
  }
  int port = vrpn_get_port_number(cname);
  ret = new vrpn_Connection (cname, port,
	  local_in_logfile_name, local_out_logfile_name,
	  remote_in_logfile_name, remote_out_logfile_name);
  if ( !ret->doing_okay() ) {
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    fprintf(stderr, "vrpn_Imager_Stream_Buffer::open_new_log_connection: Could not create connection (files already exist?)", now, vrpn_TEXT_ERROR);
    delete ret;
    return NULL;
  }
  ret->setAutoDeleteStatus(true);	// destroy when refcount hits zero.
  ret->addReference();

  return ret;
}

bool vrpn_Imager_Stream_Buffer::setup_handlers_for_logging_connection(vrpn_Connection *c)
{
  // Create a vrpn_Imager_Remote on this connection and set its callbacks so
  // that they will do what needs doing; the callbacks point to the
  // Imager_Stream_Buffer object, not to the imager_remote object; access it
  // through the member variable pointer.
  d_imager_remote = new vrpn_Imager_Remote(d_imager_server_name, c);
  if (d_imager_remote == NULL) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::setup_handlers_for_logging_connection(): Cannot create vprn_Imager_Remote\n");
    return false;
  }
  d_imager_remote->register_description_handler(this, handle_image_description);

  return true;
}

bool vrpn_Imager_Stream_Buffer::teardown_handlers_for_logging_connection(vrpn_Connection *c)
{
  if (!d_imager_remote) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::teardown_handlers_for_logging_connection(): No imager remote\n");
    return false;
  }
  if (d_imager_remote->unregister_description_handler(this, handle_image_description) != 0) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::teardown_handlers_for_logging_connection(): Cannot unregister handler\n");
    return false;
  }

  delete d_imager_remote;
  d_imager_remote = NULL;
  return true;
}

bool vrpn_Imager_Stream_Buffer::make_new_logging_connection(const char *local_in_logfile_name,
                                    const char *local_out_logfile_name,
                                    const char *remote_in_logfile_name,
                                    const char *remote_out_logfile_name)
{
  // Open a new connection to do logging on before deleting the old one so
  // that we keep at least one connection open to the server at all time.
  // This will prevent it from doing its "dropped last connection" things
  // which will include resetting the imager server.
  vrpn_Connection *new_log_connection = open_new_log_connection(
    local_in_logfile_name, local_out_logfile_name,
    remote_in_logfile_name, remote_out_logfile_name);
  if (new_log_connection == NULL) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::make_new_logging_connection(): Cannot open connection\n");
    return false;
  }

  // Unhook the callbacks from the existing logging connection so that
  // we don't end up with two callbacks for each message.
  if (!teardown_handlers_for_logging_connection(d_log_connection)) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::make_new_logging_connection(): Cannot teardown connection\n");
    return false;
  }

  // Hook the callbacks up to the new connection so that we will get reports
  // from the server.
  if (!setup_handlers_for_logging_connection(new_log_connection)) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::make_new_logging_connection(): Cannot setup connection\n");
    return false;
  }

  // Mainloop the new connection object until it becomes connected or we
  // time out.  If we time out, then put things back on the old connection
  // and tell the thread it is time to self-destruct.  The way we check
  // for connected cannot be just that the connection's connected() method
  // returns true (because our end can be marked connected before the other
  // end decides it has complete the connection.  Rather, we check to see
  // that we've got a new description report from the server -- indicating
  // that it has seen the new report.  This also lets us know that the old
  // log file will have accumulated all images up to the new report, so we
  // can shut it off without losing any images in the switch to the new
  // log file (there may be duplicates, but not losses).
  vrpn_int32 unused;
  d_shared_state.get_imager_description(unused, unused, unused, unused);
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  d_ready_to_drop_old_connection = false;
  while ( !d_ready_to_drop_old_connection && (vrpn_TimevalDiff(now, start).tv_sec < 3)) {
    vrpn_SleepMsecs(1);
    new_log_connection->mainloop();   // Enable connection set-up to occur
    new_log_connection->save_log_so_far();
    d_log_connection->mainloop();     // Eat up (and log) any incoming messages
    d_log_connection->save_log_so_far();
    vrpn_gettimeofday(&now, NULL);
  };
  if (!d_ready_to_drop_old_connection) {
    fprintf(stderr,"vrpn_Imager_Stream_Buffer::make_new_logging_connection(): Could not connect new logging connection\n");
    teardown_handlers_for_logging_connection(new_log_connection);
    setup_handlers_for_logging_connection(d_log_connection);
    new_log_connection->removeReference();
    d_shared_state.time_to_exit(true);
    return false;
  }

  // Delete the old connection object by reducing its reference count.
  d_log_connection->removeReference();

  // Set up to use the new connection
  d_log_connection = new_log_connection;
  return true;
}

void vrpn_Imager_Stream_Buffer::handle_request_logging(const char *local_in_logfile_name,
                                    const char *local_out_logfile_name,
                                    const char *remote_in_logfile_name,
                                    const char *remote_out_logfile_name)
{
  // Request that the logging thread start new logs.
  d_shared_state.set_logfile_request(local_in_logfile_name,
    local_out_logfile_name, remote_in_logfile_name, remote_out_logfile_name);

  // Wait until we hear back from the logging thread or time out;
  // return empty if timeout and the strings we got back if not.
  // Remember to deallocated the memory if we got a response.
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  do {
    char *lil, *lol, *ril, *rol;
    if (d_shared_state.get_logfile_result(&lil, &lol, &ril, &rol)) {
      send_report_logging(lil, lol, ril, rol);
      delete [] lil;
      delete [] lol;
      delete [] ril;
      delete [] rol;
      return;
    }
    vrpn_SleepMsecs(1);
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalDiff(now, start).tv_sec < 2);

  // Timeout, report failure of logging by saying that there are empty log file names.
  send_report_logging("","","","");
}

/* Static */
// We've gotten a new imager description, so fill it into the shared data structure
// so that the parent object can hear about it.
void vrpn_Imager_Stream_Buffer::handle_image_description(void *pvISB, const struct timeval msg_time)
{
  vrpn_Imager_Stream_Buffer *me = static_cast<vrpn_Imager_Stream_Buffer *>(pvISB);

  me->d_shared_state.set_imager_description(
    me->d_imager_remote->nRows(),
    me->d_imager_remote->nCols(),
    me->d_imager_remote->nDepth(),
    me->d_imager_remote->nChannels());

  // We've gotten a description report on the new connection, so we're ready
  // to drop the old connection.
  me->d_ready_to_drop_old_connection = true;
}

// Stop the logging thread function, cleanly if possible.  Returns true if
// the function stopped cleanly, false if it had to be killed.
bool vrpn_Imager_Stream_Buffer::stop_logging_thread(void)
{
  // Set the flag telling the logging thread to stop.
  d_shared_state.time_to_exit(true);

  // Wait for up to three seconds for the logging thread to die a clean death.
  // If it does, return true.
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  do {
    if (!d_logging_thread->running()) {
      return true;
    }
    vrpn_SleepMsecs(1);
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalDiff(now, start).tv_sec < 3);

  d_logging_thread->kill();
  return false;
}
