// vrpn_FileConnection.C

// {{{ includes and defines

#include "vrpn_FileConnection.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and netinet/in.h and ...
#include "vrpn_Shared.h"
#ifndef _WIN32
#include <netinet/in.h>
#endif

#define CHECK(x) if (x == -1) return -1

// this should be a shared declaration with those
// at the top of vrpn_Connection.C!
#define BROKEN                  (-2)


// Since the PRELOAD feature was added, I suspect it would
// take some work to revert to the non-preload mode.  That is,
// both modes were not tested as features were added.  It shouldn't
// be hard to go back, but... -PBW 3/99

// }}}
// {{{ constructor

vrpn_File_Connection::vrpn_File_Connection (const char * file_name,
                         const char * local_logfile_name,
                         long local_log_mode) :
    vrpn_Connection (file_name, -1, local_logfile_name, local_log_mode),
    d_controllerId (register_sender("vrpn File Controller")),
    d_set_replay_rate_type(
        register_message_type("vrpn File set replay rate")),
    d_reset_type (register_message_type("vrpn File reset")),
    d_play_to_time_type (register_message_type("vrpn File play to time")),
    d_file (NULL),
    d_logHead (NULL),
    d_logTail (NULL),
    d_currentLogEntry (NULL)
{
    const char * bare_file_name;

    register_handler(d_set_replay_rate_type, handle_set_replay_rate,
                     this, d_controllerId);
    register_handler(d_reset_type, handle_reset, this, d_controllerId);
    register_handler(d_play_to_time_type, handle_play_to_time,
                     this, d_controllerId);

    // necessary to initialize properly in mainloop()
    d_last_time.tv_usec = d_last_time.tv_sec = 0;
    
    bare_file_name = vrpn_copy_file_name(file_name);
    if (!bare_file_name) {
        fprintf(stderr, "vrpn_File_Connection:  Out of memory!\n");
        status = BROKEN;
        return;
    }

    d_file = fopen(bare_file_name, "rb");
    if (!d_file) {
        fprintf(stderr, "vrpn_File_Connection:  "
                "Could not open file \"%s\".\n", bare_file_name);
        delete [] (char *) bare_file_name;
        status = BROKEN;
        return;
    }

    delete [] (char *) bare_file_name;

    // PRELOAD
    // TCH 16 Sept 1998

    //fprintf(stderr, "vrpn_File_Connection::vrpn_File_Connection: Preload...\n");

    if (read_cookie() < 0) {
        status = BROKEN;
        return;
    }
    while (!read_entry()) {
      // empty loop body - read all the entries
    }
    d_currentLogEntry = d_logHead;

    //fprintf(stderr, "vrpn_File_Connection::vrpn_File_Connection: Done preload.\n");

    d_startEntry = d_logHead;
    d_start_time = d_startEntry->data.msg_time;  
    d_time = d_start_time;

    // this needs to be a parameter if we want this to be optional
    int fPlayToFirstUserMessage = 1;
    // This is useful to play the initial system messages
    //(the sender/type ones) automatically.  These might not be
    // time synched so if we don't play them automatically they
    // can mess up playback if their timestamps are later then
    // the first user message.
    if (fPlayToFirstUserMessage) {
        play_to_user_message();
        d_startEntry = d_currentLogEntry;
        if (d_startEntry) {
            d_start_time = d_startEntry->data.msg_time;
        }
    }
}

// }}}
// {{{ play_to_user_message

// Advances through the file, calling callbacks, up until
// a user message (type >= 0) is encountered)
// NOTE: assumes pre-load (could be changed to not)
void vrpn_File_Connection::play_to_user_message()
{
    if (!d_currentLogEntry) {
        return;
    }
    
    while (d_currentLogEntry != NULL && d_currentLogEntry->data.type < 0) {

        vrpn_HANDLERPARAM &header = d_currentLogEntry->data;

        if (header.type != vrpn_CONNECTION_UDP_DESCRIPTION) {
            if (system_messages[-header.type](this, header)) {
                fprintf(stderr, "vrn_File_Connection::play_to_user_message: "
                        "Nonzero system return.\n");
            }
        }
        d_currentLogEntry = d_currentLogEntry->next;

        if (d_currentLogEntry) {
            // we advance d_time one ahead because they're may be 
            // a large gap in the file (forward or backwards) between
            // the last system message and first user one
            d_time = d_currentLogEntry->data.msg_time;
        }        
    }
}

