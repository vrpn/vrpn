#include <stdio.h>                      // for sprintf, fprintf, stderr, etc
#include "vrpn_Shared.h"                // for vrpn_gettimeofday

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_WARNING, etc
#include "vrpn_Connection.h"            // for vrpn_HANDLERPARAM, etc
#include "vrpn_NationalInstruments.h"

#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
#include "server_src/NIUtil.cpp"
#endif


vrpn_National_Instruments_Server::vrpn_National_Instruments_Server (const char* name, vrpn_Connection * c, 
                             const char *boardName,
                             int numInChannels, int numOutChannels,
                             double minInputReportDelaySecs,
                             bool inBipolar, int inputMode, int inputRange, bool driveAIS, int inputGain,
                             bool outBipolar, double minOutVoltage, double maxOutVoltage) :
        vrpn_Analog(name, c),
        vrpn_Analog_Output(name, c),
#if defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
        d_analog_task_handle(0),
        d_analog_out_task_handle(0),
#else
        d_device_number(-1),
#endif
        d_in_gain(inputGain),
        d_in_min_delay(minInputReportDelaySecs),
        d_out_min_voltage(minOutVoltage),
        d_out_max_voltage(maxOutVoltage)
{
  // Set the polarity.  0 is bipolar, 1 is unipolar.
  if (inBipolar) {
    d_in_polarity = 0;
  } else {
    d_in_polarity = 1;
  }
  if (outBipolar) {
    d_out_polarity = 0;
  } else {
    d_out_polarity = 1;
  }

  setNumInChannels( numInChannels );
  setNumOutChannels( numOutChannels );

#if defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
  char	portName[1024];
  int32 error=0;

  // Input mode that we are using
  int32 terminalConfig;
  switch(inputMode) {
	  case vrpn_NI_INPUT_MODE_DIFFERENTIAL:
		  terminalConfig = DAQmx_Val_Diff;
		  break;
	  case vrpn_NI_INPUT_MODE_REF_SINGLE_ENDED:
		  terminalConfig = DAQmx_Val_RSE;
		  break;
	  case vrpn_NI_INPUT_MODE_NON_REF_SINGLE_ENDED:
		  terminalConfig = DAQmx_Val_NRSE;
		  break;
	  default:
		  fprintf(stderr,"vrpn_National_Instruments_Server::vrpn_National_Instruments_Server(): Invalid inputMode (%d)\n", inputMode);
		  return;
  }

  // Minimum and maximum values that we expect to read.
  // Set the high end of the voltage range.  If we're bipolar, then
  // we get half the range above and half below zero.
  double	min_val = 0.0, max_val = inputRange;
  if (inBipolar) {
	  min_val = -max_val/2;
	  max_val = max_val/2;
  }

  // If we are going analog input, then we specify the range of input
  // ports that we use from 0 through the number requested.  We make a
  // task to handle reading of these channels as a group.  We then
  // start this task.
  if (numInChannels > 0) {
	  sprintf(portName, "%s/ai0:%d", boardName, numInChannels-1);
	  error = DAQmxCreateTask("", &d_analog_task_handle);
	  if (error == 0) {
		  error = DAQmxCreateAIVoltageChan(
			  d_analog_task_handle			// The handle we'll use to read values
			  , portName					// Device name and port range
			  , ""							// Our nickname for the channel
			  , terminalConfig				// Single-ended vs. differentil
			  , min_val, max_val			// Range of values we expect to read
			  , DAQmx_Val_Volts, ""			// Units, no custom scale
			);
		  if (error == 0) {
			  fprintf(stderr,"vrpn_National_Instruments_Server::vrpn_National_Instruments_Server(): Cannot create input voltage channel\n");
			  error = DAQmxStartTask(d_analog_task_handle);
		  }
	  }
	  if (error) {
		  fprintf(stderr,"vrpn_National_Instruments_Server::vrpn_National_Instruments_Server(): Cannot create input voltage task\n");
		  reportError(error);
		  return;
	  }
  }

  // If we are going analog output, then we specify the range of output
  // ports that we use from 0 through the number requested.  We make a
  // task to handle writing of these channels as a group.  Start the
  // task.  Also, set the outputs to their minimum value at startup.
  if (numOutChannels > 0) {
	  sprintf(portName, "%s/ao0:%d", boardName, numOutChannels-1);
	  error = DAQmxCreateTask("", &d_analog_out_task_handle);
	  if (error == 0) {
		  error = DAQmxCreateAOVoltageChan(
			  d_analog_out_task_handle		// The handle we'll use to write values
			  , portName					// Device name and port range
			  , ""							// Our nickname for the channel
			  , minOutVoltage, maxOutVoltage// Range of values we expect to write
			  , DAQmx_Val_Volts, ""			// Units, no custom scale
			);
		  if (error) {
			  fprintf(stderr,"vrpn_National_Instruments_Server::vrpn_National_Instruments_Server(): Cannot create output voltage channel\n");
			  reportError(error);
			  return;
		  }
	  }
	  if (error == 0) {
		  error = DAQmxStartTask(d_analog_out_task_handle);
	  }
	  if (error) {
		  fprintf(stderr,"vrpn_National_Instruments_Server::vrpn_National_Instruments_Server(): Cannot create or start output voltage task\n");
		  reportError(error);
		  return;
	  }

	  // Set the output values to zero or to the minimum if the minimum
	  // is above 0.  Also send these values to the board.
	  float64 minval = 0.0;
	  if (minval < minOutVoltage) { minval = minOutVoltage; }
	  int i;
	  for (i = 0; i < vrpn_Analog_Output::o_num_channel; i++) {
		  vrpn_Analog_Output::o_channel[i] = minval;
	  }
	  if (!setValues()) {
	    fprintf(stderr, "vrpn_National_Instruments_Server::vrpn_National_Instruments_Server(): Could not set values\n");
	    return;
	  }
  }

#elif defined(VRPN_USE_NATIONAL_INSTRUMENTS)
  short i;
  short	  update_mode = 0;	//< Mode 0 is immediate
  short	  ref_source = 0;	//< Reference source, 0 = internal, 1 = external
  double  ref_voltage = 10.0;

  // Open the board
  d_device_number = NIUtil::findDevice(boardName);
  if (d_device_number == -1) {
      fprintf(stderr, "vrpn_National_Instruments_Server: Error opening the D/A board %s\n", boardName);
      return;
  }

  // Set the parameters for each input channel.
  if (vrpn_Analog::num_channel > 0) {
    int ret = AI_Clear(d_device_number);
    if (ret < 0) {
      fprintf(stderr,"vrpn_National_Instruments_Server: Cannot clear analog input (error %d)\n", ret);
      setNumInChannels(0);
    }
  }
  for (i = 0; i < vrpn_Analog::num_channel; i++) {
    int ret = AI_Configure(d_device_number, i, inputMode, inputRange, d_in_polarity, driveAIS);
    if (ret < 0) {
      fprintf(stderr,"vrpn_National_Instruments_Server: Cannot configure input channel %d (error %d)\n", i, ret);
      setNumInChannels(0);
      break;
    }
  }

  // Set the parameters for each output channel.  Set the voltage for each channel to the minimum.
  for (i = 0; i < o_num_channel; i++) {

    int ret = AO_Configure(d_device_number, i, d_out_polarity, ref_source, ref_voltage, update_mode);
    // Code -10403 shows up but did not cause problems for the NI server, so we ignore it (probably at our peril)
    if ( (ret < 0) && (ret != -10403) ) {
      fprintf(stderr,"vrpn_National_Instruments_Server: Cannot configure output channel %d (error %d)\n", i, ret);
      fprintf(stderr,"     polarity: %d, reference source: %d, reference_voltage: %lg, update_mode: %d\n",
          d_out_polarity, ref_source, ref_voltage, update_mode);
      setNumOutChannels(0);
      break;
    }

    if (AO_VWrite(d_device_number, i, d_out_min_voltage)) {
      fprintf(stderr,"vrpn_National_Instruments_Server: Cannot set output channel %d to %lg\n", i, d_out_min_voltage);
      setNumOutChannels(0);
    }
  }

#else
  fprintf(stderr,"vrpn_National_Instruments_Server: Support for NI not compiled in, edit vrpn_Configure.h and recompile VRPN\n");
  // Handle parameters that are not used, avoiding compiler warnings.
  const char *unused = boardName;
  unused++;
  inputMode = inputMode + 1;
  inputRange = inputRange + 1;
  driveAIS = !driveAIS;
#endif

  // Check if we have a connection
  if (d_connection == NULL) {
    fprintf(stderr, "vrpn_National_Instruments_Server: Can't get connection!\n");
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

  // No report yet sent.
  d_last_report_time.tv_sec = 0;
  d_last_report_time.tv_usec = 0;
}

// virtual
vrpn_National_Instruments_Server::~vrpn_National_Instruments_Server()
{
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    if( d_analog_task_handle != 0 ) 
    {
        /*********************************************/
        // DAQmx Stop Code
        /*********************************************/
        DAQmxStopTask(d_analog_task_handle);
        DAQmxClearTask(d_analog_task_handle);
        d_analog_task_handle = 0 ;
    }

    if( d_analog_out_task_handle != 0 ) 
    {
        /*********************************************/
        // DAQmx Stop Code
        /*********************************************/
        DAQmxStopTask(d_analog_out_task_handle);
        DAQmxClearTask(d_analog_out_task_handle);
        d_analog_out_task_handle = 0 ;
    }
#endif
}

// virtual
void vrpn_National_Instruments_Server::mainloop(void)
{
  // Let the server code do its thing (ping/pong messages)
  server_mainloop();

  // See if it has been long enough since we sent the last report.
  // If so, then read the channels and send a new report.
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  if (vrpn_TimevalDurationSeconds(now, d_last_report_time) >= d_in_min_delay) {
    d_last_report_time = now;

#if defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
	// Read one complete set of channel values from the board and store
	// the results (which are already in volts) into the channels.
	if (vrpn_Analog::num_channel > 0) {
		int32 error;
		int32 channelsRead = 0;
		float64	data[vrpn_CHANNEL_MAX];
		error = DAQmxReadAnalogF64(d_analog_task_handle, 1, 1.0, DAQmx_Val_GroupByChannel,
			data, vrpn_CHANNEL_MAX, &channelsRead, NULL);
		if (channelsRead != vrpn_Analog::num_channel) {
			send_text_message("vrpn_National_Instruments_Server::mainloop(): Cannot read channel", now, vrpn_TEXT_ERROR);
			setNumInChannels(0);
			return;
		}
		if (error) {
			  reportError(error);
			  return;
		}
		int i;
		for (i = 0; i < channelsRead; i++) {
			channel[i] = data[i];
		}
	}

#elif defined(VRPN_USE_NATIONAL_INSTRUMENTS)
    // Read from the board, convert to volts, and store in the channel array
    i16 value;
    int i;
    for (i = 0; i < vrpn_Analog::num_channel; i++) {
      int ret = AI_Read(d_device_number, i, d_in_gain, &value);
      if (ret != 0) {
        send_text_message("vrpn_National_Instruments_Server::mainloop(): Cannot read channel", now, vrpn_TEXT_ERROR);
        setNumInChannels(0);
        return;
      }
      if(0 == d_in_polarity){ //Bipolar: We multiply by 10, but it will only go halfway (it is signed).
		channel[i] = (value / 65536.0) * (10.0 / d_in_gain);		    
      } else { //Unipolar (cast it to unsigned, so it will be from zero to almost 10 volts).
		channel[i] = ((vrpn_uint16)value / 65536.0) * (10.0 / d_in_gain);
      }
    }
#endif

    // Send a report.
    vrpn_Analog::report();
  }
}

int vrpn_National_Instruments_Server::setNumInChannels (int sizeRequested) {
  if (sizeRequested < 0) sizeRequested = 0;
  if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;
  vrpn_Analog::num_channel = sizeRequested;

  return vrpn_Analog::num_channel;
}

int vrpn_National_Instruments_Server::setNumOutChannels (int sizeRequested) {
  if (sizeRequested < 0) sizeRequested = 0;
  if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;

  o_num_channel = sizeRequested;
  return o_num_channel;
}

/* static */
int VRPN_CALLBACK vrpn_National_Instruments_Server::handle_request_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
    vrpn_National_Instruments_Server* me = (vrpn_National_Instruments_Server*)userdata;

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
    if (value < me->d_out_min_voltage) {
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message): voltage %g is too low.  Clamping to %g.", value, me->d_out_min_voltage);
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
      value = me->d_out_min_voltage;
    }
    if (value > me->d_out_max_voltage) {
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message): voltage %g is too high.  Clamping to %g.", value, me->d_out_max_voltage);
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
      value = me->d_out_max_voltage;
    }
    me->o_channel[chan_num] = value;

    // Send the new value to the D/A board
