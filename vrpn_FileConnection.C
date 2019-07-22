// vrpn_FileConnection.C

// {{{ includes and defines

#include "vrpn_FileConnection.h"

#ifndef _WIN32_WCE
#include <fcntl.h> // for SEEK_SET
#endif
#include <limits.h> // for LONG_MAX, LONG_MIN
#include <stdio.h>  // for NULL, fprintf, stderr, etc
#include <string.h> // for memcpy

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h
// and netinet/in.h and ...
#include "vrpn_Shared.h" // for timeval, etc
#if !(defined(_WIN32) && defined(VRPN_USE_WINSOCK_SOCKETS))
#include <netinet/in.h> // for ntohl
#endif

// Global variable used to indicate whether File Connections should
// pre-load all of their records into memory when opened.  This is the
// default behavior, but fails on very large files that eat up all
// of the memory.
// This is used to initialize the data member for each new file connection
// so that it will do what is expected.  This setting is stored per file
// connection so that a given file connection will behave consistently.

bool vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD = true;

// Global variable used to indicate whether File Connections should
// keep already-read messages stored in memory.  If not, then we have
// to re-load the file starting at the beginning on rewind.
// This is used to initialize the data member for each new file connection
// so that it will do what is expected.  This setting is stored per file
// connection so that a given file connection will behave consistently.

bool vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE = true;

// Global variable used to indicate whether File Connections should
// play through all system messages and get to the first user message
// when opened or reset to the beginning.  This defaults to "true".
// User code should set this
// to "false" before calling vrpn_open_client_connection() or creating
// a new vrpn_File_Connection object if it wants that file connection
// to not skip the messages.  The value is only checked at connection creation
// time;
// the connection behaves consistently once created.  Leaving this true
// can help with offsets in time that happen at the beginning of files.

bool vrpn_FILE_CONNECTIONS_SHOULD_SKIP_TO_USER_MESSAGES = true;

#define CHECK(x)                                                               \
    if (x == -1) return -1

#include "vrpn_Log.h" // for vrpn_Log

struct timeval;

// }}}
// {{{ constructor

vrpn_File_Connection::vrpn_File_Connection(const char *station_name,
                                           const char *local_in_logfile_name,
                                           const char *local_out_logfile_name)
    : vrpn_Connection(local_in_logfile_name, local_out_logfile_name, NULL, NULL)
    , d_controllerId(register_sender("vrpn File Controller"))
    , d_set_replay_rate_type(register_message_type("vrpn_File set_replay_rate"))
    , d_reset_type(register_message_type("vrpn_File reset"))
    , d_play_to_time_type(register_message_type("vrpn_File play_to_time"))
    , d_fileName(NULL)
    , d_file(NULL)
    , d_logHead(NULL)
    , d_logTail(NULL)
    , d_currentLogEntry(NULL)
    , d_startEntry(NULL)
    , d_preload(vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD)
    , d_accumulate(vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE)
{
    d_last_told.tv_sec = 0;
    d_last_told.tv_usec = 0;

    d_earliest_user_time.tv_sec = d_earliest_user_time.tv_usec = 0;
    d_earliest_user_time_valid = false;
    d_highest_user_time.tv_sec = d_highest_user_time.tv_usec = 0;
    d_highest_user_time_valid = false;

    // Because we are a file connection, our status should be CONNECTED
    // Later set this to BROKEN if there is a problem opening/reading the file.
    if (!d_endpoints.is_valid(0)) {
        fprintf(stderr, "vrpn_File_Connection::vrpn_File_Connection(): NULL "
                        "zeroeth endpoint\n");
    } else {
        connectionStatus = CONNECTED;
        d_endpoints.front()->status = CONNECTED;
    }

    // If we are preloading, then we must accumulate messages.
    if (d_preload) {
        d_accumulate = true;
    }

    // These are handlers for messages that may be sent from a
    // vrpn_File_Controller object that may attach itself to us.
    register_handler(d_set_replay_rate_type, handle_set_replay_rate, this,
                     d_controllerId);
    register_handler(d_reset_type, handle_reset, this, d_controllerId);
    register_handler(d_play_to_time_type, handle_play_to_time, this,
                     d_controllerId);

    // necessary to initialize properly in mainloop()
    d_last_time.tv_usec = d_last_time.tv_sec = 0;

    d_fileName = vrpn_copy_file_name(station_name);
    if (!d_fileName) {
        fprintf(stderr, "vrpn_File_Connection:  Out of memory!\n");
        connectionStatus = BROKEN;
        return;
    }

    d_file = fopen(d_fileName, "rb");
    if (!d_file) {
        fprintf(stderr, "vrpn_File_Connection:  "
                        "Could not open file \"%s\".\n",
                d_fileName);
        connectionStatus = BROKEN;
        return;
    }

    // Read the cookie from the file.  It will print an error message if it
    // can't read it, so we just pass the broken status on up the chain.
    if (read_cookie() < 0) {
        connectionStatus = BROKEN;
        return;
    }

    // If we are supposed to preload the entire file into memory buffers,
    // then keep reading until we get to the end.  Otherwise, just read the
    // first message to get things going.
    if (d_preload) {
        while (!read_entry()) {
        }
    }
    else {
        read_entry();
    }

    // Initialize the "current message" pointer to the first log-file
    // entry that was read, and set the start time for the file and
    // the current time to the one in this message.
    if (d_logHead) {
        d_currentLogEntry = d_logHead;
        d_startEntry = d_logHead;
        d_start_time = d_startEntry->data.msg_time;
        d_time = d_start_time;
    }
    else {
        fprintf(stderr, "vrpn_File_Connection: Can't read first message\n");
        connectionStatus = BROKEN;
        return;
    }

    // This is useful to play the initial system messages
    // (the sender/type ones) automatically.  These might not be
    // time synched so if we don't play them automatically they
    // can mess up playback if their timestamps are later than
    // the first user message.
    if (vrpn_FILE_CONNECTIONS_SHOULD_SKIP_TO_USER_MESSAGES) {
        play_to_user_message();
        if (d_currentLogEntry) {
            d_start_time = d_currentLogEntry->data.msg_time;
            d_time = d_start_time;
        }
    }

    // Add this to the list of known connections.
    vrpn_ConnectionManager::instance().addConnection(this, station_name);
}

