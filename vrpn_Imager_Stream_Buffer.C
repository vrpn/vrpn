#include <stdio.h> // for fprintf, NULL, stderr, etc

#include "vrpn_BaseClass.h" // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Imager_Stream_Buffer.h"

vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer(
    const char *name, const char *imager_server_name, vrpn_Connection *c)
    : vrpn_Auxiliary_Logger_Server(name, c)
    , vrpn_Imager_Server(
          name, c, 0, 0) // Default number of rows and columns for the device.
    , d_logging_thread(NULL)
    , d_imager_server_name(NULL)
{
    // Copy the name of the server we are to connect to when we are logging.
    try { d_imager_server_name = new char[strlen(imager_server_name) + 1]; }
    catch (...) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: "
                        "Out of memory\n");
        d_connection = NULL;
        return;
    }
    strcpy(d_imager_server_name, imager_server_name);

    // Create the logging thread but do not run it yet.
    vrpn_ThreadData td;
    td.pvUD = this;
    try { d_logging_thread = new vrpn_Thread(static_logging_thread_func, td); }
    catch (...) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: "
                        "can't create logging thread\n");
        d_connection = NULL;
        return;
    }

    // Register a handler for the got first connection message.
    got_first_connection_m_id =
        d_connection->register_message_type(vrpn_got_first_connection);
    if (got_first_connection_m_id == -1) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: "
                        "can't register got first connection type\n");
        d_connection = NULL;
        return;
    }
    if (register_autodeleted_handler(got_first_connection_m_id,
                                     static_handle_got_first_connection, this,
                                     vrpn_ANY_SENDER)) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: "
                        "can't register got first connection handler\n");
        d_connection = NULL;
    }

    // The base server class implements throttling for us, so we could just go
    // ahead
    // and try to send the messages all the time using the normal frame
    // begin/end and
    // region routines.  If we do this, though, we're going to have to unpack
    // and repack
    // all of the messages. If we re-implement the throttling code, then we can
    // just
    // watch the packets as they go by and see what types they are, discarding
    // as
    // appropriate (but we still have to queue and watch them).
    //   If we implement the throttling
    // code down in the thread that listens to the server, we can avoid putting
    // them into the queue at all.  In that case, there can be a frame or more
    // in
    // the queue that would drain even after the throttle message was received.
    //   We can subtract the number of frames in the buffer from the request if
    //   the
    // request is large enough, thus removing the problem, but it won't work for
    // the common case of requesting 0 or 1 frames.  This will work in the
    // steady
    // state, where a sender requests one more each time it gets one, but there
    // will be an initial bolus of images.
    //    Nonetheless, this seems like the cleanest solution.  So, we will
    //    install
    // a handler for the throttling message that will pass it on down to the
    // thread
    // that is baby-sitting the server object.
    if (register_autodeleted_handler(d_throttle_frames_m_id,
                                     static_handle_throttle_message, this,
                                     vrpn_ANY_SENDER)) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::vrpn_Imager_Stream_Buffer: "
                        "can't register throttle handler\n");
        d_connection = NULL;
    }
}

vrpn_Imager_Stream_Buffer::~vrpn_Imager_Stream_Buffer()
{
    if (d_logging_thread) {
      stop_logging_thread();
      delete d_logging_thread;
      d_logging_thread = NULL;
    }
    if (d_imager_server_name) {
        try {
          delete[] d_imager_server_name;
        } catch (...) {
          fprintf(stderr, "vrpn_Imager_Stream_Buffer::~vrpn_Imager_Stream_Buffer(): delete failed\n");
          return;
        }
        d_imager_server_name = NULL;
    }
}

