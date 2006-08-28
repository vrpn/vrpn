#ifndef VRPN_FUNCTIONGENERATOR_H
#define VRPN_FUNCTIONGENERATOR_H

#include "vrpn_Shared.h"
#include "vrpn_Analog.h" // for vrpn_CHANNEL_MAX


const vrpn_uint32 vrpn_FUNCTION_CHANNELS_MAX = vrpn_CHANNEL_MAX;

extern const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REQUEST;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_ALL_CHANNEL_REQUEST;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_START;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_STOP;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_REFERENCE_CHANNEL;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REPLY;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_START_REPLY;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_STOP_REPLY;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE_REPLY;
extern const char* vrpn_FUNCTION_MESSAGE_TYPE_REFERENCE_CHANNEL_REPLY;


enum vrpn_FunctionGenerator_FunctionTypes 
{ 
	NULL_FUNCTION = 0,
	SINE_FUNCTION,
	RAMP_FUNCTION,
	DEGAUSS_FUNCTION,
	
	CUSTOM_FUNCTION = 999 
};


enum vrpn_FunctionGenerator_RepeatStyle
{
	REPEAT_NUMBER,		// repeat a specific number of repititions
	REPEAT_TO_TIME,		// repeat for a specific amount of time
	REPEAT_MANUAL		// repeat until told to stop
};

class VRPN_API vrpn_FunctionGenerator_channel;

// a base class for all functions that vrpn_FunctionGenerator
// can generate
class VRPN_API vrpn_FunctionGenerator_function
{
public:
	virtual vrpn_FunctionGenerator_FunctionTypes getFunctionType( ) const = 0;

	// concrete classes should implement this to generate the appropriate
	// values for the function the class represents.  nValue samples should be
	// generated beginning at time startTime, and these samples should be placed
	// in the provided buffer.  several data members of 'channel' can modify the
	// times for which values are generated.
	// returns the time of the last sample generated.
	virtual vrpn_float32 generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
										 vrpn_float32 startTime, vrpn_float32 sampleRate,
										 vrpn_FunctionGenerator_channel* channel ) const = 0;

	// concrete classes should implement this function to return the time, 
	// in seconds, in which the function is written to cycle.
	virtual vrpn_float32 getCycleTime( ) const = 0;

	// concrete classes should implement this to encode their 
	// function information into the specified buffer 'buf'.  The
	// remaining length in the buffer is stored in 'len'.  At return,
	// 'len' should be set to the number of characters remaining in the
	// buffer and the number of characters written should be returned,
	// save in case of failure, when negative should be returned.
	virtual vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const = 0;

	// concrete classes should implement this to decode their
	// function information from the specified buffer.  The remaining
	// length in the buffer is stored in 'len'.  At return, 'len' should
	// be set to the number of characters remaining in the the buffer 
	// and the number of characters read should be returned, save in case
	// of failure, when negative should be returned
	virtual vrpn_int32 decode_from( const char** buf, vrpn_int32& len ) = 0;

protected:
	vrpn_float32 lerp( vrpn_float32 t, vrpn_float32 t0, vrpn_float32 t1,
						vrpn_float32 v0, vrpn_float32 v1 ) const
	{  return v0 + ( t - t0 ) * ( v1 - v0 ) / ( t1 - t0 );  }

};


// the NULL function:  generate all zeros
class VRPN_API vrpn_FunctionGenerator_function_NULL
: public virtual vrpn_FunctionGenerator_function
{
public:
	vrpn_FunctionGenerator_function_NULL( ) { }
	virtual ~vrpn_FunctionGenerator_function_NULL( ) { }

	vrpn_FunctionGenerator_FunctionTypes getFunctionType( ) const
	{  return NULL_FUNCTION;  }

	vrpn_float32 generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
								vrpn_float32 startTime, vrpn_float32 sampleRate, 
								vrpn_FunctionGenerator_channel* channel ) const;
	vrpn_float32 getCycleTime( ) const;
	vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const;
	vrpn_int32 decode_from( const char** buf, vrpn_int32& len );
};


