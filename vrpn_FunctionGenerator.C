
#include "vrpn_FunctionGenerator.h"


const double PI=.14159265358979323846264338327950288419716939937510;

const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL = "vrpn_FunctionGenerator channel";
const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REQUEST = "vrpn_FunctionGenerator channel request";
const char* vrpn_FUNCTION_MESSAGE_TYPE_ALL_CHANNEL_REQUEST = "vrpn_FunctionGenerator all channel request";
const char* vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE = "vrpn_FunctionGenerator sample rate";
const char* vrpn_FUNCTION_MESSAGE_TYPE_START = "vrpn_FunctionGenerator start";
const char* vrpn_FUNCTION_MESSAGE_TYPE_STOP = "vrpn_FunctionGenerator stop";
const char* vrpn_FUNCTION_MESSAGE_TYPE_REFERENCE_CHANNEL = "vrpn_FunctionGenerator reference channel";
const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REPLY = "vrpn_FunctionGenerator channel reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_START_REPLY = "vrpn_FunctionGenerator start reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_STOP_REPLY = "vrpn_FunctionGenerator stop reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE_REPLY = "vrpn_FunctionGenerator sample rate reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_REFERENCE_CHANNEL_REPLY = "vrpn_FunctionGenerator reference channel reply";


/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_function_NULL

vrpn_float32 vrpn_FunctionGenerator_function_NULL::
generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
				 vrpn_float32 startTime, vrpn_float32 sampleRate, 
				 vrpn_FunctionGenerator_channel* ) const
{
	for( vrpn_uint32 i = 0; i <= nValues - 1; i++ )
	{
		buf[i] = 0;
	}
	return startTime + nValues / sampleRate;
}

vrpn_float32 vrpn_FunctionGenerator_function_NULL::
getCycleTime( ) const
{
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_function_NULL::
encode_to( char** , vrpn_int32& ) const
{
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_function_NULL::
decode_from( const char** , vrpn_int32& )
{
	return 0;
}
//
// end vrpn_FunctionGenerator_function_NULL
////////////////////////////////////////
////////////////////////////////////////


/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_function_sine

vrpn_float32 vrpn_FunctionGenerator_function_sine::
generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
				 vrpn_float32 startTime, vrpn_float32 sampleRate, 
				 vrpn_FunctionGenerator_channel* channel ) const
{
	vrpn_float64 t = startTime;
	vrpn_float64 dt = 1 / sampleRate;
	for( vrpn_uint32 i = 0; i <= nValues - 1; i++ )
	{
		buf[i] = (vrpn_float32) ( channel->offset + channel->gain * amplitude
				* sin( 2 * PI * channel->scaleTime * frequency * t + channel->phaseFromRef ) );
		t += dt;
	}
	return (vrpn_float32) (t - dt);
}

vrpn_float32 vrpn_FunctionGenerator_function_sine::
getCycleTime( ) const
{
	return 1/frequency;
}


vrpn_int32 vrpn_FunctionGenerator_function_sine::
encode_to( char** buf, vrpn_int32& len ) const
{
	if( len < 2 * sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_sine::encode_to:  "
				"payload error (wanted %d got %d).\n", 2 * sizeof( vrpn_float32 ), len );
		return -1;
	}
	vrpn_int32 retval = 0;
	retval |= vrpn_buffer( buf, &len, frequency );
	retval |= vrpn_buffer( buf, &len, amplitude );
	if( retval < 0 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_sine::encode_to:  "
				"payload error (couldn't buffer).\n" );
		return -1;
	}
	 return 2 * sizeof( vrpn_float32 );
}


vrpn_int32 vrpn_FunctionGenerator_function_sine::
decode_from( const char** buf, vrpn_int32& len )
{
	if( len < 2 * sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_sine::decode_from:  "
				"payload error (wanted %d got %d).\n", 2 * sizeof( vrpn_float32 ), len );
		return -1;
	}

	vrpn_float32 newfreq, newamp;
	vrpn_int32 retval = 0;
	retval |= vrpn_unbuffer( buf, &newfreq );
	len -= sizeof( newfreq );

	retval |= vrpn_unbuffer( buf, &newamp );
	len -= sizeof( newamp );
	
	if( 0 > retval )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_sine::decode_from:  "
				"payload error (couldn't unbuffer).\n" );
		return -1;
	}
	frequency = newfreq;
	amplitude = newamp;

	return 2 * sizeof( vrpn_float32 );
}
//
// end vrpn_FunctionGenerator_function_sine
////////////////////////////////////////
////////////////////////////////////////



/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_function_degauss

vrpn_FunctionGenerator_function_degauss::
vrpn_FunctionGenerator_function_degauss( )
:	initialValue(1.0f),
	finalValue(0.1f),
	frequency(1.0f),
	decay(0.9f)
{

}

vrpn_float32 vrpn_FunctionGenerator_function_degauss::
generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
				 vrpn_float32 startTime, vrpn_float32 sampleRate, 
				 vrpn_FunctionGenerator_channel* channel ) const
{
	vrpn_float64 t = startTime;
	vrpn_float64 dt = 1 / sampleRate;
	for( vrpn_uint32 i = 0; i <= nValues - 1; i++ )
	{
		buf[i] = (vrpn_float32) ( channel->offset + channel->gain * initialValue
				* pow( static_cast<double>(decay), frequency * t )
				* sin( 2 * PI * channel->scaleTime * frequency * t + channel->phaseFromRef ) );
		t += dt;
	}
	return (vrpn_float32) (t - dt);
}


vrpn_int32 vrpn_FunctionGenerator_function_degauss::
encode_to( char** buf, vrpn_int32& len ) const
{
	if( len < 4 * sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_degauss::encode_to:  "
				"payload error (wanted %d got %d).\n", 4 * sizeof( vrpn_float32 ), len );
		return -1;
	}
	vrpn_int32 retval = 0;
	retval |= vrpn_buffer( buf, &len, initialValue );
	retval |= vrpn_buffer( buf, &len, finalValue );
	retval |= vrpn_buffer( buf, &len, frequency );
	retval |= vrpn_buffer( buf, &len, decay );
	if( retval < 0 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_degauss::encode_to:  "
				"payload error (couldn't buffer).\n" );
		return -1;
	}
	return 2 * sizeof( vrpn_float32 );
}


