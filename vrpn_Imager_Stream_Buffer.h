#ifndef	VRPN_IMAGER_STREAM_BUFFER_H
#define	VRPN_IMAGER_STREAM_BUFFER_H
#include  "vrpn_Auxilliary_Logger.h"
#include  "vrpn_Imager.h"

// This is a fairly complicated class that implements a multi-threaded
// full-rate logger and partial-rate forwarder for a vrpn_Imager_Server
// object.  It is intended to allow previewing of microscopy experiments
// at a subset of the camera's video rate while logging the video at
// full rate, possibly on a remote computer and possibly on the original
// video server computer.  Its architecture is described in the "Full-rate
// logging" section of the vrpn_Imager.html document in the VRPN web
// page.

//-------------------------------------------------------------------
// This is the data structure that is shared between the initial
// thread (which listens for client connections) and the non-blocking
// thread that sometimes exists to listen to the vrpn_Imager_Server.
// All of its methods must be thread-safe, so it creates a semaphore
// for access and uses it in all of the non-atomic methods.
// Note that some of the things in here are pointers to objects that
// are in the parent class, and they are here just to provide the
// semaphore protection.  The parent class should only access these
// things through this shared state object.

class VRPN_API vrpn_Imager_Stream_Shared_State {
public:
  vrpn_Imager_Stream_Shared_State() { init(); }

  void init(void) {
    d_time_to_exit = false;
    d_description_updated = false;
    d_nRows = d_nCols = d_nDepth = d_nChannels = 0;
    d_new_log_request = false;
    d_request_lil = NULL;
    d_request_lol = NULL;
    d_request_ril = NULL;
    d_request_rol = NULL;
    d_new_log_result = false;
    d_result_lil = NULL;
    d_result_lol = NULL;
    d_result_ril = NULL;
    d_result_rol = NULL;
  }

  // Accessors for the "time to exit" flag; set by the initial thread and
  // read by the logging thread.
  bool time_to_exit(void) { d_sem.p(); bool ret = d_time_to_exit; d_sem.v(); return ret; }
  void time_to_exit(bool do_exit) { d_sem.p(); d_time_to_exit = do_exit; d_sem.v(); }

  // Accessors for the parameters stored based on the
  // imager server's reports.  Returns false if nothing has
  // been set since the last time it was read, true (and fills in
  // the values) if it has.
  bool get_imager_description(vrpn_int32 &nRows, vrpn_int32 &nCols,
                              vrpn_int32 &nDepth, vrpn_int32 &nChannels) {
    d_sem.p();
    bool ret = d_description_updated;
    if (d_description_updated) {
      nRows = d_nRows; nCols = d_nCols; nDepth = d_nDepth; nChannels = d_nChannels;
    }
    d_description_updated = false;
    d_sem.v();
    return ret;
  }
  void set_imager_description(vrpn_int32 nRows, vrpn_int32 nCols,
                              vrpn_int32 nDepth, vrpn_int32 nChannels) {
    d_sem.p();
    d_nRows = nRows; d_nCols = nCols; d_nDepth = nDepth; d_nChannels = nChannels;
    d_description_updated = true;
    d_sem.v();
  }

  // Accessors for the initial thread to pass new logfile names down to the
  // logging thread, which will cause it to initiate a changeover of logging
  // connections.  Space for the return strings will be allocated in these functions
  // and must be deleted by the logging thread ONLY IF the get function fills the
  // values in (it returns true if it does).
  bool get_logfile_request(char **lil, char **lol, char **ril, char **rol) {
    d_sem.p();
    bool ret = d_new_log_request;
    if (d_new_log_request) {
      // Allocate space to return the names in the handles passed in.
      // Copy the values from our local storage to the return values.
      if ( (*lil = new char[strlen(d_request_lil)+1]) != NULL) {
        strcpy(*lil, d_request_lil);
      }
      if ( (*lol = new char[strlen(d_request_lol)+1]) != NULL) {
        strcpy(*lol, d_request_lol);
      }
      if ( (*ril = new char[strlen(d_request_ril)+1]) != NULL) {
        strcpy(*ril, d_request_ril);
      }
      if ( (*rol = new char[strlen(d_request_rol)+1]) != NULL) {
        strcpy(*rol, d_request_rol);
      }

      // Delete and NULL the local storage pointers.
      delete [] d_request_lil; d_request_lil = NULL;
      delete [] d_request_lol; d_request_lol = NULL;
      delete [] d_request_ril; d_request_ril = NULL;
      delete [] d_request_rol; d_request_rol = NULL;
    }
    d_new_log_request = false;
    d_sem.v();
    return ret;
  }
  void set_logfile_request(const char *lil, const char *lol, const char *ril, const char *rol) {
    d_sem.p();

    // Allocate space for each string and then copy into it.
    if ( (d_request_lil = new char[strlen(lil)+1]) != NULL) {
      strcpy(d_request_lil, lil);
    }
    if ( (d_request_lol = new char[strlen(lol)+1]) != NULL) {
      strcpy(d_request_lol, lol);
    }
    if ( (d_request_ril = new char[strlen(ril)+1]) != NULL) {
      strcpy(d_request_ril, ril);
    }
    if ( (d_request_rol = new char[strlen(rol)+1]) != NULL) {
      strcpy(d_request_rol, rol);
    }

    d_new_log_request = true;
    d_sem.v();
  }