void vrpn_Imager_Stream_Buffer::mainloop(void)
{
    // Required from all servers
    server_mainloop();

    // See if we have a new image description from a logging thread.  If so,
    // fill in our values and send a description to any attached clients.
    const char *channelBuffer = NULL;
    if (d_shared_state.get_imager_description(d_nRows, d_nCols, d_nDepth,
                                              d_nChannels, &channelBuffer)) {
        int i;
        const char *bufptr = channelBuffer;
        for (i = 0; i < d_nChannels; i++) {
            d_channels[i].unbuffer(&bufptr);
        }
        try {
          delete[] const_cast<char *>(channelBuffer);
        } catch (...) {
          fprintf(stderr, "vrpn_Imager_Stream_Buffer::mainloop(): delete failed\n");
          return;
        }
        send_description();
    }

    // See if we have any messages waiting in the queue from the logging thread.
    // If we do, get an initial count and then send that many messages to the
    // client.  Don't go looking again this iteration or we may never return --
    // the server is quite possibly packing frames faster than we can send them.
    // Note that the messages in the queue have already been transcoded for our
    // and sender ID.
    unsigned count = d_shared_state.get_logger_to_client_queue_size();
    if (count) {
        unsigned i;
        for (i = 0; i < count; i++) {
            // Read the next message from the queue.
            vrpn_HANDLERPARAM p;
            if (!d_shared_state.retrieve_logger_to_client_message(&p)) {
                fprintf(stderr, "vrpn_Imager_Stream_Buffer::mainloop(): Could "
                                "not retrieve message from queue\n");
                break;
            }

            // Decrement the in-buffer frame count whenever we see a begin_frame
            // message.  This will un-block the way for later frames to be
            // buffered.
            if (p.type == d_begin_frame_m_id) {
                d_shared_state.decrement_frames_in_queue();
            }

            // Pack and send the message to the client, then delete the buffer
            // associated with the message.  Send them all reliably.  Send them
            // all using our sender ID.
            if (d_connection->pack_message(p.payload_len, p.msg_time, p.type,
                                           d_sender_id, p.buffer,
                                           vrpn_CONNECTION_RELIABLE) != 0) {
                fprintf(stderr, "vrpn_Imager_Stream_Buffer::mainloop(): Could "
                                "not pack message\n");
                break;
            }
            try {
              delete[] const_cast<char *>(p.buffer);
            } catch (...) {
              fprintf(stderr, "vrpn_Imager_Stream_Buffer::mainloop(): delete failed\n");
              return;
            }
        }
    }
}

/* static */
// This method just passes the call on to the virtual function.
int vrpn_Imager_Stream_Buffer::static_handle_got_first_connection(
    void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Imager_Stream_Buffer *me =
        static_cast<vrpn_Imager_Stream_Buffer *>(userdata);
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
        send_text_message(
            "handle_got_first_connection: Thread running when it should not be",
            now, vrpn_TEXT_ERROR);
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
        send_text_message(
            "handle_got_first_connection: Failed to start logging thread", now,
            vrpn_TEXT_ERROR);
        try {
          delete d_logging_thread;
        } catch (...) {
          fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_got_first_connection(): delete failed\n");
          d_logging_thread = NULL;
          return;
        }
        d_logging_thread = NULL;
        return;
    }
    struct timeval start, now;
    vrpn_gettimeofday(&start, NULL);
    do {
        const char *channelBuffer = NULL;
        if (d_shared_state.get_imager_description(
                d_nRows, d_nCols, d_nDepth, d_nChannels, &channelBuffer)) {
            int i;
            const char *bufptr = channelBuffer;
            for (i = 0; i < d_nChannels; i++) {
                d_channels[i].unbuffer(&bufptr);
            }
            try {
              delete[] const_cast<char *>(channelBuffer);
            } catch (...) {
              fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_got_first_connection(): delete failed\n");
              return;
            }
            return;
        }

        vrpn_SleepMsecs(1);
        vrpn_gettimeofday(&now, NULL);
    } while (vrpn_TimevalDiff(now, start).tv_sec < 3);

    // Timed out, so we won't be hearing from the server!
    vrpn_gettimeofday(&now, NULL);
    send_text_message("handle_got_first_connection: Didn't hear from server.",
                      now, vrpn_TEXT_WARNING);
}

// Handle dropped last connection on our primary connection by shutting down the
// connection to the imager server (killing the logging thread).
void vrpn_Imager_Stream_Buffer::handle_dropped_last_connection(void)
{
    if (!stop_logging_thread()) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_dropped_last_"
                        "connection(): Had to kill logging thread\n");
    }
}