// }}}
// {{{ destructor

// virtual
vrpn_File_Connection::~vrpn_File_Connection (void)
{
    vrpn_LOGLIST * np;

    close_file();

    while (d_logHead) {
        np = d_logHead->next;
        if (d_logHead->data.buffer)
            delete [] (char *) d_logHead->data.buffer;
        delete d_logHead;
        d_logHead = np;
    }
}

// }}}
// {{{ jump_to_time

int vrpn_File_Connection::jump_to_time(vrpn_float64 newtime)
{
    return jump_to_time(vrpn_MsecsTimeval(newtime * 1000));
}

// assumes preload(?)
int vrpn_File_Connection::jump_to_time(timeval newtime)
{
    d_time = vrpn_TimevalSum(d_start_time, newtime);
    
    if (!d_currentLogEntry) {
        d_currentLogEntry = d_logTail;
    }
    
    // search backwards
    while (vrpn_TimevalGreater(d_currentLogEntry->data.msg_time, d_time)) {
        if (d_currentLogEntry->prev) {
            d_currentLogEntry = d_currentLogEntry->prev;
        } else {
            return 1;
        }
    }
    
    // or forwards, as needed
    while (vrpn_TimevalGreater(d_time, d_currentLogEntry->data.msg_time)) {
        if (d_currentLogEntry->next) {
            d_currentLogEntry = d_currentLogEntry->next;
        } else {
            return 1;
        }        
    }
    
    return 0;
}

// }}}
// {{{ vrpn_File_Connection::FileTime_Accumulator

vrpn_File_Connection::FileTime_Accumulator::FileTime_Accumulator()
    : d_replay_rate( 1.0 )
{
    d_filetime_accum_since_last_playback.tv_sec = 0;
    d_filetime_accum_since_last_playback.tv_usec = 0;
    
    d_time_of_last_accum.tv_sec = 0;
    d_time_of_last_accum.tv_usec = 0;
}


void vrpn_File_Connection::FileTime_Accumulator::accumulate_to(
    const timeval & now_time )
{
    timeval & accum       = d_filetime_accum_since_last_playback;
    timeval & last_accum  = d_time_of_last_accum;
    
    accum  // updated elapsed filetime
        = vrpn_TimevalSum (         // summed with previous elapsed time
            accum,
            vrpn_TimevalScale(      // scaled by replay rate
                vrpn_TimevalDiff(   // elapsed wallclock time
                    now_time,
                    d_time_of_last_accum ),
                d_replay_rate));
    last_accum = now_time;          // wallclock time of this whole mess
}


void vrpn_File_Connection::FileTime_Accumulator::set_replay_rate(
    vrpn_float32 new_rate)
{
    timeval now_time;
    gettimeofday(&now_time, NULL);
    accumulate_to( now_time );

    d_replay_rate = new_rate;
    //fprintf(stderr, "Set replay rate!\n");
}


void vrpn_File_Connection::FileTime_Accumulator::reset_at_time(
    const timeval & now_time )
{
    // this function is confusing.  It doesn't appear to do anything.
    // (I added the next three lines. did I delete what was previously there?)
    d_filetime_accum_since_last_playback.tv_sec = 0;
    d_filetime_accum_since_last_playback.tv_usec = 0;
    d_time_of_last_accum = now_time;
}


// }}}
// {{{ mainloop

// {{{ -- comment