// }}}
// {{{ play_to_user_message

// Advances through the file, calling callbacks, up until
// a user message (type >= 0) is encountered)
void vrpn_File_Connection::play_to_user_message(void)
{
    // As long as the current message is a system message, play it.
    // Also stop if we get to the end of the file.
    while (d_currentLogEntry && (d_currentLogEntry->data.type < 0)) {
        playone();
    }

    // we advance d_time one ahead because they're may be
    // a large gap in the file (forward or backwards) between
    // the last system message and first user one
    if (d_currentLogEntry) {
        d_time = d_currentLogEntry->data.msg_time;
    }
}

// }}}
// {{{ destructor

// virtual
vrpn_File_Connection::~vrpn_File_Connection(void)
{
    vrpn_LOGLIST *np;

    // Remove myself from the "known connections" list
    //   (or the "anonymous connections" list).
    vrpn_ConnectionManager::instance().deleteConnection(this);

    close_file();
    try {
      delete[] d_fileName;
    } catch (...) {
      fprintf(stderr, "vrpn_File_Connection::~vrpn_File_Connection: delete failed\n");
      return;
    }
    d_fileName = NULL;

    // Delete any messages that are in memory, and their data buffers.
    while (d_logHead) {
        np = d_logHead->next;
        if (d_logHead->data.buffer) {
            try {
              delete[] d_logHead->data.buffer;
            } catch (...) {
              fprintf(stderr, "vrpn_File_Connection::~vrpn_File_Connection: delete failed\n");
              return;
            }
        }
        try {
          delete d_logHead;
        } catch (...) {
          fprintf(stderr, "vrpn_File_Connection::~vrpn_File_Connection: delete failed\n");
          return;
        }
        d_logHead = np;
    }
}

// }}}
// {{{ jump_to_time

// newtime is an elapsed time from the start of the file
int vrpn_File_Connection::jump_to_time(vrpn_float64 newtime)
{
    return jump_to_time(vrpn_MsecsTimeval(newtime * 1000));
}

// If the time is before our current time (or there is no current
// time) then reset back to the beginning.  Whether or not we did
// that, search forwards until we get to or past the time we are
// searching for.
// newtime is an elapsed time from the start of the file
int vrpn_File_Connection::jump_to_time(timeval newtime)
{
    if (d_earliest_user_time_valid) {
        d_time = vrpn_TimevalSum(d_earliest_user_time, newtime);
    }
    else {
        d_time = vrpn_TimevalSum(d_start_time, newtime);
    } // XXX get rid of this option - dtm

    // If the time is earlier than where we are, or if we have
    // run past the end (no current entry), jump back to
    // the beginning of the file before searching.
    if (!d_currentLogEntry ||
        vrpn_TimevalGreater(d_currentLogEntry->data.msg_time, d_time)) {
        reset();
    }

    // Search forwards, as needed.  Do not play the messages as they are
    // passed, just skip over them until we get to a message that has a
    // time greater than or equal to the one we are looking for.  That is,
    // one whose time is not less than ours.
    while (!vrpn_TimevalGreater(d_currentLogEntry->data.msg_time, d_time)) {
        int ret = advance_currentLogEntry();
        if (ret != 0) {
            return 0; // Didn't get where we were going!
        }
    }

    return 1; // Got where we were going!
}

int vrpn_File_Connection::jump_to_filetime(timeval absolute_time)
{
    if (d_earliest_user_time_valid) {
        return jump_to_time(
            vrpn_TimevalDiff(absolute_time, d_earliest_user_time));
    }
    else {
        return jump_to_time(vrpn_TimevalDiff(absolute_time, d_start_time));
    } // XX get rid of this option - dtm
}

// }}}
// {{{ vrpn_File_Connection::FileTime_Accumulator
vrpn_File_Connection::FileTime_Accumulator::FileTime_Accumulator()
    : d_replay_rate(1.0)
{
    d_filetime_accum_since_last_playback.tv_sec = 0;
    d_filetime_accum_since_last_playback.tv_usec = 0;

    d_time_of_last_accum.tv_sec = 0;
    d_time_of_last_accum.tv_usec = 0;
}

void vrpn_File_Connection::FileTime_Accumulator::accumulate_to(
    const timeval &now_time)
{
    timeval &accum = d_filetime_accum_since_last_playback;
    timeval &last_accum = d_time_of_last_accum;

    accum                  // updated elapsed filetime
        = vrpn_TimevalSum( // summed with previous elapsed time
            accum,
            vrpn_TimevalScale(    // scaled by replay rate
                vrpn_TimevalDiff( // elapsed wallclock time
                    now_time, d_time_of_last_accum),
                d_replay_rate));
    last_accum = now_time; // wallclock time of this whole mess
}

void vrpn_File_Connection::FileTime_Accumulator::set_replay_rate(
    vrpn_float32 new_rate)
{
    timeval now_time;
    vrpn_gettimeofday(&now_time, NULL);
    accumulate_to(now_time);

    d_replay_rate = new_rate;
    // fprintf(stderr, "Set replay rate!\n");
}

