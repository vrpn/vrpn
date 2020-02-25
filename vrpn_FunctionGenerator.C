
#include <stdio.h>                      // for fflush, fprintf, stderr
#include <string.h>                     // for NULL, strlen, strcpy

#include "vrpn_FunctionGenerator.h"

//#define DEBUG_VRPN_FUNCTION_GENERATOR

const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL = "vrpn_FunctionGenerator channel";
const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REQUEST = "vrpn_FunctionGenerator channel request";
const char* vrpn_FUNCTION_MESSAGE_TYPE_ALL_CHANNEL_REQUEST = "vrpn_FunctionGenerator all channel request";
const char* vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE = "vrpn_FunctionGenerator sample rate";
const char* vrpn_FUNCTION_MESSAGE_TYPE_START = "vrpn_FunctionGenerator start";
const char* vrpn_FUNCTION_MESSAGE_TYPE_STOP = "vrpn_FunctionGenerator stop";
const char* vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REPLY = "vrpn_FunctionGenerator channel reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_START_REPLY = "vrpn_FunctionGenerator start reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_STOP_REPLY = "vrpn_FunctionGenerator stop reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE_REPLY = "vrpn_FunctionGenerator sample rate reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_INTERPRETER_REQUEST = "vrpn_FunctionGenerator interpreter-description request";
const char* vrpn_FUNCTION_MESSAGE_TYPE_INTERPRETER_REPLY = "vrpn_FunctionGenerator interpreter-description reply";
const char* vrpn_FUNCTION_MESSAGE_TYPE_ERROR = "vrpn_FunctionGenerator error report";

vrpn_FunctionGenerator_function::~vrpn_FunctionGenerator_function() {}

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

vrpn_FunctionGenerator_function* vrpn_FunctionGenerator_function_NULL::
clone( ) const
{
  vrpn_FunctionGenerator_function *ret;
  try { ret = new vrpn_FunctionGenerator_function_NULL(); }
  catch (...) { return NULL; }
  return ret;
}


//
// end vrpn_FunctionGenerator_function_NULL
////////////////////////////////////////
////////////////////////////////////////


////////////////////////////////////////
////////////////////////////////////////
//
// class vrpn_FunctionGenerator_function_script
vrpn_FunctionGenerator_function_script::
vrpn_FunctionGenerator_function_script( )
: script( NULL )
{
  try {
    this->script = new char[1];
    script[0] = '\0';
  } catch (...) {}
}


vrpn_FunctionGenerator_function_script::
vrpn_FunctionGenerator_function_script( const char* script )
  : script( NULL )
{
  try {
    this->script = new char[strlen(script) + 1];
    strcpy(this->script, script);
  } catch (...) {}
}


vrpn_FunctionGenerator_function_script::
vrpn_FunctionGenerator_function_script( const vrpn_FunctionGenerator_function_script& s )
  : script(NULL)
{
  try {
    this->script = new char[strlen(s.script) + 1];
    strcpy(this->script, s.script);
  } catch (...) {}
}


vrpn_FunctionGenerator_function_script::~vrpn_FunctionGenerator_function_script( )
{
	if( script != NULL ) {
          try {
            delete[] script;
          } catch (...) {
            fprintf(stderr, "vrpn_FunctionGenerator_function_script::~vrpn_FunctionGenerator_function_script(): delete failed\n");
            return;
          }
	  script = NULL;
	}
}

vrpn_float32 vrpn_FunctionGenerator_function_script::
generateValues( vrpn_float32* buf, vrpn_uint32 nValues, vrpn_float32 startTime, 
			    vrpn_float32 sampleRate, vrpn_FunctionGenerator_channel* /*channel*/ ) const
{
	for( vrpn_uint32 i = 0; i <= nValues - 1; i++ )
	{
		buf[i] = 0;
	}
	return startTime + nValues / sampleRate;
};


vrpn_int32 vrpn_FunctionGenerator_function_script::
encode_to( char** buf, vrpn_int32& len ) const
{
	vrpn_uint32 length = static_cast<vrpn_uint32>(strlen( this->script ));
	vrpn_int32 bytes = length + sizeof( vrpn_uint32 );
	if( len < bytes )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_script::encode_to:  "
				"payload error (wanted %d got %d).\n", bytes, len );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, length ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_script::encode_to:  "
				"payload error (couldn't buffer length).\n" );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, this->script, length ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_script::encode_to:  "
				"payload error (couldn't buffer script).\n" );
		fflush( stderr );
		return -1;
	}
	return bytes;
}