vrpn_int32 vrpn_FunctionGenerator_function_degauss::
decode_from( const char** buf, vrpn_int32& len )
{
	if( len < 4 * sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_degauss::decode_from:  "
				"payload error (wanted %d got %d).\n", 4 * sizeof( vrpn_float32 ), len );
		return -1;
	}

	vrpn_float32 newInitVal, newFinalVal, newFreq, newDecay;
	vrpn_int32 retval = 0;
	retval |= vrpn_unbuffer( buf, &newInitVal );
	len -= sizeof( newInitVal );

	retval |= vrpn_unbuffer( buf, &newFinalVal );
	len -= sizeof( newFinalVal );
	
	retval |= vrpn_unbuffer( buf, &newFreq );
	len -= sizeof( newFreq );
	
	retval |= vrpn_unbuffer( buf, &newDecay );
	len -= sizeof( newDecay );
	
	if( 0 > retval )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_degauss::decode_from:  "
				"payload error (couldn't unbuffer).\n" );
		return -1;
	}
	initialValue = newInitVal;
	finalValue = newFinalVal;
	frequency = newFreq;
	decay = newDecay;

	return 2 * sizeof( vrpn_float32 );
}


void vrpn_FunctionGenerator_function_degauss::
calculateCycleTime( )
{
	// final = initial * (decay) ^ (frequency * cycleTime)
	// so cycleTime = log(final/initial) / ( log(decay) * frequency )
	cycleTime = (vrpn_float32) ( log( finalValue / initialValue ) / log( decay ) / frequency );
}


//
// end vrpn_FunctionGenerator_function_degauss
////////////////////////////////////////
////////////////////////////////////////



/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_function_ramp

vrpn_FunctionGenerator_function_ramp::
vrpn_FunctionGenerator_function_ramp( )
: 	upVoltage(1),
	downVoltage(1),
	initialFlat(1),
	rampHighUp(1),
	rampHighDwell(1),
	rampHighDown(1),
	midFlat(1),
	rampLowDown(1),
	rampLowDwell(1),
	rampLowUp(1)
{ }


vrpn_float32 vrpn_FunctionGenerator_function_ramp::
generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
				 vrpn_float32 startTime, vrpn_float32 sampleRate, 
				 vrpn_FunctionGenerator_channel* channel ) const
{
	vrpn_float64 t = startTime + channel->phaseFromRef;
	vrpn_float64 dt = channel->scaleTime / sampleRate;
	vrpn_float64 cycle = getCycleTime( );
	if( t > cycle )
	{
		while( t > cycle )
			t -= cycle;
	}
	for( vrpn_uint32 i = 0; i <= nValues - 1; i++ )
	{
		if( t > cycle )
		{
			while( t > cycle )
				t -= cycle;
		}
		if( t < initialFlat )
			buf[i] = 0;
		else if( t < rampHighUp )
			buf[i] = lerp( (vrpn_float32) t, initialFlat, rampHighUp, 0, upVoltage );
		else if( t < rampHighDwell )
			buf[i] = upVoltage;
		else if( t < rampHighDown )
			buf[i] = lerp( (vrpn_float32) t, rampHighDwell, rampHighDown, upVoltage, 0 );
		else if( t < midFlat )
			buf[i] = 0;
		else if( t < rampLowDown )
			buf[i] = lerp( (vrpn_float32) t, midFlat, rampLowDown, 0, downVoltage );
		else if( t < rampLowDwell )
			buf[i] = downVoltage;
		else if( t < rampLowUp )
			buf[i] = lerp( (vrpn_float32) t, rampLowDwell, rampLowUp, downVoltage, 0 );
		else // we shouldn't be here
			buf[i] = 0;
		buf[i] *= channel->gain;
		buf[i] += channel->offset;
		t += dt;
	}
	return (vrpn_float32) ( t - dt );
}

vrpn_float32 vrpn_FunctionGenerator_function_ramp::
getCycleTime( ) const
{
	return upVoltage + downVoltage + initialFlat + rampHighUp
		 + rampHighDwell + rampHighDown + midFlat + rampLowDown
		 + rampLowDwell + rampLowUp;
}


vrpn_int32 vrpn_FunctionGenerator_function_ramp::
encode_to( char** buf, vrpn_int32& len ) const
{
	if( len < 10 * sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_ramp::encode_to:  "
				"payload error (wanted %d got %d).\n", 10 * sizeof( vrpn_float32 ), len );
		return -1;
	}
	vrpn_int32 retval = 0;
	retval |= vrpn_buffer( buf, &len, upVoltage );
	retval |= vrpn_buffer( buf, &len, downVoltage );
	retval |= vrpn_buffer( buf, &len, initialFlat );
	retval |= vrpn_buffer( buf, &len, rampHighUp );
	retval |= vrpn_buffer( buf, &len, rampHighDwell );
	retval |= vrpn_buffer( buf, &len, rampHighDown );
	retval |= vrpn_buffer( buf, &len, midFlat );
	retval |= vrpn_buffer( buf, &len, rampLowDown );
	retval |= vrpn_buffer( buf, &len, rampLowDwell );
	retval |= vrpn_buffer( buf, &len, rampLowUp );
	if( retval < 0 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_ramp::encode_to:  "
				"payload error (couldn't buffer).\n" );
		return -1;
	}
	return 10 * sizeof( vrpn_float32 );
}