void vrpn_File_Connection::FileTime_Accumulator::reset_at_time(
    const timeval &now_time)
{
    // this function is confusing.  It doesn't appear to do anything.
    // (I added the next three lines. did I delete what was previously there?)
    d_filetime_accum_since_last_playback.tv_sec = 0;
    d_filetime_accum_since_last_playback.tv_usec = 0;
    d_time_of_last_accum = now_time;
}
// {{{ end vrpn_File_Connection::FileTime_Accumulator
// }}}

// }}}
// {{{ mainloop

// {{{ -- comment

// [juliano 10/11/99] the problem described below is now fixed
// [juliano 8/26/99]  I believe there to be a bug in mainloop.
//
//     Essentially, the computation of end_time is sample-and-hold
//     integration, using the value of d_rate at the end of the integration
//     window.  Consider the case where you are happily playing from your
//     file, then pause it.  Pausing is accomplished by setting d_rate to
//     zero.  Then, 10 minutes later, you unpause by setting d_rate to one.
//
//     Now, you have an integration window of 10 minutes.  The way it's
//     currently implemented, you will compute end_time as d_rate * 10
//     minutes.  This is obviously not correct.
//
//     So, what is the ideal way this would work?  Well, if you had continuous
//     integration and asynchronous events, then the new message would come at
//     the same time as it would if we executed the same scenerio with a VCR.
//
//     Since our value of d_rate changes by impulses at specified times (it
//     only changes from inside set_replay_rate) sample-and-hold integration
//     can still be used to do exact computation.  What we need to do is
//     accumulate "virtial time", and compare with it.  By "virtual time" I
//     mean the analog to VITC timestamps on a videotape in a VCR.  We will
//     accumulate it in d_virtual_time_elapsed_since_last_event.  The units
//     need to have enough precision that we won't run into any of the
//     problems that have already been worked around in the code below.  It
//     represents the amount of time elapsed sice d_last_time was last set.
//
//     set_replay_rate will do sample-and-hold integration over the window
//     from d_last_time to the result of vrpn_gettimeofday(), and it will
//     accumulate the result into d_virtual_time_elapsed_since_last_event.
//     then it will set d_last_time to the result of vrpn_gettimeofday().
//
//     mainloop will also do sample-and-hold integration over the window
//     [d_last_time:vrpn_gettimeofday()], and accumulate the result into
//     d_virtual_time_elapsed_since_last_event.  Then it will compute end_time
//     by adding d_time and d_virtual_time_elapsed_since_last_event.  iff an
//     event is played from the file, d_last_time will be set to now_time and
//     d_virtual_time_elapsed_since_last_event will be zero'd.

// }}}

// virtual
int vrpn_File_Connection::mainloop(const timeval * /*timeout*/)
{
    // XXX timeout ignored for now, needs to be added

    timeval now_time;
    vrpn_gettimeofday(&now_time, NULL);

    if ((d_last_time.tv_sec == 0) && (d_last_time.tv_usec == 0)) {
        // If first iteration, consider 0 time elapsed
        d_last_time = now_time;
        d_filetime_accum.reset_at_time(now_time);
        return 0;
    }

    // now_time:    current wallclock time (on method entry)
    // d_last_time: wallclock time of last call to mainloop
    //              (juliano-8/26/99) NO!  It is the time the
    //              wallclock read (at the top of mainloop) when the
    //              last event was played back from the file.
    //              If you call mainloop frequently enough,
    //              these are not necessarily the same!
    //              (may call mainloop too soon and then no event
    //              is played back from the file)
    // d_time:      current time in file
    // end_time:    computed time in file
    // d_rate:      wallclock -> fileclock rate scale factor
    // goal:        compute end_time, then advance to it
    //
    // scale elapsed time by d_rate (rate of replay);
    // this gives us the time to advance (skip_time)
    // our clock to (next_time).
    // -- see note above!
    //
    // const timeval real_elapsed_time  // amount of ellapsed wallclock time
    //  = vrpn_TimevalDiff( now_time, d_last_time );
    // const timeval skip_time          // scale it by d_rate
    //    = vrpn_TimevalScale( real_elapsed_time, d_rate );
    // const timeval end_time           // add it to the last file-time
    //    = vrpn_TimevalSum( d_time, skip_time );
    //
    // ------ new way of calculating end_time ------------

    d_filetime_accum.accumulate_to(now_time);
    const timeval end_time =
        vrpn_TimevalSum(d_time, d_filetime_accum.accumulated());

    // (winston) Had to add need_to_play() because at fractional rates
    // (even just 1/10th) the d_time didn't accumulate properly
    // because tiny intervals after scaling were too small
    // for a timeval to represent (1us minimum).
    //
    // (juliano-8/26/99) if ((end_time - timestamp of next event) < 1us)
    // then you have run out of precision in the struct timeval when
    // need_to_play differences those two timevals.  I.e., they
    // appear to be the same time.
    // need_to_play will return n:n>1 only if this difference
    // is non-zero.
    //
    // (juliano-8/25/99) need_to_play is not a boolean function!
    // it returns n:n>0 if you need to play
    //            n=0   if the timevals compare equal
    //            n=-1  if there was an error reading the next event
    //                  from the log file
    const int need_to_play_retval = need_to_play(end_time);

    if (need_to_play_retval > 0) {
        d_last_time = now_time;
        d_filetime_accum.reset_at_time(now_time);
        const int rv = play_to_filetime(end_time);
        return rv;
    }
    else if (need_to_play_retval == 0) {
        // (winston) we don't set d_last_time so that we can more
        // accurately measure the (larger) interval next time around
        //
        // (juliano-8/26/99) sounds right.  Only set d_last_time
        // if you actually played some event from the file.
        // You may get here if you have your data in more than one
        // file, and are trying to play back from the files in lockstep.
        // The tracker group does this to run the hybrid tracking
        // algorithm on both an inertial data file and a hiball
        // tracker file that were recorded with synchronized clocks.
        return 0;
    }
    else {
        // return something to indicate there was an error
        // reading the file
        return -1;

        // an error occurred while reading the next event from the file
        // let's close the connection.
        // XXX(jj) is this the right thing to do?
        // XXX(jj) for now, let's leave it how it was
        // XXX(jj) come back to this and do it right
        /*
                fprintf( stderr, "vrpn_File_Connection::mainloop(): error
           reading "
                         "next event from file.  Skipping to end of file. "
                         "XXX Please edit this function and fix it.  It should
           probably"
                         " close the connection right here and now.\n");
                d_last_time = now_time;
                d_filetime_accum.reset_at_time( now_time );
                return play_to_filetime(end_time);
        */
    }
}