vrpn_int32 vrpn_FunctionGenerator_function_script::
decode_from( const char** buf, vrpn_int32& len )
{
	vrpn_int32 newlen;
	if( 0 > vrpn_unbuffer( buf, &newlen ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_script::decode_from:  "
				"payload error (couldn't unbuffer length).\n" );
		fflush( stderr );
		return -1;
	}
	len -= sizeof( vrpn_uint32);

	if( len < newlen )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_script::decode_from:  "
				"payload error (wanted %d got %d).\n", newlen, len );
		fflush( stderr );
		return -1;
	}

        char* newscript = NULL;
        try { newscript = new char[newlen + 1]; }
        catch (...) {
          fprintf(stderr, "vrpn_FunctionGenerator_function_script:: "
            "Out of memory.\n");
          fflush(stderr);
          return -1;
        }
	if( 0 > vrpn_unbuffer( buf, newscript, newlen ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_function_script::decode_from:  "
				"payload error (couldn't unbuffer).\n" );
                try {
                  delete[] newscript;
                } catch (...) {
                  fprintf(stderr, "vrpn_FunctionGenerator_function_script::decode_from(): delete failed\n");
                  return -1;
                }
		fflush( stderr );
		return -1;
	}
	newscript[newlen] = '\0';
        if (this->script != NULL) {
                try {
                  delete[] this->script;
                } catch (...) {
                  fprintf(stderr, "vrpn_FunctionGenerator_function_script::decode_from(): delete failed\n");
                  return -1;
                }
        }
	this->script = newscript;
	len -= newlen;
	return newlen + sizeof( vrpn_uint32 );
}


vrpn_FunctionGenerator_function* vrpn_FunctionGenerator_function_script::
clone( ) const
{
	return new vrpn_FunctionGenerator_function_script( *this );
}

char* vrpn_FunctionGenerator_function_script::
getScript( ) const
{
  char* retval = NULL;
  try {
    retval = new char[strlen(this->script) + 1];
  } catch (...) {
    return NULL;
  }
  if (this->script) { strcpy(retval, this->script); }
  return retval;
}


vrpn_bool vrpn_FunctionGenerator_function_script::setScript( char* script )
{
	if( script == NULL ) return false;
        if (this->script != NULL) {
            try {
              delete[] this->script;
            } catch (...) {
              fprintf(stderr, "vrpn_FunctionGenerator_function_script::setScript(): delete failed\n");
              return false;
            }
        }
        try {
          this->script = new char[strlen(script) + 1];
          strcpy(this->script, script);
        } catch (...) { return false; }
	return true;
}

//
// end vrpn_FunctionGenerator_function_script
////////////////////////////////////////
////////////////////////////////////////


/////////////////////////////////////////
/////////////////////////////////////////
// 
// class vrpn_FunctionGenerator_channel

vrpn_FunctionGenerator_channel::
vrpn_FunctionGenerator_channel( )
{
  try { function = new vrpn_FunctionGenerator_function_NULL; }
  catch (...) { function = NULL; }
}


vrpn_FunctionGenerator_channel::
vrpn_FunctionGenerator_channel( vrpn_FunctionGenerator_function* function )
{
	this->function = function->clone();
}


vrpn_FunctionGenerator_channel::~vrpn_FunctionGenerator_channel( )
{
    try {
      delete function;
    } catch (...) {
      fprintf(stderr, "vrpn_FunctionGenerator_channel::~vrpn_FunctionGenerator_channel(): delete failed\n");
      return;
    }
}


void vrpn_FunctionGenerator_channel::setFunction( vrpn_FunctionGenerator_function* function )
{
    try {
      delete (this->function);
    } catch (...) {
      fprintf(stderr, "vrpn_FunctionGenerator_channel::setFunction(): delete failed\n");
      return;
    }
    this->function = function->clone();
}


vrpn_int32 vrpn_FunctionGenerator_channel::
encode_to( char** buf, vrpn_int32& len ) const
{
	if( static_cast<unsigned>(len) < sizeof( vrpn_FunctionGenerator_function::FunctionCode ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_channel::encode_to:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_FunctionGenerator_function::FunctionCode )) );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, (int) this->function->getFunctionCode() ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_channel::encode_to:  "
				"unable to buffer function type.\n" );
		fflush( stderr );
		return -1;
	}
	return function->encode_to( buf, len );
}


