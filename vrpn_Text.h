#ifndef	VRPN_TEXT_H

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Connection.h"

#define	vrpn_MAX_TEXT_LEN	1024

// Base class for text.  Definition of types.

#define vrpn_TEXT_NORMAL	(0)
#define vrpn_TEXT_WARNING	(1)
#define vrpn_TEXT_ERROR		(2)

typedef	struct {
	struct timeval	msg_time;	// Time of the message
	char		message[vrpn_MAX_TEXT_LEN];	// The message
	vrpn_uint32	type;
	vrpn_uint32	level;
} vrpn_TEXTCB;

typedef void (*vrpn_TEXTHANDLER)(void *userdata, const vrpn_TEXTCB info);

class vrpn_Text {
  public:
	vrpn_Text (char *name, vrpn_Connection *c = NULL);
        virtual ~vrpn_Text (void);

	// Called once through each main loop iteration to handle
	// message updates.
	virtual void mainloop (void);	// Report changes to conneciton

        vrpn_Connection * connectionPtr (void);

  protected:
	vrpn_Connection *connection;
	struct timeval	timestamp;
	vrpn_int32 my_id;		// ID of this text device to connection
	vrpn_int32 message_id;		// ID of text message to connection

	virtual vrpn_int32 encode_to(char *buf, vrpn_uint32 type, vrpn_uint32 level , const char *msg);
	virtual vrpn_int32 decode_to(char *buf, vrpn_uint32 *type, vrpn_uint32 *level , const char *msg);
};

//----------------------------------------------------------
//************** Users deal with the following *************

// Open a text device that is on the other end of a connection
// and handle updates from it.  This is the type of text device that user
// code will deal with.

class vrpn_Text_Sender: public vrpn_Text {
  public:
	vrpn_Text_Sender(char *name, vrpn_Connection *c = NULL) : vrpn_Text(name, c){};
	int send_message(const char *msg, vrpn_uint32 type = vrpn_TEXT_NORMAL,
			 vrpn_uint32 level = 0);

  protected:
};


class vrpn_Text_Receiver: public vrpn_Text {
  public:
	vrpn_Text_Receiver (char * name, vrpn_Connection * c = NULL);
	virtual ~vrpn_Text_Receiver (void);
	virtual int register_message_handler(void *userdata,
                vrpn_TEXTHANDLER handler);
        virtual int unregister_message_handler(void *userdata,
                vrpn_TEXTHANDLER handler);

  protected:
	static int handle_message (void * userdata, vrpn_HANDLERPARAM p);
	typedef struct vrpn_TCS {
                void                    *userdata;
                vrpn_TEXTHANDLER        handler;
                struct vrpn_TCS         *next;
        } vrpn_TEXTMESSAGELIST;

        vrpn_TEXTMESSAGELIST            *change_list;
};

#define	VRPN_TEXT_H
#endif