// }}}
// {{{ need_to_play

// checks if there is at least one log entry that occurs
// between the current d_time and the given filetime
//
// [juliano 9/24/99] the above comment is almost right
//     the upper bound of the interval is not open,
//     but closed at time_we_want_to_play_to.
//
//     this function checks if the next message to play back
//     from the stream file has a timestamp LESSTHAN OR EQUAL TO
//     the argument to this function (which is the time that we
//     wish to play to).  If it does, then a pos value is returned
//
//     you can pause playback of a streamfile by ceasing to increment
//     the value that is passed to this function.  However, if the next
//     message in the streamfile has the same timestamp as the previous
//     one, it will get played anyway.  Pause will not be achieved until
//     all such messages have been played.
//
//     Beware: make sure you put the correct timestamps on individual
//     messages when recording them in chunks (batches)
//     to a time to a streamfile.
//
int vrpn_File_Connection::need_to_play(timeval time_we_want_to_play_to)
{
    // This read_entry() call may be useful to test the state, but
    // it should be the case that d_currentLogEntry is non-NULL except
    // when we are at the end.  This is because read_entry() and the
    // constructor now both read the next one in when they are finished.
    if (!d_currentLogEntry) {
        int retval = read_entry();
        if (retval < 0) {
            return -1;
        } // error reading from file
        if (retval > 0) {
            return 0;
        } // end of file;  nothing to replay
        d_currentLogEntry =
            d_logTail; // If read_entry() returns 0, this will be non-NULL
    }

    vrpn_HANDLERPARAM &header = d_currentLogEntry->data;

    // [juliano 9/24/99]  is this right?
    //   this is a ">" test, not a ">=" test
    //   consequently, this function keeps returning true until the
    //   next message is timestamped greater.  So, if a group of
    //   messages share a timestamp, you cannot pause streamfile
    //   replay anywhere inside the group.
    //
    //   this is true, but do you ever want to pause in the middle of
    //   such a group?  This was a problem prior to fixing the
    //   timeval overflow bug, but now that it's fixed, (who cares?)

    return vrpn_TimevalGreater(time_we_want_to_play_to, header.msg_time);
}

// }}}
// {{{ play_to_time and play_to_filetime

// plays to an elapsed end_time (in seconds)
int vrpn_File_Connection::play_to_time(vrpn_float64 end_time)
{
    return play_to_time(vrpn_MsecsTimeval(end_time * 1000));
}

// plays to an elapsed end_time
int vrpn_File_Connection::play_to_time(timeval end_time)
{
    if (d_earliest_user_time_valid) {
        return play_to_filetime(
            vrpn_TimevalSum(d_earliest_user_time, end_time));
    }
    else {
        return play_to_filetime(vrpn_TimevalSum(d_start_time, end_time));
    }
}

// plays all entries between d_time and end_filetime
// returns -1 on error, 0 on success
int vrpn_File_Connection::play_to_filetime(const timeval end_filetime)
{
    vrpn_uint32 playback_this_iteration = 0;

    if (vrpn_TimevalGreater(d_time, end_filetime)) {
        // end_filetime is BEFORE d_time (our current time in the stream)
        // so, we need to go backwards in the stream
        // currently, this is implemented by
        //   * rewinding the stream to the beginning
        //   * playing log messages one at a time until we get to end_filetime
        reset();
    }

    int ret;
    while ((ret = playone_to_filetime(end_filetime)) == 0) {
        //   * you get here ONLY IF playone_to_filetime returned 0
        //   * that means that it played one entry

        playback_this_iteration++;
        if ((get_Jane_value() > 0) &&
            (playback_this_iteration >= get_Jane_value())) {
            // Early exit from the loop
            // Don't reset d_time to end_filetime
            return 0;
        }
    }

    if (ret == 1) {
        // playone_to_filetime finished or EOF no error for us
        // Set log position to the exact requested ending time,
        // don't leave it at the last log entry time
        d_time = end_filetime;
        ret = 0;
    }

    return ret;
}

// }}}
// {{{ rest

// returns 1 if we're at the EOF, -1 on error
int vrpn_File_Connection::eof()
{
    if (d_currentLogEntry) {
        return 0;
    }
    // read from disk if not in memory
    int ret = read_entry();
    if (ret == 0) {
        d_currentLogEntry =
            d_logTail; // If read_entry() returns zero, this will be non-NULL
    }

    return ret;
}