vrpn_int32 vrpn_FunctionGenerator_channel::
decode_from( const char** buf, vrpn_int32& len )
{
	if( static_cast<unsigned>(len) < sizeof( vrpn_FunctionGenerator_function::FunctionCode ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_channel::decode_from:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_FunctionGenerator_function::FunctionCode )) );
		fflush( stderr );
		return -1;
	}
	int myCode;
	if( 0 > vrpn_unbuffer( buf, &myCode ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_channel::decode_from:  "
				"unable to unbuffer function type.\n" );
		fflush( stderr );
		return -1;
	}
	// if we don't have the right function type, create a new
	// one of the appropriate type and delete the old one
	if( myCode != function->getFunctionCode() )
	{
	   vrpn_FunctionGenerator_function* oldFunc = this->function;
	   vrpn_FunctionGenerator_function::FunctionCode newCode 
			= vrpn_FunctionGenerator_function::FunctionCode( myCode );
           try {
		switch( newCode )
		{
		case vrpn_FunctionGenerator_function::FUNCTION_NULL:
			this->function = new vrpn_FunctionGenerator_function_NULL;
			break;
		case vrpn_FunctionGenerator_function::FUNCTION_SCRIPT:
			this->function = new vrpn_FunctionGenerator_function_script;
			break;
		default:
			fprintf( stderr, "vrpn_FunctionGenerator_channel::decode_from:  "
					"unknown function type.\n" );
			fflush( stderr );
			return -1;
		}
            } catch (...) {
              fprintf(stderr, "vrpn_FunctionGenerator_channel::decode_from:  "
                "Out of memory.\n");
              fflush(stderr);
              return -1;
            }
            try {
              delete oldFunc;
            } catch (...) {
              fprintf(stderr, "vrpn_FunctionGenerator_channel::decode_from(): delete failed\n");
              return -1;
            }
	}
	return this->function->decode_from( buf, len );
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
: vrpn_BaseClass( name, c ),
	sampleRate( 0 ),
	numChannels( 0 )
{
	vrpn_BaseClass::init( );

	unsigned i;
	for( i = 0; i <= vrpn_FUNCTION_CHANNELS_MAX - 1; i++ )
	{
          channels[i] = new vrpn_FunctionGenerator_channel;
	}
}


vrpn_FunctionGenerator::~vrpn_FunctionGenerator( )
{
	unsigned i;
	for( i = 0; i <= vrpn_FUNCTION_CHANNELS_MAX - 1; i++ )
	{
          try {
            delete channels[i];
          } catch (...) {
            fprintf(stderr, "vrpn_FunctionGenerator::~vrpn_FunctionGenerator(): delete failed\n");
            return;
          }
	}
}


const vrpn_FunctionGenerator_channel* vrpn_FunctionGenerator::
getChannel( vrpn_uint32 channelNum )
{
	if( channelNum > vrpn_FUNCTION_CHANNELS_MAX - 1 )
		return NULL;
	return channels[channelNum];
}