// Handles a throttle request by passing it on down to the non-blocking
// thread to deal with.
int vrpn_Imager_Stream_Buffer::static_handle_throttle_message(
    void *userdata, vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Stream_Buffer *me =
        static_cast<vrpn_Imager_Stream_Buffer *>(userdata);

    // Get the requested number of frames from the buffer
    vrpn_int32 frames_to_send;
    if (vrpn_unbuffer(&bufptr, &frames_to_send)) {
        return -1;
    }

    me->d_shared_state.set_throttle_request(frames_to_send);
    return 0;
}

// The function that is called to become the logging thread.  It is passed
// a pointer to "this" so that it can acces the object that created it.
// The static function basically just unpacks the "this" pointer and
// then calls the non-static function.
void vrpn_Imager_Stream_Buffer::static_logging_thread_func(
    vrpn_ThreadData &threadData)
{
    vrpn_Imager_Stream_Buffer *me =
        static_cast<vrpn_Imager_Stream_Buffer *>(threadData.pvUD);
    me->logging_thread_func();
}

// Note that it must use semaphores to get at the data that will be shared
// between the main thread and itself.
// This function does all the work of the logging thread, including all
// interactions with the vrpn_Imager_Server connection(s) and the client
// object; it forwards information both ways to the main thread that is
// communicating with the end-user client connection.
// DO NOT CALL VRPN message sends on the client object's connection from
// this function or those it calls, because we're not the thread that is
// connected to the client object's connection and such calls are not
// thread-safe.
// Instead, pass the data needed to make the calls to the initial thread.
void vrpn_Imager_Stream_Buffer::logging_thread_func(void)
{
    // Initialize everything to a clean state.
    d_log_connection = NULL;
    d_imager_remote = NULL;
    d_server_dropped_due_to_throttle = 0; // None dropped yet!
    d_server_frames_to_send = -1;         // Send as many as you get

    // Open a connection to the server object, not asking it to log anything.
    // (Logging will be started later if we receive a message from our client.)
    // Create a vrpn_Imager_Remote object and set its callbacks to fill things
    // into the shared data structures.
    d_log_connection = open_new_log_connection("", "", "", "");
    if (d_log_connection == NULL) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::logging_thread_func(): "
                        "Cannot open connection\n");
        return;
    }
    if (!setup_handlers_for_logging_connection(d_log_connection)) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::logging_thread_func(): "
                        "Cannot set up handlers\n");
        return;
    }

    // Keep doing mainloop() on the client object(s) and checking
    // for commands that we're supposed to issue until we're
    // told that we're supposed to die.  Sleep a little between iterations
    // to avoid eating CPU time.
    while (!d_shared_state.time_to_exit()) {
        // Check to see if the client has sent a new throttling message.
        // If so, then adjust our state accordingly.  Note that we are
        // duplicating
        // the effort of the vrpn_Imager_Server base class here; it will still
        // be
        // keeping its own shadow copy of these values, but will not be doing
        // anything
        // with them because we'll never be calling its send routines.  This
        // duplicates
        // the code in the handle_throttle_message() in the vrpn_Imager_Server
        // base
        // class.
        vrpn_int32 frames_to_send;
        if (d_shared_state.get_throttle_request(&frames_to_send)) {
            // If the requested number of frames is negative, then we set
            // for unbounded sending.  The next time a begin_frame message
            // arrives, it will start the process going again.
            if (frames_to_send < 0) {
                d_server_frames_to_send = -1;

                // If we were sending continuously, store the number of frames
                // to send.  Decrement by the number of frames already in the
                // outgoing buffer, but don't let the number go below zero.
            }
            else if (d_server_frames_to_send == -1) {
                int frames_in_queue = d_shared_state.get_frames_in_queue();
                if (frames_to_send >= frames_in_queue) {
                    frames_to_send -= frames_in_queue;
                }
                d_server_frames_to_send = frames_to_send;

                // If we already had a throttle limit set, then increment it
                // by the count.
            }
            else {
                d_server_frames_to_send += frames_to_send;
            }
        }

        // Check to see if we've been asked to create new log files.  If we
        // have,
        // then attempt to do so.  If that works, pass back the names of the
        // files
        // created to the initial thread.  If it did not work, return empty
        // log-file
        // names.
        char *lil, *lol, *ril, *rol;
        if (d_shared_state.get_logfile_request(&lil, &lol, &ril, &rol)) {
            if (make_new_logging_connection(lil, lol, ril, rol)) {
                d_shared_state.set_logfile_result(lil, lol, ril, rol);
            }
            else {
                d_shared_state.set_logfile_result("", "", "", "");
            }
            // Delete the allocated space only if there were return values.
            try {
              delete[] lil;
              delete[] lol;
              delete[] ril;
              delete[] rol;
            } catch (...) {
              fprintf(stderr, "vrpn_Imager_Stream_Buffer::logging_thread_func(): delete failed\n");
              return;
            }
        }

        // Handle all of the messages coming from the server.
        if (d_imager_remote) {
            d_imager_remote->mainloop();
        }
        if (d_log_connection) {
            d_log_connection->mainloop();
            d_log_connection->save_log_so_far();
        }

        vrpn_SleepMsecs(1);
    }

    // Now that we've been told to die, clean up everything and return.
    if (d_imager_remote) {
        try {
          delete d_imager_remote;
        } catch (...) {
          fprintf(stderr, "vrpn_Imager_Stream_Buffer::logging_thread_func(): delete failed\n");
          return;
        }
        d_imager_remote = NULL;
    }
    if (d_log_connection) {
        d_log_connection->removeReference();
        d_log_connection = NULL;
    }
}

