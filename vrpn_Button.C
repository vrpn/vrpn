#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef linux
#include <linux/lp.h>
#endif

#ifndef _WIN32
#include <strings.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif 

#include "vrpn_Button.h"
#include "vrpn_Shared.h"		 // defines gettimeofday for WIN32

#define BUTTON_READY 	  (1)
#define BUTTON_FAIL	  (-1)

// Bits in the status register
const unsigned char PORT_ERROR = (1 << 3);
const unsigned char PORT_SLCT = (1 << 4);
const unsigned char PORT_PE = (1 << 5);
const unsigned char PORT_ACK = (1 << 6);
const unsigned char PORT_BUSY = (1 << 7);


vrpn_Button::vrpn_Button(char *name, vrpn_Connection *c): num_buttons(0)
{

   // If the connection is valid, use it to register this button by
   // name and the button change report by name.
   connection = c;
   if (connection != NULL) {
      my_id = connection->register_sender(name);
      message_id = connection->register_message_type("Button Toggle");
      if ( (my_id == -1) || (message_id == -1) ) {
      	fprintf(stderr,"vrpn_Button: Can't register IDs\n");
         connection = NULL;
      }
   }

   // Set the time to 0 just to have something there.
   timestamp.tv_usec = timestamp.tv_sec = 0;
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

void	vrpn_Button::report_changes(void)
{  int	i;
   if (connection) {
      for (i = 0; i < num_buttons; i++) {
       if (buttons[i] != lastbuttons[i]) {
   	char	msgbuf[1000];
      	int	len = encode_to(msgbuf,i,buttons[i]);

      	if (connection->pack_message(len, timestamp,
      			message_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      		fprintf(stderr,"Tracker: can't write message: tossing\n");
      	}
#ifdef	VERBOSE
         printf("vrpn_Button %d %s\n",i, buttons[i]?"pressed":"released");
#endif
         lastbuttons[i] = buttons[i];
        }
      }
   } else {
   	fprintf(stderr,"vrpn_Button: No valid connection\n");
   }
}

#ifndef _WIN32
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
     default:
	fprintf(stderr,"vrpn_parallel_Button: Bad port number (%d)\n",portno);
	status = BUTTON_FAIL;
	break;
   }

   // Open the port
   if ( (port = open(portname, O_RDWR)) < 0) {
	perror("vrpn_Button::vrpn_Button(): Can't open port");
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
	int   status_register;

	// Make sure we're ready to read
	if (status != BUTTON_READY) {
		return;
	}

	// Read from the status register
#ifdef	linux
	if (ioctl(port, LPGETSTATUS, &status_register) == -1) {
		perror("vrpn_Button_Python::read(): ioctl() failed");
		return;
	}
#else
	status_register = 0;
#endif

	// Assign values to the bits based on the control signals
	buttons[0] = ((status_register & PORT_SLCT) == 0);
	buttons[1] = ((status_register & PORT_BUSY) != 0);
	buttons[2] = ((status_register & PORT_PE) == 0);
	buttons[3] = ((status_register & PORT_ERROR) == 0);
	buttons[4] = ((status_register & PORT_ACK) == 0);

   gettimeofday(&timestamp, NULL);
}


vrpn_Button_Remote::vrpn_Button_Remote(char *name) : 
	vrpn_Button(name, vrpn_get_connection_by_name(name)),
	change_list(NULL)
{
	int	i;

	// Register a handler for the change callback from this device.
	if (connection->register_handler(message_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Button_Remote: can't register handler\n");
		connection = NULL;
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
	if (connection) { connection->mainloop(); }
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

#endif // #ifndef _WIN32