int vrpn_FunctionGenerator::
register_types( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::register_types\n" );
	fflush( stdout );
#endif
	channelMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL );
	requestChannelMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REQUEST );
	requestAllChannelsMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_ALL_CHANNEL_REQUEST );
	sampleRateMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE );
	startFunctionMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_START );
	stopFunctionMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_STOP );
	requestInterpreterMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_INTERPRETER_REQUEST );

	channelReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_CHANNEL_REPLY );
	startFunctionReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_START_REPLY );
	stopFunctionReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_STOP_REPLY );
	sampleRateReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_SAMPLE_RATE_REPLY );
	interpreterReplyMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_INTERPRETER_REPLY );
	errorMessageID = d_connection->register_message_type( vrpn_FUNCTION_MESSAGE_TYPE_ERROR );

	gotConnectionMessageID = d_connection->register_message_type( vrpn_got_connection );

	if( channelMessageID == -1 || requestChannelMessageID == -1
		|| requestAllChannelsMessageID == -1
		|| sampleRateMessageID == -1 || startFunctionMessageID == -1
		|| stopFunctionMessageID == -1 || channelReplyMessageID == -1 
		|| startFunctionReplyMessageID == -1 || stopFunctionReplyMessageID == -1 
		|| sampleRateReplyMessageID == -1 || gotConnectionMessageID == -1
		|| requestInterpreterMessageID == -1 || interpreterReplyMessageID == -1 
		|| errorMessageID == -1 )
	{
		fprintf( stderr, "vrpn_FunctionGenerator::register_types:  error registering types.\n" );
		fflush( stderr );
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
vrpn_FunctionGenerator_Server( const char* name, vrpn_uint32 numChannels, vrpn_Connection * c )
: vrpn_FunctionGenerator( name, c )
{
	this->numChannels = numChannels;
	// Check if we have a connection
	if( d_connection == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server:  Can't get connection!\n" );
		fflush( stderr );
		return;
	}
	
	if( register_autodeleted_handler( channelMessageID, handle_channel_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register change channel request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( requestChannelMessageID, handle_channelRequest_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register channel request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( requestAllChannelsMessageID, handle_allChannelRequest_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register all-channel request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( startFunctionMessageID, handle_start_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register start request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( stopFunctionMessageID, handle_stop_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register stop request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( sampleRateMessageID, handle_sample_rate_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register sample-rate request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( requestInterpreterMessageID, handle_interpreter_request_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server: can't register interpreter request handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
}


vrpn_FunctionGenerator_Server::
~vrpn_FunctionGenerator_Server( )
{

}


vrpn_uint32 vrpn_FunctionGenerator_Server::
setNumChannels( vrpn_uint32 numChannels )
{
	if( numChannels > vrpn_FUNCTION_CHANNELS_MAX )
		numChannels = vrpn_FUNCTION_CHANNELS_MAX;
	this->numChannels = numChannels;
	return this->numChannels;
}


void vrpn_FunctionGenerator_Server::
mainloop( )
{
	// call the base class' server mainloop
	server_mainloop( );
}


//static 
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_channel_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_channel_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
        vrpn_FunctionGenerator_channel* channel = NULL;
        try { channel = new vrpn_FunctionGenerator_channel(); }
        catch (...) { return -1; }
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
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_channelRequest_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_channelRequest_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	vrpn_uint32 channelNum = vrpn_FUNCTION_CHANNELS_MAX + 1; // an invalid number
	if( 0 > me->decode_channel_request( p.buffer, p.payload_len, channelNum ) )
	{
		// the decode failed
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_channelRequest_message:  "
				"unable to decode channel number.\n" );
		fflush( stderr );
		return -1;
	}
	if( channelNum > vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_channelRequest_message:  "
				"invalid channel number %d.\n", channelNum );
		fflush( stderr );
		return -1;
	}
	me->sendChannelReply( channelNum );
	
	return 0;
}


//static 
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_allChannelRequest_message( void* userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_allChannelRequest_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	unsigned i;
	for( i = 0; i < vrpn_FUNCTION_CHANNELS_MAX; i++ )
	{
		// XXX will this work as-is, or do we need to
		// force buffers to be flushed periodically?
		me->sendChannelReply( i );
	}
	return 0;
}


//static 
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_start_message( void* userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_start_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	me->start( );
	return 0;
}


//static 
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_stop_message( void* userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_stop_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	me->stop( );
	return 0;
}


//static 
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_sample_rate_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_sample_rate_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	vrpn_float32 rate = 0;
	if( 0 > me->decode_sampleRate_request( p.buffer, p.payload_len, rate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::handle_sample_rate_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		me->sendSampleRateReply( );
		return -1;
	}
	me->setSampleRate( rate );
	return 0;
}


//static 
int VRPN_CALLBACK vrpn_FunctionGenerator_Server::
handle_interpreter_request_message( void* userdata, vrpn_HANDLERPARAM /*p*/)
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_interpreter_request_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Server* me = (vrpn_FunctionGenerator_Server*) userdata;
	me->sendInterpreterDescription( );
	return 0;
}



int vrpn_FunctionGenerator_Server::
sendChannelReply( vrpn_uint32 channelNum )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::sendChannelReply\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_channel_reply( &buf, buflen, channelNum ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendChannelReply:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->channelReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendChannelReply:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendSampleRateReply( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::sendSampleRateReply\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( this->encode_sampleRate_reply( &buf, buflen, sampleRate ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendSampleRateReply:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->sampleRateReplyMessageID,	this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendSampleRateReply:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendStartReply( vrpn_bool started )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::sendStartReply\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_start_reply( &buf, buflen, started ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStartReply:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->startFunctionReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStartReply:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendStopReply( vrpn_bool stopped )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::sendStopReply\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_stop_reply( &buf, buflen, stopped ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStopReply:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->stopFunctionReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendStopReply:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendInterpreterDescription( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::sendInterpreterDescription\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_interpreterDescription_reply( &buf, buflen, getInterpreterDescription() ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendInterpreterDescription:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->interpreterReplyMessageID, this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendInterpreterDescription:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	return 0;
}


int vrpn_FunctionGenerator_Server::
sendError( FGError error, vrpn_int32 channel )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::sendError\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( this->encode_error_report( &buf, buflen, error, channel ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendError:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->errorMessageID,	this->d_sender_id, 
										msgbuf, vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Server::sendError:  "
				"could not write message.\n" );
			fflush( stderr );
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
: vrpn_FunctionGenerator( name, c )
{
	// Check if we have a connection
	if( d_connection == NULL )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote:  Can't get connection!\n" );
		fflush( stderr );
		return;
	}
	
	if( register_autodeleted_handler( channelReplyMessageID, handle_channelReply_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote: can't register channel reply handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( startFunctionReplyMessageID, handle_startReply_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote: can't register start reply handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( stopFunctionReplyMessageID, handle_stopReply_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote: can't register stop reply handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( sampleRateReplyMessageID, handle_sampleRateReply_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote: can't register sample-rate reply handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( interpreterReplyMessageID, handle_interpreterReply_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote: can't register interpreter reply handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
	if( register_autodeleted_handler( errorMessageID, handle_error_message, this, d_sender_id ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote: can't register error message handler\n" );
		fflush( stderr );
		d_connection = NULL;
	}
	
}


int vrpn_FunctionGenerator_Remote::
setChannel( const vrpn_uint32 channelNum, const vrpn_FunctionGenerator_channel* channel )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::setChannel\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_channel( &buf, buflen, channelNum, channel ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::setChannel:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->channelMessageID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::setChannel:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::setChannel:  "
				"no connection.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestChannel( const vrpn_uint32 channelNum )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::requestChannel\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_channel_request( &buf, buflen, channelNum ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestChannel:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->requestChannelMessageID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestChannel:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestChannel:  "
				"no connection.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}



int vrpn_FunctionGenerator_Remote::
requestAllChannels( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::requestAllChannels\n" );
	fflush( stdout );
#endif
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
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestAllChannels:  "
				"no connection.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestStart( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::requestStart\n" );
	fflush( stdout );
#endif
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
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestStart:  "
				"no connection.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestStop( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::requestStop\n" );
	fflush( stdout );
#endif
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
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestStop:  "
				"no connection.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestSampleRate( vrpn_float32 rate )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::requestSampleRate\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		if( 0 > this->encode_sampleRate_request( &buf, buflen, rate ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"could not buffer message.\n" );
			fflush( stderr );
			return -1;
		}
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->sampleRateMessageID, this->d_sender_id, msgbuf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestSampleRate:  "
				"no connection.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


int vrpn_FunctionGenerator_Remote::
requestInterpreterDescription( )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::requestInterpreterDescription\n" );
	fflush( stdout );
#endif
	vrpn_gettimeofday( &timestamp, NULL );
	if( this->d_connection )
	{
		int buflen = vrpn_CONNECTION_TCP_BUFLEN;
		char* buf = &msgbuf[0];
		// nothing to encode; the message type is the symbol
		if( d_connection->pack_message( vrpn_CONNECTION_TCP_BUFLEN - buflen, timestamp, 
										this->requestInterpreterMessageID, this->d_sender_id, buf, 
										vrpn_CONNECTION_RELIABLE ) )
		{
			fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestInterpreterDescription:  "
				"could not write message.\n" );
			fflush( stderr );
			return -1;
		}
	}
	else
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::requestInterpreterDescription:  "
				"no connection.\n" );
		fflush( stderr );
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
	return channel_reply_list.register_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
unregister_channel_reply_handler( void *userdata,
								 vrpn_FUNCTION_CHANGE_REPLY_HANDLER handler )
{
	return channel_reply_list.unregister_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
register_start_reply_handler( void *userdata,
							 vrpn_FUNCTION_START_REPLY_HANDLER handler )
{
	return start_reply_list.register_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
unregister_start_reply_handler( void *userdata,
							   vrpn_FUNCTION_START_REPLY_HANDLER handler )
{
	return start_reply_list.unregister_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
register_stop_reply_handler( void *userdata,
							vrpn_FUNCTION_STOP_REPLY_HANDLER handler )
{
	return stop_reply_list.register_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
unregister_stop_reply_handler( void *userdata,
							  vrpn_FUNCTION_STOP_REPLY_HANDLER handler )
{
	return stop_reply_list.unregister_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
register_sample_rate_reply_handler( void *userdata,
								   vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler )
{
	return sample_rate_reply_list.register_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
unregister_sample_rate_reply_handler( void *userdata,
									 vrpn_FUNCTION_SAMPLE_RATE_REPLY_HANDLER handler )
{
	return sample_rate_reply_list.unregister_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
register_interpreter_reply_handler( void *userdata,
								   vrpn_FUNCTION_INTERPRETER_REPLY_HANDLER handler )
{
	return interpreter_reply_list.register_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
unregister_interpreter_reply_handler( void *userdata,
									 vrpn_FUNCTION_INTERPRETER_REPLY_HANDLER handler )
{
	return interpreter_reply_list.unregister_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
register_error_handler( void *userdata,
						vrpn_FUNCTION_ERROR_HANDLER handler )
{
	return error_list.register_handler(userdata, handler);
}


int vrpn_FunctionGenerator_Remote::
unregister_error_handler( void *userdata,
						  vrpn_FUNCTION_ERROR_HANDLER handler )
{
	return error_list.unregister_handler(userdata, handler);
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_channelReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_channelReply_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	vrpn_uint32 channelNum = vrpn_FUNCTION_CHANNELS_MAX + 1; // an invalid number
	if( 0 > me->decode_channel_reply( p.buffer, p.payload_len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_channelReply_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		return -1;
	}
	if( channelNum >= vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_channelReply_message:  "
				"invalid channel %d.\n", channelNum );
		fflush( stderr );
		return -1;
	}

	vrpn_FUNCTION_CHANNEL_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.channelNum = channelNum;
	cb.channel = me->channels[channelNum];

    me->channel_reply_list.call_handlers( cb );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_startReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_startReply_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	vrpn_bool isStarted = false;
	if( 0 > me->decode_start_reply( p.buffer, p.payload_len, isStarted ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_startReply_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		return -1;
	}

	vrpn_FUNCTION_START_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.isStarted = isStarted;

    me->start_reply_list.call_handlers( cb );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_stopReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_stopReply_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	vrpn_bool isStopped = false;
	if( 0 > me->decode_stop_reply( p.buffer, p.payload_len, isStopped ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_stopReply_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		return -1;
	}

	vrpn_FUNCTION_STOP_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.isStopped = isStopped;

    me->stop_reply_list.call_handlers( cb );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_sampleRateReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_sampleRateReply_message\n" );
	fflush( stdout );
#endif
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	if( 0 > me->decode_sampleRate_reply( p.buffer, p.payload_len ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_sampleRateReply_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		return -1;
	}

	vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB cb;
	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;
	cb.sampleRate = me->sampleRate;

    me->sample_rate_reply_list.call_handlers( cb );
	return 0;
}


//static 
int vrpn_FunctionGenerator_Remote::
handle_interpreterReply_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_interpreterReply_message\n" );
	fflush( stdout );
#endif
	vrpn_FUNCTION_INTERPRETER_REPLY_CB cb;
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	if( 0 > me->decode_interpreterDescription_reply( p.buffer, p.payload_len, &(cb.description) ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_interpreterReply_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		return -1;
	}

	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;

    me->interpreter_reply_list.call_handlers( cb );
	return 0;
}


// static
int vrpn_FunctionGenerator_Remote::
handle_error_message( void* userdata, vrpn_HANDLERPARAM p )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::handle_error_message\n" );
	fflush( stdout );
#endif
	vrpn_FUNCTION_ERROR_CB cb;
	vrpn_FunctionGenerator_Remote* me = (vrpn_FunctionGenerator_Remote*) userdata;
	if( 0 > me->decode_error_reply( p.buffer, p.payload_len, cb.err, cb.channel ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::handle_error_message:  "
				"unable to decode.\n" );
		fflush( stderr );
		return -1;
	}

	cb.msg_time.tv_sec = p.msg_time.tv_sec;
	cb.msg_time.tv_usec = p.msg_time.tv_usec;

    me->error_list.call_handlers( cb );
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
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_channel\n" );
	fflush( stdout );
#endif
	if( channelNum > vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"invalid channel nubmer %d.\n", channelNum );
		fflush( stderr );
		return -1;
	}
	if( static_cast<unsigned>(len) < sizeof( vrpn_uint32 ) )
	{
		// the channel's encode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least encode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"couldn't buffer (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_int32)) );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"message payload error (couldn't buffer channel number).\n" );
		fflush( stderr );
		return -1;
	}
	if( 0 > channel->encode_to( buf, len ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel:  "
				"message payload error (couldn't buffer channel).\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_channel( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum,
				vrpn_FunctionGenerator_channel& channel )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_channel\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_uint32 ) )
	{
		// the channel's decode_from function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least decode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel:  "
				"channel message payload error (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_int32)) );
		fflush( stderr );
		return -1;
	}
	const char* mybuf = buf;
	vrpn_int32 mylen = len;
	vrpn_uint32 myNum = 0;
	if( 0 > vrpn_unbuffer( &mybuf, &myNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel:  "
				"message payload error (couldn't unbuffer)\n" );
		fflush( stderr );
		return -1;
	}
	mylen -= sizeof( myNum );
	channelNum = myNum;
	if( 0 > channel.decode_from( &mybuf, mylen ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel:  "
				"error while decoding channel %d\n", channelNum );
		fflush( stderr );
		return -1;
	}

	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_channel_reply( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_channel_reply\n" );
	fflush( stdout );
#endif
	if( channelNum >= vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channel_reply:  "
				"invalid channel\n" );
		fflush( stderr );
		return -1;
	}
	if( static_cast<unsigned>(len) < sizeof( vrpn_uint32 ) )
	{
		// the channel's encode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least encode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channel_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_uint32)) );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channel_reply:  "
				"unable to buffer channel number.\n" );
		fflush( stderr );
		return -1;
	}
	if( 0 > channels[channelNum]->encode_to( buf, len ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_channel_reply:  "
				"unable to encode channel.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_channel_reply( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_channel_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_uint32 ) )
	{
		// the channel's decode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least decode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_channel_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_uint32)) );
		fflush( stderr );
		return -1;
	}
	const char* mybuf = buf;
	vrpn_int32 mylen = len;
	vrpn_uint32 myNum = 0;
	if( 0 > vrpn_unbuffer( &mybuf, &myNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_channel_reply:  "
				"unable to unbuffer channel number.\n" );
		fflush( stderr );
		return -1;
	}
	if( myNum >= vrpn_FUNCTION_CHANNELS_MAX )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_channel_reply:  "
				"invalid channel:  %d\n", myNum );
		fflush( stderr );
		return -1;
	}
	channelNum = myNum;
	mylen -= sizeof( vrpn_uint32 );
	return channels[channelNum]->decode_from( &mybuf, mylen );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
encode_channel_request( char** buf, vrpn_int32& len, const vrpn_uint32 channelNum )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_channel_request\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_uint32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel_request:  "
				"channel message payload error (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_int32)) );
		fflush( stderr );
		return -1;
	}
	vrpn_int32 mylen = len;
	if( 0 > vrpn_buffer( buf, &mylen, channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_channel_request:  "
				"unable to buffer channel %d", channelNum );
		fflush( stderr );
		return -1;
	}
	len = mylen;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_channel_request( const char* buf, const vrpn_int32 len, vrpn_uint32& channelNum )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_channel_request\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_uint32 ) )
	{
		// the channel's encode_to function will check that the length is
		// sufficient for the channel's info, so just check that we can
		// at least encode the channel number.
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel_request:  "
				"channel message payload error (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_int32)) );
		fflush( stderr );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &channelNum ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_channel_request:  "
				"unable to unbuffer channel %d", channelNum );
		fflush( stderr );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
encode_sampleRate_request( char** buf, vrpn_int32& len, const vrpn_float32 sampleRate )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_sampleRate_request\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_sampleRate_request:  "
				"channel message payload error (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_float32)) );
		fflush( stderr );
		return -1;
	}
	vrpn_int32 mylen = len;
	if( 0 > vrpn_buffer( buf, &mylen, sampleRate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::encode_sampleRate_request:  "
				"unable to buffer sample rate" );
		fflush( stderr );
		return -1;
	}
	len = mylen;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
decode_sampleRate_request( const char* buf, const vrpn_int32 len, vrpn_float32& sampleRate )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_sampleRate_request\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_sampleRate_request:  "
				"channel message payload error (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_float32)) );
		fflush( stderr );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &sampleRate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::decode_sampleRate_request:  "
				"unable to unbuffer sample rate" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_start_reply( char** buf, vrpn_int32& len, const vrpn_bool isStarted )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_start_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_start_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_bool)) );
		fflush( stderr );
		return -1;
	}
	return vrpn_buffer( buf, &len, isStarted );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_start_reply( const char* buf, const vrpn_int32 len, vrpn_bool& isStarted )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_start_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_start_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_bool)) );
		fflush( stderr );
		return -1;
	}
	const char* mybuf = buf;
	if( 0 > vrpn_unbuffer( &mybuf, &isStarted ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_start_reply:  "
				"unable to unbuffer stop condition.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_stop_reply( char** buf, vrpn_int32& len, const vrpn_bool isStopped )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_stop_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_stop_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_bool)) );
		fflush( stderr );
		return -1;
	}
	return vrpn_buffer( buf, &len, isStopped );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_stop_reply( const char* buf, const vrpn_int32 len, vrpn_bool& isStopped )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_stop_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_bool ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_stop_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_bool)) );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_unbuffer( &buf, &isStopped ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_stop_reply:  "
				"unable to unbuffer stop condition.\n" );
		fflush( stderr );
		return -1;
	}
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_sampleRate_reply( char** buf, vrpn_int32& len, const vrpn_float32 sampleRate )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_sampleRate_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_sampleRate_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_float32)) );
		fflush( stderr );
		return -1;
	}
	return vrpn_buffer( buf, &len, sampleRate );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_sampleRate_reply( const char* buf, const vrpn_int32 len )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_sampleRate_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_float32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_sampleRate_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_float32)) );
		fflush( stderr );
		return -1;
	}
	vrpn_float32 myRate = 0;
	if( 0 > vrpn_unbuffer( &buf, &myRate ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_sampleRate_reply:  "
				"unable to unbuffer sample rate.\n" );
		fflush( stderr );
		return -1;
	}
	this->sampleRate = myRate;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_interpreterDescription_reply( char** buf, vrpn_int32& len, const char* desc )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_interpreterDescription_reply\n" );
	fflush( stdout );