/* static */
int VRPN_CALLBACK
vrpn_Imager_Stream_Buffer::static_handle_server_messages(void *pvISB,
                                                         vrpn_HANDLERPARAM p)
{
    vrpn_Imager_Stream_Buffer *me =
        static_cast<vrpn_Imager_Stream_Buffer *>(pvISB);
    return me->handle_server_messages(p);
}

int vrpn_Imager_Stream_Buffer::handle_server_messages(
    const vrpn_HANDLERPARAM &p)
{
    // Handle begin_frame message very specially, because it has all sorts
    // of interactions with throttling and missed-frame reporting.
    if (p.type == d_server_begin_frame_m_id) {
        // This duplicates code from the send_begin_frame() method in
        // the vrpn_Imager_Server base class that handles throttling.
        // It further adds code to handle throttling when the queue to
        // the initial thread is too full.

        // If we are throttling frames and the frame count has gone to zero,
        // then increment the number of frames missed and do not add this
        // message to the queue.
        if (d_server_frames_to_send == 0) {
            d_server_dropped_due_to_throttle++;
            return 0;
        }

        // If there are too many frames in the queue already,
        // add one to the number lost due to throttling (which
        // will prevent region and end-of-frame messages until the next
        // begin_frame message) and break without forwarding the message.
        if (d_shared_state.get_frames_in_queue() >= 2) {
            d_server_dropped_due_to_throttle++;
            return 0;
        }

        // If we missed some frames due to throttling, but are now
        // sending frames again, report how many we lost due to
        // throttling.  This is incremented both for client-requested
        // throttling and to queue-overflow throttling.
        if (d_server_dropped_due_to_throttle > 0) {
            // We create a new message header and body, using the server's
            // type IDs, and then transcode and send the message through
            // the initial connection.
            vrpn_HANDLERPARAM tp = p;
            vrpn_float64
                fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
            char *msgbuf = (char *)fbuf;
            int buflen = sizeof(fbuf);
            tp.type = d_server_discarded_frames_m_id;

            if (vrpn_buffer(&msgbuf, &buflen,
                            d_server_dropped_due_to_throttle)) {
                fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_server_"
                                "messages: Can't pack count\n");
                return -1;
            }
            tp.buffer = static_cast<char *>(static_cast<void *>(fbuf));
            tp.payload_len = sizeof(fbuf) - buflen;

            if (!transcode_and_send(tp)) {
                fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_server_"
                                "messages: Can't send discarded frames "
                                "count\n");
                return -1;
            }

            d_server_dropped_due_to_throttle = 0;
        }

        // If we are throttling, then decrement the number of frames
        // left to send.
        if (d_server_frames_to_send > 0) {
            d_server_frames_to_send--;
        }

        // No throttling going on, so add the message to the outgoing queue and
        // also increment the count of how many outstanding frames are in the
        // queue.
        if (!transcode_and_send(p)) {
            fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_server_messages:"
                            " Can't transcode and send\n");
            return -1;
        }
        d_shared_state.increment_frames_in_queue();

        // Handle the end_frame and all of the region messages in a similar
        // manner,
        // dropping them if throttling is going on and passing them on if not.
        // This duplicates code from the send_end_frame() and the region
        // send methods in the vrpn_Imager_Server base class that handles
        // throttling.
    }
    else if ((p.type == d_server_end_frame_m_id) ||
             (p.type == d_server_regionu8_m_id) ||
             (p.type == d_server_regionu12in16_m_id) ||
             (p.type == d_server_regionu16_m_id) ||
             (p.type == d_server_regionf32_m_id)) {

        // If we are discarding frames, do not queue this message.
        if (d_server_dropped_due_to_throttle > 0) {
            return 0;
        }

        // No throttling going on, so add this message to the outgoing queue.
        if (!transcode_and_send(p)) {
            fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_server_messages:"
                            " Can't transcode and send\n");
            return -1;
        }

        // Send these messages on without modification
        // (Currently, these are description messages and discarded-frame
        // messages.  It also includes the generic pong response and any
        // text messages.)
    }
    else if ((p.type == d_server_description_m_id) ||
             (p.type == d_server_discarded_frames_m_id) ||
             (p.type == d_server_text_m_id) || (p.type == d_server_pong_m_id)) {
        if (!transcode_and_send(p)) {
            fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_server_messages:"
                            " Can't transcode and send\n");
            return -1;
        }

        // Ignore these messages without passing them on.
    }
    else if ((p.type == d_server_ping_m_id)) {
        return 0;

        // We need to throw a warning here on unexpected types so that we get
        // some
        // warning if additional messages are added.  This code is fragile
        // because it
        // relies on us knowing the types of base-level and imager messages and
        // catching
        // them all.  This way, at least we'll know if we miss one.
    }
    else {
        // We create a new message header and body, using the server's
        // type IDs, and then transcode and send the message through
        // the initial connection.  This is a text message saying that we
        // got a message of unknown type.
        vrpn_HANDLERPARAM tp = p;
        char buffer[2 * sizeof(vrpn_int32) + vrpn_MAX_TEXT_LEN];
        char msg[vrpn_MAX_TEXT_LEN];
        tp.type = d_server_text_m_id;
        tp.buffer = buffer;
        tp.payload_len = sizeof(buffer);
        sprintf(msg, "Unknown message type from server: %d",
                static_cast<int>(p.type));
        encode_text_message_to_buffer(buffer, vrpn_TEXT_ERROR, 0, msg);
        if (!transcode_and_send(tp)) {
            fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_server_messages:"
                            " Can't transcode text message\n");
            return -1;
        }
    }

    return 0;
}