vrpn_int32 vrpn_FunctionGenerator_function_ramp::
decode_from( const char** buf, vrpn_int32& len )
{
	if( len < 10 * sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_ramp::decode_from:  "
				"payload error (wanted %d got %d).\n", 10 * sizeof( vrpn_float32 ), len );
		return -1;
	}

	vrpn_float32 newupVoltage, newdownVoltage, newinitialFlat, newrampHighUp,
		 newrampHighDwell, newrampHighDown, newmidFlat, newrampLowDown,
		 newrampLowDwell, newrampLowUp;
	vrpn_int32 retval = 0;
	retval |= vrpn_unbuffer( buf, &newupVoltage );
	len -= sizeof( newupVoltage );

	retval |= vrpn_unbuffer( buf, &newdownVoltage );
	len -= sizeof( newdownVoltage );

	retval |= vrpn_unbuffer( buf, &newinitialFlat );
	len -= sizeof( newinitialFlat );

	retval |= vrpn_unbuffer( buf, &newrampHighUp );
	len -= sizeof( newrampHighUp );

	retval |= vrpn_unbuffer( buf, &newrampHighDwell );
	len -= sizeof( newrampHighDwell );

	retval |= vrpn_unbuffer( buf, &newrampHighDown );
	len -= sizeof( newrampHighDown );

	retval |= vrpn_unbuffer( buf, &newmidFlat );
	len -= sizeof( newmidFlat );

	retval |= vrpn_unbuffer( buf, &newrampLowDown );
	len -= sizeof( newrampLowDown );

	retval |= vrpn_unbuffer( buf, &newrampLowDwell );
	len -= sizeof( newrampLowDwell );

	retval |= vrpn_unbuffer( buf, &newrampLowUp );
	len -= sizeof( newrampLowUp );

	if( 0 > retval )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_ramp::decode_from:  "
				"payload error (couldn't unbuffer).\n" );
		return -1;
	}

	upVoltage = newupVoltage;
	downVoltage = newdownVoltage;
	initialFlat = newinitialFlat;
	rampHighUp = newrampHighUp;
	rampHighDwell = newrampHighDwell;
	rampHighDown = newrampHighDown;
	midFlat = newmidFlat;
	rampLowDown = newrampLowDown;
	rampLowDwell = newrampLowDwell;
	rampLowUp = newrampLowUp;

	return 10 * sizeof( vrpn_float32 );
}
//
// end vrpn_FunctionGenerator_function_ramp
////////////////////////////////////////
////////////////////////////////////////


/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_function_custom
vrpn_FunctionGenerator_function_custom::
vrpn_FunctionGenerator_function_custom( vrpn_uint32 length, vrpn_float32* times, vrpn_float32* values )
{
	if( length == 0 || times == NULL || values == NULL )
	{
		length = 1;
		d_times = new vrpn_float32[1];
		d_times[0] = 0;
		d_values = new vrpn_float32[1];
		d_values[0] = 0;
		return;
	}
	d_length = length;
	d_times = new vrpn_float32[length];
	d_values = new vrpn_float32[length];
	if ( (d_times == NULL) || (d_values == NULL) ) {
	  fprintf(stderr,"vrpn_FunctionGenerator_function_custom::vrpn_FunctionGenerator_function_custom(): Out of memory\n");
	  return;
	}
	for( vrpn_uint32 i = 0; i < d_length; i++ ){
	  d_times[i] = times[i];
	  d_values[i] = values[i];
	}
}


vrpn_FunctionGenerator_function_custom::
~vrpn_FunctionGenerator_function_custom( )
{
	delete[] d_times;
	delete[] d_values;
}


bool vrpn_FunctionGenerator_function_custom::
set( vrpn_uint32 length, vrpn_float32* times, vrpn_float32* values )
{
  if( length == 0 || times == NULL || values == NULL ) {
    fprintf(stderr, "vrpn_FunctionGenerator_function_custom::set(): Zero or NULL passed in\n");
    return false;
  } else if (length != d_length) {
    fprintf(stderr, "vrpn_FunctionGenerator_function_custom::set(): Different length (%d vs. %d)\n", length, d_length);
    return false;
  } else {
    for( vrpn_uint32 i = 0; i < d_length; i++ ){
      d_times[i] = times[i];
      d_values[i] = values[i];
    }
    return true;
  }
}


vrpn_float32 vrpn_FunctionGenerator_function_custom::
generateValues(vrpn_float32* buf, vrpn_uint32 nValues,
	       vrpn_float32 startTime, vrpn_float32 sampleRate, 
	       vrpn_FunctionGenerator_channel* channel ) const
{
	vrpn_float64 t = startTime;
	vrpn_float64 dt = 1 / sampleRate;
	for( vrpn_uint32 i = 0; i <= nValues - 1; i++ )
	{
//		buf[i] = (vrpn_float32) ( channel->offset + channel->gain * amplitude
//				* sin( channel->scaleTime * frequency * t + channel->phaseFromRef ) );
		t += dt;
	}
	return (vrpn_float32) (t - dt);
}

vrpn_float32 vrpn_FunctionGenerator_function_custom::
getCycleTime( ) const
{
	return d_times[d_length - 1] - d_times[0];
}


vrpn_int32 vrpn_FunctionGenerator_function_custom::
encode_to( char** buf, vrpn_int32& len ) const
{
	vrpn_int32 bytes = ( 2 * d_length + 1 ) * sizeof( vrpn_float32 );
	if( len < bytes )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_custom::encode_to:  "
				"payload error (wanted %d got %d).\n", bytes, len );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, d_length ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_custom::encode_to:  "
				"payload error (couldn't buffer length).\n" );
		return -1;
	}
	vrpn_int32 retval = 0;
	for( vrpn_uint32 i = 0; i < d_length; i++ )
	{
		retval |= vrpn_buffer( buf, &len, d_times[i] );
		retval |= vrpn_buffer( buf, &len, d_values[i] );
	}
	if( retval < 0 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_custom::encode_to:  "
				"payload error (couldn't buffer).\n" );
		return -1;
	}
	return bytes;
}