#endif
	vrpn_int32 dlength = static_cast<vrpn_int32>(strlen( desc ));
	if( len < dlength + (vrpn_int32) sizeof( vrpn_int32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_interpreterDescription_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, dlength + static_cast<unsigned long>(sizeof( vrpn_int32 )) );
		fflush( stderr );
		return -1;
	}
	if( 0 > vrpn_buffer( buf, &len, dlength ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_interpreterDescription_reply:  "
				"unable to buffer description length.\n" );
		fflush( stderr );
		return -1;
	}

	return vrpn_buffer( buf, &len, desc, dlength );
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_interpreterDescription_reply( const char* buf, const vrpn_int32 len, char** desc )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_interpreterDescription_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( vrpn_int32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_interpreterDescription_reply:  "
				"insufficient buffer space given (got %d, wanted at least %lud).\n", 
				len, static_cast<unsigned long>(sizeof( vrpn_int32)) );
		fflush( stderr );
		return -1;
	}
	vrpn_int32 dlength = 0;
	if( 0 > vrpn_unbuffer( &buf, &dlength ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_interpreterDescription_reply:  "
				"unable to unbuffer description length.\n" );
		fflush( stderr );
		return -1;
	}
        try { *desc = new char[dlength + 1]; }
        catch (...) {
          fprintf(stderr, "vrpn_FunctionGenerator_Remote::decode_interpreterDescription_reply:  "
            "Out of memory.\n");
          fflush(stderr);
          return -1;
        }
	int retval = vrpn_unbuffer( &buf, *desc, dlength );
	(*desc)[dlength] = '\0';
	return retval;
}