#if defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
	if (!me->setValues()) {
	  me->send_text_message( "vrpn_National_Instruments_Server::handle_request_message(): Could not set values", p.msg_time, vrpn_TEXT_ERROR );
	  return -1;
	}

#elif	defined(VRPN_USE_NATIONAL_INSTRUMENTS)
    if (me->d_device_number != -1) {
      AO_VWrite(me->d_device_number, (short)(chan_num), value);
    }
#endif
    return 0;
}

/* static */
int vrpn_National_Instruments_Server::handle_request_channels_message(void* userdata,
    vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_National_Instruments_Server* me = (vrpn_National_Instruments_Server*)userdata;
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
		if (value < me->d_out_min_voltage) {
		  char msg[1024];
		  sprintf( msg, "Error:  (handle_request_messages): voltage %g is too low.  Clamping to %g.", value, me->d_out_min_voltage);
		  me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
		  value = me->d_out_min_voltage;
		}
		if (value > me->d_out_max_voltage) {
		  char msg[1024];
		  sprintf( msg, "Error:  (handle_request_messages): voltage %g is too high.  Clamping to %g.", value, me->d_out_max_voltage);
		  me->send_text_message( msg, p.msg_time, vrpn_TEXT_WARNING );
		  value = me->d_out_max_voltage;
		}
		me->o_channel[chan_num] = value;

		// Send the new value to the D/A board
#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
		if (me->d_device_number != -1) {
		  AO_VWrite(me->d_device_number, (short)(chan_num), value);
		}
#endif
    }

