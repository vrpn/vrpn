#include <stddef.h> // for size_t
#include <stdio.h>  // for fprintf, stderr

#include "vrpn_Auxiliary_Logger.h"

vrpn_Auxiliary_Logger::vrpn_Auxiliary_Logger(const char *name,
                                             vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
{
    vrpn_BaseClass::init();
}

int vrpn_Auxiliary_Logger::register_types(void)
{
    request_logging_m_id = d_connection->register_message_type(
        "vrpn_Auxiliary_Logger Logging_request");
    report_logging_m_id = d_connection->register_message_type(
        "vrpn_Auxiliary_Logger Logging_response");
    request_logging_status_m_id = d_connection->register_message_type(
        "vrpn_Auxiliary_Logger Logging_status_request");

    if ((request_logging_m_id == -1) || (report_logging_m_id == -1) ||
        (request_logging_status_m_id == -1)) {
        d_connection = NULL;
        return -1;
    }
    else {
        return 0;
    }
}

// Send a message of the specified type.
// Note that the pointers may be NULL, so we need to check for this.
bool vrpn_Auxiliary_Logger::pack_log_message_of_type(
    vrpn_int32 type, const char *local_in_logfile_name,
    const char *local_out_logfile_name, const char *remote_in_logfile_name,
    const char *remote_out_logfile_name)
{
    if (!d_connection) {
        return false;
    }

    // Figure out the lengths, handling NULL pointers.
    vrpn_int32 lil = 0;
    if (local_in_logfile_name) {
        lil = static_cast<vrpn_int32>(strlen(local_in_logfile_name));
    }
    vrpn_int32 lol = 0;
    if (local_out_logfile_name) {
        lol = static_cast<vrpn_int32>(strlen(local_out_logfile_name));
    }
    vrpn_int32 ril = 0;
    if (remote_in_logfile_name) {
        ril = static_cast<vrpn_int32>(strlen(remote_in_logfile_name));
    }
    vrpn_int32 rol = 0;
    if (remote_out_logfile_name) {
        rol = static_cast<vrpn_int32>(strlen(remote_out_logfile_name));
    }

    struct timeval now;
    vrpn_int32 bufsize =
        static_cast<vrpn_int32>(4 * sizeof(lil) + lil + lol + ril + rol);
    std::vector<char> buf;
    try {
      buf.resize(bufsize);
    } catch (...) {
      fprintf(stderr, "vrpn_Auxiliary_Logger::pack_log_message_of_type(): "
        "Out of memory.\n");
      return false;
    }

    // Pack a message with the requested type from our sender ID that has
    // first the four lengths of the strings and then the four strings.
    // Do not include the NULL termination of the strings in the buffer.

    vrpn_gettimeofday(&now, NULL);
    char *bpp = buf.data();
    char **bp = &bpp;
    vrpn_int32 bufleft = bufsize;
    vrpn_buffer(bp, &bufleft, lil);
    vrpn_buffer(bp, &bufleft, lol);
    vrpn_buffer(bp, &bufleft, ril);
    vrpn_buffer(bp, &bufleft, rol);
    if (lil) {
        vrpn_buffer(bp, &bufleft, local_in_logfile_name, lil);
    }
    if (lol) {
        vrpn_buffer(bp, &bufleft, local_out_logfile_name, lol);
    }
    if (ril) {
        vrpn_buffer(bp, &bufleft, remote_in_logfile_name, ril);
    }
    if (rol) {
        vrpn_buffer(bp, &bufleft, remote_out_logfile_name, rol);
    }
    int ret =
        d_connection->pack_message(bufsize - bufleft, now, type, d_sender_id,
                                   buf.data(), vrpn_CONNECTION_RELIABLE);
    return (ret == 0);
}

// Unpack the strings from the buffer.  Return non-NULL pointers to
// strings (an empty string when a file name is empty).
bool vrpn_Auxiliary_Logger::unpack_log_message_from_buffer(
    const char *buf, vrpn_int32 buflen, char **local_in_logfile_name,
    char **local_out_logfile_name, char **remote_in_logfile_name,
    char **remote_out_logfile_name)
{
    const char *bufptr = buf;

    // Make sure that the buffer contains at least enough space for the four
    // length
    // values, then pull the lengths of the strings out of the buffer
    vrpn_int32 localInNameLen, localOutNameLen, remoteInNameLen,
        remoteOutNameLen;
    if (static_cast<size_t>(buflen) < 4 * sizeof(localInNameLen)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger::unpack_log_message_from_buffer:"
                        " Buffer too small for lengths.\n");
        return false;
    }
    vrpn_unbuffer(&bufptr, &localInNameLen);
    vrpn_unbuffer(&bufptr, &localOutNameLen);
    vrpn_unbuffer(&bufptr, &remoteInNameLen);
    vrpn_unbuffer(&bufptr, &remoteOutNameLen);

    // Make sure we have enough room in the buffer for the four sizes and also
    // the four
    // strings of the appropriate size.  If so, allocate the space for the
    // strings and then
    // copy them out and NULL-terminate them.  They are not NULL-terminated in
    // the message
    // buffer.
    int size = 4 * sizeof(localInNameLen) + localInNameLen + localOutNameLen +
               remoteInNameLen + remoteOutNameLen;
    if (buflen != size) {
        fprintf(stderr, "vrpn_Auxiliary_Logger::unpack_log_message_from_buffer:"
                        " Buffer size incorrect\n");
        return false;
    }
    (*local_in_logfile_name) = NULL;
    (*local_out_logfile_name) = NULL;
    (*remote_in_logfile_name) = NULL;
    (*remote_out_logfile_name) = NULL;
    if (localInNameLen > 0) {
        try { (*local_in_logfile_name) = new char[localInNameLen + 1]; }
        catch (...) {
            fprintf(stderr, "vrpn_Auxiliary_Logger::unpack_log_message_from_"
                            "buffer: Out of memory\n");
            return false;
        }
        memcpy(*local_in_logfile_name, bufptr, localInNameLen);
        (*local_in_logfile_name)[localInNameLen] = '\0';
        bufptr += localInNameLen;
    } else {
        (*local_in_logfile_name) = NULL;
    }
    if (localOutNameLen > 0) {
        try { (*local_out_logfile_name) = new char[localOutNameLen + 1]; }
        catch (...) {
            fprintf(stderr, "vrpn_Auxiliary_Logger::unpack_log_message_from_"
                            "buffer: Out of memory\n");
            return false;
        }
        memcpy(*local_out_logfile_name, bufptr, localOutNameLen);
        (*local_out_logfile_name)[localOutNameLen] = '\0';
        bufptr += localOutNameLen;
    } else {
        (*local_out_logfile_name) = NULL;
    }
    if (remoteInNameLen > 0) {
        try { (*remote_in_logfile_name) = new char[remoteInNameLen + 1]; }
        catch (...) {
            fprintf(stderr, "vrpn_Auxiliary_Logger::unpack_log_message_from_"
                            "buffer: Out of memory\n");
            return false;
        }
        memcpy(*remote_in_logfile_name, bufptr, remoteInNameLen);
        (*remote_in_logfile_name)[remoteInNameLen] = '\0';
        bufptr += remoteInNameLen;
    } else {
        (*remote_in_logfile_name) = NULL;
    }
    if (remoteOutNameLen > 0) {
        try { (*remote_out_logfile_name) = new char[remoteOutNameLen + 1]; }
        catch (...) {
            fprintf(stderr, "vrpn_Auxiliary_Logger::unpack_log_message_from_"
                            "buffer: Out of memory\n");
            return false;
        }
        memcpy(*remote_out_logfile_name, bufptr, remoteOutNameLen);
        (*remote_out_logfile_name)[remoteOutNameLen] = '\0';
        bufptr += remoteOutNameLen;
    } else {
        (*remote_out_logfile_name) = NULL;
    }

    return true;
}

