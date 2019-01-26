// vrpn_BaseClass.C

#include <stddef.h> // for size_t
#include <stdio.h>  // for fprintf, NULL, stderr, etc
#include <string.h> // for strcmp, strlen

#include "vrpn_BaseClass.h"
#include "vrpn_Shared.h" // for timeval, vrpn_buffer, etc

//#define	VERBOSE

/** Definition of the system TextPrinter object that prints messages for
    all created objects.
*/
static vrpn_TextPrinter vrpn_System_TextPrinter_instance;
// SWIG does not like this statement
#ifndef SWIG
vrpn_TextPrinter &vrpn_System_TextPrinter = vrpn_System_TextPrinter_instance;
#endif

vrpn_TextPrinter::vrpn_TextPrinter()
    : d_first_watched_object(NULL)
    , d_ostream(stdout)
    , d_severity_to_print(vrpn_TEXT_WARNING)
    , d_level_to_print(0)
{
}

/** Deletes any callbacks that are still registered. */
vrpn_TextPrinter::~vrpn_TextPrinter()
{
    vrpn::SemaphoreGuard guard(d_semaphore);
/* XXX No longer removes these.  We get into trouble with the
   system-defined vrpn_System_TextPrinter destructor because it
   may run after the vrpn_ConnectionManager destructor has run,
   which (if we have some undeleted objects) will leave objects
   that don't have a NULL connection pointer, but whose pointers
   point to already-deleted connections.  This causes a crash. */
/* XXX Did the above change to making the vrpn_System_TextPrinter
   static remove the race condition, so that it is safe to put
   this back in? */
#if 0

    vrpn_TextPrinter_Watch_Entry    *victim, *next;
    vrpn_BaseClass  *obj;

    victim = d_first_watched_object;
    while (victim != NULL) {
	next = victim->next;
	obj = victim->obj;

        // Guard against the case where the object has set its connection pointer
        // to NULL, which is how some objects notify themselves that they are
        // broken.
        if (obj->connectionPtr()) {
          obj->connectionPtr()->unregister_handler(obj->d_text_message_id, text_message_handler, victim, obj->d_sender_id);
        }
        try {
          delete victim;
        } catch (...) {
          fprintf(stderr, "vrpn_TextPrinter::~vrpn_TextPrinter: delete failed\n");
          return;
        }
	victim = next;
    }
#endif // XXX
}

/** Adds an object to the list of watched objects.  Returns 0 on success and
    -1 on failure.  Registering a watched object is idempotent: doing so again
    for an already-registered object has no effect, nor is it an error.  Objects
    are considered to be the same if they share the same connection and they
   have
    the same service name.  What we're going for here is to get only one
   print-out of any
    message.
*/
int vrpn_TextPrinter::add_object(vrpn_BaseClass *o)
{
    vrpn::SemaphoreGuard guard(d_semaphore);
    vrpn_TextPrinter_Watch_Entry *victim;

    // Make sure we have an actual object.
    if (o == NULL) {
        fprintf(stderr,
                "vrpn_TextPrinter::add_object(): NULL pointer passed\n");
        return -1;
    }

#ifdef VERBOSE
    printf("vrpn_TextPrinter: adding object %s\n", o->d_servicename);
#endif

    // If the object is already in the list, we are done.  It is considered the
    // same
    // object if it has the same connection and the same service name.
    victim = d_first_watched_object;
    while (victim != NULL) {
        if ((o->d_connection == victim->obj->d_connection) &&
            (strcmp(o->d_servicename, victim->obj->d_servicename) == 0)) {
            return 0;
        }
        victim = victim->next;
    }

    // Add the object to the beginning of the list.
    try { victim = new vrpn_TextPrinter_Watch_Entry; }
    catch (...) {
        fprintf(stderr, "vrpn_TextPrinter::add_object(): out of memory\n");
        return -1;
    }
    victim->obj = o;
    victim->me = this;
    victim->next = d_first_watched_object;
    d_first_watched_object = victim;

    // Register a callback for the object
    if (o->d_connection->register_handler(o->d_text_message_id,
                                          text_message_handler, victim,
                                          o->d_sender_id) != 0) {
        fprintf(stderr,
                "vrpn_TextPrinter::add_object(): Can't register callback\n");
        d_first_watched_object = victim->next;
        try {
          delete victim;
        } catch (...) {
          fprintf(stderr, "vrpn_TextPrinter::add_object: delete failed\n");
          return -1;
        }
        return -1;
    }

    return 0;
}