#if defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
	if (!me->setValues()) {
	  me->send_text_message( "vrpn_National_Instruments_Server::handle_request_channels_message(): Could not set values", p.msg_time, vrpn_TEXT_ERROR );
	  return -1;
	}
#endif

    return 0;
}

#if defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
bool vrpn_National_Instruments_Server::setValues() {
	// Send all current values to the board, including the
	// changed one.
	float64 outbuffer[vrpn_CHANNEL_MAX];
	int i;
	for (i = 0; i < vrpn_Analog_Output::o_num_channel ; i++) {
	  outbuffer[i] = vrpn_Analog_Output::o_channel[i];
	}
	int32 error;
	error = DAQmxWriteAnalogF64(d_analog_out_task_handle, 1, true, 1.0,
	  DAQmx_Val_GroupByChannel, outbuffer, NULL, NULL);
	if (error) {
	  reportError(error);
	  return false;
	}
	/*
	printf("Debug: Setting %d channels:", o_num_channel);
	for (i = 0; i < o_num_channel; i++) {
		printf(" %lg", outbuffer[i]);
	}
	printf("\n");
	*/
        return true;
}
#endif

/* static */
int VRPN_CALLBACK vrpn_National_Instruments_Server::handle_got_connection( void* userdata, vrpn_HANDLERPARAM )
{
  vrpn_National_Instruments_Server* me = (vrpn_National_Instruments_Server*) userdata;
  if( me->report_num_channels( ) == false ) {
    fprintf( stderr, "Error:  failed sending active channels to client.\n" );
  }
  return 0;
}

