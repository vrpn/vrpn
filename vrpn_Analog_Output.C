#include "vrpn_Analog_Output.h"
#include <stdio.h>

vrpn_Analog_Output::vrpn_Analog_Output(const char* name, vrpn_Connection* c) :
	vrpn_BaseClass(name, c),
	o_num_channel(0)
{
   // Call the base class' init routine
   vrpn_BaseClass::init();

   // Set the time to 0 just to have something there.
   o_timestamp.tv_usec = o_timestamp.tv_sec = 0;
   // Initialize the values in the channels, 
   // gets rid of uninitialized memory read error in Purify
   // and makes sure any initial value change gets reported. 
   for (vrpn_int32 i=0; i< vrpn_CHANNEL_MAX; i++) {
       o_channel[i] = 0;
   }
}

int vrpn_Analog_Output::register_types(void)
{
    request_m_id = d_connection->register_message_type("vrpn_Analog_Output Change_request");
    request_channels_m_id = d_connection->register_message_type("vrpn_Analog_Output Change_Channels_request");
    if ((request_m_id == -1) || request_channels_m_id == -1) {
	    return -1;
    } 
    else {
	    return 0;
    }
}

void vrpn_Analog_Output::o_print(void ) {
    printf("Analog_Output Report: ");
    for (vrpn_int32 i = 0; i < o_num_channel; i++) {
        //printf("Channel[%d]= %f\t", i, o_channel[i]);
        printf("%f\t", o_channel[i]);
    }
    printf("\n");
}

vrpn_int32 vrpn_Analog_Output::encode_to(char *buf)
{
    // Message includes: vrpn_float64 AnalogNum, vrpn_float64 state
    // Byte order of each needs to be reversed to match network standard
    // XXX This is passing an int in the double for the num_channel

    vrpn_float64    double_chan = o_num_channel;
    int		        buflen = 125 *sizeof(double);

    vrpn_buffer(&buf, &buflen, double_chan);
    for (int i = 0; i < o_num_channel; i++) {
        vrpn_buffer(&buf, &buflen, o_channel[i]);
    }

    return (o_num_channel + 1) * sizeof(vrpn_float64);
}

vrpn_Analog_Output_Server::vrpn_Analog_Output_Server(const char* name, vrpn_Connection* c) :
    vrpn_Analog_Output(name, c),
    change_list(NULL)
{
    o_num_channel = 0;

    // Register handlers for the change request callbacks from this device,
    // if we have a connection
    if (d_connection != NULL) {
        if (register_autodeleted_handler(request_m_id, handle_request_message, this, d_sender_id)) {
            fprintf(stderr, "vrpn_Analog_Output: can't register handler\n");
            d_connection = NULL;
        }
        if (register_autodeleted_handler(request_channels_m_id, handle_request_channels_message, this, d_sender_id)) {
            fprintf(stderr, "vrpn_Analog_Output: can't register handler\n");
            d_connection = NULL;
        }
    }
    else {
        fprintf(stderr, "vrpn_Analog_Output: Can't get connection!\n");
    }
}

// virtual
vrpn_Analog_Output_Server::~vrpn_Analog_Output_Server(void) {
	vrpn_ANALOGCHANGELIST	*next;

	// Our handlers have all been registered using the
	// register_autodeleted_handler() method, so we don't have to
	// worry about unregistering them here.

	// Delete all of the callback handlers that other code had registered
	// with this object. This will free up the memory taken by the list
	while (change_list != NULL) {
		next = change_list->next;
		delete change_list;
		change_list = next;
	}
}

vrpn_int32 vrpn_Analog_Output_Server::setNumChannels (vrpn_int32 sizeRequested) {
  if (sizeRequested < 0) sizeRequested = 0;
  if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;

  o_num_channel = sizeRequested;

  return o_num_channel;
}

int vrpn_Analog_Output_Server::handle_request_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
    vrpn_Analog_Output_Server* me = (vrpn_Analog_Output_Server*)userdata;
    vrpn_ANALOGCB ap;
    vrpn_ANALOGCHANGELIST* handler = me->change_list;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      fprintf(stderr,"vrpn_Analog_Output_Server::handle_request_message(): Index out of bounds\n");
      return 0;
    }
    me->o_channel[chan_num] = value;

    // Fill in the parameters for the callbacks...treat both handle_request_change_channel and
    // handle_request_channels the same for now, so just make all the channels available to the user
    // callbacks in the server
    ap.msg_time = p.msg_time;
    ap.num_channel = me->o_num_channel;
    for (i = 0; i < me->o_num_channel; i++) {
        ap.channel[i] = me->o_channel[i];
    }
    while (handler != NULL) {
        handler->handler(handler->userdata, ap);
        handler = handler->next;
    }

    return 0;
}