// Transcode the sender and type fields from the logging server connection to
// the initial client connection and pack the resulting message into the queue
// from the logging thread to the initial thread.  The data buffer is copied;
// this space is allocated by the logging thread and must be freed by the
// initial thread.
// Returns true on success and false on failure.  The sender is set to the
// d_sender_id of our server object.
bool vrpn_Imager_Stream_Buffer::transcode_and_send(const vrpn_HANDLERPARAM &p)
{
    // Copy the contents of the buffer to a newly-allocated one that will be
    // passed to the initial thread.
    char *newbuf;
    try { newbuf = new char[p.payload_len]; }
    catch (...) {
        fprintf(
            stderr,
            "vrpn_Imager_Stream_Buffer::transcode_and_send(): Out of memory\n");
        d_connection = NULL;
        return false;
    }
    memcpy(newbuf, p.buffer, p.payload_len);

    // Copy the contents of the handlerparam to a newly-allocated one that will
    // be passed to the initial thread.  Change the sender to match ours, set
    // the
    // buffer pointer to the new buffer, and transcode the type.
    vrpn_HANDLERPARAM newp = p;
    newp.buffer = newbuf;
    newp.sender = d_sender_id;
    newp.type = transcode_type(p.type);
    if (newp.type == -1) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::transcode_and_send(): "
                        "Unknown type (%d)\n",
                static_cast<int>(p.type));
        try {
          delete[] newbuf;
        } catch (...) {
          fprintf(stderr, "vrpn_Imager_Stream_Buffer::transcode_and_send(): delete failed\n");
          return false;
        }
        return false;
    }

    // Add the message to the queue of messages going to the initial thread.
    if (!d_shared_state.insert_logger_to_client_message(newp)) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::transcode_and_send(): "
                        "Can't queue message\n");
        return false;
    }

    return true;
}