// plays at most one entry which comes before end_filetime
// returns
//   -1 on error (including EOF, call eof() to test)
//    0 for normal result (played one entry)
int vrpn_File_Connection::playone()
{
    static const timeval tvMAX = {LONG_MAX, 999999L};

    int ret = playone_to_filetime(tvMAX);
    if (ret != 0) {
        // consider a 1 return from playone_to_filetime() to be
        // an error since it should never reach tvMAX
        return -1;
    }
    else {
        return 0;
    }
}

// plays at most one entry which comes before end_filetime
// returns
//   -1 on error (including EOF, call eof() to test)
//    0 for normal result (played one entry)
//    1 if we hit end_filetime
int vrpn_File_Connection::playone_to_filetime(timeval end_filetime)
{
    vrpn_Endpoint *endpoint = d_endpoints.front();
    timeval now;
    int retval;

    // If we don't have a currentLogEntry, then we've gone past the end of the
    // file.
    if (!d_currentLogEntry) {
        return 1;
    }

    vrpn_HANDLERPARAM &header = d_currentLogEntry->data;

    if (vrpn_TimevalGreater(header.msg_time, end_filetime)) {
        // there are no entries to play after the current
        // but before end_filetime
        return 1;
    }

    // TCH July 2001
    // XXX A big design decision:  do we re-log messages exactly,
    // or do we mark them with the time they were played back?
    // Maybe this should be switchable, but the latter is what
    // I need yesterday.
    vrpn_gettimeofday(&now, NULL);
    retval = endpoint->d_inLog->logIncomingMessage(
        header.payload_len, now, header.type, header.sender, header.buffer);
    if (retval) {
        fprintf(stderr, "Couldn't log \"incoming\" message during replay!\n");
        return -1;
    }

    // advance current file position
    d_time = header.msg_time;

    // Handle this log entry
    if (header.type >= 0) {
#ifdef VERBOSE
        printf("vrpn_FC: Msg Sender (%s), Type (%s), at (%ld:%ld)\n",
               endpoint->other_senders[header.sender].name,
               endpoint->other_types[header.type].name, header.msg_time.tv_sec,
               header.msg_time.tv_usec);
#endif
        if (endpoint->local_type_id(header.type) >= 0) {
            if (do_callbacks_for(endpoint->local_type_id(header.type),
                                 endpoint->local_sender_id(header.sender),
                                 header.msg_time, header.payload_len,
                                 header.buffer)) {
                return -1;
            }
        }
    }
    else { // system handler

        if (header.type != vrpn_CONNECTION_UDP_DESCRIPTION) {
            if (doSystemCallbacksFor(header, endpoint)) {
                fprintf(stderr, "vrpn_File_Connection::playone_to_filename:  "
                                "Nonzero system return.\n");
                return -1;
            }
        }
    }

    return advance_currentLogEntry();
}

// Advance to next entry.  If there is no next entry, and if we have
// not preloaded, then try to read one in.
int vrpn_File_Connection::advance_currentLogEntry(void)
{
    // If we don't have a currentLogEntry, then we've gone past the end of the
    // file.
    if (!d_currentLogEntry) {
        return 1;
    }

    d_currentLogEntry = d_currentLogEntry->next;
    if (!d_currentLogEntry && !d_preload) {
        int retval = read_entry();
        if (retval != 0) {
            return -1; // error reading from file or EOF
        }
        d_currentLogEntry =
            d_logTail; // If read_entry() returns zero, this will be non-NULL
    }

    return 0;
}

double vrpn_File_Connection::get_length_secs()
{
    return vrpn_TimevalMsecs(get_length()) / 1000;
}

// virtual
timeval vrpn_File_Connection::get_length()
{
    timeval len = {0, 0};

    if (!d_earliest_user_time_valid || !d_highest_user_time_valid) {
        this->get_lowest_user_timestamp();
        this->get_highest_user_timestamp();
    }

    len = vrpn_TimevalDiff(d_highest_user_time, d_earliest_user_time);
    return len;
}

timeval vrpn_File_Connection::get_lowest_user_timestamp()
{
    if (!d_earliest_user_time_valid) find_superlative_user_times();
    return d_earliest_user_time;
}

timeval vrpn_File_Connection::get_highest_user_timestamp()
{
    if (!d_highest_user_time_valid) find_superlative_user_times();
    return d_highest_user_time;
}

