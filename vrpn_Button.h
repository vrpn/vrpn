#ifndef	VRPN_BUTTON_H

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Connection.h"

#define	vrpn_BUTTON_MAX_BUTTONS	(100)

// Base class for buttons.  Definition
// of remote button class for the user is at the end.

#define vrpn_BUTTON_MOMENTARY	10
#define vrpn_BUTTON_TOGGLE_OFF	20
#define vrpn_BUTTON_TOGGLE_ON	21
#define vrpn_BUTTON_LIGHT_OFF	30
#define vrpn_BUTTON_LIGHT_ON	31
#define vrpn_ALL_ID		-99

class vrpn_Button {
  public:
	vrpn_Button(char *name, vrpn_Connection *c = NULL);

	// Print the status of the button
	void print(void);

	// Called once through each main loop iteration to handle
	// button updates.
	virtual void mainloop(void) = 0;	// Report changes to conneciton

        vrpn_Connection *connectionPtr();

	virtual void set_momentary(int which_button);
        virtual void set_toggle(int which_button, int current_state);
        virtual void set_all_momentary(void);
        virtual void set_all_toggle(int default_state);

  protected:
	vrpn_Connection *connection;
	unsigned char	buttons[vrpn_BUTTON_MAX_BUTTONS];
        unsigned char	lastbuttons[vrpn_BUTTON_MAX_BUTTONS];
	long     minrate[vrpn_BUTTON_MAX_BUTTONS];
	int	num_buttons;
	struct timeval	timestamp;
	long my_id;			// ID of this button to connection
	long change_message_id;		// ID of change button message to connection
	long admin_message_id;		// ID of admin button message to connection
	virtual void report_changes();
	virtual int encode_to(char *buf, int button, int state);
};

class vrpn_Button_Filter:public vrpn_Button{
	public:
		int     buttonstate[vrpn_BUTTON_MAX_BUTTONS];
	        virtual void set_momentary(int which_button);
       		virtual void set_toggle(int which_button, int current_state);
		virtual void set_all_momentary(void);
		virtual void set_all_toggle(int default_state);
		void set_alerts(int);

	protected:
		int send_alerts;
		vrpn_Button_Filter(char *,vrpn_Connection *c=NULL);
		long alert_message_id;   //used to send back to alert button box for lights
		void report_changes(void);
};

#ifndef VRPN_CLIENT_ONLY

// Button device that is connected to a parallel port and uses the
// status bits to read from the buttons.  There can be up to 5 buttons
// read this way.
class vrpn_parallel_Button: public vrpn_Button_Filter {
  public:
	// Open a button connected to the local machine, talk to the
	// outside world through the connection.
	vrpn_parallel_Button(char *name, vrpn_Connection *connection,
		int portno);

  protected:
	int	port;
	int	status;

	virtual void read(void) = 0;
#ifdef _WIN32
	int openGiveIO(void);
#endif // _WIN32

};

// Open a Python that is connected to a parallel port on this Linux box.
class vrpn_Button_Python: public vrpn_parallel_Button {
  public:
	vrpn_Button_Python(char *name, vrpn_Connection *c, int p):
		vrpn_parallel_Button(name,c,p) {};

	virtual void mainloop(void);
  protected:
  	virtual void read(void);
};

#endif  // VRPN_CLIENT_ONLY


//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle a change in button state.  This is called when
// the button callback is called (when a message from its counterpart
// across the connetion arrives).
#define VRPN_BUTTON_OFF	(0)
#define VRPN_BUTTON_ON	(1)

typedef	struct {
	struct timeval	msg_time;	// Time of button press/release
	int		button;		// Which button (numbered from zero)
	int		state;		// New state (0 = off, 1 = on)
} vrpn_BUTTONCB;
typedef void (*vrpn_BUTTONCHANGEHANDLER)(void *userdata,
					 const vrpn_BUTTONCB info);

// Open a button that is on the other end of a connection
// and handle updates from it.  This is the type of button that user code will
// deal with.

class vrpn_Button_Remote: public vrpn_Button {
  public:
	// The name of the button device to connect to
	vrpn_Button_Remote(char *name);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// (un)Register a callback handler to handle a button state change
	virtual int register_change_handler(void *userdata,
		vrpn_BUTTONCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_BUTTONCHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RBCS {
		void				*userdata;
		vrpn_BUTTONCHANGEHANDLER	handler;
		struct vrpn_RBCS		*next;
	} vrpn_BUTTONCHANGELIST;
	vrpn_BUTTONCHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
};

#define	VRPN_BUTTON_H
#endif