  // Accessors for the logfile thread to pass new logfile names back up to the
  // initial thread, reporting a changeover of logging
  // connections.  Space for the return strings will be allocated in these functions
  // and must be deleted by the initial thread ONLY IF the get function fills the
  // values in (it returns true if it does).
  bool get_logfile_result(char **lil, char **lol, char **ril, char **rol) {
    d_sem.p();
    bool ret = d_new_log_result;
    if (d_new_log_result) {
      // Allocate space to return the names in the handles passed in.
      // Copy the values from our local storage to the return values.
      if ( (*lil = new char[strlen(d_result_lil)+1]) != NULL) {
        strcpy(*lil, d_result_lil);
      }
      if ( (*lol = new char[strlen(d_result_lol)+1]) != NULL) {
        strcpy(*lol, d_result_lol);
      }
      if ( (*ril = new char[strlen(d_result_ril)+1]) != NULL) {
        strcpy(*ril, d_result_ril);
      }
      if ( (*rol = new char[strlen(d_result_rol)+1]) != NULL) {
        strcpy(*rol, d_result_rol);
      }

      // Delete and NULL the local storage pointers.
      delete [] d_result_lil; d_result_lil = NULL;
      delete [] d_result_lol; d_result_lol = NULL;
      delete [] d_result_ril; d_result_ril = NULL;
      delete [] d_result_rol; d_result_rol = NULL;
    }
    d_new_log_result = false;
    d_sem.v();
    return ret;
  }
  void set_logfile_result(const char *lil, const char *lol, const char *ril, const char *rol) {
    d_sem.p();

    // Allocate space for each string and then copy into it.
    if ( (d_result_lil = new char[strlen(lil)+1]) != NULL) {
      strcpy(d_result_lil, lil);
    }
    if ( (d_result_lol = new char[strlen(lol)+1]) != NULL) {
      strcpy(d_result_lol, lol);
    }
    if ( (d_result_ril = new char[strlen(ril)+1]) != NULL) {
      strcpy(d_result_ril, ril);
    }
    if ( (d_result_rol = new char[strlen(rol)+1]) != NULL) {
      strcpy(d_result_rol, rol);
    }

    d_new_log_result = true;
    d_sem.v();
  }

protected:
  vrpn_Semaphore  d_sem;  // Semaphore to control access to data items.

  // Is it time for the logging thread to exit?
  bool  d_time_to_exit;

  // Stored copies of the value in the vrpn_Imager_Remote and a flag telling
  // whether they have changed since last read.
  bool  d_description_updated; // Do we have a new description from imager server?
  vrpn_int32  d_nRows;
  vrpn_int32  d_nCols;
  vrpn_int32  d_nDepth;
  vrpn_int32  d_nChannels;

  // Names of the log files passed from the initial thread to the logging
  // thread and a flag telling whether they have been changed since last
  // read.
  bool  d_new_log_request;
  char *d_request_lil;
  char *d_request_lol;
  char *d_request_ril;
  char *d_request_rol;

  // Names of the log files passed from the logging thread to the initial
  // thread and a flag telling whether they have been changed since last
  // read.
  bool  d_new_log_result;
  char *d_result_lil;
  char *d_result_lol;
  char *d_result_ril;
  char *d_result_rol;
};

//-------------------------------------------------------------------
// This class is a vrpn_Imager_Server; it has one or two instances of
// vrpn_Imager_Clients to talk to the server it is forwarding packets
// to.  It does not use their callback parsers, but rather hooks its own
// callbacks directly to the connection object for the server it is
// buffering.