class VRPN_API vrpn_FunctionGenerator_function_sine 
: public vrpn_FunctionGenerator_function
{
public:
	vrpn_FunctionGenerator_function_sine( )
	{  amplitude = frequency = 1;  }

	virtual ~vrpn_FunctionGenerator_function_sine( ) { }

	vrpn_FunctionGenerator_FunctionTypes getFunctionType( ) const
	{  return SINE_FUNCTION;  }

	vrpn_float32 generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
								vrpn_float32 startTime, vrpn_float32 sampleRate, 
								vrpn_FunctionGenerator_channel* channel ) const;
	vrpn_float32 getCycleTime( ) const;
	vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const;
	vrpn_int32 decode_from( const char** buf, vrpn_int32& len );

	vrpn_float32 getAmplitude( ) { return amplitude; }
	bool setAmplitude( vrpn_float32 newVal ) { amplitude = newVal;  return true; }

	vrpn_float32 getFrequency( ) { return frequency; }
	bool setFrequency( vrpn_float32 newVal ) { frequency = newVal;  return true; }

protected:	
	vrpn_float32 amplitude;
	vrpn_float32 frequency;

};



/*
	A generalized ramp ,square-wave and saw-tooth function

      /|-------|\                     <-- up voltage
     /           \
---|/             \|----|\      /|    <-- zero voltage
                          \|__|/      <-- down voltage
 t0| t1|   t2  | t3| t4 |t5|t6|t7|
*/
class VRPN_API vrpn_FunctionGenerator_function_ramp
: public vrpn_FunctionGenerator_function
{
public:
	vrpn_FunctionGenerator_function_ramp( );

	virtual ~vrpn_FunctionGenerator_function_ramp( ) { }

	vrpn_FunctionGenerator_FunctionTypes getFunctionType( ) const
	{  return RAMP_FUNCTION;  }

	vrpn_float32 generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
								vrpn_float32 startTime, vrpn_float32 sampleRate, 
								vrpn_FunctionGenerator_channel* channel ) const;
	vrpn_float32 getCycleTime( ) const;
	vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const;
	vrpn_int32 decode_from( const char** buf, vrpn_int32& len );

	vrpn_float32 getUpVoltage( ) { return upVoltage; }
	bool setUpVoltage( vrpn_float32 newVal ) { upVoltage = newVal;  return true; }

	vrpn_float32 getDownVoltage( ) { return downVoltage; }
	bool setDownVoltage( vrpn_float32 newVal ) { downVoltage = newVal;  return true; }

	vrpn_float32 getInitialFlat( ) { return initialFlat; }
	bool setInitialFlat( vrpn_float32 newVal ) { initialFlat = newVal;  return true; }

	vrpn_float32 getRampHighUp( ) { return rampHighUp; }
	bool setRampHighUp( vrpn_float32 newVal ) { rampHighUp = newVal;  return true; }

	vrpn_float32 getRampHighDwell( ) { return rampHighDwell; }
	bool setRampHighDwell( vrpn_float32 newVal ) { rampHighDwell = newVal;  return true; }

	vrpn_float32 getRampHighDown( ) { return rampHighDown; }
	bool setRampHighDown( vrpn_float32 newVal ) { rampHighDown = newVal;  return true; }

	vrpn_float32 getMidFlat( ) { return midFlat; }
	bool setMidFlat( vrpn_float32 newVal ) { midFlat = newVal;  return true; }

	vrpn_float32 getRampLowDown( ) { return rampLowDown; }
	bool setRampLowDown( vrpn_float32 newVal ) { rampLowDown = newVal;  return true; }

	vrpn_float32 getRampLowDwell( ) { return rampLowDwell; }
	bool setRampLowDwell( vrpn_float32 newVal ) { rampLowDwell = newVal;  return true; }

	vrpn_float32 getRampLowUp( ) { return rampLowUp; }
	bool setRampLowUp( vrpn_float32 newVal ) { rampLowUp = newVal;  return true; }

protected:
	vrpn_float32 upVoltage;
	vrpn_float32 downVoltage;

	vrpn_float32 initialFlat;	// t0:  time output held at zero until up ramp begins
	vrpn_float32 rampHighUp;	// t1:  time output ramps from zero to upVoltage
	vrpn_float32 rampHighDwell;	// t2:  time output held at upVoltage
	vrpn_float32 rampHighDown;	// t3:  time output ramps from upVoltage to zero
	vrpn_float32 midFlat;		// t4:  time output held at zero between up and down ramps
	vrpn_float32 rampLowDown;	// t5:  time output ramps from zero to downVoltage
	vrpn_float32 rampLowDwell;	// t6:  time output held at downVoltage
	vrpn_float32 rampLowUp;		// t7:  time output ramps from downVoltage to zero

};