vrpn_int32 vrpn_FunctionGenerator_function_custom::
decode_from( const char** buf, vrpn_int32& len )
{
	vrpn_uint32 newlen;
	if( 0 > vrpn_unbuffer( buf, &newlen ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_custom::encode_to:  "
				"payload error (couldn't unbuffer length).\n" );
		return -1;
	}

	vrpn_int32 bytes = ( 2 * newlen + 1 ) * sizeof( vrpn_float32 );
	if( len < bytes )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_custom::decode_from:  "
				"payload error (wanted %d got %d).\n", bytes, len );
		return -1;
	}

	vrpn_float32* newtimes = new vrpn_float32[newlen];
	vrpn_float32* newvalues = new vrpn_float32[newlen];

	vrpn_int32 retval = 0;
	for( vrpn_uint32 i = 0; i <= newlen - 1; i++ )
	{
		retval |= vrpn_unbuffer( buf, &newtimes[i] );
		retval |= vrpn_unbuffer( buf, &newvalues[i] );
	}	
	if( 0 > retval )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_custom::decode_from:  "
				"payload error (couldn't unbuffer).\n" );
		return -1;
	}
	this->set( newlen, newtimes, newvalues );
	delete[] newtimes;
	delete[] newvalues;
	len -= bytes;
	return bytes;
}
//
// end vrpn_FunctionGenerator_function_custom
////////////////////////////////////////
////////////////////////////////////////



/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_channel

vrpn_FunctionGenerator_channel::
vrpn_FunctionGenerator_channel( )
: triggered( true ),
  phaseFromRef( 0 ),
  gain( 1 ),
  offset( 0 ),
  repeat( REPEAT_MANUAL ),
  repeatNum( 0 ),
  repeatTime( 0 ), 
  scaleTime( 1 )
{
	function = new vrpn_FunctionGenerator_function_NULL( );
}


vrpn_FunctionGenerator_channel::
vrpn_FunctionGenerator_channel( vrpn_FunctionGenerator_function* function )
: triggered( true ),
  phaseFromRef( 0 ),
  gain( 1 ),
  offset( 0 ),
  repeat( REPEAT_MANUAL ),
  repeatNum( 0 ),
  repeatTime( 0 ), 
  scaleTime( 1 )
{
	this->function = function;
}


vrpn_FunctionGenerator_channel::
~vrpn_FunctionGenerator_channel( )
{
	delete function;
}



void vrpn_FunctionGenerator_channel::
setFunction( vrpn_FunctionGenerator_function* function )
{
	delete (this->function);
	this->function = function;
}


vrpn_int32 vrpn_FunctionGenerator_channel::
encode_to( char** buf, vrpn_int32& len ) const
{
	vrpn_int32 retval = 0;
	retval |= vrpn_buffer( buf, &len, phaseFromRef );
	retval |= vrpn_buffer( buf, &len, gain );
	retval |= vrpn_buffer( buf, &len, offset );
	retval |= vrpn_buffer( buf, &len, scaleTime );
	retval |= vrpn_buffer( buf, &len, triggered );
	retval |= vrpn_buffer( buf, &len, repeat );
	retval |= vrpn_buffer( buf, &len, repeatNum );
	retval |= vrpn_buffer( buf, &len, repeatTime );
	if( retval < 0 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_channel::encode_to:  "
				"payload error (couldn't buffer).\n" );
		return -1;
	}
	return function->encode_to( buf, len );
}


vrpn_int32 vrpn_FunctionGenerator_channel::
decode_from( const char** buf, vrpn_int32& len )
{
	vrpn_float32 newPhase, newGain, newOffset, newScale,
		newRepeatNum, newRepeatTime;
	vrpn_bool newTriggered;
	vrpn_int32 newRepeat;

	vrpn_int32 retval = 0;
	retval |= vrpn_unbuffer( buf, &newPhase );
	len -= sizeof( newPhase );

	retval |= vrpn_unbuffer( buf, &newGain );
	len -= sizeof( newGain );
	
	retval |= vrpn_unbuffer( buf, &newOffset );
	len -= sizeof( newOffset );
	
	retval |= vrpn_unbuffer( buf, &newScale );
	len -= sizeof( newScale );
	
	retval |= vrpn_unbuffer( buf, &newTriggered );
	len -= sizeof( newTriggered );
	
	retval |= vrpn_unbuffer( buf, &newRepeat );
	len -= sizeof( newRepeat );
	
	retval |= vrpn_unbuffer( buf, &newRepeatNum );
	len -= sizeof( newRepeatNum );
	
	retval |= vrpn_unbuffer( buf, &newRepeatTime );
	len -= sizeof( newRepeatTime );
	
	if( 0 > retval )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_channel::decode_from:  "
				"payload error (couldn't unbuffer).\n" );
		return -1;
	}

	phaseFromRef = newPhase;
	gain = newGain;
	offset = newOffset;
	scaleTime = newScale;
	triggered = newTriggered;
	repeat = vrpn_FunctionGenerator_RepeatStyle( newRepeat );
	repeatNum = newRepeatNum;
	repeatTime = newRepeatTime;
	return function->decode_from( buf, len );
}

//
// end vrpn_FunctionGenerator_channel
////////////////////////////////////////
////////////////////////////////////////


/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator

vrpn_FunctionGenerator::
vrpn_FunctionGenerator( const char* name, vrpn_Connection * c )
:vrpn_BaseClass( name, c )
{
	vrpn_BaseClass::init( );

	sampleRate = 0;
	referenceChannel = 0;
	for( int i = 0; i <= vrpn_FUNCTION_CHANNELS_MAX - 1; i++ )
	{
		channels[i] = new vrpn_FunctionGenerator_channel( );
	}
}


vrpn_FunctionGenerator::
~vrpn_FunctionGenerator( )
{
	for( int i = 0; i <= vrpn_FUNCTION_CHANNELS_MAX - 1; i++ )
	{
		 delete channels[i];
	}
}


const vrpn_FunctionGenerator_channel* const vrpn_FunctionGenerator::
getChannel( vrpn_uint32 channelNum )
{
	if( channelNum < 0 || channelNum > vrpn_FUNCTION_CHANNELS_MAX - 1 )
		return NULL;
	return channels[channelNum];
}