// Transcode the type from the logging thread's connection type to
// the initial thread's connection type.  Return -1 if we don't
// recognize the type.
vrpn_int32 vrpn_Imager_Stream_Buffer::transcode_type(vrpn_int32 type)
{
    if (type == d_server_description_m_id) {
        return d_description_m_id;
    }
    else if (type == d_server_begin_frame_m_id) {
        return d_begin_frame_m_id;
    }
    else if (type == d_server_end_frame_m_id) {
        return d_end_frame_m_id;
    }
    else if (type == d_server_discarded_frames_m_id) {
        return d_discarded_frames_m_id;
    }
    else if (type == d_server_regionu8_m_id) {
        return d_regionu8_m_id;
    }
    else if (type == d_server_regionu12in16_m_id) {
        return d_regionu12in16_m_id;
    }
    else if (type == d_server_regionu16_m_id) {
        return d_regionu16_m_id;
    }
    else if (type == d_server_regionf32_m_id) {
        return d_regionf32_m_id;
    }
    else if (type == d_server_text_m_id) {
        return d_text_message_id;
    }
    else if (type == d_server_ping_m_id) {
        return d_ping_message_id;
    }
    else if (type == d_server_pong_m_id) {
        return d_pong_message_id;
    }
    else {
        return -1;
    }
}

vrpn_Connection *vrpn_Imager_Stream_Buffer::open_new_log_connection(
    const char *local_in_logfile_name, const char *local_out_logfile_name,
    const char *remote_in_logfile_name, const char *remote_out_logfile_name)
{
    vrpn_Connection *ret = NULL;

    // Find the relevant part of the name (skip past last '@'
    // if there is one); also find the port number.
    const char *cname = d_imager_server_name;
    const char *where_at; // Part of name past last '@'
    if ((where_at = strrchr(cname, '@')) != NULL) {
        cname = where_at + 1; // Chop off the front of the name
    }

    // Pass "true" to force_connection so that it will open a new
    // connection even if we already have one with that name.
    ret = vrpn_get_connection_by_name(
        where_at, local_in_logfile_name, local_out_logfile_name,
        remote_in_logfile_name, remote_out_logfile_name, NULL, true);
    if (!ret || !ret->doing_okay()) {
        struct timeval now;
        vrpn_gettimeofday(&now, NULL);
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::open_new_log_connection: "
                        "Could not create connection (files already exist?)");
        if (ret) {
            try {
              delete ret;
            } catch (...) {
              fprintf(stderr, "vrpn_Imager_Stream_Buffer::open_new_log_connection(): delete failed\n");
              return NULL;
            }
            return NULL;
        }
    }

    return ret;
}