// [juliano 10/11/99] the problem described below is now fixed
// [juliano 8/26/99]  I beleive there to be a bug in mainloop.
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
//     from d_last_time to the result of gettimeofday(), and it will
//     accumulate the result into d_virtual_time_elapsed_since_last_event.
//     then it will set d_last_time to the result of gettimeofday().
//
//     mainloop will also do sample-and-hold integration over the window
//     [d_last_time:gettimeofday()], and accumulate the result into
//     d_virtual_time_elapsed_since_last_event.  Then it will compute end_time
//     by adding d_time and d_virtual_time_elapsed_since_last_event.  iff an
//     event is played from the file, d_last_time will be set to now_time and
//     d_virtual_time_elapsed_since_last_event will be zero'd.

// }}}

// virtual
int vrpn_File_Connection::mainloop( const timeval * /*timeout*/ )
{

    // XXX timeout ignored for now, needs to be added

    timeval now_time;
    gettimeofday(&now_time, NULL);

    if ((d_last_time.tv_sec == 0) && (d_last_time.tv_usec == 0)) {
        // If first iteration, consider 0 time elapsed
        d_last_time = now_time;
        d_filetime_accum.reset_at_time( now_time );
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
    //const timeval real_elapsed_time  // amount of ellapsed wallclock time
    //  = vrpn_TimevalDiff( now_time, d_last_time );
    //const timeval skip_time          // scale it by d_rate
    //    = vrpn_TimevalScale( real_elapsed_time, d_rate );
    //const timeval end_time           // add it to the last file-time
    //    = vrpn_TimevalSum( d_time, skip_time );
    //
    // ------ new way of calculating end_time ------------

    d_filetime_accum.accumulate_to( now_time );
    const timeval end_time = vrpn_TimevalSum(
        d_time, d_filetime_accum.accumulated() );
    
    // (winston) Had to add need_to_play() because at fractional rates
    // (even just 1/10th) the d_time didn't accumulate properly
    // because tiny intervals after scaling were too small for
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
        d_filetime_accum.reset_at_time( now_time );
        const int rv = play_to_filetime(end_time);
        return rv;
    } else if (need_to_play_retval == 0) {
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
    } else {
        // an error occurred while reading the next event from the file
        // let's close the connection.
        // XXX(jj) is this the right thing to do?
        // XXX(jj) for now, let's leave it how it was
        // XXX(jj) come back to this and do it right
        fprintf( stderr, "vrpn_File_Connection::mainloop(): error reading "
                 "next event from file.  Skipping to end of file. "
                 "XXX Please edit this function and fix it.  It should probably"
                 " close the connection right here and now.\n");
        d_last_time = now_time;
        d_filetime_accum.reset_at_time( now_time );
        return play_to_filetime(end_time);
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
int vrpn_File_Connection::need_to_play( timeval time_we_want_to_play_to )
{
    if (!d_currentLogEntry) {
        int retval = read_entry();
        if (retval < 0)
            return -1;  // error reading from file
        if (retval > 0)
            return 0;  // end of file;  nothing to replay
        d_currentLogEntry = d_logTail;  // better be non-NULL
    } 

    vrpn_HANDLERPARAM & header = d_currentLogEntry->data;

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
    
    return vrpn_TimevalGreater( time_we_want_to_play_to, header.msg_time );
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
    return play_to_filetime(vrpn_TimevalSum(d_start_time, end_time));
}



// plays all entries between d_time and end_filetime
// returns -1 on error, 0 on success
int vrpn_File_Connection::play_to_filetime(const timeval end_filetime)
{
    if (vrpn_TimevalGreater(d_time, end_filetime)) {
        // end_filetime is BEFORE d_time (our current time in the stream)
        // so, we need to go backwards in the stream
        // currently, this is implemented by
        //   * rewinding the stream to the beginning
        //   * playing log messages one at a time until we get to end_filetime
        reset();
    }
    
    int ret;
    
    while (!(ret = playone_to_filetime(end_filetime))) {
        /* empty loop */
        // juliano(8/25/99)
        //   * you get here ONLY IF playone_to_filetime returned 0
        //   * that means that it played one entry
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
        d_currentLogEntry = d_logTail;  // better be non-NULL
    }

    return ret;
}

// plays at most one entry which comes before end_filetime
// returns
//   -1 on error (including EOF, call eof() to test)
//    0 for normal result (played one entry)
int vrpn_File_Connection::playone()
{
    static timeval tvMAX = { LONG_MAX, LONG_MAX };

    int ret = playone_to_filetime(tvMAX);
    if (ret != 0) {
        // consider a 1 return from playone_to_filetime() to be
        // an error since it should never reach tvMAX
        return -1;
    } else {
        return 0;
    }
}

// plays at most one entry which comes before end_filetime
// returns
//   -1 on error (including EOF, call eof() to test)
//    0 for normal result (played one entry)
//    1 if we hit end_filetime
int vrpn_File_Connection::playone_to_filetime( timeval end_filetime )
{
    // read from disk if not in memory
    if (!d_currentLogEntry) {
        int retval = read_entry();
        if (retval != 0)
            return -1;  // error reading from file or EOF
        d_currentLogEntry = d_logTail;  // better be non-NULL
    }

    vrpn_HANDLERPARAM & header = d_currentLogEntry->data;

    if (vrpn_TimevalGreater(header.msg_time, end_filetime)) {
        // there are no entries to play after the current
        // but before end_filetime
        return 1;
    }

    // advance current file position
    d_time = header.msg_time;

    // Handle this log entry
    if (header.type >= 0) {
#ifdef	VERBOSE
	printf("vrpn_FC: Msg Sender (%s), Type (%s), at (%ld:%ld)\n",
		endpoint.other_senders[header.sender].name,
		endpoint.other_types[header.type].name,
		header.msg_time.tv_sec, header.msg_time.tv_usec);
#endif
        if (endpoint.local_type_id(header.type) >= 0)                
            if (do_callbacks_for(endpoint.local_type_id(header.type),
                                 endpoint.local_sender_id(header.sender),
                                 header.msg_time, header.payload_len,
                                 header.buffer))
                return -1;     
            
    } else {  // system handler            

        if (header.type != vrpn_CONNECTION_UDP_DESCRIPTION) {
            if (system_messages[-header.type](this, header)) {
                fprintf(stderr, "vrpn_File_Connection::playone_to_filename:  "
                        "Nonzero system return.\n");
                return -1;
            }
        }
    }        

    // Advance to next entry
    if (d_currentLogEntry) {
        d_currentLogEntry = d_currentLogEntry->next;
    }

    return 0;
}


double vrpn_File_Connection::get_length_secs()
{
    return vrpn_TimevalMsecs(get_length())/1000;
}


//virtual
timeval vrpn_File_Connection::get_length()
{
    timeval len = {0, 0};

    if (d_logHead && d_logTail) {
        len = vrpn_TimevalDiff(d_logTail->data.msg_time,
                               d_start_time);
    }

    return len;
}


timeval vrpn_File_Connection::get_lowest_user_timestamp()
{
    timeval low = {LONG_MAX, LONG_MAX};

    // The first timestamp in the file may not be the lowest
    // because of time synchronization.
    if (d_logHead) {        
        vrpn_LOGLIST *pEntry;
        for (pEntry = d_logHead; pEntry != NULL; pEntry = pEntry->next) {            
            if (pEntry->data.type >= 0 &&
                vrpn_TimevalGreater(low, pEntry->data.msg_time)) {

                low = pEntry->data.msg_time;
            }
        }
    }

    return low;
}


// Returns the time since the connection opened.
// Some subclasses may redefine time.

// virtual
int vrpn_File_Connection::time_since_connection_open( timeval * elapsed_time )
{

    *elapsed_time = vrpn_TimevalDiff(d_time, d_start_time);

    return 0;
}

// {{{ read_cookie and read_entry

// Reads a cookie from the logfile and calls check_vrpn_cookie()
// (from vrpn_Connection.C) to check it.
// Returns -1 on no cookie or cookie mismatch (which should cause abort),
// 0 otherwise.

// virtual
int vrpn_File_Connection::read_cookie (void)
{
    char readbuf [501];  // HACK!
    int retval;

    retval = fread(readbuf, vrpn_cookie_size(), 1, d_file);
    if (retval <= 0) {
        fprintf(stderr, "vrpn_File_Connection::read_cookie:  "
                "No cookie.  If you're sure this is a logfile, "
                "run add_vrpn_cookie on it and try again.\n");
        return -1;
    }

    retval = check_vrpn_cookie(readbuf);
    if (retval < 0)
        return -1;

    return 0;
}

// virtual
int vrpn_File_Connection::read_entry (void)
{
    vrpn_LOGLIST * newEntry;
    int retval;

    newEntry = new vrpn_LOGLIST;
    if (!newEntry) {
        fprintf(stderr, "vrpn_File_Connection::read_entry: Out of memory.\n");
        return -1;
    }

    if (!d_file) {
        fprintf(stderr, "vrpn_File_Connection::read_entry: no open file\n");
        delete newEntry;
        return -1;
    }

    // get the header of the next message

    vrpn_HANDLERPARAM & header = newEntry->data;
    retval = fread(&header, sizeof(header), 1, d_file);

    // return 1 if nothing to read OR end-of-file;
    // the latter isn't an error state
    if (retval <= 0) {
        // Don't close the file because we might get a reset message...
        // close_file();
        delete newEntry;
        return 1;
    }

    header.type = ntohl(header.type);
    header.sender = ntohl(header.sender);
    header.msg_time.tv_sec = ntohl(header.msg_time.tv_sec);
    header.msg_time.tv_usec = ntohl(header.msg_time.tv_usec);
    header.payload_len = ntohl(header.payload_len);

    // get the body of the next message

    header.buffer = new char [header.payload_len];
    if (!header.buffer) {
        fprintf(stderr, "vrpn_File_Connection::read_entry:  "
                "Out of memory.\n");
        return -1;
    }

    retval = fread((char *) header.buffer, 1, header.payload_len, d_file);

    // return 1 if nothing to read OR end-of-file;
    // the latter isn't an error state
    if (retval <= 0) {
        // Don't close the file because we might get a reset message...
        //close_file();
        return 1;
    }

    // doubly-linked list maintenance
    newEntry->next = NULL;
    newEntry->prev = d_logTail;
    if (d_logTail) {
        d_logTail->next = newEntry;
    }
    d_logTail = newEntry;
    if (!d_logHead) {
        d_logHead = d_logTail;
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
    d_currentLogEntry = d_startEntry;
    d_time = d_startEntry->data.msg_time;
    // reset for mainloop()
    d_last_time.tv_usec = d_last_time.tv_sec = 0;
    d_filetime_accum.reset_at_time( d_last_time );
    
    return 0;
}

// }}}
// {{{ static handlers

// static
int vrpn_File_Connection::handle_set_replay_rate(
    void * userdata, vrpn_HANDLERPARAM p )
{
    vrpn_File_Connection * me = (vrpn_File_Connection *) userdata;

    vrpn_int32 value = ntohl(*(vrpn_int32 *) (p.buffer));
    me->set_replay_rate(*((vrpn_float32 *) &value));

    return 0;
}

// static
int vrpn_File_Connection::handle_reset( void * userdata, vrpn_HANDLERPARAM )
{
    vrpn_File_Connection * me = (vrpn_File_Connection *) userdata;

//fprintf(stderr, "In vrpn_File_Connection::handle_reset().\n");

    return me->reset();
}

// static
int vrpn_File_Connection::handle_play_to_time(
    void * userdata, vrpn_HANDLERPARAM p )
{
    vrpn_File_Connection * me = (vrpn_File_Connection *) userdata;
    timeval newtime;

    newtime.tv_sec = ((vrpn_int32 *) (p.buffer))[0];
    newtime.tv_usec = ((vrpn_int32 *) (p.buffer))[1];

    return me->play_to_time(newtime);
}

int vrpn_File_Connection::send_pending_reports(void)
{
    // Do nothing except clear the buffer - 
    // file connections aren't really connected to anything. 

   d_tcp_num_out = 0;	// Clear the buffer for the next time
   d_udp_num_out = 0;	// Clear the buffer for the next time
   return 0;
}

// }}}