/** Removes an object from the list of watched objects.  Returns 0 on success
   and -1 on failure.  Unregistering a non-watched object has no effect, nor is
   it an error.  Objects are considered to be the same if they share the same
   connection and they have the same service name.
*/
void vrpn_TextPrinter::remove_object(vrpn_BaseClass *o)
{
    vrpn::SemaphoreGuard guard(d_semaphore);
    vrpn_TextPrinter_Watch_Entry *victim, **snitch;

#ifdef VERBOSE
    printf("vrpn_TextPrinter: removing object %s\n", o->d_servicename);
#endif

    // Make sure we have an actual object.
    if (o == NULL) {
        fprintf(stderr,
                "vrpn_TextPrinter::remove_object(): NULL pointer passed\n");
        return;
    }

    // Find the entry in the list (if it is there).
    // Starts this pointing at the first watched object, so it will
    // update that object if it is the one who pointed us at the one
    // to be deleted.
    snitch = &d_first_watched_object;
    victim = *snitch;
    while ((victim != NULL) &&
           ((o->d_connection != victim->obj->d_connection) ||
            (strcmp(o->d_servicename, victim->obj->d_servicename) != 0))) {

        snitch = &((*snitch)->next);
        victim = victim->next;
    }

    // If the object is on the list, unregister its callback and delete it.
    if (victim != NULL) {
        // Unregister the callback for the object, unless its d_connetion
        // pointer
        // is NULL (which is a convention used by devices to indicate that they
        // are broken, so we need to guard against it here).
        if (o->d_connection) {
            if (o->d_connection->unregister_handler(
                    o->d_text_message_id, text_message_handler, victim,
                    o->d_sender_id) != 0) {
                fprintf(stderr, "vrpn_TextPrinter::remove_object(): Can't "
                                "unregister callback\n");
            }
        }

        // Remove the entry from the list
        *snitch = victim->next;
        try {
          delete victim;
        } catch (...) {
          fprintf(stderr, "vrpn_TextPrinter::remove_object: delete failed\n");
          return;
        }

        // We're done.
        return;
    }

    // Object not in the list, so we're done.
    return;
}

/** Prints out a text message that comes in, identifying the severity, level and
   sender of the
    message. The message is not printed if the severity or level do not pass the
   threshold
    for severity or level.
*/

int vrpn_TextPrinter::text_message_handler(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_TextPrinter_Watch_Entry *entry =
        (vrpn_TextPrinter_Watch_Entry *)userdata;
    vrpn_TextPrinter *me = entry->me;
    vrpn_BaseClass *obj = entry->obj;
    vrpn_TEXT_SEVERITY severity;
    vrpn_uint32 level;
    char message[vrpn_MAX_TEXT_LEN];
    vrpn::SemaphoreGuard guard(me->d_semaphore);

#ifdef VERBOSE
    printf("vrpn_TextPrinter: text handler called\n");
#endif

    // Make sure we have a valid ostream.
    if (me->d_ostream == NULL) {
        return 0;
    };

    // Decode the message
    if (vrpn_BaseClassUnique::decode_text_message_from_buffer(
            message, &severity, &level, p.buffer) != 0) {
        fprintf(
            stderr,
            "vrpn_TextPrinter::text_message_handler(): Can't decode message\n");
        return -1;
    }

    // If the severity and level criteria pass, then print the annotated
    // message.
    // The printing is "VRPN", then one of Message/Warning/Error, then the level
    // of the
    // text, "from" the name of the service sending the message, and then the
    // message.
    if ((severity > me->d_severity_to_print) ||
        ((severity == me->d_severity_to_print) &&
         (level >= me->d_level_to_print))) {

        fprintf(me->d_ostream, "VRPN ");

        switch (severity) {
        case vrpn_TEXT_NORMAL:
            fprintf(me->d_ostream, "Message\n");
            break;
        case vrpn_TEXT_WARNING:
            fprintf(me->d_ostream, "Warning\n");
            break;
        case vrpn_TEXT_ERROR:
            fprintf(me->d_ostream, "Error\n");
            break;
        default:
            fprintf(me->d_ostream, "UNKNOWN SEVERITY\n");
            break;
        }

        fprintf(me->d_ostream, " (%d) from %s: %s\n", level,
                obj->d_connection->sender_name(p.sender), message);
    }
    return 0;
}

