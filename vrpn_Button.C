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

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"

#ifdef _WIN32

#ifndef __CYGWIN__
#include <conio.h>  // for _inp()
#endif
#else
#include <netinet/in.h>
#endif

#if (defined(sgi)&&(!defined(VRPN_CLIENT_ONLY)))
//need these for the gl based setdblight calls
#include <gl/gl.h>
#include <gl/device.h>
#endif


#include "vrpn_Button.h"

#define BUTTON_READY 	  (1)
#define BUTTON_FAIL	  (-1)

// Bits in the status register
#ifndef VRPN_CLIENT_ONLY
static const unsigned char PORT_ERROR = (1 << 3);
static const unsigned char PORT_SLCT = (1 << 4);
static const unsigned char PORT_PE = (1 << 5);
static const unsigned char PORT_ACK = (1 << 6);
static const unsigned char PORT_BUSY = (1 << 7);
static const unsigned char 
	BIT_MASK = PORT_ERROR | PORT_SLCT | PORT_PE | PORT_ACK | PORT_BUSY;
#endif

static int client_msg_handler(void *userdata, vrpn_HANDLERPARAM p);
#define PACK_ADMIN_MESSAGE(i,event) { \
  char	msgbuf[1000]; \
  vrpn_int32	len = encode_to(msgbuf,i, event); \
  if (connection->pack_message(len, timestamp, \
			       admin_message_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {\
      		fprintf(stderr,"vrpn_Button: can't write message: tossing\n");\
      	}\
        }
#define PACK_ALERT_MESSAGE(i,event) { \
  char	msgbuf[1000]; \
  vrpn_int32	len = encode_to(msgbuf,i, event); \
  if (connection->pack_message(len, timestamp, \
			       alert_message_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {\
      		fprintf(stderr,"vrpn_Button: can't write message: tossing\n");\
      	}\
        }

#define PACK_MESSAGE(i,event) { \
  char	msgbuf[1000]; \
  vrpn_int32	len = encode_to(msgbuf,i, event); \
  if (connection->pack_message(len, timestamp, \
			       change_message_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {\
      		fprintf(stderr,"vrpn_Button: can't write message: tossing\n");\
      	}\
        }

vrpn_Button::vrpn_Button(const char *name, vrpn_Connection *c)
	: num_buttons(0)
{

   // If the connection is valid, use it to register this button by
   // name and the button change report by name.
   connection = c;
  char * servicename;
  servicename = vrpn_copy_service_name(name);
   if (connection != NULL) {
      my_id = connection->register_sender(servicename);
      //used to handle button strikes
      change_message_id = connection->register_message_type("Button Change");
      //to handle button state changes -- see Buton_Filter should register a handler
      //for this ID -- ideally the message will be ignored otherwise
      admin_message_id = connection->register_message_type("Button Admin");
   }

   // Set the time to 0 just to have something there.
   timestamp.tv_usec = timestamp.tv_sec = 0;
   for (vrpn_int32 i=0; i< vrpn_BUTTON_MAX_BUTTONS; i++) {
	lastbuttons[i]=0;
   }

  if (servicename)
    delete [] servicename;
}

// virtual
vrpn_Button::~vrpn_Button (void) {

  // do nothing

}


vrpn_Button_Filter::vrpn_Button_Filter(const char *name,
									   vrpn_Connection *c)
	:vrpn_Button(name, c)
{
	  if ( (my_id == -1) || (admin_message_id== -1) ) {
        fprintf(stderr,"vrpn_Button: Can't register IDs\n");
         connection = NULL;
      }
      connection->register_handler(admin_message_id,
                                   client_msg_handler,
                                   this);
      //setup message id type for alert messages to alert a device about changes
      alert_message_id = connection->register_message_type("Button Alert");
      send_alerts=0;	//used to turn on/off alerts -- send and admin message from
			//remote to turn it on -- or server side call set_alerts();

      //set button default buttonstates
      for (vrpn_int32 i=0; i< vrpn_BUTTON_MAX_BUTTONS; i++) {
           buttonstate[i] = vrpn_BUTTON_MOMENTARY;
      }
      return;
}
void vrpn_Button_Filter::set_alerts(vrpn_int32 i){
	if(i==0 || i==1) send_alerts=i;
	else fprintf(stderr,"Invalid send_alert state\n");
	return;
}

void vrpn_Button_Filter::set_momentary(vrpn_int32 which_button) {
  if (which_button >= num_buttons) {
       fprintf(stderr, "vrpn_Button::set_momentary() buttons id %d is greater\
	 then the number of buttons(%d)\n", which_button, num_buttons);
    return;
  }
  buttonstate[which_button] = vrpn_BUTTON_MOMENTARY;
  if(send_alerts)PACK_ALERT_MESSAGE(which_button,vrpn_BUTTON_TOGGLE_OFF);
}

void vrpn_Button::set_momentary(vrpn_int32 which_button) {
  if (which_button >= num_buttons) {
       fprintf(stderr, "vrpn_Button::set_momentary() buttons id %d is greater\
	 then the number of buttons(%d)\n", which_button, num_buttons);
    return;
  }
  PACK_ADMIN_MESSAGE(which_button,vrpn_BUTTON_MOMENTARY);
}

void vrpn_Button_Filter::set_toggle(vrpn_int32 which_button, vrpn_int32 current_state) {
  if (which_button >= num_buttons) {
    fprintf(stderr, "vrpn_Button::set_toggle() buttons id %d is greater then the number of buttons(%d)\n", which_button, num_buttons);
    return;
  }
  if (current_state==vrpn_BUTTON_TOGGLE_ON){
     buttonstate[which_button] = vrpn_BUTTON_TOGGLE_ON;
     if(send_alerts)PACK_ALERT_MESSAGE(which_button,vrpn_BUTTON_TOGGLE_ON);
  }
  else{ 
    buttonstate[which_button] = vrpn_BUTTON_TOGGLE_OFF;
    if(send_alerts)PACK_ALERT_MESSAGE(which_button,vrpn_BUTTON_TOGGLE_OFF);
  }
    
}
void vrpn_Button::set_toggle(vrpn_int32 which_button, vrpn_int32 current_state) {
  if (which_button >= num_buttons) {
    fprintf(stderr, "vrpn_Button::set_toggle() buttons id %d is greater then the number of buttons(%d)\n", which_button, num_buttons);
    return;
  }
  if (current_state==vrpn_BUTTON_TOGGLE_ON) {
    PACK_ADMIN_MESSAGE(which_button,vrpn_BUTTON_TOGGLE_ON);
  }
  else{
    PACK_ADMIN_MESSAGE(which_button,vrpn_BUTTON_TOGGLE_OFF);
  }
}

void vrpn_Button_Filter::set_all_momentary(void) {
  for (vrpn_int32 i=0; i< num_buttons; i++){
    if (buttonstate[i] != vrpn_BUTTON_MOMENTARY) {
      buttonstate[i] = vrpn_BUTTON_MOMENTARY;  
      if(send_alerts) PACK_ALERT_MESSAGE(i,vrpn_BUTTON_TOGGLE_OFF);
    }
  }
}


void vrpn_Button::set_all_momentary(void) {
    PACK_ADMIN_MESSAGE(vrpn_ALL_ID,vrpn_BUTTON_MOMENTARY);
}

void vrpn_Button::set_all_toggle(vrpn_int32 default_state) {
        PACK_ADMIN_MESSAGE(vrpn_ALL_ID,default_state);
}

void vrpn_Button_Filter::set_all_toggle(vrpn_int32 default_state) {
  for (vrpn_int32 i=0; i< num_buttons; i++){
    if (buttonstate[i] == vrpn_BUTTON_MOMENTARY){
        buttonstate[i] = default_state;
	if(send_alerts){PACK_ALERT_MESSAGE(i,default_state); }
    }
  }
}

void	vrpn_Button::print(void)
{
   vrpn_int32	i;
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


vrpn_int32	vrpn_Button::encode_to(char *buf, vrpn_int32 button, vrpn_int32 state)
{
   // Message includes: vrpn_int32 buttonNum, vrpn_int32 state
   // Byte order of each needs to be reversed to match network standard

   vrpn_uint32 *longbuf = (vrpn_uint32*)(void*)(buf);
   vrpn_int32	index = 0;

   longbuf[index++] = htonl(button);
   longbuf[index++] = htonl(state);

   return index*sizeof(vrpn_uint32);
}

/*
static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
  if (t2.tv_sec == -1) return 0;
  return (t1.tv_usec - t2.tv_usec) +
    1000000L * (t1.tv_sec - t2.tv_sec);
}
*/

static int client_msg_handler(void *userdata, vrpn_HANDLERPARAM p) {
  vrpn_Button_Filter * instance = (vrpn_Button_Filter *) userdata;
  vrpn_int32 * bp = (vrpn_int32 *)(p.buffer);
  vrpn_int32 event= ntohl(bp[1]);
  vrpn_int32 buttonid = ntohl(bp[0]);

  if (event== vrpn_BUTTON_MOMENTARY) {
    if (buttonid == vrpn_ALL_ID)
      instance->set_all_momentary();
    else instance->set_momentary(buttonid);
  }else if (event== vrpn_BUTTON_TOGGLE_OFF || event==vrpn_BUTTON_TOGGLE_ON ){
    if (buttonid == vrpn_ALL_ID)
      instance->set_all_toggle(event);
    else instance->set_toggle(buttonid,event);
  } 
  return 0;
}


void vrpn_Button_Filter::report_changes (void){
   vrpn_int32 i;

//   vrpn_Button::report_changes();
   if (connection) {
      for (i = 0; i < num_buttons; i++) {
	switch (buttonstate[i]) {
        case vrpn_BUTTON_MOMENTARY:
              if (buttons[i] != lastbuttons[i])
              PACK_MESSAGE(i, buttons[i]);
          break;
        case vrpn_BUTTON_TOGGLE_ON:
          if (buttons[i] && !lastbuttons[i]) {
            buttonstate[i] = vrpn_BUTTON_TOGGLE_OFF;
            if(send_alerts) PACK_ALERT_MESSAGE(i,vrpn_BUTTON_TOGGLE_OFF);
            PACK_MESSAGE(i, 0);
          }
          break;
        case vrpn_BUTTON_TOGGLE_OFF:
          if (buttons[i] && !lastbuttons[i]) {
            buttonstate[i] = vrpn_BUTTON_TOGGLE_ON;
            if(send_alerts)PACK_ALERT_MESSAGE(i,vrpn_BUTTON_TOGGLE_ON);
            PACK_MESSAGE(i, 1);
          }
          break;
        default:
 		fprintf(stderr,"vrpn_Button::report_changes(): Button %d in \
			invalid state (%d)\n",i,buttonstate[i]);
        }
        lastbuttons[i] = buttons[i];
      }

   } else {
        fprintf(stderr,"vrpn_Button: No valid connection\n");
   }
}

void	vrpn_Button::report_changes(void)
{
  vrpn_int32	i;

   if (connection) {
      for (i = 0; i < num_buttons; i++) {
	if (buttons[i] != lastbuttons[i])
	      PACK_MESSAGE(i, buttons[i]);
	lastbuttons[i] = buttons[i];
      }

   } else {
   	fprintf(stderr,"vrpn_Button: No valid connection\n");
   }
}


#ifndef VRPN_CLIENT_ONLY

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

vrpn_Button_Example_Server::vrpn_Button_Example_Server(const char *name,
						       vrpn_Connection *c,
						       int numbuttons,
						       vrpn_float64 rate)
	: vrpn_Button_Filter(name, c)
{
	if (numbuttons > vrpn_BUTTON_MAX_BUTTONS) {
		num_buttons = vrpn_BUTTON_MAX_BUTTONS;
	} else {
		num_buttons = numbuttons;
	}

	_update_rate = rate;

	// IN A REAL SERVER, open the device that will service the buttons here
}

void vrpn_Button_Example_Server::mainloop(const struct timeval * timeout)
{
	struct timeval current_time;
	int	i;

	timeout = timeout;	// Keep the compiler happy

	// See if its time to generate a new report
	// IN A REAL SERVER, this check would not be done; although the
	// time of the report would be updated to the current time so
	// that the correct timestamp would be issued on the report.
	gettimeofday(&current_time, NULL);
	if ( duration(current_time,timestamp) >= 1000000.0/_update_rate) {

	  // Update the time
	  timestamp.tv_sec = current_time.tv_sec;
	  timestamp.tv_usec = current_time.tv_usec;

	  // Update the values for the buttons, to say that each one has
	  // switched its state.
	  // THIS CODE WILL BE REPLACED by the user code that tells how
	  // many revolutions each dial has changed since the last report.
	  for (i = 0; i < num_buttons; i++) {
		  buttons[i] = !lastbuttons[i];
	  }

	  // Send reports. Stays the same in a real server.
	  report_changes();
	}
}


vrpn_parallel_Button::vrpn_parallel_Button(const char *name,
					   vrpn_Connection *c,
					   int portno)
	: vrpn_Button_Filter(name, c)
{      
#ifdef linux
    char *portname;
    switch (portno) {
	case 1: portname = "/dev/lp0";
		break;
	case 2: portname = "/dev/lp1";
		break;
	case 3: portname = "/dev/lp2";
		break;
	default:
	    fprintf(stderr, "vrpn_parallel_Button: "
			    "Bad port number (%d)\n",portno);
	    status = BUTTON_FAIL;
	    portname = "UNKNOWN";
	    break;
    }

      // Open the port
    if ( (port = open(portname, O_RDWR)) < 0) {
	perror("vrpn_parallel_Button::vrpn_parallel_Button(): "
	       "Can't open port");
	fprintf(stderr, "vrpn_parallel_Button::vrpn_parallel_Button(): "
		        "Can't open port %s\n",portname);
	status = BUTTON_FAIL;
	return;
    }
#elif _WIN32
      // if on NT we need a device driver to do direct reads
      // from the parallel port
    if (!openGiveIO()) {
       fprintf(stderr, "vrpn_Button: need giveio driver for port access!\n");
       fprintf(stderr, "vrpn_Button: can't use vrpn_Button()\n");
       status = BUTTON_FAIL;
       return;
    }
    switch (portno) {
	  // we use this mapping, although LPT? can actually
	  // have different addresses.
	case 1: port = 0x378; break;  // usually LPT1
	case 2: port = 0x3bc; break;  // usually LPT2
	case 3: port = 0x278; break;  // usually LPT3
	default:
	    fprintf(stderr,"vrpn_parallel_Button: Bad port number (%d)\n",portno);
	    status = BUTTON_FAIL;
	    break;
    }
#else  // _WIN32      
    fprintf(stderr, "vrpn_parallel_Button: not supported on this platform\n?");
    status = BUTTON_FAIL;
    portno = portno;  // unused argument
    return;
#endif

#if defined(linux) || defined(_WIN32)
   // Set the INIT line on the device to provide power to the python
   // XXX apparently, we don't need to do this.

     // Zero the button states
    num_buttons = 5;	//XXX This is specific to the python
    for (vrpn_int32 i = 0; i < num_buttons; i++) {
	buttons[i] = lastbuttons[i] = 0;
    }

    status = BUTTON_READY;
    gettimeofday(&timestamp, NULL);
#endif
}

vrpn_Button_Python::vrpn_Button_Python (const char * name, vrpn_Connection * c,
                                        int p) :
    vrpn_parallel_Button (name, c, p) {

}

// virtual
vrpn_Button_Python::~vrpn_Button_Python (void) {

}

void vrpn_Button_Python::mainloop(const struct timeval * /*timeout*/ )
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
    for (int i = 0; i < 3; i++) 
      if (ioctl(port, LPGETSTATUS, &status_register[i]) == -1) {
	    perror("vrpn_Button_Python::read(): ioctl() failed");
	    return;
      }

#elif _WIN32
  #ifndef __CYGWIN__
	static const int STATUS_REGISTER_OFFSET = 1;
    for (int i = 0; i < 3; i++) {
		status_register[i] = _inp(port + STATUS_REGISTER_OFFSET);
    }
  #else
    status_register[0] = status_register[1] = status_register[2] = 0;
  #endif
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


vrpn_Button_Remote::vrpn_Button_Remote(const char *name, vrpn_Connection *cn):
	vrpn_Button(name, cn ? cn : vrpn_get_connection_by_name(name)),
	change_list(NULL)
{
	vrpn_int32	i;

	// Register a handler for the change callback from this device,
	// if we got a connection.
	if (connection != NULL) {
	  if (connection->register_handler(change_message_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Button_Remote: can't register handler\n");
		connection = NULL;
	  }
	} else {
		fprintf(stderr,"vrpn_Button_Remote: Can't get connection!\n");
	}

	// XXX These should be read from a description message that comes
	// from the button device (as a response to a query?).  For now,
	// we assume an SGI button box.
	num_buttons = 32;
	for (i = 0; i < num_buttons; i++) {
		buttons[i] = lastbuttons[i] = 0;
	}
	gettimeofday(&timestamp, NULL);
}

// virtual
vrpn_Button_Remote::~vrpn_Button_Remote (void)
{
	vrpn_BUTTONCHANGELIST	*next;

	// Unregister all of the handlers that have been registered with the
	// connection so that they won't yank once the object has been deleted.
	if (connection) {
	  if (connection->unregister_handler(change_message_id, handle_change_message,
		this, my_id)) {
		fprintf(stderr,"vrpn_Button_Remote: can't unregister handler\n");
		fprintf(stderr,"   (internal VRPN error -- expect a seg fault)\n");
	  }
	}

	// Delete all of the callback handlers that other code had registered
	// with this object. This will free up the memory taken by the list
	while (change_list != NULL) {
		next = change_list->next;
		delete change_list;
		change_list = next;
	}
}

void vrpn_Button_Remote::mainloop(const struct timeval * timeout)
{
	if (connection) {
		connection->mainloop(timeout); 
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
	vrpn_int32 *params = (vrpn_int32*)(p.buffer);
	vrpn_BUTTONCB	bp;
	vrpn_BUTTONCHANGELIST *handler = me->change_list;

	// Fill in the parameters to the button from the message
	if (p.payload_len != 2*sizeof(vrpn_int32)) {
		fprintf(stderr,"vrpn_Button: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 2*sizeof(vrpn_int32));
		return -1;
	}
	bp.msg_time = p.msg_time;
	bp.button = (vrpn_int32)ntohl(params[0]);
	bp.state = (vrpn_int32)ntohl(params[1]);

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, bp);
		handler = handler->next;
	}

	return 0;
}

  // We use _inp (inport) to read the parallel port status register
  // since we haven't found a way to do it through the Win32 API.
  // On NT we need a special device driver (giveio) to be installed
  // so that we can get permission to execute _inp.  On Win95 we
  // don't need to do anything.
  //
  // The giveio driver is from the May 1996 issue of Dr. Dobb's,
  // an article by Dale Roberts called "Direct Port I/O and Windows NT"
  //
  // This function returns 1 if either running Win95 or running NT and
  // the giveio device was opened.  0 if device not opened.
#ifndef VRPN_CLIENT_ONLY
#ifdef _WIN32
int vrpn_parallel_Button::openGiveIO(void)
{
    OSVERSIONINFO osvi;

    memset(&osvi, 0, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

      // if Win95: no initialization required
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {	
	return 1;
    }

      // else if NT: use giveio driver
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
	HANDLE h = CreateFile("\\\\.\\giveio", GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
	      // maybe driver not installed?
	    return 0;
	}
	CloseHandle(h);
	  // giveio opened successfully
	return 1;
    }

	// else GetVersionEx gave unexpected result
    fprintf(stderr, "vrpn_parallel_Button::openGiveIO: unknown windows version\n");
    return 0;
}
#endif // _WIN32
#endif // VRPN_CLIENT_ONLY