class VRPN_API vrpn_FunctionGenerator_function_degauss
: public vrpn_FunctionGenerator_function
{
public:
	vrpn_FunctionGenerator_function_degauss( );
	virtual ~vrpn_FunctionGenerator_function_degauss( ) { }

	vrpn_FunctionGenerator_FunctionTypes getFunctionType( ) const
	{  return DEGAUSS_FUNCTION;  }

	vrpn_float32 generateValues( vrpn_float32* buf, vrpn_uint32 nValues,
								vrpn_float32 startTime, vrpn_float32 sampleRate, 
								vrpn_FunctionGenerator_channel* channel ) const;
	vrpn_float32 getCycleTime( ) const { return cycleTime; }
	vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const;
	vrpn_int32 decode_from( const char** buf, vrpn_int32& len );

	vrpn_float32 getInitialValue( ) { return initialValue; }
	bool setInitialValue( vrpn_float32 newVal ) 
	{
		if( initialValue <= 0 ) return false;
		initialValue = newVal;
		calculateCycleTime();
		return true;
	}

	vrpn_float32 getFinalValue( ) { return finalValue; }
	bool setFinalValue( vrpn_float32 newVal ) 
	{
		if( finalValue <= 0 ) return false;
		finalValue = newVal;
		calculateCycleTime();
		return true;
	}

	vrpn_float32 getFrequency( ) { return frequency; }
	bool setFrequency( vrpn_float32 newVal ) 
	{ 
		if( frequency <= 0 ) return false;
		frequency = newVal; 
		calculateCycleTime();
		return true;
	}

	vrpn_float32 getDecayRate( ) { return decay; }
	bool setDecayRate( vrpn_float32 newVal ) 
	{ 
		if( newVal <= 0 || newVal >= 1 ) return false;
		decay = newVal;
		calculateCycleTime();
		return true;
	}

protected:
	vrpn_float32 initialValue;
	vrpn_float32 finalValue;
	vrpn_float32 frequency;
	vrpn_float32 decay; // 0 < decay < 1.  fraction each cycle decays in amplitude.
	vrpn_float32 cycleTime;
	void calculateCycleTime( );
};


class VRPN_API vrpn_FunctionGenerator_function_custom
: public vrpn_FunctionGenerator_function
{
public:
	vrpn_FunctionGenerator_function_custom( vrpn_uint32 length, vrpn_float32* times, vrpn_float32* values );
	virtual ~vrpn_FunctionGenerator_function_custom( );

	vrpn_FunctionGenerator_FunctionTypes getFunctionType( ) const
	{  return CUSTOM_FUNCTION;  }

	vrpn_float32 generateValues(vrpn_float32* buf, vrpn_uint32 nValues,
				    vrpn_float32 startTime, vrpn_float32 sampleRate, 
				    vrpn_FunctionGenerator_channel* channel ) const;
	vrpn_float32 getCycleTime( ) const;
	vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const;
	vrpn_int32 decode_from( const char** buf, vrpn_int32& len );

	vrpn_uint32 getLength( ) {  return d_length;  }
	const vrpn_float32* getTimes( ) {  return d_times;  }
	const vrpn_float32* getValues( ) {  return d_values;  }

	// the function will make its own copy of the times and values
	bool set( vrpn_uint32 length, vrpn_float32* times, vrpn_float32* values );
protected:
	vrpn_float32* d_times;  // assumed to always increase
	vrpn_float32* d_values;
	vrpn_uint32 d_length;

};


class VRPN_API vrpn_FunctionGenerator_channel
{
	// note:  the channel will delete its function when the function is
	// no longer needed (e.g., when the channel is destroyed or the function changed)
public:
	vrpn_FunctionGenerator_channel( );
	vrpn_FunctionGenerator_channel( vrpn_FunctionGenerator_function* function );
	virtual ~vrpn_FunctionGenerator_channel( );

	const vrpn_FunctionGenerator_function* getFunction( )  { return function; }
	void setFunction( vrpn_FunctionGenerator_function* function );

	// should this channel wait for a start trigger, or should the server immediately
	// start generating a function on this channel.  true -> wait for a start message.
	vrpn_bool triggered;

	// phase shift between this channel and reference channel.
	vrpn_float32 phaseFromRef;

	vrpn_float32 gain;

	vrpn_float32 offset;

	vrpn_float32 scaleTime;

	// a function 'f(u)' will generate value 'v' at time 't' as:
	//    v = offset + gain * f( scaleTime * t + ( triggered ? 0 : phaseFromRef ) )