void vrpn_File_Connection::find_superlative_user_times()
{
    timeval high = {0, 0};
    timeval low = {LONG_MAX, 999999L};

    // Remember where we were when we asked this question
    bool retval = store_stream_bookmark();
    if (retval == false) {
#ifdef VERBOSE
        printf("vrpn_File_Connection::find_superlative_user_times:  didn't "
               "successfully save bookmark.\n");
#endif
        return;
    }

    // Go to the beginning of the file and then run through all
    // of the messages to find the one with the lowest/highest value
    reset();
    do {
        if (d_currentLogEntry && (d_currentLogEntry->data.type >= 0)) {
            if (vrpn_TimevalGreater(d_currentLogEntry->data.msg_time, high)) {
                high = d_currentLogEntry->data.msg_time;
            }
            if (vrpn_TimevalGreater(low, d_currentLogEntry->data.msg_time)) {
                low = d_currentLogEntry->data.msg_time;
            }
        }
    } while (d_currentLogEntry && (advance_currentLogEntry() == 0));

    // We have our value.  Set it and go back where
    // we came from, but don't play the records along
    // the way.
    retval = return_to_bookmark();
    if (retval == false) {
        //  oops.  we've really screwed things up.
        fprintf(stderr, "vrpn_File_Connection::find_superlative_user_times "
                        "messed up the location in the file stream.\n");
        reset();
        return;
    }

    if (high.tv_sec != LONG_MIN) // we found something
    {
        d_highest_user_time = high;
        d_highest_user_time_valid = true;
    }
#ifdef VERBOSE
    else {
        fprintf( stderr, "vrpn_File_Connection::find_superlative_user_times:  did not find a highest-time user message\n"
    }
#endif

    if (low.tv_sec != LONG_MAX) // we found something
    {
        d_earliest_user_time = low;
        d_earliest_user_time_valid = true;
    }
#ifdef VERBOSE
    else {
        fprintf( stderr, "vrpn_File_Connection::find_superlative_user_times:  did not find an earliest user message\n"
    }
#endif

} // end find_superlative_user_times

vrpn_File_Connection::vrpn_FileBookmark::vrpn_FileBookmark()
{
    valid = false;
    file_pos = -1;
    oldTime.tv_sec = 0;
    oldTime.tv_usec = 0;
    oldCurrentLogEntryPtr = NULL;
    oldCurrentLogEntryCopy = NULL;
}

vrpn_File_Connection::vrpn_FileBookmark::~vrpn_FileBookmark()
{
    if (oldCurrentLogEntryCopy == NULL) return;
    if (oldCurrentLogEntryCopy->data.buffer != NULL) {
      try {
        delete[] (oldCurrentLogEntryCopy->data.buffer);
      } catch (...) {
        fprintf(stderr, "vrpn_File_Connection::vrpn_FileBookmark::~vrpn_FileBookmark: delete failed\n");
        return;
      }
    }
    try {
      delete oldCurrentLogEntryCopy;
    } catch (...) {
      fprintf(stderr, "vrpn_File_Connection::vrpn_FileBookmark::~vrpn_FileBookmark: delete failed\n");
      return;
    }
}

bool vrpn_File_Connection::store_stream_bookmark()
{
    if (d_preload) {
        // everything is already in memory, so just remember where we were
        d_bookmark.oldCurrentLogEntryPtr = d_currentLogEntry;
        d_bookmark.oldTime = d_time;
    }
    else if (d_accumulate) // but not pre-load
    {
        // our current location will remain in memory
        d_bookmark.oldCurrentLogEntryPtr = d_currentLogEntry;
        d_bookmark.file_pos = ftell(d_file);
        d_bookmark.oldTime = d_time;
    }
    else // !preload and !accumulate
    {
        d_bookmark.oldTime = d_time;
        d_bookmark.file_pos = ftell(d_file);
        if (d_currentLogEntry == NULL) // at the end of the file
        {
            if (d_bookmark.oldCurrentLogEntryCopy != NULL) {
              if (d_bookmark.oldCurrentLogEntryCopy->data.buffer != NULL) {
                try {
                  delete[](d_bookmark.oldCurrentLogEntryCopy->data.buffer);
                } catch (...) {
                  fprintf(stderr, "vrpn_File_Connection::store_stream_bookmark: delete failed\n");
                  return false;
                }
              }
              try {
                delete d_bookmark.oldCurrentLogEntryCopy;
              } catch (...) {
                fprintf(stderr, "vrpn_File_Connection::store_stream_bookmark: delete failed\n");
                return false;
              }
            }
            d_bookmark.oldCurrentLogEntryCopy = NULL;
        }
        else {
            if (d_bookmark.oldCurrentLogEntryCopy == NULL) {
                try {d_bookmark.oldCurrentLogEntryCopy = new vrpn_LOGLIST; }
                catch (...) {
                    fprintf(stderr, "Out of memory error:  "
                                    "vrpn_File_Connection::store_stream_"
                                    "bookmark\n");
                    d_bookmark.valid = false;
                    return false;
                }
                d_bookmark.oldCurrentLogEntryCopy->next =
                    d_bookmark.oldCurrentLogEntryCopy->prev = NULL;
                d_bookmark.oldCurrentLogEntryCopy->data.buffer = NULL;
            }
            d_bookmark.oldCurrentLogEntryCopy->next = d_currentLogEntry->next;
            d_bookmark.oldCurrentLogEntryCopy->prev = d_currentLogEntry->prev;
            d_bookmark.oldCurrentLogEntryCopy->data.type =
                d_currentLogEntry->data.type;
            d_bookmark.oldCurrentLogEntryCopy->data.sender =
                d_currentLogEntry->data.sender;
            d_bookmark.oldCurrentLogEntryCopy->data.msg_time =
                d_currentLogEntry->data.msg_time;
            d_bookmark.oldCurrentLogEntryCopy->data.payload_len =
                d_currentLogEntry->data.payload_len;
            if (d_bookmark.oldCurrentLogEntryCopy->data.buffer != NULL) {
              try {
                delete[] d_bookmark.oldCurrentLogEntryCopy->data.buffer;
              } catch (...) {
                fprintf(stderr, "vrpn_File_Connection::store_stream_bookmark: delete failed\n");
                return false;
              }
            }
            try { d_bookmark.oldCurrentLogEntryCopy->data.buffer =
                new char[d_currentLogEntry->data.payload_len]; }
            catch (...) {
                d_bookmark.valid = false;
                return false;
            }
            memcpy(const_cast<char *>(d_bookmark.oldCurrentLogEntryCopy->data.buffer),
                   d_currentLogEntry->data.buffer,
                   d_currentLogEntry->data.payload_len);
        }
    }
    d_bookmark.valid = true;
    return true;
}

bool vrpn_File_Connection::return_to_bookmark()
{
    int retval = 0;
    if (!d_bookmark.valid) return false;
    if (d_preload) {
        d_time = d_bookmark.oldTime;
        d_currentLogEntry = d_bookmark.oldCurrentLogEntryPtr;
    }
    else if (d_accumulate) // but not pre-load
    {
        d_time = d_bookmark.oldTime;
        d_currentLogEntry = d_bookmark.oldCurrentLogEntryPtr;
        retval |= fseek(d_file, d_bookmark.file_pos, SEEK_SET);
    }
    else // !preload and !accumulate
    {
        if (d_bookmark.oldCurrentLogEntryCopy == NULL) {
            // we were at the end of the file.
            d_currentLogEntry = d_logHead = d_logTail = NULL;
            d_time = d_bookmark.oldTime;
            retval |= fseek(d_file, d_bookmark.file_pos, SEEK_SET);
        }
        else {
            char *newBuffer = NULL;
            try { newBuffer = 
                new char[d_bookmark.oldCurrentLogEntryCopy->data.payload_len]; }
            catch (...) {
                return false;
            }
            d_time = d_bookmark.oldTime;
            retval |= fseek(d_file, d_bookmark.file_pos, SEEK_SET);
            if (d_currentLogEntry == NULL) // we are at the end of the file
            {
                try { d_currentLogEntry = new vrpn_LOGLIST; }
                catch (...) { return false; }
                d_currentLogEntry->data.buffer = 0;
            }
            d_currentLogEntry->next = d_bookmark.oldCurrentLogEntryCopy->next;
            d_currentLogEntry->prev = d_bookmark.oldCurrentLogEntryCopy->prev;
            d_currentLogEntry->data.type =
                d_bookmark.oldCurrentLogEntryCopy->data.type;
            d_currentLogEntry->data.sender =
                d_bookmark.oldCurrentLogEntryCopy->data.sender;
            d_currentLogEntry->data.msg_time =
                d_bookmark.oldCurrentLogEntryCopy->data.msg_time;
            d_currentLogEntry->data.payload_len =
                d_bookmark.oldCurrentLogEntryCopy->data.payload_len;
            const char *temp = d_currentLogEntry->data.buffer;
            d_currentLogEntry->data.buffer = newBuffer;
            memcpy(const_cast<char *>(d_currentLogEntry->data.buffer),
                   d_bookmark.oldCurrentLogEntryCopy->data.buffer,
                   d_currentLogEntry->data.payload_len);
            if (temp) {
              try {
                delete[] temp;
              } catch (...) {
                fprintf(stderr, "vrpn_File_Connection::return_to_bookmark: delete failed\n");
                return false;
              }
            }
            d_logHead = d_logTail = d_currentLogEntry;
        }
    }
    return (retval == 0);
}

const char *vrpn_File_Connection::get_filename() { return d_fileName; }

// Returns the time since the connection opened.
// Some subclasses may redefine time.
// virtual
int vrpn_File_Connection::time_since_connection_open(timeval *elapsed_time)
{
    if (!d_earliest_user_time_valid) {
        this->find_superlative_user_times();
    }
    if (d_earliest_user_time_valid) {
        *elapsed_time = vrpn_TimevalDiff(d_time, d_earliest_user_time);
    }
    else {
        *elapsed_time = vrpn_TimevalDiff(d_time, d_start_time);
    } // XXX get rid of this option - dtm

    return 0;
}

// virtual
vrpn_File_Connection *vrpn_File_Connection::get_File_Connection(void)
{
    return this;
}

// {{{ read_cookie and read_entry

// Reads a cookie from the logfile and calls check_vrpn_cookie()
// (from vrpn_Connection.C) to check it.
// Returns -1 on no cookie or cookie mismatch (which should cause abort),
// 0 otherwise.

// virtual
int vrpn_File_Connection::read_cookie(void)
{
    char readbuf[128]; // XXX HACK!
    size_t bytes = fread(readbuf, vrpn_cookie_size(), 1, d_file);
    if (bytes == 0) {
        fprintf(stderr, "vrpn_File_Connection::read_cookie:  "
                        "No cookie.  If you're sure this is a logfile, "
                        "run add_vrpn_cookie on it and try again.\n");
        return -1;
    }
    readbuf[vrpn_cookie_size()] = '\0';

    int retval = check_vrpn_file_cookie(readbuf);
    if (retval < 0) {
        return -1;
    }

    // TCH July 2001
    if (!d_endpoints.is_valid(0)) {
        fprintf(stderr, "vrpn_File_Connection::read_cookie:  "
                        "No endpoints[0].  Internal failure.\n");
        return -1;
    }
    d_endpoints.front()->d_inLog->setCookie(readbuf);

    return 0;
}

// virtual
int vrpn_File_Connection::read_entry(void)
{
    vrpn_LOGLIST *newEntry;
    size_t retval;

    try { newEntry = new vrpn_LOGLIST; }
    catch (...) {
        fprintf(stderr, "vrpn_File_Connection::read_entry: Out of memory.\n");
        return -1;
    }

    // Only print this message every second or so
    if (!d_file) {
        struct timeval now;
        vrpn_gettimeofday(&now, NULL);
        if (now.tv_sec != d_last_told.tv_sec) {
            fprintf(stderr, "vrpn_File_Connection::read_entry: no open file\n");
            memcpy(&d_last_told, &now, sizeof(d_last_told));
        }
        try {
          delete newEntry;
        } catch (...) {
          fprintf(stderr, "vrpn_File_Connection::read_entry: delete failed\n");
          return -1;
        }
        return -1;
    }

    // Get the header of the next message.  This was done as a horrible
    // hack in the past, where we read the sizeof a struct from the file,
    // including a pointer.  This of course changed on 64-bit architectures.
    // The pointer value was not needed.  We now read it as an array of
    // 32-bit values and then stuff these into the structure.  Unfortunately,
    // we now need to both send and read the bogus pointer value if we want
    // to be compatible with old versions of log files.

    vrpn_HANDLERPARAM &header = newEntry->data;
    vrpn_int32 values[6];
    retval = fread(values, sizeof(vrpn_int32), 6, d_file);

    // return 1 if nothing to read OR end-of-file;
    // the latter isn't an error state
    if (retval <= 0) {
        // Don't close the file because we might get a reset message...
        try {
          delete newEntry;
        } catch (...) {
          fprintf(stderr, "vrpn_File_Connection::read_entry: delete failed\n");
          return -1;
        }
        return 1;
    }

    header.type = ntohl(values[0]);
    header.sender = ntohl(values[1]);
    header.msg_time.tv_sec = ntohl(values[2]);
    header.msg_time.tv_usec = ntohl(values[3]);
    header.payload_len = ntohl(values[4]);
    header.buffer =
        NULL; // values[5] is ignored -- it used to hold the bogus pointer.

    // get the body of the next message

    if (header.payload_len > 0) {
        try { header.buffer = new char[header.payload_len]; }
        catch (...) {
            fprintf(stderr, "vrpn_File_Connection::read_entry:  "
                            "Out of memory.\n");
            return -1;
        }

        retval = fread(const_cast<char *>(header.buffer), 1, header.payload_len, d_file);
    }

    // return 1 if nothing to read OR end-of-file;
    // the latter isn't an error state
    if (retval <= 0) {
        // Don't close the file because we might get a reset message...
        return 1;
    }

    // If we are accumulating messages, keep the list of them up to
    // date.  If we are not, toss the old to make way for the new.
    // Whenever this function returns 0, we need to have set the
    // Head and Tail to something non-NULL.

    if (d_accumulate) {

        // doubly-linked list maintenance, putting this one at the tail.
        newEntry->next = NULL;
        newEntry->prev = d_logTail;
        if (d_logTail) {
            d_logTail->next = newEntry;
        }
        d_logTail = newEntry;

        // If we've not gotten any messages yet, this one is also the
        // head.
        if (!d_logHead) {
            d_logHead = d_logTail;
        }
    }
    else { // Don't keep old list entries.

        // If we had a message before, get rid of it and its data now.  We
        // could use either Head or Tail here because they will point
        // to the same message.
        if (d_logTail) {
            if (d_logTail->data.buffer) {
                try {
                  delete[] d_logTail->data.buffer;
                } catch (...) {
                  fprintf(stderr, "vrpn_File_Connection::read_entry: delete failed\n");
                  return -1;
                }
            }
            try {
              delete d_logTail;
            } catch (...) {
              fprintf(stderr, "vrpn_File_Connection::read_entry: delete failed\n");
              return -1;
            }
        }

        // This is the only message in memory, so it is both the
        // head and the tail of the memory list.
        d_logHead = d_logTail = newEntry;

        // The new entry is not linked to any others (there are no others)
        newEntry->next = NULL;
        newEntry->prev = NULL;
    }

    return 0;
}
// }}}

// virtual
int vrpn_File_Connection::close_file()
{
    if (d_file) {
        fclose(d_file);
    }
    d_file = NULL;
    return 0;
}

int vrpn_File_Connection::reset()
{
    // make it as if we never saw any messages from our previous activity
    d_endpoints.front()->drop_connection();

    // If we are accumulating, reset us back to the beginning of the memory
    // buffer chain. Otherwise, go back to the beginning of the file and
    // then read the magic cookie and then the first entry again.
    if (d_accumulate) {
        d_currentLogEntry = d_startEntry;
    }
    else {
        rewind(d_file);
        read_cookie();
        read_entry();
        d_startEntry = d_currentLogEntry = d_logHead;
    }
    d_time = d_startEntry->data.msg_time;
    // reset for mainloop()
    d_last_time.tv_usec = d_last_time.tv_sec = 0;
    d_filetime_accum.reset_at_time(d_last_time);

    // This is useful to play the initial system messages
    // (the sender/type ones) automatically.  These might not be
    // time synched so if we don't play them automatically they
    // can mess up playback if their timestamps are later then
    // the first user message.
    if (vrpn_FILE_CONNECTIONS_SHOULD_SKIP_TO_USER_MESSAGES) {
        play_to_user_message();
    }

    return 0;
}

// static
int vrpn_File_Connection::handle_set_replay_rate(void *userdata,
                                                 vrpn_HANDLERPARAM p)
{
    vrpn_File_Connection *me = (vrpn_File_Connection *)userdata;

    const char *bufPtr = p.buffer;
    me->set_replay_rate(vrpn_unbuffer<vrpn_float32>(bufPtr));

    return 0;
}

// static
int vrpn_File_Connection::handle_reset(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_File_Connection *me = (vrpn_File_Connection *)userdata;

    // fprintf(stderr, "In vrpn_File_Connection::handle_reset().\n");

    return me->reset();
}

// static
int vrpn_File_Connection::handle_play_to_time(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    vrpn_File_Connection *me = (vrpn_File_Connection *)userdata;
    timeval newtime;

    newtime.tv_sec = ((const vrpn_int32 *)(p.buffer))[0];
    newtime.tv_usec = ((const vrpn_int32 *)(p.buffer))[1];

    return me->play_to_time(newtime);
}

int vrpn_File_Connection::send_pending_reports(void)
{
    // Do nothing except clear the buffer -
    // file connections aren't really connected to anything.

    d_endpoints.front()->clearBuffers(); // Clear the buffer for the next time
    return 0;
}

// }}}