int vrpn_FunctionGenerator::
register_types( )
{
	channelMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL );
	requestChannelMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REQUEST );
	requestAllChannelsMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_ALL_CHANNEL_REQUEST );
	sampleRateMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE );
	startFunctionMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_START );
	stopFunctionMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_STOP );
	referenceChannelMessagID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_REFERENCE_CHANNEL );
	channelReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REPLY );
	startFunctionReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_START_REPLY );
	stopFunctionReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_STOP_REPLY );
	sampleRateReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE_REPLY );
	refChannelReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_REFERENCE_CHANNEL_REPLY );
	gotConnectionMessageID = d_connection->register_message_type( vrpn_got_connection );

	if( channelMessageID == -1 || requestChannelMessageID == -1
		|| requestAllChannelsMessageID == -1
		|| sampleRateMessageID == -1 || startFunctionMessageID == -1
		|| stopFunctionMessageID == -1 || referenceChannelMessagID == -1
		|| channelReplyMessageID == -1 || startFunctionReplyMessageID == -1
		|| stopFunctionReplyMessageID == -1 || sampleRateReplyMessageID == -1
		|| refChannelReplyMessageID == -1 || gotConnectionMessageID == -1 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator::register_types:  error registering types.\n" );
		return -1;
	}
	return 0;
}

//
//  end of vrpn_FunctionGenerator
// 
//////////////////////////////////////////
//////////////////////////////////////////



///////////////////////////////////////////
///////////////////////////////////////////
//
// vrpn_FunctionGenerator_Server
//

vrpn_FunctionGenerator_Server::
vrpn_FunctionGenerator_Server( const char* name, vrpn_Connection * c )
: vrpn_FunctionGenerator( name, c )
{
	
}


vrpn_FunctionGenerator_Server::
~vrpn_FunctionGenerator_Server( )
{

}


void vrpn_FunctionGenerator_Server::
mainloop( )
{
	// call the base class's server mainloop
	server_mainloop( );
}


//static 
int vrpn_FunctionGenerator_Server::
handle_channel_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	vrpn_FunctionGenerator_channel* channel = new vrpn_FunctionGenerator_channel( );
	vrpn_uint32 channelNum = vrpn_FUNCTION_CHANNELS_MAX + 1; // an invalid number
	if( 0 > me->decode_channel( p.buffer, p.payload_len, channelNum, *channel ) )
	{
		if( channelNum < vrpn_FUNCTION_CHANNELS_MAX )
		{
			// the decode function was able to decode the channel
			// number, but must have had some problem decoding the channel.
			// let the remotes know the channel didn't change
			me->sendChannelReply( channelNum );
		}
		// else, we couldn't even decode the channel number
	}

	// let the server implementation see if this channel is acceptable
	me->setChannel( channelNum, channel );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Server::
handle_channelRequest_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	vrpn_uint32 channelNum = vrpn_FUNCTION_CHANNELS_MAX + 1; // an invalid number
	if( 0 > me->decode_channel_request( p.buffer, p.payload_len, channelNum ) )
	{
		// the decode failed
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_channelRequest_message:  "
				"unable to decode channel number.\n" );
		return -1;
	}
	if( channelNum > vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_channelRequest_message:  "
				"invalid channel number %d.\n", channelNum );
		return -1;
	}
	me->sendChannelReply( channelNum );
	
	return 0;
}


//static 
int vrpn_FunctionGenerator_Server::
handle_allChannelRequest_message( void* userdata, vrpn_HANDLERPARAM)
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	for( int i = 0; i < vrpn_FUNCTION_CHANNELS_MAX; i++ )
	{
		// XXX will this work as-is, or do we need to
		// force buffers to be flushed periodically?
		me->sendChannelReply( i );
	}
	return 0;
}


//static 
int vrpn_FunctionGenerator_Server::
handle_start_message( void* userdata, vrpn_HANDLERPARAM)
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	me->start( );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Server::
handle_stop_message( void* userdata, vrpn_HANDLERPARAM)
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	me->stop( );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Server::
handle_sample_rate_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	vrpn_float32 rate = 0;
	if( 0 > me->decode_sample_rate( p.buffer, p.payload_len, rate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_sample_rate_message:  "
				"unable to decode.\n" );
		me->sendSampleRateReply( );
		return -1;
	}
	me->setSampleRate( rate );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Server::
handle_reference_channel_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	vrpn_uint32 channelNum = vrpn_FUNCTION_CHANNELS_MAX + 1; // an invalid number
	if( 0 > me->decode_reference_channel( p.buffer, p.payload_len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_reference_channel_message:  "
				"unable to decode.\n" );
		me->sendReferenceChannelReply( );
		return -1;
	}
	me->setReferenceChannel( channelNum );
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendChannelReply( vrpn_uint32 channelNum )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( this->encode_channel_reply( &buf, buflen, channelNum ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendSampleRateReply:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->channelReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendChannelReply:  "
				"could not write message.\n" );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendSampleRateReply( )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( this->encode_sample_rate_reply( &buf, buflen, sampleRate ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendSampleRateReply:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->sampleRateReplyMessageID,	this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendSampleRateReply:  "
				"could not write message.\n" );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendReferenceChannelReply( )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( this->encode_reference_channel_reply( &buf, buflen, referenceChannel ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendReferenceChannelReply:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->refChannelReplyMessageID,	this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendReferenceChannelReply:  "
				"could not write message.\n" );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendStartReply( vrpn_bool started )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_start_reply( &buf, buflen, started ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStartReply:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->startFunctionReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStartReply:  "
				"could not write message.\n" );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendStopReply( vrpn_bool stopped )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_stop_reply( &buf, buflen, stopped ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStopReply:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->stopFunctionReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStopReply:  "
				"could not write message.\n" );
			return -1;
		}
	}
	return 0;
}

//
// end vrpn_FunctionGenerator_Server
//  (except for encode and decode functions)
//////////////////////////////////////////
//////////////////////////////////////////