class VRPN_API vrpn_Imager_Stream_Buffer :
               public vrpn_Auxilliary_Logger_Server,
               public vrpn_Imager_Server {
public:
  // Name of this object (the server side of the vrpn_Imager that is
  // buffered and the vrpn_Auxilliary_Logger that the client will connect to).
  // (Optional, can be NULL) pointer to the server connection on which to
  // communicate.
  // Name of the vrpn_Imager_Server to connect to (packets from this server will
  // be forwarded to the main connection, and logging will occur on the connection
  // to this imager_server).  This server may be local or remote; if local,
  // include "@localhost" in the name because new connections will be made to it.
  vrpn_Imager_Stream_Buffer(const char * name, const char * imager_server_name, vrpn_Connection * c);

  // Get rid of any logging thread and then clean up.
  virtual ~vrpn_Imager_Stream_Buffer();

  // Required for servers.
  virtual void mainloop(void);

protected:

  // Handle a logging-request message.  The request contains four file
  // names, two for local (to the Auxilliary server itself) and two for
  // remote (the far side of its connection to the server).  It must
  // also respond to the client with a message saying what logging has
  // been set up (using the send_logging_response function).  Logging is
  // turned off on a particular file by sending an empty-string name ("").
  // The in/out local/remote are with respect to the connection that the
  // logging is to occur on, which is to the imager server whose name is
  // passed in to the constructor, not the connection that the client has
  // sent the request on.
  // Make sure to send a response saying what you did.
  virtual void handle_request_logging(const char *local_in_logfile_name,
                                      const char *local_out_logfile_name,
                                      const char *remote_in_logfile_name,
                                      const char *remote_out_logfile_name);

  // Static portion of handling (unpacking) the request_logging message.  It
  // then calls the non-static virtual method above.
  static int VRPN_CALLBACK static_handle_request_logging(void *userdata, vrpn_HANDLERPARAM p );

  // Handle dropped last connection on our primary connection by shutting down the
  // connection to the imager server.  The static method in the base class looks up this
  // pointer and calls the virtual method.
  virtual void handle_dropped_last_connection(void);

  // Handle got first connection request by (having the second thread) create
  // a connection to the server and waiting until we get a description message
  // from the imager server we're listening to.  Timeout after a while if the
  // connection cannot be made or the server does not respond.
  virtual void handle_got_first_connection(void);
  vrpn_int32 got_first_connection_m_id;    // ID of message that we got the first connection
  static int VRPN_CALLBACK static_handle_got_first_connection(void *userdata, vrpn_HANDLERPARAM p );

  // State shared between the initial thread and the logging thread.
  vrpn_Imager_Stream_Shared_State d_shared_state;

  //----------------------------------------------------------------------
  // The section below includes methods and member variables that should
  // only be used by the logging thread.  They are not protected by
  // semaphores and so should not be accessed except within the
  // logging_thread_func().

  // This class spawns a new thread to handle uninterrupted communication
  // and logging with the vrpn_Imager_Server that we are forwarding messages
  // for.  This is created in the constructor and shut down (hopefully gently)
  // in the destructor.  There are a number of semaphores that are used by
  // the initial thread and the logging thread to communicate.
  vrpn_Thread   *d_logging_thread;

  // The function that is called to become the logging thread.  It is passed
  // a pointer to "this" so that it can acces the object that created it.
  // Note that it must use semaphores to get at the data that will be shared
  // between the main thread and itself.  The static function basically just
  // pulls the "this" pointer out and then calls the non-static function.
  static void static_logging_thread_func(vrpn_ThreadData &threadData);
  void logging_thread_func(void);

  // Stop the logging thread function, cleanly if possible.  Returns true if
  // the function stopped cleanly, false if it had to be killed.
  bool stop_logging_thread(void);

  // Name of the vrpn_Imager_Server object we are to connect to and
  // log/pass messages from.
  char *d_imager_server_name;

  // Are we ready to drop the old connection (new one has received its
  // descriptor message)?
  bool d_ready_to_drop_old_connection;

  // The connection that is used to talk to the client.
  vrpn_Connection *d_log_connection;
  vrpn_Connection *open_new_log_connection(
                  const char *local_in_logfile_name,
                  const char *local_out_logfile_name,
                  const char *remote_in_logfile_name,
                  const char *remote_out_logfile_name);

  // These will create/destroy the d_imager_remote and other callback handlers
  // needed to provide the handling of messages from the logging connection
  // passed in; they are used by the initial-connection code and by the
  // code that handles handing off from an old connection to a new connection
  // when a new logging message is received.
  bool setup_handlers_for_logging_connection(vrpn_Connection *c);
  bool teardown_handlers_for_logging_connection(vrpn_Connection *c);

  // This is yet another "create me some logs" function; it handles the
  // hand-off from one log file to another within the logging thread.
  // It is called by the main logging thread function when a request comes in
  // from the initial thread to perform logging.
  bool make_new_logging_connection(const char *local_in_logfile_name,
                                    const char *local_out_logfile_name,
                                    const char *remote_in_logfile_name,
                                    const char *remote_out_logfile_name);

  // The imager remote to listen to the vrpn_Imager_Server, along
  // with the callback functions that support its operation.
  vrpn_Imager_Remote *d_imager_remote;
  static void VRPN_CALLBACK handle_image_description(void *pvISB, const struct timeval msg_time);
};

//-----------------------------------------------------------
// Client code should connect to the server twice, once as
// a vrpn_Imager_Server and once as a vrpn_Auxilliary_Logger_Server.
// There is not a special remote class for this.

#endif