/// Change the level of printing for the object (sets the minimum level to
/// print). Default is Warnings and Errors of all levels.
void vrpn_TextPrinter::set_min_level_to_print(vrpn_TEXT_SEVERITY severity,
                                              vrpn_uint32 level)
{
    vrpn::SemaphoreGuard guard(d_semaphore);
    d_severity_to_print = severity;
    d_level_to_print = level;
}

/// Change the ostream that will be used to print messages.  Setting a
/// NULL ostream results in no printing.
void vrpn_TextPrinter::set_ostream_to_use(FILE *o)
{
    vrpn::SemaphoreGuard guard(d_semaphore);
    d_ostream = o;
}

/** Assigns the connection passed in to the object, or else tries to
    create a new connection based on the object name.  If this succeeds,
    it calls the sender and type registration routines for the object.
    Sets the servicename field to be the part of the name coming before
    the "@" sign in the name.

    vrpn_BaseClassUnique is a virtual base class so it will only be called once,
   while vrpn_BaseClass may be called multiple times. Setting d_connection and
   d_servicename, only needs to be done once for each object (even if it
   inherits from multiple device classes).  So these things should technically
   go into the vrpn_BaseClassUnique constructor, except that it is unable to
   accept parameters.  If the vrpn_BaseClassUnique constructor *did* take the
   service-name, connnection, and use-ref-count parameters then every derived
   class (not just those at the top level) would have to make an explicit call
   to the vrpn_BaseClassUnique constructor.  As it stands, these derived classes
   will instead use the 0-parameter version of the vrpn_BaseClassUnique
   constructor implicitly. As a result, this constructor must make sure that it
   only executes the code therein once.
*/

vrpn_BaseClass::vrpn_BaseClass(const char *name, vrpn_Connection *c)
  : vrpn_BaseClassUnique()
{
    // Has a constructor on this BaseClassUnique been called before?
    // Note that this might also be true if it was called once before but
    // failed.
    bool firstTimeCalled = (d_connection == NULL);

    if (firstTimeCalled) {
        // Get the connection for this object established. If the user passed in
        // a NULL connection object, then we determine the connection from the
        // name of the object itself (for example, Tracker0@mumble.cs.unc.edu
        // will make a connection to the machine mumble on the standard VRPN
        // port).
        //
        // The vrpn_BaseClassUnique destructor handles telling the connection we
        // are no longer referring to it.  Since we only add the reference once
        // here (when d_connection is NULL), it is okay for the unique
        // destructor to remove the reference.
        if (c) { // using existing connection.
            d_connection = c;
            d_connection->addReference();
        } else {
            // This will implicitly add the reference to the connection.
            d_connection = vrpn_get_connection_by_name(name);
        }

        // Get the part of the name for this device that does not include the
        // connection.
        // The vrpn_BaseClassUnique destructor handles the deletion of the
        // space.
        d_servicename = vrpn_copy_service_name(name);
    }
}

vrpn_BaseClass::~vrpn_BaseClass()
{
    // Remove us from the list of objects with messages to be printed
    vrpn_System_TextPrinter.remove_object(this);
}

/** This would normally be found in the constructor, but the constructor
    cannot call virtual functions in the derived class (since it does not
    yet exist). This function needs to be called at the beginning of the
    constructor of each class that derives directly from vrpn_BaseClass
....(i.e. the top-level device classes such as Button, Analog, Tracker, etc)
*/

int vrpn_BaseClass::init(void)
{
    // In the case of multiple inheritance from this base class, the rest of
    //  the code in this function will be executed each time init is called.

    if (d_connection) {
        // Register the sender and types
        // that this device type uses.  If one of these fails, set the
        // connection
        // for this object to NULL to indicate failure, and print an error
        // message.
        if (register_senders() || register_types()) {
            fprintf(stderr, "vrpn_BaseClassUnique: Can't register IDs\n");
            d_connection = NULL;
            return -1;
        }

        // Register the text and ping/pong types, which will be available to all
        // classes for use.
        d_text_message_id =
            d_connection->register_message_type("vrpn_Base text_message");
        if (d_text_message_id == -1) {
            fprintf(stderr,
                    "vrpn_BaseClassUnique: Can't register Text type ID\n");
            d_connection = NULL;
            return -1;
        }
        d_ping_message_id =
            d_connection->register_message_type("vrpn_Base ping_message");
        if (d_ping_message_id == -1) {
            fprintf(stderr,
                    "vrpn_BaseClassUnique: Can't register ping type ID\n");
            d_connection = NULL;
            return -1;
        }
        d_pong_message_id =
            d_connection->register_message_type("vrpn_Base pong_message");
        if (d_pong_message_id == -1) {
            fprintf(stderr,
                    "vrpn_BaseClassUnique: Can't register pong type ID\n");
            d_connection = NULL;
            return -1;
        }

        // Sign us up with the standard print function.
        vrpn_System_TextPrinter.add_object(this);
        return 0;
    }
    else {
        // Error if we don't have a connection.
        return -1;
    }
}