///////////////////////////////////////////
///////////////////////////////////////////
//
// vrpn_FunctionGenerator_Remote
//

vrpn_FunctionGenerator_Remote::
vrpn_FunctionGenerator_Remote( const char* name, vrpn_Connection * c )
: vrpn_FunctionGenerator( name, c ),
  channel_reply_list( NULL ),
  start_reply_list( NULL ),
  stop_reply_list( NULL ),
  sample_rate_reply_list( NULL ),
  reference_channel_reply_list( NULL )
{
}


vrpn_FunctionGenerator_Remote::
~vrpn_FunctionGenerator_Remote( )
{
	while( channel_reply_list != NULL )
	{
		vrpn_FGCHANNELREPLYLIST* member = channel_reply_list;
		channel_reply_list = member->next;
		delete member;
	}
	while( start_reply_list != NULL )
	{
		vrpn_FGSTARTREPLYLIST* member = start_reply_list;
		start_reply_list = member->next;
		delete member;
	}
	while( stop_reply_list != NULL )
	{
		vrpn_FGSTOPREPLYLIST* member = stop_reply_list;
		stop_reply_list = member->next;
		delete member;
	}
	while( sample_rate_reply_list != NULL )
	{
		vrpn_FGSAMPLERATEREPLYLIST* member = sample_rate_reply_list;
		sample_rate_reply_list = member->next;
		delete member;
	}
	while( reference_channel_reply_list != NULL )
	{
		vrpn_FGREFERENCECHANNELREPLYLIST* member = reference_channel_reply_list;
		reference_channel_reply_list = member->next;
		delete member;
	}
}


int vrpn_FunctionGenerator_Remote::
setChannel( const vrpn_uint32 channelNum, const vrpn_FunctionGenerator_channel* channel )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_channel( &buf, buflen, channelNum, channel ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::setChannel:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->channelMessageID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::setChannel:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::setChannel:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestChannel( const vrpn_uint32 channelNum )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_requestChannel( &buf, buflen, channelNum ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestChannel:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->requestChannelMessageID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestChannel:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestChannel:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}



int vrpn_FunctionGenerator_Remote::
requestAllChannels( )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		// nothing to encode; the message type is the symbol
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->requestAllChannelsMessageID, this->d_sender_id, buf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestAllChannels:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestAllChannels:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestStart( )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		// nothing to encode; the message type is the symbol
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->startFunctionMessageID, this->d_sender_id, buf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestStart:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestStart:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestStop( )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		// nothing to encode; the message type is the symbol
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->stopFunctionMessageID, this->d_sender_id, buf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestStop:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestStop:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestSampleRate( vrpn_float32 rate )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_sampleRate( &buf, buflen, rate ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->sampleRateMessageID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestSetReferenceChannel( const vrpn_uint32 channelNum )
{
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_referenceChannel( &buf, buflen, channelNum ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"could not buffer message.\n" );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->referenceChannelMessagID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"could not write message.\n" );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"no connection.\n" );
		return -1;
	}
	return 0;
}


void vrpn_FunctionGenerator_Remote::
mainloop( )
{
	if( d_connection != NULL )
	{
		d_connection->mainloop( );
		client_mainloop( );
	}
}


int vrpn_FunctionGenerator_Remote::
register_channel_reply_handler( void *userdata,
							   vrpn_FUNCTION_CHANGE_REPLY_HANDLER handler )
{
	if( handler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_channel_reply_handler:  "
				"NULL handler.\n" );
		return -1;
	}
	vrpn_FGCHANNELREPLYLIST* newHandler = new vrpn_FGCHANNELREPLYLIST;
	if( newHandler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_channel_reply_handler:  "
				"out of memory.\n" );
		return -1;
	}
	newHandler->userdata = userdata;
	newHandler->handler = handler;
	newHandler->next = channel_reply_list;
	channel_reply_list = newHandler;
	return 0;
}


int vrpn_FunctionGenerator_Remote::
unregister_channel_reply_handler( void *userdata,
								 vrpn_FUNCTION_CHANGE_REPLY_HANDLER handler )
{
	// The pointer at *snitch points to victim
	vrpn_FGCHANNELREPLYLIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &channel_reply_list;
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
		   "vrpn_FunctionGenerator_Remote::unregister_channel_reply_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


int vrpn_FunctionGenerator_Remote::
register_start_reply_handler( void *userdata,
							 vrpn_FUNCTION_START_REPLY_HANDLER handler )
{
	if( handler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_start_reply_handler:  "
				"NULL handler.\n" );
		return -1;
	}
	vrpn_FGSTARTREPLYLIST* newHandler = new vrpn_FGSTARTREPLYLIST;
	if( newHandler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_start_reply_handler:  "
				"out of memory.\n" );
		return -1;
	}
	newHandler->userdata = userdata;
	newHandler->handler = handler;
	newHandler->next = start_reply_list;
	start_reply_list = newHandler;
	return 0;
}


int vrpn_FunctionGenerator_Remote::
unregister_start_reply_handler( void *userdata,
							   vrpn_FUNCTION_START_REPLY_HANDLER handler )
{
	// The pointer at *snitch points to victim
	vrpn_FGSTARTREPLYLIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &start_reply_list;
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
		   "vrpn_FunctionGenerator_Remote::unregister_start_reply_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


int vrpn_FunctionGenerator_Remote::
register_stop_reply_handler( void *userdata,
							vrpn_FUNCTION_STOP_REPLY_HANDLER handler )
{
	if( handler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_stop_reply_handler:  "
				"NULL handler.\n" );
		return -1;
	}
	vrpn_FGSTOPREPLYLIST* newHandler = new vrpn_FGSTOPREPLYLIST;
	if( newHandler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_stop_reply_handler:  "
				"out of memory.\n" );
		return -1;
	}
	newHandler->userdata = userdata;
	newHandler->handler = handler;
	newHandler->next = stop_reply_list;
	stop_reply_list = newHandler;
	return 0;
}