	vrpn_FunctionGenerator_RepeatStyle repeat;
	vrpn_float32 repeatNum;  // the number of cycles to repeat, if that's what we're doing
	vrpn_float32 repeatTime;  // the time to repeat, if that's what we're doing
	
	// these return zero on success and negative on some failure.
	// the remaining length 
	vrpn_int32 encode_to( char** buf, vrpn_int32& len ) const;
	vrpn_int32 decode_from( const char** buf, vrpn_int32& len );

protected:
	vrpn_FunctionGenerator_function* function;
	
};


class VRPN_API vrpn_FunctionGenerator : public vrpn_BaseClass
{
public:
	vrpn_FunctionGenerator( const char* name, vrpn_Connection* c = NULL );
	virtual ~vrpn_FunctionGenerator( );

	// returns the requested channel, or null if channelNum is 
	// greater than the maximum number of channels.
	const vrpn_FunctionGenerator_channel* const getChannel( vrpn_uint32 channelNum );

	vrpn_float32 getSampleRate( )
	{  return sampleRate;  }

	// returns the channel number of the reference channel
	vrpn_uint32 getReferenceChannel( )
	{  return referenceChannel;  }

protected:
	vrpn_float32 sampleRate;  // samples per second
	vrpn_uint32 referenceChannel;  // index indicating which channel is reference for phase calc'ns
	vrpn_FunctionGenerator_channel* channels[vrpn_FUNCTION_CHANNELS_MAX];

	vrpn_int32 channelMessageID;             // id for channel message (remote -> server)
	vrpn_int32 requestChannelMessageID;	     // id for messages requesting channel info be sent (remote -> server)
	vrpn_int32 requestAllChannelsMessageID;  // id for messages requesting channel info of all channels be sent (remote -> server)
	vrpn_int32 sampleRateMessageID;		     // id for message to request a sampling rate (remote -> server)
	vrpn_int32 startFunctionMessageID;       // id for message to start generating the function (remote -> server)
	vrpn_int32 stopFunctionMessageID;        // id for message to stop generating the function (remote -> server)
	vrpn_int32 referenceChannelMessagID;     // id for message to set reference channel (remote -> server)

	vrpn_int32 channelReplyMessageID;        // id for reply for channel message (server -> remote)
	vrpn_int32 startFunctionReplyMessageID;  // id for reply to start-function message (server -> remote)
	vrpn_int32 stopFunctionReplyMessageID;   // id for reply to stop-function message (server -> remote)
	vrpn_int32 sampleRateReplyMessageID;     // id for reply to request-sample-rate message (server -> remote)
	vrpn_int32 refChannelReplyMessageID;	 // id for reply to request-reference-channel message (server -> remote)

	vrpn_int32	gotConnectionMessageID;  // for new-connection message

	virtual int register_types( );


	char msgbuf[vrpn_CONNECTION_TCP_BUFLEN];
	struct timeval timestamp;


}; // end class vrpn_FunctionGenerator


class VRPN_API vrpn_FunctionGenerator_Server : public vrpn_FunctionGenerator
{
public:
	vrpn_FunctionGenerator_Server( const char* name, vrpn_Connection* c = NULL );
	virtual ~vrpn_FunctionGenerator_Server( );

	virtual void mainloop( );

	// sub-classes should implement these functions.  they will be called when messages 
	// are received for the particular request.  at the end of these functions, servers 
	// should call the appropriate send*Reply function, even if the requested change 
	// was rejected.
	virtual void setChannel( vrpn_uint32 channelNum, vrpn_FunctionGenerator_channel* channel ) = 0;
	virtual void start( ) = 0;
	virtual void stop( ) = 0;
	virtual void setReferenceChannel( vrpn_uint32 channelNum ) = 0;
	virtual void setSampleRate( vrpn_float32 rate ) = 0;

	// sub-classes should not override these methods; these take care of
	// receiving requests
	static int VRPN_CALLBACK handle_channel_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_channelRequest_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_allChannelRequest_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_start_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_stop_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_sample_rate_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_reference_channel_message( void* userdata, vrpn_HANDLERPARAM p );

protected:
	
	// sub-classes should call these functions to inform the remote side of
	// changes (or of non-changes, when a requested change cannot be accepted).
	// returns 0 on success and negative on failure.
	int sendChannelReply( vrpn_uint32 channelNum );
	int sendSampleRateReply( );
	int sendReferenceChannelReply( );
	int sendStartReply( vrpn_bool started );
	int sendStopReply( vrpn_bool stopped );