vrpn_Auxiliary_Logger_Server::vrpn_Auxiliary_Logger_Server(const char *name,
                                                           vrpn_Connection *c)
    : vrpn_Auxiliary_Logger(name, c)
{
    // Register a handler for the dropped last connection message.
    dropped_last_connection_m_id =
        d_connection->register_message_type(vrpn_dropped_last_connection);
    if (dropped_last_connection_m_id == -1) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::vrpn_Auxiliary_Logger_"
                        "Server: can't register dropped last connection "
                        "type\n");
        d_connection = NULL;
        return;
    }
    if (register_autodeleted_handler(dropped_last_connection_m_id,
                                     static_handle_dropped_last_connection,
                                     this, vrpn_ANY_SENDER)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::vrpn_Auxiliary_Logger_"
                        "Server: can't register dropped last connection "
                        "handler\n");
        d_connection = NULL;
    }

    // Register a handler for the request logging message.
    if (register_autodeleted_handler(request_logging_m_id,
                                     static_handle_request_logging, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::vrpn_Auxiliary_Logger_"
                        "Server: can't register logging request handler\n");
        d_connection = NULL;
    }

    // Register a handler for the request logging-status message
    if (register_autodeleted_handler(request_logging_status_m_id,
                                     static_handle_request_logging_status, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::vrpn_Auxiliary_Logger_"
                        "Server: can't register logging-status request "
                        "handler\n");
        d_connection = NULL;
    }
}

