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
	unsigned long	type;
	unsigned long	level;
} vrpn_TEXTCB;

typedef void (*vrpn_TEXTHANDLER)(void *userdata, const vrpn_TEXTCB info);

class vrpn_Text {
  public:
	vrpn_Text(char *name, vrpn_Connection *c = NULL);

	// Called once through each main loop iteration to handle
	// message updates.
	virtual void mainloop(void);	// Report changes to conneciton

        vrpn_Connection *connectionPtr();

  protected:
	vrpn_Connection *connection;
	struct timeval	timestamp;
	long my_id;			// ID of this text device to connection
	long message_id;		// ID of text message to connection

	virtual int encode_to(char *buf, unsigned long type, unsigned long level , const char *msg);
	virtual int decode_to(char *buf, unsigned long *type, unsigned long *level , const char *msg);
};

//----------------------------------------------------------
//************** Users deal with the following *************

// Open a text device that is on the other end of a connection
// and handle updates from it.  This is the type of text device that user
// code will deal with.

class vrpn_Text_Sender: public vrpn_Text {
  public:
	vrpn_Text_Sender(char *name, vrpn_Connection *c = NULL) : vrpn_Text(name, c){};
	int send_message(const char *msg, unsigned long type = vrpn_TEXT_NORMAL,
			 unsigned long level = 0);

  protected:
};


class vrpn_Text_Receiver: public vrpn_Text{
  public:
	vrpn_Text_Receiver (char * name, vrpn_Connection * c = NULL);
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