	vrpn_int32 decode_channel( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum,
								vrpn_FunctionGenerator_channel& channel );
	vrpn_int32 decode_channel_request( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum );
	vrpn_int32 decode_sample_rate( const char* buf, const vrpn_int32 len, vrpn_float32& sampleRate );
	vrpn_int32 decode_reference_channel( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum );

	vrpn_int32 encode_channel_reply( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum );
	vrpn_int32 encode_start_reply( char** buf, vrpn_int32& len, const vrpn_bool isStarted );
	vrpn_int32 encode_stop_reply( char** buf, vrpn_int32& len, const vrpn_bool isStopped );
	vrpn_int32 encode_sample_rate_reply( char** buf, vrpn_int32& len, const vrpn_float32 sampleRate );
	vrpn_int32 encode_reference_channel_reply( char** buf, vrpn_int32& len, const vrpn_uint32 referenceChannel );


}; // end class vrpn_FunctionGenerator_Server





//----------------------------------------------------------
// ************** Users deal with the following *************

// User routine to handle function-generator channel replies.  This
// is called when the function-generator server replies with new
// setting for some channel.
typedef	struct _vrpn_FUNCTION_CHANNEL_REPLY_CB
{
	struct timeval	msg_time;	// Time of the report
	vrpn_uint32	channelNum;		// Which channel is being reported
	vrpn_FunctionGenerator_channel*	channel;
} vrpn_FUNCTION_CHANNEL_REPLY_CB;
typedef void (*vrpn_FUNCTION_CHANGE_REPLY_HANDLER)( void *userdata,
					  const vrpn_FUNCTION_CHANNEL_REPLY_CB info );


// User routine to handle function-generator start replies.  This
// is called when the function-generator server reports that it
// has started generating functions.
typedef	struct _vrpn_FUNCTION_START_REPLY_CB
{
	struct timeval	msg_time;	// Time of the report
	vrpn_bool isStarted;		// did the function generation start?
} vrpn_FUNCTION_START_REPLY_CB;
typedef void (*vrpn_FUNCTION_START_REPLY_HANDLER)( void *userdata,
					     const vrpn_FUNCTION_START_REPLY_CB info );

// User routine to handle function-generator stop replies.  This
// is called when the function-generator server reports that it
// has stopped generating functions.
typedef	struct _vrpn_FUNCTION_STOP_REPLY_CB
{
	struct timeval	msg_time;	// Time of the report
	vrpn_bool isStopped;		// did the function generation stop?
} vrpn_FUNCTION_STOP_REPLY_CB;
typedef void (*vrpn_FUNCTION_STOP_REPLY_HANDLER)( void *userdata,
					     const vrpn_FUNCTION_STOP_REPLY_CB info );

// User routine to handle function-generator sample-rate replies.  
// This is called when the function-generator server reports that 
// the function-generation sample rate has changed.
typedef	struct _vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB
{
	struct timeval	msg_time;	// Time of the report
	vrpn_float32 sampleRate;		
} vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB;
typedef void (*vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER)( void *userdata,
					     const vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB info );

// User routine to handle function-generator reference-channel replies.  
// This is called when the function-generator server reports that the
// reference channel has changed.
typedef	struct _vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_CB
{
	struct timeval	msg_time;	// Time of the report
	vrpn_uint32 referenceChannel;
} vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_CB;
typedef void (*vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_HANDLER)( void *userdata,
					     const vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_CB info );


class VRPN_API vrpn_FunctionGenerator_Remote : public vrpn_FunctionGenerator
{
public:
	vrpn_FunctionGenerator_Remote( const char* name, vrpn_Connection* c = NULL );
	virtual ~vrpn_FunctionGenerator_Remote( );

	int setChannel( const vrpn_uint32 channelNum, const vrpn_FunctionGenerator_channel* channel );
	int requestChannel( const vrpn_uint32 channelNum );
	int requestAllChannels( );
	int requestStart( );
	int requestStop( );
	int requestSampleRate( const vrpn_float32 rate );
	int requestSetReferenceChannel( const vrpn_uint32 channelNum );

	virtual void mainloop( );
	