/** Registers the senders (usually only one, that part of the name of the
    device coming before the "@" sign).  For example, the sender for
    Tracker0@mumble.cs.unc.edu is Tracker0.  Both the remote device and the
    server device will register the same sender.  If for some reason, there
    is a different sender or more than one sender, this function should be
    overridden by both the remote and server objects.

    This routine returns 0 on success and -1 on failure.
*/

int vrpn_BaseClass::register_senders()
{
    if (d_connection == NULL) {
        return -1;
    }
    d_sender_id = d_connection->register_sender(d_servicename);
    if (d_sender_id == -1) {
        return -1;
    }
    else {
        return 0;
    }
}

vrpn_BaseClassUnique::vrpn_BaseClassUnique()
    : shutup(false)
    , // don't suppress the "No response from server" messages
    d_connection(NULL)
    , d_servicename(NULL)
    , d_sender_id(-1)
    , d_num_autodeletions(0)
    , d_first_mainloop(1)
    , d_unanswered_ping(0)
    , d_flatline(0)
{
    // Initialize variables
    d_time_first_ping.tv_sec = d_time_first_ping.tv_usec = 0;
}

/** Unregister all of the message handlers that were to be autodeleted.
    Delete space allocated in the constructor.
*/

vrpn_BaseClassUnique::~vrpn_BaseClassUnique()
{
    int i;

    // Unregister all of the handlers that were to be autodeleted,
    // if we have a connection.
    if (d_connection != NULL) {
        for (i = 0; i < d_num_autodeletions; i++) {
            d_connection->unregister_handler(
                d_handler_autodeletion_record[i].type,
                d_handler_autodeletion_record[i].handler,
                d_handler_autodeletion_record[i].userdata,
                d_handler_autodeletion_record[i].sender);
        }
        d_num_autodeletions = 0;
    }

    // notify the connection that this object is no longer using it.
    // This was added in the vrpn_BaseClass constructor for exactly one of the
    // objects that are sharing this unique destructor.
    if (d_connection != NULL) {
        d_connection->removeReference();
    }

    // Delete the space allocated in the constructor for the servicename
    // Because this destructor may be called multiple times, set the pointer
    // to NULL after deleting it, so it won't get deleted again.
    if (d_servicename != NULL) {
        try {
          delete[] d_servicename;
        } catch (...) {
          fprintf(stderr, "vrpn_BaseClassUnique::~vrpn_BaseClassUnique: delete failed\n");
          return;
        }
        d_servicename = NULL;
    }
}

/** This function is a wrapper for the vrpn_Connection register_handler()
    routine.  It also keeps track of all of the handlers registered by an
    object and unregisters them automatically when the object is destroyed.
    This routine should be used, rather than the Connection one, to ensure
    that they are all unregistered.  If they are not, and a message comes
    in after the object is destroyed, it will likely cause a Segmentation
    Violation.

    The function returns 0 on success and -1 on failure.
*/

int vrpn_BaseClassUnique::register_autodeleted_handler(
    vrpn_int32 type, vrpn_MESSAGEHANDLER handler, void *userdata,
    vrpn_int32 sender)
{
    // Make sure we have a Connection
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_BaseClassUnique::register_autodeleted_handler: "
                        "No vrpn_Connection.\n");
        return -1;
    }

    // Make sure we have an empy entry to fill in.
    if (d_num_autodeletions >= vrpn_MAX_BCADRS) {
        fprintf(stderr,
                "vrpn_BaseClassUnique::register_autodeleted_handler: "
                "Too many handlers registered.  Increase vrpn_MAX_BCADRS "
                "and recompile VRPN.  Please report to vrpn@cs.unc.edu.\n");
        return -1;
    }

    // Fill in the values so we know what to delete, and bump the count
    d_handler_autodeletion_record[d_num_autodeletions].handler = handler;
    d_handler_autodeletion_record[d_num_autodeletions].sender = sender;
    d_handler_autodeletion_record[d_num_autodeletions].type = type;
    d_handler_autodeletion_record[d_num_autodeletions].userdata = userdata;
    d_num_autodeletions++;

    // Call the register command.
    return d_connection->register_handler(type, handler, userdata, sender);
}