bool vrpn_Imager_Stream_Buffer::setup_handlers_for_logging_connection(
    vrpn_Connection *c)
{
    // Create a vrpn_Imager_Remote on this connection and set its callbacks so
    // that they will do what needs doing; the callbacks point to the
    // Imager_Stream_Buffer object, not to the imager_remote object; access it
    // through the member variable pointer.
    d_imager_remote = new vrpn_Imager_Remote(d_imager_server_name, c);
    if (d_imager_remote == NULL) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::setup_handlers_for_logging_"
                        "connection(): Cannot create vrpn_Imager_Remote\n");
        return false;
    }
    d_imager_remote->register_description_handler(this,
                                                  handle_image_description);

    // Figure out the remote type IDs from the server for the messages we want
    // to forward.  This is really dangerous, because we need to make sure to
    // explicitly list all the ones we might need.  If we forget an important
    // one (or it gets added later to either the base class or the imager class)
    // then it won't get forwarded.
    d_server_description_m_id =
        c->register_message_type("vrpn_Imager Description");
    d_server_begin_frame_m_id =
        c->register_message_type("vrpn_Imager Begin_Frame");
    d_server_end_frame_m_id = c->register_message_type("vrpn_Imager End_Frame");
    d_server_discarded_frames_m_id =
        c->register_message_type("vrpn_Imager Discarded_Frames");
    d_server_regionu8_m_id = c->register_message_type("vrpn_Imager Regionu8");
    d_server_regionu16_m_id = c->register_message_type("vrpn_Imager Regionu16");
    d_server_regionu12in16_m_id =
        c->register_message_type("vrpn_Imager Regionu12in16");
    d_server_regionf32_m_id = c->register_message_type("vrpn_Imager Regionf32");
    d_server_text_m_id = c->register_message_type("vrpn_Base text_message");
    d_server_ping_m_id = c->register_message_type("vrpn_Base ping_message");
    d_server_pong_m_id = c->register_message_type("vrpn_Base pong_message");

    // Set up handlers for the other messages from the server so that they can
    // be passed on up to the initial thread and on to the client as
    // appropriate.
    // Be sure to strip the "@" part off the device name before registering the
    // sender
    // so that it is the same as the one used by the d_imager_remote.
    c->register_handler(
        vrpn_ANY_TYPE, static_handle_server_messages, this,
        c->register_sender(vrpn_copy_service_name(d_imager_server_name)));

    return true;
}

bool vrpn_Imager_Stream_Buffer::teardown_handlers_for_logging_connection(
    vrpn_Connection *c)
{
    if (!d_imager_remote) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::teardown_handlers_for_"
                        "logging_connection(): No imager remote\n");
        return false;
    }
    if (d_imager_remote->unregister_description_handler(
            this, handle_image_description) != 0) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::teardown_handlers_for_"
                        "logging_connection(): Cannot unregister handler\n");
        return false;
    }

    // Tear down handlers for the other messages from the server.
    c->unregister_handler(
        vrpn_ANY_TYPE, static_handle_server_messages, this,
        c->register_sender(vrpn_copy_service_name(d_imager_server_name)));

    try {
      delete d_imager_remote;
    } catch (...) {
      fprintf(stderr, "vrpn_Imager_Stream_Buffer::teardown_handlers_for_logging_connection(): delete failed\n");
      return false;
    }
    d_imager_remote = NULL;
    return true;
}

bool vrpn_Imager_Stream_Buffer::make_new_logging_connection(
    const char *local_in_logfile_name, const char *local_out_logfile_name,
    const char *remote_in_logfile_name, const char *remote_out_logfile_name)
{
    // Open a new connection to do logging on before deleting the old one so
    // that we keep at least one connection open to the server at all time.
    // This will prevent it from doing its "dropped last connection" things
    // which will include resetting the imager server.
    vrpn_Connection *new_log_connection = open_new_log_connection(
        local_in_logfile_name, local_out_logfile_name, remote_in_logfile_name,
        remote_out_logfile_name);
    if (new_log_connection == NULL) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::make_new_logging_"
                        "connection(): Cannot open connection\n");
        return false;
    }

    // Unhook the callbacks from the existing logging connection so that
    // we don't end up with two callbacks for each message.
    if (!teardown_handlers_for_logging_connection(d_log_connection)) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::make_new_logging_"
                        "connection(): Cannot teardown connection\n");
        return false;
    }

    // Hook the callbacks up to the new connection so that we will get reports
    // from the server.
    if (!setup_handlers_for_logging_connection(new_log_connection)) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::make_new_logging_"
                        "connection(): Cannot setup connection\n");
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
    struct timeval start, now;
    vrpn_gettimeofday(&start, NULL);
    now = start;
    d_ready_to_drop_old_connection = false;
    while (!d_ready_to_drop_old_connection &&
           (vrpn_TimevalDiff(now, start).tv_sec < 3)) {
        new_log_connection->mainloop(); // Enable connection set-up to occur
        new_log_connection->save_log_so_far();
        d_log_connection->mainloop(); // Eat up (and log) any incoming messages
        d_log_connection->save_log_so_far();
        vrpn_gettimeofday(&now, NULL);
        vrpn_SleepMsecs(1);
    };
    if (!d_ready_to_drop_old_connection) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::make_new_logging_"
                        "connection(): Could not connect new logging "
                        "connection\n");
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