	// (un)Register a callback handler to handle a channel reply
	virtual int register_channel_reply_handler( void *userdata,
		vrpn_FUNCTION_CHANGE_REPLY_HANDLER handler );
	virtual int unregister_channel_reply_handler( void *userdata,
		vrpn_FUNCTION_CHANGE_REPLY_HANDLER handler );
	
	// (un)Register a callback handler to handle a start reply
	virtual int register_start_reply_handler( void *userdata,
		vrpn_FUNCTION_START_REPLY_HANDLER handler );
	virtual int unregister_start_reply_handler( void *userdata,
		vrpn_FUNCTION_START_REPLY_HANDLER handler );
	
	// (un)Register a callback handler to handle a stop reply
	virtual int register_stop_reply_handler( void *userdata,
		vrpn_FUNCTION_STOP_REPLY_HANDLER handler );
	virtual int unregister_stop_reply_handler( void *userdata,
		vrpn_FUNCTION_STOP_REPLY_HANDLER handler );
	
	// (un)Register a callback handler to handle a sample-rate reply
	virtual int register_sample_rate_reply_handler( void *userdata,
		vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler );
	virtual int unregister_sample_rate_reply_handler( void *userdata,
		vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler );
	
	// (un)Register a callback handler to handle a reference-channel reply
	virtual int register_reference_channel_reply_handler( void *userdata,
		vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_HANDLER handler );
	virtual int unregister_reference_channel_reply_handler( void *userdata,
		vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_HANDLER handler );
	
	static int VRPN_CALLBACK handle_channelReply_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK VRPN_CALLBACK handle_startReply_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_stopReply_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_sampleRateReply_message( void* userdata, vrpn_HANDLERPARAM p );
	static int VRPN_CALLBACK handle_referenceChannelReply_message( void* userdata, vrpn_HANDLERPARAM p );

protected:
	typedef	struct vrpn_FGCRL
	{
		void* userdata;
		vrpn_FUNCTION_CHANGE_REPLY_HANDLER handler;
		struct vrpn_FGCRL* next;
	} vrpn_FGCHANNELREPLYLIST;
	vrpn_FGCHANNELREPLYLIST* channel_reply_list;

	
	typedef	struct vrpn_FGStartRL
	{
		void* userdata;
		vrpn_FUNCTION_START_REPLY_HANDLER handler;
		struct vrpn_FGStartRL* next;
	} vrpn_FGSTARTREPLYLIST;
	vrpn_FGSTARTREPLYLIST* start_reply_list;

	
	typedef	struct vrpn_FGStopRL
	{
		void* userdata;
		vrpn_FUNCTION_STOP_REPLY_HANDLER handler;
		struct vrpn_FGStopRL* next;
	} vrpn_FGSTOPREPLYLIST;
	vrpn_FGSTOPREPLYLIST* stop_reply_list;


	typedef	struct vrpn_FGSRRL
	{
		void* userdata;
		vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler;
		struct vrpn_FGSRRL* next;
	} vrpn_FGSAMPLERATEREPLYLIST;
	vrpn_FGSAMPLERATEREPLYLIST* sample_rate_reply_list;


	typedef	struct vrpn_FGRCRL
	{
		void* userdata;
		vrpn_FUNCTION_REFERENCE_CHANNEL_REPLY_HANDLER handler;
		struct vrpn_FGRCRL* next;
	} vrpn_FGREFERENCECHANNELREPLYLIST;
	vrpn_FGREFERENCECHANNELREPLYLIST* reference_channel_reply_list;


	vrpn_int32 decode_channelReply( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum );
	vrpn_int32 decode_start_reply( const char* buf, const vrpn_int32 len, vrpn_bool& isStarted );
	vrpn_int32 decode_stop_reply( const char* buf, const vrpn_int32 len, vrpn_bool& isStopped );
	vrpn_int32 decode_sample_rate_reply( const char* buf, const vrpn_int32 len );
	vrpn_int32 decode_reference_channel_reply( const char* buf, const vrpn_int32 len );

	vrpn_int32 encode_channel( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum, 
								 const vrpn_FunctionGenerator_channel* channel );
	vrpn_int32 encode_requestChannel( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum );
	vrpn_int32 encode_sampleRate( char** buf, vrpn_int32& len, const vrpn_float32 sampleRate );
	vrpn_int32 encode_referenceChannel( char** buf, vrpn_int32& len, const vrpn_uint32 referenceChannel );

}; // end class vrpn_FunctionGenerator_Remote


#endif // VRPN_FUNCTIONGENERATOR_H
