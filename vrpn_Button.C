#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h> // for memcpy

#ifdef linux
#include <linux/lp.h>
#endif

#ifndef _WIN32
#include <strings.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif 

#ifdef	_WIN32
#include <io.h>
#endif

#include "vrpn_Button.h"


#define BUTTON_READY 	  (1)
#define BUTTON_FAIL	  (-1)

// Bits in the status register
const unsigned char PORT_ERROR = (1 << 3);
const unsigned char PORT_SLCT = (1 << 4);
const unsigned char PORT_PE = (1 << 5);
const unsigned char PORT_ACK = (1 << 6);
const unsigned char PORT_BUSY = (1 << 7);
const unsigned char 
BIT_MASK = PORT_ERROR | PORT_SLCT | PORT_PE | PORT_ACK | PORT_BUSY;

static int client_msg_handler(void *userdata, vrpn_HANDLERPARAM p);

vrpn_Button::vrpn_Button(char *name, vrpn_Connection *c): num_buttons(0)
{

   // If the connection is valid, use it to register this button by
   // name and the button change report by name.
   connection = c;
  char * servicename;
  servicename = vrpn_copy_service_name(name);
   if (connection != NULL) {
      my_id = connection->register_sender(servicename);
      message_id = connection->register_message_type("Button Toggle");
      if ( (my_id == -1) || (message_id == -1) ) {
      	fprintf(stderr,"vrpn_Button: Can't register IDs\n");
         connection = NULL;
      }
      connection->register_handler(message_id, 
				   /*(vrpn_MESSAGEHANDLER)*/client_msg_handler,
				   this);
   }

   // Set the time to 0 just to have something there.
   timestamp.tv_usec = timestamp.tv_sec = 0;
   for (int i=0; i< vrpn_BUTTON_MAX_BUTTONS; i++) {
	   buttonstate[i] = BUTTON_MOMENTARY;
	   lastbuttons[i] = 0;
   }
  if (servicename)
    delete [] servicename;
}

void vrpn_Button::set_momentary(int which_button) {
  if (which_button >= num_buttons) {
    fprintf(stderr, "vrpn_Button::set_momentary() buttons id %d is greater then the number of buttons(%d)\n", which_button, num_buttons);
    return;
  }
  buttonstate[which_button] = BUTTON_MOMENTARY;
}

void vrpn_Button::set_toggle(int which_button, int current_state) {
  if (which_button >= num_buttons) {
    fprintf(stderr, "vrpn_Button::set_toggle() buttons id %d is greater then the number of buttons(%d)\n", which_button, num_buttons);
    return;
  }
  if (current_state==BUTTON_TOGGLE_ON) 
    buttonstate[which_button] = BUTTON_TOGGLE_ON;
  else 
    buttonstate[which_button] = BUTTON_TOGGLE_OFF;
    
}

void vrpn_Button::set_all_momentary(void) {
  for (int i=0; i< num_buttons; i++)
    if (buttonstate[i] != BUTTON_MOMENTARY) {
      buttonstate[i] = BUTTON_MOMENTARY;  
    }
}

void vrpn_Button::set_all_toggle(int default_state) {
  for (int i=0; i< num_buttons; i++)
    if (buttonstate[i] == BUTTON_MOMENTARY) 
      buttonstate[i] = 
	(default_state==BUTTON_TOGGLE_ON)? BUTTON_TOGGLE_ON: BUTTON_TOGGLE_OFF;
}

void	vrpn_Button::print(void)
{
   int	i;
   printf("CurrButtons: ");
   for (i = num_buttons-1; i >= 0; i--) {
   	printf("%c",buttons[i]?'1':'0');
   }
   printf("\n");
   printf("LastButtons: ");
   for (i = num_buttons-1; i >= 0; i--) {
   	printf("%c",lastbuttons[i]?'1':'0');
   }
   printf("\n");
}

vrpn_Connection *vrpn_Button::connectionPtr() {
  return connection;
}


int	vrpn_Button::encode_to(char *buf, int button, int state)
{
   // Message includes: long buttonNum, long state
   // Byte order of each needs to be reversed to match network standard

   unsigned long *longbuf = (unsigned long*)(void*)(buf);
   int	index = 0;

   longbuf[index++] = htonl(button);
   longbuf[index++] = htonl(state);

   return index*sizeof(unsigned long);
}