void vrpn_Imager_Stream_Buffer::handle_request_logging(
    const char *local_in_logfile_name, const char *local_out_logfile_name,
    const char *remote_in_logfile_name, const char *remote_out_logfile_name)
{
    // Request that the logging thread start new logs.
    d_shared_state.set_logfile_request(
        local_in_logfile_name, local_out_logfile_name, remote_in_logfile_name,
        remote_out_logfile_name);

    // Wait until we hear back from the logging thread or time out;
    // return empty if timeout and the strings we got back if not.
    // Remember to deallocated the memory if we got a response.
    struct timeval start, now;
    vrpn_gettimeofday(&start, NULL);
    do {
        char *lil, *lol, *ril, *rol;
        if (d_shared_state.get_logfile_result(&lil, &lol, &ril, &rol)) {
            send_report_logging(lil, lol, ril, rol);
            try {
              delete[] lil;
              delete[] lol;
              delete[] ril;
              delete[] rol;
            } catch (...) {
              fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_request_logging(): delete failed\n");
              return;
            }
            return;
        }
        vrpn_SleepMsecs(1);
        vrpn_gettimeofday(&now, NULL);
    } while (vrpn_TimevalDiff(now, start).tv_sec < 2);

    // Timeout, report failure of logging by saying that there are empty log
    // file names.
    send_report_logging("", "", "", "");
}

void vrpn_Imager_Stream_Buffer::handle_request_logging_status()
{
    char *local_in;
    char *local_out;
    char *remote_in;
    char *remote_out;
    d_shared_state.get_logfile_names(&local_in, &local_out, &remote_in,
                                     &remote_out);
    send_report_logging(local_in, local_out, remote_in, remote_out);
    try {
      if (local_in) delete[] local_in;
      if (local_out) delete[] local_out;
      if (remote_in) delete[] remote_in;
      if (remote_out) delete[] remote_out;
    } catch (...) {
      fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_request_logging_status(): delete failed\n");
      return;
    }
}

/* Static */
// We've gotten a new imager description, so fill it into the shared data
// structure
// so that the parent object can hear about it.
void vrpn_Imager_Stream_Buffer::handle_image_description(
    void *pvISB, const struct timeval)
{
    vrpn_Imager_Stream_Buffer *me =
        static_cast<vrpn_Imager_Stream_Buffer *>(pvISB);

    // Pack up description messages for all of the channels into a buffer that
    // is at
    // least large enough to hold them all.
    // msgbuf must be float64-aligned!
    vrpn_float64 *fbuf;
    try { fbuf = new vrpn_float64[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)]; }
    catch (...) {
        fprintf(stderr, "vrpn_Imager_Stream_Buffer::handle_image_description():"
                        " Out of memory\n");
        me->d_shared_state.time_to_exit(true);
        me->d_connection = NULL;
        return;
    }
    char *buffer = reinterpret_cast<char *>(fbuf);
    int i;
    char *bufptr = buffer;
    vrpn_int32 buflen = sizeof(vrpn_float64) * vrpn_CONNECTION_TCP_BUFLEN /
                        sizeof(vrpn_float64);
    for (i = 0; i < me->d_imager_remote->nChannels(); i++) {
        me->d_imager_remote->channel(i)->buffer(&bufptr, &buflen);
    }

    me->d_shared_state.set_imager_description(
        me->d_imager_remote->nRows(), me->d_imager_remote->nCols(),
        me->d_imager_remote->nDepth(), me->d_imager_remote->nChannels(),
        buffer);

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