int vrpn_FunctionGenerator_Remote::
unregister_stop_reply_handler( void *userdata,
							  vrpn_FUNCTION_STOP_REPLY_HANDLER handler )
{
	// The pointer at *snitch points to victim
	vrpn_FGSTOPREPLYLIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &stop_reply_list;
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
		   "vrpn_FunctionGenerator_Remote::unregister_stop_reply_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


int vrpn_FunctionGenerator_Remote::
register_sample_rate_reply_handler( void *userdata,
								   vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler )
{
	if( handler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_sample_rate_reply_handler:  "
				"NULL handler.\n" );
		return -1;
	}
	vrpn_FGSAMPLERATEREPLYLIST* newHandler = new vrpn_FGSAMPLERATEREPLYLIST;
	if( newHandler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_sample_rate_reply_handler:  "
				"out of memory.\n" );
		return -1;
	}
	newHandler->userdata = userdata;
	newHandler->handler = handler;
	newHandler->next = sample_rate_reply_list;
	sample_rate_reply_list = newHandler;
	return 0;
}


int vrpn_FunctionGenerator_Remote::
unregister_sample_rate_reply_handler( void *userdata,
									 vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler )
{
	// The pointer at *snitch points to victim
	vrpn_FGSAMPLERATEREPLYLIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &sample_rate_reply_list;
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
		   "vrpn_FunctionGenerator_Remote::unregister_sample_rate_reply_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


int vrpn_FunctionGenerator_Remote::
register_reference_channel_reply_handler( void *userdata,
										 vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_HANDLER handler )
{
	if( handler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_reference_channel_reply_handler:  "
				"NULL handler.\n" );
		return -1;
	}
	vrpn_FGREFERENCECHANNELREPLYLIST* newHandler = new vrpn_FGREFERENCECHANNELREPLYLIST;
	if( newHandler == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::register_reference_channel_reply_handler:  "
				"out of memory.\n" );
		return -1;
	}
	newHandler->userdata = userdata;
	newHandler->handler = handler;
	newHandler->next = reference_channel_reply_list;
	reference_channel_reply_list = newHandler;
	return 0;
}


int vrpn_FunctionGenerator_Remote::
unregister_reference_channel_reply_handler( void *userdata,
										   vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_HANDLER handler )
{
	// The pointer at *snitch points to victim
	vrpn_FGREFERENCECHANNELREPLYLIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &reference_channel_reply_list;
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
		   "vrpn_FunctionGenerator_Remote::unregister_reference_channel_reply_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_channelReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	vrpn_uint32 channelNum = vrpn_FUNCTION_CHANNELS_MAX + 1; // an invalid number
	if( 0 > me->decode_channelReply( p.buffer, p.payload_len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_channelReply_message:  "
				"unable to decode.\n" );
		return -1;
	}
	if( channelNum >= vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_channelReply_message:  "
				"invalid channel %d.\n", channelNum );
		return -1;
	}

	vrpn_FUNCTION_CHANNEL_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.channelNum = channelNum;
	cb.channel = me->channels[channelNum];

	vrpn_FGCHANNELREPLYLIST* handler = me->channel_reply_list;
	while( handler != NULL )
	{
		handler->handler( handler->userdata, cb );
		handler = handler->next;
	}
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_startReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	vrpn_bool isStarted = false;
	if( 0 > me->decode_start_reply( p.buffer, p.payload_len, isStarted ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_startReply_message:  "
				"unable to decode.\n" );
		return -1;
	}

	vrpn_FUNCTION_START_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.isStarted = isStarted;

	vrpn_FGSTARTREPLYLIST* handler = me->start_reply_list;
	while( handler != NULL )
	{
		handler->handler( handler->userdata, cb );
		handler = handler->next;
	}
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_stopReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	vrpn_bool isStopped = false;
	if( 0 > me->decode_stop_reply( p.buffer, p.payload_len, isStopped ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_stopReply_message:  "
				"unable to decode.\n" );
		return -1;
	}

	vrpn_FUNCTION_STOP_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.isStopped = isStopped;

	vrpn_FGSTOPREPLYLIST* handler = me->stop_reply_list;
	while( handler != NULL )
	{
		handler->handler( handler->userdata, cb );
		handler = handler->next;
	}
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_sampleRateReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	if( 0 > me->decode_sample_rate_reply( p.buffer, p.payload_len ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_sampleRateReply_message:  "
				"unable to decode.\n" );
		return -1;
	}

	vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.sampleRate = me->sampleRate;

	vrpn_FGSAMPLERATEREPLYLIST* handler = me->sample_rate_reply_list;
	while( handler != NULL )
	{
		handler->handler( handler->userdata, cb );
		handler = handler->next;
	}
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_referenceChannelReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	if( 0 > me->decode_reference_channel_reply( p.buffer, p.payload_len ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_referenceChannelReply_message:  "
				"unable to decode.\n" );
		return -1;
	}

	vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.referenceChannel = me->referenceChannel;

	vrpn_FGREFERENCECHANNELREPLYLIST* handler = me->reference_channel_reply_list;
	while( handler != NULL )
	{
		handler->handler( handler->userdata, cb );
		handler = handler->next;
	}
	return 0;
}





//
// end vrpn_FunctionGenerator_Remote
//  (except for encode and decode functions)
/////////////////////////////////////////////
/////////////////////////////////////////////



////////////////////////////////////////
////////////////////////////////////////
//
// encode and decode functions for 
// vrpn_FunctionGenerator_Server and
// vrpn_FunctionGenerator_Remote
//