int vrpn_BaseClassUnique::encode_text_message_to_buffer(
    char *buf, vrpn_TEXT_SEVERITY severity, vrpn_uint32 level, const char *msg)
{
    char *bufptr = buf;
    int buflen = 2 * sizeof(vrpn_int32) + vrpn_MAX_TEXT_LEN;
    vrpn_uint32 severity_as_uint = severity;

    // Send the type, level and string message
    vrpn_buffer(&bufptr, &buflen, severity_as_uint);
    vrpn_buffer(&bufptr, &buflen, level);
    vrpn_buffer(&bufptr, &buflen, msg, -1); // -1 means "pack until NULL"

    return 0;
}

int vrpn_BaseClassUnique::decode_text_message_from_buffer(
    char *msg, vrpn_TEXT_SEVERITY *severity, vrpn_uint32 *level,
    const char *buf)
{
    const char *bufptr = buf;
    vrpn_uint32 severity_as_uint;

    // Read the type, level and message
    vrpn_unbuffer(&bufptr, &severity_as_uint);
    *severity = (vrpn_TEXT_SEVERITY)(severity_as_uint);
    vrpn_unbuffer(&bufptr, level);
    // Negative length means "unpack until NULL"
    if (vrpn_unbuffer(&bufptr, msg, -(int)(vrpn_MAX_TEXT_LEN)) != 0) {
        return -1;
    }

    return 0;
}

int vrpn_BaseClassUnique::send_text_message(const char *msg,
                                            struct timeval timestamp,
                                            vrpn_TEXT_SEVERITY type,
                                            vrpn_uint32 level)
{
    char buffer[2 * sizeof(vrpn_int32) + vrpn_MAX_TEXT_LEN];
    size_t len = strlen(msg) + 1; // +1 is for the NULL terminator

    if (len > vrpn_MAX_TEXT_LEN) {
        fprintf(stderr, "vrpn_BaseClassUnique::send_message: Attempt to encode "
                        "string that is too long\n");
        return -1;
    }

    // send type, level and message

    encode_text_message_to_buffer(buffer, type, level, msg);
    if (d_connection) {
        d_connection->pack_message(sizeof(buffer), timestamp, d_text_message_id,
                                   d_sender_id, buffer,
                                   vrpn_CONNECTION_RELIABLE);
    }

    return 0;
}

/** This routine handles functions that all servers should perform in their
   mainloop().
    It should be called each time through by each server's mainloop() function.
    Performed functions include:
    Sending pong ("server is alive") messages so that clients can know if they
   have
        connected to the server.
*/

void vrpn_BaseClassUnique::server_mainloop(void)
{
    // Set up to handle pong message.  This should be sent whenever there is
    // a ping request from a client.

    if (d_first_mainloop && (d_connection != NULL)) {
        register_autodeleted_handler(d_ping_message_id, handle_ping, this,
                                     d_sender_id);
        d_first_mainloop = 0;
    }
}

/** This routine handles functions that all clients should perform in their
   mainloop().
    It should be called each time through a client's mainloop() function.
    Performed functions include:
    Handling the Ping/Pong messages that tell the client if the server is alive:
        Client initiates ping/pong cycle when client is created and when its
   connection is dropped
        This initiation is done the first time through client_mainloop()
        It is done again in a handler for the "dropped_connection" system
   message
        During ping/pong cycle, client sends ping requests once/second and waits
   for response
        At the start of the cycle, d_unanswered_ping is set to 1 and
   d_first_ping_time is set
        Handler for pong message sets d_unanswered_ping to 0 when we get one
        Prints warning messages every second after 3+ seconds with no pong
        Prints error messages every second after 10+ seconds with no pong
   (flatlined)
        Server responds to ping message with pong message
        Handler for ping set up the first time through server_mainloop()
*/