vrpn_int32 vrpn_FunctionGenerator_Server::
encode_error_report( char** buf, vrpn_int32& len, const FGError error, const vrpn_int32 channel )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::encode_error_report\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( FGError ) + sizeof( vrpn_int32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_error_report:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( FGError) + sizeof( vrpn_int32 )) );
		fflush( stderr );
		return -1;
	}
	vrpn_int32 mylen = len;
	if( 0 > vrpn_buffer( buf, &mylen, error ) || 0 > vrpn_buffer( buf, &mylen, channel ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Server::encode_error_report:  "
				"unable to buffer error & channel" );
		fflush( stderr );
		return -1;
	}
	len = mylen;
	return 0;
}


vrpn_int32 vrpn_FunctionGenerator_Remote::
decode_error_reply( const char* buf, const vrpn_int32 len, FGError& error, vrpn_int32& channel )
{
#ifdef DEBUG_VRPN_FUNCTION_GENERATOR
	fprintf( stdout, "FG::decode_error_reply\n" );
	fflush( stdout );
#endif
	if( static_cast<unsigned>(len) < sizeof( FGError ) + sizeof( vrpn_int32 ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_error_reply:  "
				"insufficient buffer space given (got %d, wanted %lud).\n", 
				len, static_cast<unsigned long>(sizeof( FGError) + sizeof( vrpn_int32 )) );
		fflush( stderr );
		return -1;
	}
	int myError = NO_FG_ERROR;
	vrpn_int32 myChannel = -1;
	if( 0 > vrpn_unbuffer( &buf, &myError ) 
		|| 0 > vrpn_unbuffer( &buf, &myChannel ) )
	{
		fprintf( stderr, "vrpn_FunctionGenerator_Remote::decode_error_reply:  "
				"unable to unbuffer error & channel.\n" );
		fflush( stderr );
		return -1;
	}
	error = FGError( myError );
	channel = myChannel;
	return 0;
}



//
// end encode & decode functions for
// vrpn_FunctionGenerator_Remote and
// vrpn_FunctionGenerator_Server
///////////////////////////////////////