vrpn_int32 vrpn_FunctionGenerator_Remote::
encode_channel( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum, 
				 const vrpn_FunctionGenerator_channel* channel )
{
	if( channelNum > vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"invalid channel nubmer %d.\n", channelNum );
		return -1;
	}
	if( len <= sizeof( vrpn_uint32 ) )
	{
		// the channel's encode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least encode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"couldn't buffer (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_int32) );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"message payload error (couldn't buffer channel number).\n" );
		return -1;
	}
	if( 0 > channels[channelNum]->encode_to( buf, len ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"message payload error (couldn't buffer channel).\n" );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_channel( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum,
				vrpn_FunctionGenerator_channel& channel )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		// the channel's decode_from function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least decode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_int32) );
		return -1;
	}
	const char* mybuf = buf;
	vrpn_int32 mylen = len;
	vrpn_uint32 myNum = 0;
	if( 0 > vrpn_unbuffer( &mybuf, &myNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel:  "
				"message payload error (couldn't unbuffer)\n" );
		return -1;
	}
	mylen -= sizeof( myNum );
	channelNum = myNum;
	if( 0 > channel.decode_from( &mybuf, mylen ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel:  "
				"error while decoding channel %d\n", channelNum );
		return -1;
	}

	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_channel_reply( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum )
{
	if( channelNum >= vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channelReply:  "
				"invalid channel\n" );
		return -1;
	}
	if( len <= sizeof( vrpn_uint32 ) )
	{
		// the channel's encode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least encode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channelReply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_uint32) );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channelReply:  "
				"unable to buffer channel number.\n" );
		return -1;
	}
	return channels[channelNum]->encode_to( buf, len );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_channelReply( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		// the channel's decode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least decode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_channelReply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_uint32) );
		return -1;
	}
	const char* mybuf = buf;
	vrpn_int32 mylen = len;
	vrpn_uint32 myNum = 0;
	if( 0 > vrpn_unbuffer( &mybuf, &myNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_channelReply:  "
				"unable to buffer channel number.\n" );
		return -1;
	}
	if( myNum >= vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_channelReply:  "
				"invalid channel:  %d\n", myNum );
		return -1;
	}
	channelNum = myNum;
	mylen -= sizeof( vrpn_uint32 );
	return channels[channelNum]->decode_from( &buf, mylen );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
encode_requestChannel( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_requestChannel:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_int32) );
		return -1;
	}
	vrpn_int32 mylen = len;
	if( 0 > vrpn_buffer( buf, &mylen, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_requestChannel:  "
				"unable to buffer channel", channelNum );
		return -1;
	}
	len = mylen;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_channel_request( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		// the channel's encode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least encode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel_request:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_int32) );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel_request:  "
				"unable to unbuffer channel", channelNum );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
encode_sampleRate( char** buf, vrpn_int32& len, const vrpn_float32 sampleRate )
{
	if( len <= sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_sampleRate:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	vrpn_int32 mylen = len;
	if( 0 > vrpn_buffer( buf, &mylen, sampleRate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_sampleRate:  "
				"unable to buffer sample rate" );
		return -1;
	}
	len = mylen;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_sample_rate( const char* buf, const vrpn_int32 len, vrpn_float32& sampleRate )
{
	if( len <= sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_sample_rate:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &sampleRate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_sample_rate:  "
				"unable to unbuffer sample rate" );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
encode_referenceChannel( char** buf, vrpn_int32& len, const vrpn_uint32 referenceChannel )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_referenceChannel:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	//vrpn_int32 mylen = len;
	if( 0 > vrpn_buffer( buf, &len, referenceChannel ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_reference_channel:  "
				"unable to unbuffer reference channel" );
		return -1;
	}
	//len = mylen;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_reference_channel( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_reference_channel:  "
				"channel message payload error (got %d, wanted at least %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_reference_channel:  "
				"unable to unbuffer reference channel" );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_start_reply( char** buf, vrpn_int32& len, const vrpn_bool isStarted )
{
	if( len <= sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_start_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_bool) );
		return -1;
	}
	return vrpn_buffer( buf, &len, isStarted );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_start_reply( const char* buf, const vrpn_int32 len, vrpn_bool& isStarted )
{
	if( len <= sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_start_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_bool) );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &isStarted ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_start_reply:  "
				"unable to unbuffer stop condition.\n" );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_stop_reply( char** buf, vrpn_int32& len, const vrpn_bool isStopped )
{
	if( len <= sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_stop_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_bool) );
		return -1;
	}
	return vrpn_buffer( buf, &len, isStopped );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_stop_reply( const char* buf, const vrpn_int32 len, vrpn_bool& isStopped )
{
	if( len <= sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_stop_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_bool) );
		return -1;
	}
	if( 0 > vrpn_unbuffer( &buf, &isStopped ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_stop_reply:  "
				"unable to unbuffer stop condition.\n" );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_sample_rate_reply( char** buf, vrpn_int32& len, const vrpn_float32 sampleRate )
{
	if( len <= sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_sample_rate_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	return vrpn_buffer( buf, &len, sampleRate );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_sample_rate_reply( const char* buf, const vrpn_int32 len )
{
	if( len <= sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_sample_rate_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	vrpn_float32 myRate = 0;
	if( 0 > vrpn_unbuffer( &buf, &myRate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_sample_rate_reply:  "
				"unable to unbuffer stop condition.\n" );
		return -1;
	}
	this->sampleRate = myRate;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_reference_channel_reply( char** buf, vrpn_int32& len, const vrpn_uint32 referenceChannel )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_reference_channel_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_int32) );
		return -1;
	}

	return vrpn_buffer( buf, &len, referenceChannel );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_reference_channel_reply( const char* buf, const vrpn_int32 len )
{
	if( len <= sizeof( vrpn_uint32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_reference_channel_reply:  "
				"insufficient buffer space given (got %d, wanted %d).\n", 
				len, sizeof( vrpn_float32) );
		return -1;
	}
	vrpn_uint32 myRef = 0;
	if( 0 > vrpn_unbuffer( &buf, &myRef ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_reference_channel_reply:  "
				"unable to unbuffer stop condition.\n" );
		return -1;
	}
	this->referenceChannel = myRef;
	return 0;
}


//
// end encode & decode functions for
// vrpn_FunctionGenerator_Remote and
// vrpn_FunctionGenerator_Server
///////////////////////////////////////