// This handles the last dropped connection message by turning off all
// logging.
void vrpn_Auxiliary_Logger_Server::handle_dropped_last_connection(void)
{
    handle_request_logging("", "", "", "");
}

/* static */
// This method just passes the call on to the virtual function.
int vrpn_Auxiliary_Logger_Server::static_handle_dropped_last_connection(
    void *userdata, vrpn_HANDLERPARAM /*p*/)
{
    vrpn_Auxiliary_Logger_Server *me =
        static_cast<vrpn_Auxiliary_Logger_Server *>(userdata);
    me->handle_dropped_last_connection();
    return 0;
}

/* static */
int vrpn_Auxiliary_Logger_Server::static_handle_request_logging_status(
    void *userdata, vrpn_HANDLERPARAM /*p*/)
{
    vrpn_Auxiliary_Logger_Server *me =
        static_cast<vrpn_Auxiliary_Logger_Server *>(userdata);
    me->handle_request_logging_status();
    return 0;
}

/* static */
// This method just parses the raw data in the Handlerparam to produce strings
// and then
// passes the call on to the virtual function.
int vrpn_Auxiliary_Logger_Server::static_handle_request_logging(
    void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Auxiliary_Logger_Server *me =
        static_cast<vrpn_Auxiliary_Logger_Server *>(userdata);
    char *localInName = NULL, *localOutName = NULL, *remoteInName = NULL,
         *remoteOutName = NULL;

    // Attempt to unpack the names from the buffer
    if (!me->unpack_log_message_from_buffer(p.buffer, p.payload_len,
                                            &localInName, &localOutName,
                                            &remoteInName, &remoteOutName)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::static_handle_request_"
                        "logging: Could not unpack buffer\n");
        return -1;
    }

    // Call the virtual function with the strings, then clean up memory and
    // return.
    me->handle_request_logging(localInName, localOutName, remoteInName,
                               remoteOutName);
    if (localInName) {
      try {
        delete[] localInName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::static_handle_request_logging: delete failed\n");
        return -1;
      }
    };
    if (localOutName) {
      try {
        delete[] localOutName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::static_handle_request_logging: delete failed\n");
        return -1;
      }
    };
    if (remoteInName) {
      try {
        delete[] remoteInName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::static_handle_request_logging: delete failed\n");
        return -1;
      }
    };
    if (remoteOutName) {
      try {
        delete[] remoteOutName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server::static_handle_request_logging: delete failed\n");
        return -1;
      }
    };
    return 0;
}

vrpn_Auxiliary_Logger_Server_Generic::vrpn_Auxiliary_Logger_Server_Generic(
    const char *logger_name, const char *connection_to_log, vrpn_Connection *c)
    : vrpn_Auxiliary_Logger_Server(logger_name, c)
    , d_connection_name(NULL)
    , d_logging_connection(NULL)
{
    // Copy the name of the connection to log and its NULL terminator.
    if ((connection_to_log == NULL) || (strlen(connection_to_log) == 0)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::vrpn_Auxiliary_"
                        "Logger_Server_Generic: Empty logging name passed "
                        "in\n");
        d_connection = NULL;
        return;
    }
    d_connection_name = new char[strlen(connection_to_log) + 1];
    memcpy(d_connection_name, connection_to_log, strlen(connection_to_log) + 1);
}

vrpn_Auxiliary_Logger_Server_Generic::~vrpn_Auxiliary_Logger_Server_Generic()
{
    if (d_logging_connection) {
        try {
          delete d_logging_connection;
        } catch (...) {
          fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::~vrpn_Auxiliary_Logger_Server_Generic: delete failed\n");
          return;
        }
        d_logging_connection = NULL;
    }

    if (d_connection_name) {
        try {
          delete[] d_connection_name;
        } catch (...) {
          fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::~vrpn_Auxiliary_Logger_Server_Generic: delete failed\n");
          return;
        }
        d_connection_name = NULL;
    }
}