void vrpn_BaseClassUnique::client_mainloop(void)
{
    struct timeval now;
    struct timeval diff;

    // The first time through, set up a callback handler for the pong message so
    // that we
    // know when we are getting them.  Also set up a handler for the system
    // dropped-connection
    // message so that we can initiate a ping cycle when that happens.  Also,
    // we'll initiate
    // the ping cycle here.

    if (d_first_mainloop && (d_connection != NULL)) {

        // Set up handlers for the pong message and for the system
        // connection-drop message
        register_autodeleted_handler(d_pong_message_id, handle_pong, this,
                                     d_sender_id);
        register_autodeleted_handler(
            d_connection->register_message_type(vrpn_dropped_connection),
            handle_connection_dropped, this);

        // Initiate a ping cycle;
        initiate_ping_cycle();

        // No longer first time through mainloop.
        d_first_mainloop = 0;
    }

    // If we are in the middle of a ping cycle...
    // Check if we've heard, if it has been long enough since we gave a warning
    // or error (>= 1 sec).
    // If it has been three seconds or more since we sent our first ping,
    // start giving warnings.  If it has been ten seconds or more since we got
    // one,
    // switch to errors.  New ping requests go out each second.

    if (d_unanswered_ping) {

        vrpn_gettimeofday(&now, NULL);
        diff = vrpn_TimevalDiff(now, d_time_last_warned);
        vrpn_TimevalNormalize(diff);

        if (diff.tv_sec >= 1) {

            // Send a new ping, since it has been a second since the last one
            d_connection->pack_message(0, now, d_ping_message_id, d_sender_id,
                                       NULL, vrpn_CONNECTION_RELIABLE);

            // Send another warning or error, and say if we're flatlined (10+
            // seconds)
            d_time_last_warned = now;
            if (!shutup) {
                diff = vrpn_TimevalDiff(now, d_time_first_ping);
                vrpn_TimevalNormalize(diff);
                if (diff.tv_sec >= 10) {
                    send_text_message(
                        "No response from server for >= 10 seconds", now,
                        vrpn_TEXT_ERROR, diff.tv_sec);
                    d_flatline = 1;
                }
                else if (diff.tv_sec >= 3) {
                    send_text_message(
                        "No response from server for >= 3 seconds", now,
                        vrpn_TEXT_WARNING, diff.tv_sec);
                }
            }
        }
    }
}

void vrpn_BaseClassUnique::initiate_ping_cycle(void)
{
    // Record when we sent the ping and say that we haven't gotten an answer
    vrpn_gettimeofday(&d_time_first_ping, NULL);
    d_connection->pack_message(0, d_time_first_ping, d_ping_message_id,
                               d_sender_id, NULL, vrpn_CONNECTION_RELIABLE);
    d_unanswered_ping = 1;

    // We didn't send a warning about this one yet...
    d_time_last_warned.tv_sec = d_time_last_warned.tv_usec = 0;
}

/** Store the time at which the last pong occurred.  Used by client_mainloop()
   to keep
    track of how long it has been since we got one.  The callback for this
   handler is registered
    in client_mainloop() the first time through.
*/

int vrpn_BaseClassUnique::handle_pong(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_BaseClassUnique *me = (vrpn_BaseClassUnique *)userdata;

    me->d_unanswered_ping = 0;

    // If we were flatlined, report that things are okay again.
    if (me->d_flatline) {
        me->send_text_message("Server connection re-established!", p.msg_time,
                              vrpn_TEXT_ERROR);
        me->d_flatline = 0;
    }

    return 0;
}

/** Respond with a "pong" (server is here) message, to the client "ping"
    (is the server there?) message.  This message handler is put in place for
   servers
    the first time through server_mainloop(). See client_mainloop() header for a
   description
    of the full algorithm.
*/

int vrpn_BaseClassUnique::handle_ping(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_BaseClassUnique *me = (vrpn_BaseClassUnique *)userdata;
    struct timeval now;

    vrpn_gettimeofday(&now, NULL);
    if (me->d_connection != NULL) {
        me->d_connection->pack_message(0, now, me->d_pong_message_id,
                                       me->d_sender_id, NULL,
                                       vrpn_CONNECTION_RELIABLE);
    }

    return 0;
}

/** This handler is called by the client code when the system reports that the
    connection has been dropped.  It initiates a ping cycle, unless we are
   already
    within a ping cycle.
*/

int vrpn_BaseClassUnique::handle_connection_dropped(void *userdata,
                                                    vrpn_HANDLERPARAM)
{
    vrpn_BaseClassUnique *me = (vrpn_BaseClassUnique *)userdata;
    struct timeval now;

    if (me->d_unanswered_ping != 0) {
        return 0;
    }

    vrpn_gettimeofday(&now, NULL);
    if (me->d_connection != NULL) {
        me->initiate_ping_cycle();
    }

    return 0;
}