#define PACK_MESSAGE(i,event) { \
  char	msgbuf[1000]; \
  int	len = encode_to(msgbuf,i, event); \
  if (connection->pack_message(len, timestamp, \
			       message_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {\
      		fprintf(stderr,"Tracker: can't write message: tossing\n");\
      	}\
	   /*#ifdef	VERBOSE \
         printf("vrpn_Button %d %s\n",i, buttons[i]?"pressed":"released");\
#endif \ */ \
        }

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
  if (t2.tv_sec == -1) return 0;
  return (t1.tv_usec - t2.tv_usec) +
    1000000L * (t1.tv_sec - t2.tv_sec);
}

static int client_msg_handler(void *userdata, vrpn_HANDLERPARAM p) {
  vrpn_Button * instance = (vrpn_Button *) userdata;
  long * bp = (long *)(p.buffer);
  int mode = ntohl(bp[0]);
  int buttonid = ntohl(bp[1]);

  if (mode == BUTTON_MOMENTARY) {
    if (buttonid == ALL_ID)
      instance->set_all_momentary();
    else instance->set_momentary(buttonid);
  }else if (mode == BUTTON_TOGGLE_OFF || mode ==BUTTON_TOGGLE_ON ){
    if (buttonid == ALL_ID)
      instance->set_all_toggle(mode);
    else instance->set_toggle(buttonid,mode);
  } 
  return 0;
}

void	vrpn_Button::report_changes(void)
{
  int	i;

   if (connection) {
      for (i = 0; i < num_buttons; i++) {
	switch (buttonstate[i]) {
	case BUTTON_MOMENTARY:
		if (buttons[i] != lastbuttons[i])
	      PACK_MESSAGE(i, buttons[i]);
	  break;
	case BUTTON_TOGGLE_ON:
	  if (buttons[i] && !lastbuttons[i]) {
	    buttonstate[i] = BUTTON_TOGGLE_OFF;
	    PACK_MESSAGE(i, 0);
	  }
	  break;
	case BUTTON_TOGGLE_OFF:
	  if (buttons[i] && !lastbuttons[i]) {
	    buttonstate[i] = BUTTON_TOGGLE_ON;
	    PACK_MESSAGE(i, 1);
	  }
	  break;
	default:
		fprintf(stderr,"vrpn_Button::report_changes(): Button %d in invalid state (%d)\n",i,buttonstate[i]);
	}
	lastbuttons[i] = buttons[i];
      }
   } else {
   	fprintf(stderr,"vrpn_Button: No valid connection\n");
   }
}

#ifndef VRPN_CLIENT_ONLY

vrpn_parallel_Button::vrpn_parallel_Button(char *name, vrpn_Connection *c,
	int portno) : vrpn_Button(name, c)
{
   int	i;
   char *portname;

   // Find out which port they want
   switch (portno) {
		case 1: portname = "/dev/lp0";
			break;
		case 2: portname = "/dev/lp1";
			break;
		case 3: portname = "/dev/lp2";
			break;
     default:
	fprintf(stderr,"vrpn_parallel_Button: Bad port number (%d)\n",portno);
	status = BUTTON_FAIL;
	portname = "UNKNOWN";
	break;
   }

   // Open the port
   if ( (port = open(portname, O_RDWR)) < 0) {
     perror("vrpn_Button::vrpn_Button(): Can't open port");
	fprintf(stderr,
		"vrpn_Button::vrpn_Button(): Can't open port %s",portname);
	status = BUTTON_FAIL;
	return;
   }

   // Set the INIT line on the device to provide power to the python
   // XXX apparently, we don't need to do this.

   // Zero the button states
   num_buttons = 5;	//XXX This is specific to the python
   for (i = 0; i < num_buttons; i++) {
   	buttons[i] = lastbuttons[i] = 0;
   }

   status = BUTTON_READY;
   gettimeofday(&timestamp, NULL);
}

void vrpn_Button_Python::mainloop(void)
{
  switch (status) {
      case BUTTON_READY:
	read();
	report_changes();
      	break;
      case BUTTON_FAIL:
      	{	static int first = 1;
         	if (first) {
         		first = 0;
	      		fprintf(stderr, "vrpn_Button_Python failure!\n");
         	}
        }
      	break;
   }
}