bool vrpn_National_Instruments_Server::report_num_channels( vrpn_uint32 class_of_service )
{
	char msgbuf[ sizeof( vrpn_int32) ];
	vrpn_int32 len = sizeof( vrpn_int32 );;
	
	encode_num_channels_to( msgbuf, o_num_channel );
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


vrpn_int32 vrpn_National_Instruments_Server::encode_num_channels_to( char* buf, vrpn_int32 num )
{
    // Message includes: int32 number of active channels
    int	buflen = sizeof(vrpn_int32);

    vrpn_buffer(&buf, &buflen, num);
    return sizeof(vrpn_int32);
}

vrpn_Analog_Output_Server_NI::vrpn_Analog_Output_Server_NI(const char* name, vrpn_Connection * c, 
                                                     const char *boardName,
						     vrpn_int16 numChannels, bool bipolar,
						     double minVoltage, double maxVoltage) :
	vrpn_Analog_Output(name, c),
	NI_device_number(-1),
	NI_num_channels(numChannels),
	min_voltage(minVoltage),
	max_voltage(maxVoltage)
{
#ifdef	VRPN_USE_NATIONAL_INSTRUMENTS
  short i;
  short	  update_mode = 0;	//< Mode 0 is immediate
  short	  ref_source = 0;	//< Reference source, 0 = internal, 1 = external
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
/*
      fprintf(stderr,"Configuring channel %d, polarity: %d, reference source: %d, reference_voltage: %lg, update_mode: %d\n",
          i, polarity, ref_source, ref_voltage, update_mode);
*/
      AO_VWrite(NI_device_number, i, min_voltage);
  }


  setNumChannels( numChannels );

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
  const char *unused = boardName;
  unused++;
  bipolar = !bipolar;
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
int VRPN_CALLBACK vrpn_Analog_Output_Server_NI::handle_request_message(void *userdata,
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
      AO_VWrite(me->NI_device_number, (short)(chan_num), value);
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
	  AO_VWrite(me->NI_device_number, (short)(chan_num), value);
	}
#endif
    }
    
    return 0;
}


/* static */
int VRPN_CALLBACK vrpn_Analog_Output_Server_NI::handle_got_connection( void* userdata, vrpn_HANDLERPARAM )
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
	
	encode_num_channels_to( msgbuf, o_num_channel );
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

//  This handles error reporting
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
void vrpn_National_Instruments_Server::reportError(int32 errnumber, vrpn_bool exitProgram)
{
    char    errBuff[2048]={'\0'};

    if( DAQmxFailed(errnumber) )
    {
        DAQmxGetExtendedErrorInfo(errBuff,2048);
        printf("DAQmx Error: %s\n",errBuff);
        if (exitProgram==vrpn_true) {
            printf("Exiting...\n") ;
            throw(errnumber) ;    //  this will quit, cause the destructor to be called
        } else {
            printf("Sleeping...\n") ;
            vrpn_SleepMsecs(1000.0*1) ;    //  so at least the log will slow down so someone can see the error
        }
    }
}    //  reportError
#endif
