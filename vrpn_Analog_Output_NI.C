#include <stdio.h>
#include "vrpn_Connection.h"
#include "vrpn_Analog_Output_NI.h"

#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
#include "server_src/NIUtil.cpp"
#endif

vrpn_Analog_Output_Server_NI::vrpn_Analog_Output_Server_NI(const char* name, vrpn_Connection* c, 
                                                     const char *boardName,
						     vrpn_int32 numChannels, bool bipolar,
						     double minVoltage, double maxVoltage) :
	vrpn_Analog_Output(name, c),
	NI_device_number(-1),
	min_voltage(minVoltage),
	max_voltage(maxVoltage),
	NI_num_channels(numChannels)
{
#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
  int i;
  int	  update_mode = 0;	//< Mode 0 is immediate
  int	  ref_source = 0;	//< Reference source, 0 = internal, 1 = external
  double  ref_voltage = 0.0;

  // Set the polarity
  if (bipolar) {
    polarity = 0;
  } else {
    polarity = 1;
  }

  // Open the D/A board and set the parameters for each channel.  Set the voltage for each
  // channel to the minimum.
  NI_device_number = NIUtil::findDevice(boardName);
  if (NI_device_number == -1) {
      fprintf(stderr, "vrpn_Analog_Output_Server_NI: Error opening the D/A board %s\n", boardName);
      return;
  }
  for (i = 0; i < NI_num_channels; i++) {
      AO_Configure(NI_device_number, i, polarity, ref_source, ref_voltage,
	  update_mode);
      AO_VWrite(NI_device_number, i, min_voltage);
 }


  this->setNumChannels( numChannels );

  // Check if we have a connection
  if (d_connection == NULL) {
	  fprintf(stderr, "vrpn_Analog_Output_Server_NI: Can't get connection!\n");
  }

  // Register a handler for the request channel change message
  if (register_autodeleted_handler(request_m_id,
	  handle_request_message, this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Analog_Output_Server_NI: can't register change channel request handler\n");
	  d_connection = NULL;
  }

  // Register a handler for the request channels change message
  if (register_autodeleted_handler(request_channels_m_id,
	  handle_request_channels_message, this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Analog_Output_Server_NI: can't register change channels request handler\n");
	  d_connection = NULL;
  }

  // Register a handler for vrpn_got_connection, so we can tell the
  // client how many channels are active
  if( register_autodeleted_handler( got_connection_m_id, handle_got_connection, this ) )
  {
	  fprintf( stderr, "vrpn_Analog_Output_Server_NI: can't register new connection handler\n");
	  d_connection = NULL;
  }
#else
  fprintf(stderr,"vrpn_Analog_Output_Server_NI: Support for NI not compiled in, edit vrpn_Configure.h and recompile\n");
#endif
}

// virtual
vrpn_Analog_Output_Server_NI::~vrpn_Analog_Output_Server_NI(void) {}

// virtual
void vrpn_Analog_Output_Server_NI::mainloop(void)
{
  // Let the server code do its thing (ping/pong messages)
  server_mainloop();
}

vrpn_int32 vrpn_Analog_Output_Server_NI::setNumChannels (vrpn_int32 sizeRequested) {
  if (sizeRequested < 0) sizeRequested = 0;
  if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;

  o_num_channel = sizeRequested;

  return o_num_channel;
}

/* static */
int vrpn_Analog_Output_Server_NI::handle_request_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
    vrpn_Analog_Output_Server_NI* me = (vrpn_Analog_Output_Server_NI*)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message):  channel %d is not active.  Squelching.", chan_num );
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
      return 0;
    }
    // Make sure the voltage value is within the allowed range.
    if (value < me->min_voltage) {
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message): voltage %g is too low.  Clamping to %g.", value, me->min_voltage);
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
      value = me->min_voltage;
    }
    if (value > me->max_voltage) {
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message): voltage %g is too high.  Clamping to %g.", value, me->max_voltage);
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
      value = me->max_voltage;
    }
    me->o_channel[chan_num] = value;

    // Send the new value to the D/A board
#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
    if (me->NI_device_number != -1) {
      AO_VWrite(me->NI_device_number, chan_num, value);
    }
#endif
    return 0;
}

/* static */
int vrpn_Analog_Output_Server_NI::handle_request_channels_message(void* userdata,
    vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_Analog_Output_Server_NI* me = (vrpn_Analog_Output_Server_NI*)userdata;
    vrpn_int32 chan_num;
    vrpn_float64 value;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) 
    {
         char msg[1024];
         sprintf( msg, "Error:  (handle_request_channels_message):  channels above %d not active; "
              "bad request up to channel %d.  Squelching.", me->o_num_channel, num );
         me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
         num = me->o_num_channel;
    }
    if (num < 0) 
    {
         char msg[1024];
         sprintf( msg, "Error:  (handle_request_channels_message):  invalid channel %d.  Squelching.", num );
         me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
         return 0;
    }
    for (chan_num = 0; chan_num < num; chan_num++) {
	vrpn_unbuffer(&bufptr, &value);

	// Make sure the voltage value is within the allowed range.
	if (value < me->min_voltage) {
	  char msg[1024];
	  sprintf( msg, "Error:  (handle_request_messages): voltage %g is too low.  Clamping to %g.", value, me->min_voltage);
	  me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
	  value = me->min_voltage;
	}
	if (value > me->max_voltage) {
	  char msg[1024];
	  sprintf( msg, "Error:  (handle_request_messages): voltage %g is too high.  Clamping to %g.", value, me->max_voltage);
	  me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
	  value = me->max_voltage;
	}
	me->o_channel[chan_num] = value;

	// Send the new value to the D/A board
#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
	if (me->NI_device_number != -1) {
	  AO_VWrite(me->NI_device_number, chan_num, value);
	}
#endif
    }
    
    return 0;
}


/* static */
int vrpn_Analog_Output_Server_NI::handle_got_connection( void* userdata, vrpn_HANDLERPARAM )
{
	vrpn_Analog_Output_Server_NI* me = (vrpn_Analog_Output_Server_NI*) userdata;
	if( me->report_num_channels( ) == false )
	{
		fprintf( stderr, "Error:  failed sending active channels to client.\n" );
	}
	return 0;
}


bool vrpn_Analog_Output_Server_NI::report_num_channels( vrpn_uint32 class_of_service )
{
	char msgbuf[ sizeof( vrpn_int32) ];
	vrpn_int32 len = sizeof( vrpn_int32 );;
	
	encode_num_channels_to( msgbuf, this->o_num_channel );
	vrpn_gettimeofday( &o_timestamp, NULL );
	if( d_connection && 
	    d_connection->pack_message( len, o_timestamp,	report_num_channels_m_id, 
							  d_sender_id, msgbuf, class_of_service ) ) 
	{
		fprintf(stderr, "vrpn_Analog_Output_Server_NI (report_num_channels): cannot write message: tossing\n");
		return false;
	}
	return true;
}


vrpn_int32 vrpn_Analog_Output_Server_NI::
encode_num_channels_to( char* buf, vrpn_int32 num )
{
    // Message includes: int32 number of active channels
    int	buflen = sizeof(vrpn_int32);

    vrpn_buffer(&buf, &buflen, num);
    return sizeof(vrpn_int32);
}