int vrpn_Analog_Output_Server::handle_request_channels_message(void* userdata,
    vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
	vrpn_int32 num;
	vrpn_int32 pad;
    vrpn_Analog_Output_Server* me = (vrpn_Analog_Output_Server*)userdata;
    vrpn_ANALOGCB ap;
    vrpn_ANALOGCHANGELIST* handler = me->change_list;

    // Read the values from the buffer
	vrpn_unbuffer(&bufptr, &num);
	vrpn_unbuffer(&bufptr, &pad);
	if (num > me->o_num_channel) num = me->o_num_channel;
    for (i = 0; i < num; i++) {
        vrpn_unbuffer(&bufptr, &(me->o_channel[i]));
    }

    // Fill in the parameters for the callbacks...treat both handle_request_change_channel and
    // handle_request_channels the same for now, so just make all the channels available to the user
    // callbacks in the server
    ap.msg_time = p.msg_time;
    ap.num_channel = me->o_num_channel;
    for (i = 0; i < me->o_num_channel; i++) {
        ap.channel[i] = me->o_channel[i];
    }
    while (handler != NULL) {
        handler->handler(handler->userdata, ap);
        handler = handler->next;
    }

    return 0;
}

int vrpn_Analog_Output_Server::register_change_handler(void* userdata,
			vrpn_ANALOGCHANGEHANDLER handler)
{
	vrpn_ANALOGCHANGELIST* new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_Analog_Output_Server::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_ANALOGCHANGELIST) == NULL) {
		fprintf(stderr,
		    "vrpn_Analog_Output_Server::register_handler: Out of memory\n");
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

int vrpn_Analog_Output_Server::unregister_change_handler(void *userdata,
			vrpn_ANALOGCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_ANALOGCHANGELIST	*victim, **snitch;

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
		   "vrpn_Analog_Output_Server::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}





vrpn_Analog_Output_Remote::vrpn_Analog_Output_Remote(const char* name, vrpn_Connection* c) :
    vrpn_Analog_Output(name, c)
{
    vrpn_int32 i;

    o_num_channel = vrpn_CHANNEL_MAX;
	for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
		o_channel[i] = 0;
	}
	gettimeofday(&o_timestamp, NULL);
}

// virtual
vrpn_Analog_Output_Remote::~vrpn_Analog_Output_Remote(void) {}
    
void vrpn_Analog_Output_Remote::mainloop()
{
    if (d_connection) {
        d_connection->mainloop();
        client_mainloop();
    }
}

bool vrpn_Analog_Output_Remote::request_change_channel_value(unsigned int chan, vrpn_float64 val,
						      vrpn_uint32 class_of_service)
{
    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf [2];
    char * msgbuf = (char*) fbuf;

    vrpn_int32  len;

    gettimeofday(&o_timestamp, NULL);
    len = encode_change_to(msgbuf, chan, val);
    if (d_connection && d_connection->pack_message(len, o_timestamp,
                                 request_m_id, d_sender_id, msgbuf,
                                 class_of_service)) {
        fprintf(stderr,"vrpn_Analog_Output_Remote: cannot write message: tossing\n");
        return false;
    }

    return true;
}

bool vrpn_Analog_Output_Remote::request_change_channels(int num, vrpn_float64* vals, vrpn_uint32 class_of_service)
{
	if (num < 0 || num > vrpn_CHANNEL_MAX) {
		fprintf(stderr, "vrpn_Analog_Output_Remote: cannot change channels: number of channels out of range\n");
		return false;
	}

    char* msgbuf = new char[2 * sizeof(vrpn_int32) + num * sizeof(vrpn_float64)];
    vrpn_int32 len;

    gettimeofday(&o_timestamp, NULL);
    len = encode_change_channels_to(msgbuf, num, vals);
    if (d_connection && d_connection->pack_message(len, o_timestamp,
                                    request_channels_m_id, d_sender_id, msgbuf, 
                                    class_of_service)) {
        fprintf(stderr, "vrpn_Analog_Output_Remote: cannot write message: tossing\n");
        return false;
    }

	delete [] msgbuf;

    return true;
}

vrpn_int32 vrpn_Analog_Output_Remote::encode_change_to(char *buf, vrpn_int32 chan, vrpn_float64 val)
{
    // Message includes: int32 channel_number, int32 padding, float64 request_value
    // Byte order of each needs to be reversed to match network standard

    vrpn_int32 pad = 0;
    int	buflen = 2 * sizeof(vrpn_int32) + sizeof(vrpn_float64);

    vrpn_buffer(&buf, &buflen, chan);
    vrpn_buffer(&buf, &buflen, pad);
    vrpn_buffer(&buf, &buflen, val);

    return 2 * sizeof(vrpn_int32) + sizeof(vrpn_float64);
}

vrpn_int32 vrpn_Analog_Output_Remote::encode_change_channels_to(char* buf, vrpn_int32 num, vrpn_float64* vals) 
{
    int i;
	vrpn_int32 pad = 0;
    int buflen = 2 * sizeof(vrpn_int32) + num * sizeof(vrpn_float64);

	vrpn_buffer(&buf, &buflen, num);
	vrpn_buffer(&buf, &buflen, pad);
    for (i = 0; i < num; i++) {
        vrpn_buffer(&buf, &buflen, vals[i]);
    }

    return 2 * sizeof(vrpn_int32) + num * sizeof(vrpn_float64);
}
