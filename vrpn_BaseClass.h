/** @file vrpn_BaseClass.h

  All types of client/server/peer objects in VRPN should be derived from the
  vrpn_BaseClass type described here.  This includes Tracker, Button, Analog,
  Clock, Dial, ForceDevice, Sound, and Text; it should include any user-defined
  objects as well.

  This class both implements code that will be shared by most (if not all)
  objects in the system and forms a skeleton for the definition of new objects
  by requiring certain virtual member functions to be defined.

  See the VRPN web pages or another simple type (such as vrpn_Analog) for an
  example of how to create a new VRPN object type using this as a base class.
*/

/*
Things to do to base objects to convert from VRPN version 4.XX to 5.00:
    In the header file:
	Include the BaseClass header
	Derive from the BaseClass
	Remove mainloop() pure virtual from the base class
	Remove connectionPtr from the base class
	Remove connection and my_id from the data objects
	Declare register_types()
    In the source file:
	Call the base-class constructor
	Call the base-class init() routine.
	Remove parts of the constructor
		Dealing with service and connection set up
		Dealing with registering the sender
		deleting the servicename
	Move the constructor code to register the types into a separate function
	Replace the message registration commands with calls to autodelete ones
	Delete the unregister commands for the message handlers
	Remove the connectionPtr function
	Remove the vrpn_get_connection_by_name clause from the remote
          constructor
	Change connection-> to d_connection->
	Change my_id to d_sender_id
	Remove the timeout parameter to all mainloop() functions
	Put a call to client_mainloop() in the Remote object mainloop() function
Things to do in the server object (server device) files to convert from 4.XX
  to 5.00:
	Replace the message registration commands with calls to autodelete ones
		(Note that the handler for update rate has been removed from
                the tracker
		class -- it should not have been there in the first place.
                This saves the
		derived class from having to unregister the old one before
                registering its
		own).
	Delete the unregister commands for the message handlers
	Change connection-> to d_connection->
	Change my_id to d_sender_id
	Remove the timeout parameter to all mainloop() functions
	Put a call to server_mainloop() in each server mainloop()
*/

#ifndef VRPN_BASECLASS
#define VRPN_BASECLASS

#include "vrpn_Shared.h"

#ifndef	VRPN_NO_STREAMS
  #ifdef VRPN_USE_OLD_STREAMS
	#include <iostream.h>
  #else
	#include <iostream>
  #endif
#endif
#include "vrpn_Connection.h"


const int vrpn_MAX_BCADRS =	100;
///< Internal value for number of BaseClass addresses

/// Since the sending of text messages has been pulled into the base class (so
/// that every object can send error/warning/info messages this way), these
/// definitions have been pulled in here as well.
typedef enum {vrpn_TEXT_NORMAL = 0, vrpn_TEXT_WARNING = 1, vrpn_TEXT_ERROR = 2} vrpn_TEXT_SEVERITY;
const unsigned	vrpn_MAX_TEXT_LEN = 1024;

class	vrpn_BaseClass;

/// Class that handles text/warning/error printing for all objects in the
/// system.
// It is a system class, with one instance of it in existence.  Each object in
// the system registers with this class when it is constructed.  By default,
// this class prints all Warning and Error messages to stdout, prefaced by
// "vrpn Warning(0) from MUMBLE: ", where the 0 indicates the level of the
// message and Warning the severity, and MUMBLE the
// name of the object that sent the message.
//  The user could create their own TextPrinter, and attach whatever objects
// they want to it.

class vrpn_TextPrinter {
  public:
    vrpn_TextPrinter();
    ~vrpn_TextPrinter();

    /// Adds an object to the list of watched objects (multiple registration
    /// of the same object will result in only one printing for each message
    /// from the object). Returns 0 on success and -1 on failure.
    int    add_object(vrpn_BaseClass *o);

    /// Remove an object from the list of watched objects (multiple deletions
    /// of the object will not cause any error condition; deletions of
    /// unregistered objects will not cause errors).
    void    remove_object(vrpn_BaseClass *o);

    /// Change the level of printing for the object (sets the minimum level to
    /// print). Default is Warnings and Errors of all levels.
    void    set_min_level_to_print(vrpn_TEXT_SEVERITY severity, vrpn_uint32 level = 0)
		{ d_severity_to_print = severity; d_level_to_print = level; };

    /// Change the ostream that will be used to print messages.  Setting a
    /// NULL ostream results in no printing.
#ifdef	VRPN_NO_STREAMS
    void   set_ostream_to_use(FILE *o) { d_ostream = o; };
#else
  #ifdef VRPN_USE_OLD_STREAMS
    void   set_ostream_to_use(ostream *o) { d_ostream = o; };
  #else
    void   set_ostream_to_use(std::ostream *o) { d_ostream = o; };
  #endif
#endif

  protected:
    /// Structure to hold the objects that are being watched.
    class vrpn_TextPrinter_Watch_Entry {
      public:
	  vrpn_BaseClass    *obj;		///< Object being watched
	  vrpn_TextPrinter  *me;
 		///< Pointer to this, because used in a static function
	  vrpn_TextPrinter_Watch_Entry	*next;
		///< Pointer to the next one in the list
    };
    vrpn_TextPrinter_Watch_Entry	*d_first_watched_object;   
		///< Head of list of objects being watched

#ifdef	VRPN_NO_STREAMS
    FILE		*d_ostream;		///< Output stream to use
#else
  #ifdef VRPN_USE_OLD_STREAMS
    ostream	*d_ostream;		///< Output stream to use
  #else
    std::ostream	*d_ostream;		///< Output stream to use
  #endif
#endif
    vrpn_TEXT_SEVERITY	d_severity_to_print;	///< Minimum severity to print
    vrpn_uint32		d_level_to_print;	///< Minimum level to print