// Fill in the buttons[] array with the current value of each of the
// buttons
void vrpn_Button_Python::read(void)
{
	int   status_register[3];
	
	// Make sure we're ready to read
	if (status != BUTTON_READY) {
		return;
	}

	// Read from the status register, read 3 times to debounce noise.
#ifdef	linux
	for (int i=0; i< 3; i++) 
	  if (ioctl(port, LPGETSTATUS, &status_register[i]) == -1) {
		perror("vrpn_Button_Python::read(): ioctl() failed");
		return;
	  }

#else
	status_register[0] = status_register[1] = status_register[2] = 0;
#endif
	status_register[0] = status_register[0] & BIT_MASK;
	status_register[1] = status_register[1] & BIT_MASK;
	status_register[2] = status_register[2] & BIT_MASK;
	
	if (!(status_register[0] == status_register[1]  &&
	      status_register[0] == status_register[2])) 
	  return;

	// Assign values to the bits based on the control signals
	buttons[0] = ((status_register[0] & PORT_SLCT) == 0);
	buttons[1] = ((status_register[0] & PORT_BUSY) != 0);
	buttons[2] = ((status_register[0] & PORT_PE) == 0);
	buttons[3] = ((status_register[0] & PORT_ERROR) == 0);
	buttons[4] = ((status_register[0] & PORT_ACK) == 0);

   gettimeofday(&timestamp, NULL);
}

#endif  // VRPN_CLIENT_ONLY


vrpn_Button_Remote::vrpn_Button_Remote(char *name) : 
	vrpn_Button(name, vrpn_get_connection_by_name(name)),
	change_list(NULL)
{
	int	i;

	// Register a handler for the change callback from this device,
	// if we got a connection.
	if (connection != NULL) {
	  if (connection->register_handler(message_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Button_Remote: can't register handler\n");
		connection = NULL;
	  }
	} else {
		fprintf(stderr,"vrpn_Button_Remote: Can't get connection!\n");
	}

	// XXX These should be read from a description message that comes
	// from the button device (as a response to a query?).  For now,
	// we assume a Python.
	num_buttons = 5;
	for (i = 0; i < num_buttons; i++) {
		buttons[i] = lastbuttons[i] = 0;
	}
	gettimeofday(&timestamp, NULL);
}

void	vrpn_Button_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); 
	}
}

int vrpn_Button_Remote::register_change_handler(void *userdata,
			vrpn_BUTTONCHANGEHANDLER handler)
{
	vrpn_BUTTONCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_Button_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_BUTTONCHANGELIST) == NULL) {
		fprintf(stderr,
		    "vrpn_Button_Remote::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = change_list;
	change_list = new_entry;

	return 0;
}

int vrpn_Button_Remote::unregister_change_handler(void *userdata,
			vrpn_BUTTONCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_BUTTONCHANGELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &change_list;
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		   "vrpn_Button_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Button_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Button_Remote *me = (vrpn_Button_Remote *)userdata;
	long *params = (long*)(p.buffer);
	vrpn_BUTTONCB	bp;
	vrpn_BUTTONCHANGELIST *handler = me->change_list;

	// Fill in the parameters to the button from the message
	if (p.payload_len != 2*sizeof(long)) {
		fprintf(stderr,"vrpn_Button: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 2*sizeof(long));
		return -1;
	}
	bp.msg_time = p.msg_time;
	bp.button = (int)ntohl(params[0]);
	bp.state = (int)ntohl(params[1]);

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, bp);
		handler = handler->next;
	}

	return 0;
}


int vrpn_Button_Remote::set_button_mode(int mode, int buttonid) {
  struct timeval current_time;
  //pack the message and send it to the button server through connection
  char msgbuf[1024];
  int len = 3, idata = 0;
	
  
  gettimeofday(&current_time, NULL);
	
  if (connection) {
    long * ip = (long *) msgbuf;
    ip[0] = htonl(mode);
    ip[1] = htonl(buttonid);
    *(msgbuf+2*sizeof(long)) = 0;
    
    if (connection->pack_message(len, current_time, message_id, my_id, 
				 msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr, "can't write message\n");
      return 1;
    }
  } else {
    fprintf(stderr, "vrpn_Button_Remote::set_button_mode: no connection\n");
    return 2;
  }
  return 0;
}