// Close an existing logging connection, then (if any of the file
// names are non-empty) open a new logging connection to the
// connection we are to log (even if this process already has a
// connection to it) and then send back the report that we've started
// logging if we are able.  If we cannot open it, then fill in all
// blank names for the return report.
void vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging(
    const char *local_in_logfile_name, const char *local_out_logfile_name,
    const char *remote_in_logfile_name, const char *remote_out_logfile_name)
{
    // If we have a logging connection open, reduce its reference
    // count (which may delete it but will leave it going if some
    // other object has a pointer to it).
    if (d_logging_connection) {
        d_logging_connection->removeReference();
        d_logging_connection = NULL;
    }

    // If at least one of the names passed in is not empty, create
    // a new logging connection.  If this fails, report no logging.

    // Find the relevant part of the name (skip past last '@'
    // if there is one); also find the port number.
    const char *cname = d_connection_name;
    const char *where_at; // Part of name past last '@'
    if ((where_at = strrchr(cname, '@')) != NULL) {
        cname = where_at + 1; // Chop off the front of the name
    }

    // Pass "true" to force_connection so that it will open a new
    // connection even if we already have one with that name.
    d_logging_connection = vrpn_get_connection_by_name(
        where_at, local_in_logfile_name, local_out_logfile_name,
        remote_in_logfile_name, remote_out_logfile_name, NULL, true);
    if (!d_logging_connection || !d_logging_connection->doing_okay()) {
        struct timeval now;
        vrpn_gettimeofday(&now, NULL);
        send_text_message("handle_request_logging: Could not create connection "
                          "(files already exist?)",
                          now, vrpn_TEXT_ERROR);
        send_report_logging(NULL, NULL, NULL, NULL);
        if (d_logging_connection) {
            try {
              delete d_logging_connection;
            } catch (...) {
              fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging: delete failed\n");
              return;
            }
            d_logging_connection = NULL;
        }
        return;
    }

    // Report the logging that we're doing.
    send_report_logging(local_in_logfile_name, local_out_logfile_name,
                        remote_in_logfile_name, remote_out_logfile_name);
}

void vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging_status()
{
    char *local_in;
    char *local_out;
    char *remote_in;
    char *remote_out;
    d_logging_connection->get_log_names(&local_in, &local_out, &remote_in,
                                        &remote_out);
    send_report_logging(local_in, local_out, remote_in, remote_out);
    if (local_in) {
      try {
        delete[] local_in;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging_status: delete failed\n");
        return;
      }
    }
    if (local_out) {
      try {
        delete[] local_out;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging_status: delete failed\n");
        return;
      }
    }
    if (remote_in) {
      try {
        delete[] remote_in;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging_status: delete failed\n");
        return;
      }
    }
    if (remote_out) {
      try {
        delete[] remote_out;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_request_logging_status: delete failed\n");
        return;
      }
    }
}

void vrpn_Auxiliary_Logger_Remote::mainloop(void)
{
    if (d_connection) {
        d_connection->mainloop();
        client_mainloop();
    }
}

vrpn_Auxiliary_Logger_Remote::vrpn_Auxiliary_Logger_Remote(const char *name,
                                                           vrpn_Connection *c)
    : vrpn_Auxiliary_Logger(name, c)
{
    // Register a handler for the report callback from this device,
    // if we got a connection.
    if (d_connection != NULL) {
        if (register_autodeleted_handler(report_logging_m_id,
                                         handle_report_message, this,
                                         d_sender_id)) {
            fprintf(stderr,
                    "vrpn_Auxiliary_Logger_Remote: can't register handler\n");
            d_connection = NULL;
        }
    }
    else {
        fprintf(stderr,
                "vrpn_Auxiliary_Logger_Remote: Can't get connection!\n");
    }
}

/* Static */
int VRPN_CALLBACK
vrpn_Auxiliary_Logger_Remote::handle_report_message(void *userdata,
                                                    vrpn_HANDLERPARAM p)
{
    vrpn_Auxiliary_Logger_Remote *me =
        static_cast<vrpn_Auxiliary_Logger_Remote *>(userdata);
    char *localInName = NULL, *localOutName = NULL, *remoteInName = NULL,
         *remoteOutName = NULL;

    // Attempt to unpack the names from the buffer
    if (!me->unpack_log_message_from_buffer(p.buffer, p.payload_len,
                                            &localInName, &localOutName,
                                            &remoteInName, &remoteOutName)) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Remote::handle_report_message: "
                        "Could not unpack buffer\n");
        return -1;
    }

    // Fill in the data type for the callback handlers.
    vrpn_AUXLOGGERCB cs;
    cs.msg_time = p.msg_time;
    cs.local_in_logfile_name = localInName;
    cs.local_out_logfile_name = localOutName;
    cs.remote_in_logfile_name = remoteInName;
    cs.remote_out_logfile_name = remoteOutName;

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_callback_list.call_handlers(cs);

    // Clean up memory and return.
    if (localInName) {
      try {
        delete[] localInName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_report_message: delete failed\n");
        return -1;
      }
    };
    if (localOutName) {
      try {
        delete[] localOutName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_report_message: delete failed\n");
        return -1;
      }
    };
    if (remoteInName) {
      try {
        delete[] remoteInName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_report_message: delete failed\n");
        return -1;
      }
    };
    if (remoteOutName) {
      try {
        delete[] remoteOutName;
      } catch (...) {
        fprintf(stderr, "vrpn_Auxiliary_Logger_Server_Generic::handle_report_message: delete failed\n");
        return -1;
      }
    };
    return 0;
}