    /// Handles the text messages that come from the connections for
    /// objects we are watching.
    static  int	text_message_handler(void *userdata, vrpn_HANDLERPARAM p);
};
extern vrpn_TextPrinter	vrpn_System_TextPrinter;

/// INTERNAL class to hold members that there should only be one copy of
/// even when a class inherits from multiple vrpn_BaseClasses because it
/// inherits from multiple user-level classes.  Note that not everything in
/// vrpnBaseClass should be here, because (for example) the registration of
/// types should be done for each parent class.
class vrpn_BaseClassUnique {
  friend class vrpn_TextPrinter;
  public:
	vrpn_BaseClassUnique();
	virtual ~vrpn_BaseClassUnique();
	
  protected:
        vrpn_Connection *d_connection;	///< Connection that this object talks to
        char *d_servicename;		///< Name of this device, not including the connection part
        vrpn_int32 d_sender_id;		///< Sender ID registered with the connection
        vrpn_int32 d_text_message_id;	///< ID for text messages
        vrpn_int32 d_ping_message_id;	///< Ask the server if they are there
        vrpn_int32 d_pong_message_id;	///< Server telling that it is there

	/// Registers a handler with the connection, and remembers to delete at destruction.
	// This is a wrapper for the vrpn_Connection call that registers
	// message handlers.  It should be used rather than the connection's
	// function because this one will remember to unregister all of its handlers
	// at object deletion time.
	// XXX In the future, should write an unregister function, in case
	// someone wants it.
	int register_autodeleted_handler(vrpn_int32 type,
		vrpn_MESSAGEHANDLER handler, void *userdata,
		vrpn_int32 sender = vrpn_ANY_SENDER);

	/// Encodes the body of the text message into a buffer, preparing for sending
	static	int encode_text_message_to_buffer(
		char *buf, vrpn_TEXT_SEVERITY severity, vrpn_uint32 level, const char *msg);

	/// Decodes the body of the text message from a buffer from the connection
	static	int decode_text_message_from_buffer(
		char *msg, vrpn_TEXT_SEVERITY *severity, vrpn_uint32 *level, const char *buf);

	/// Sends a NULL-terminated text message from the device d_sender_id
	int send_text_message(const char *msg, struct timeval timestamp,
		vrpn_TEXT_SEVERITY type = vrpn_TEXT_NORMAL, vrpn_uint32 level = 0);

	/// Handles functions that all servers should provide in their mainloop() (ping/pong, for example)
	/// Should be called by all servers in their mainloop()
	void	server_mainloop(void);

	/// Handles functions that all clients should provide in their mainloop() (warning of no server, for example)
	/// Should be called by all clients in their mainloop()
	void	client_mainloop(void);

  private:
      struct {
	  vrpn_MESSAGEHANDLER	handler;
	  vrpn_int32		sender;
	  vrpn_int32		type;
	  void			*userdata;
      } d_handler_autodeletion_record[vrpn_MAX_BCADRS];
      int   d_num_autodeletions;

      int	d_first_mainloop;		///< First time client_mainloop() or server_mainloop() called?
      struct	timeval	d_time_first_ping;	///< When was the first ping of this unanswered group sent?
      struct	timeval	d_time_last_warned;	///< When is the last time we sent a warning?
      int	d_unanswered_ping;		///< Do we have an outstanding ping request?
      int	d_flatline;			///< Has it been 10+ seconds without a response?

      /// Used by client/server code to request/send "server is alive" (pong) message
      static	int handle_ping(void *userdata, vrpn_HANDLERPARAM p);
      static	int handle_pong(void *userdata, vrpn_HANDLERPARAM p);
      static	int handle_connection_dropped(void *userdata, vrpn_HANDLERPARAM p);
      void	initiate_ping_cycle(void);
};

//---------------------------------------------------------------
/// Class from which all user-level (and other) classes that communicate
/// with vrpn_Connections should derive.

class vrpn_BaseClass : virtual public vrpn_BaseClassUnique {

  public:

	/// Names the device and assigns or opens connection,
        /// calls registration methods
	vrpn_BaseClass (const char * name, vrpn_Connection * c = NULL);

	virtual ~vrpn_BaseClass();

	/// Called once through each main loop iteration to handle updates.
	/// Remote object mainloop() should call d_connection->mainloop().
	/// Server object mainloop() should service the device, and should not
	/// normally call d_connection->mainloop().
	virtual void mainloop () = 0;

	/// Returns a pointer to the connection this object is using
        virtual	vrpn_Connection *connectionPtr() {return d_connection;};

  protected:

	/// Initialize things that the constructor can't. Returns 0 on
        /// success, -1 on failure.
	virtual int init(void);

	/// Register the sender for this device (by default, the name of the
        /// device). Return 0 on success, -1 on fail.
	virtual int register_senders(void);

	/// Register the types of messages this device sends/receives.
        /// Return 0 on success, -1 on fail.
	virtual int register_types(void) = 0;
};

// End of defined VRPN_BASECLASS for vrpn_BaseClass.h
#endif
