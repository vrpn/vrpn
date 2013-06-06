
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <vrpn_Connection.h>
#include <vrpn_FileConnection.h>


// Written 12/18/03 by David Marshburn

static 	vrpn_Connection * connection = NULL;
static   vrpn_File_Connection *fcon = NULL;

static char * device_name = NULL;

static bool doHead = false;
static unsigned long msgs_to_print = ULONG_MAX;

static bool done = false;

//--------------------------------------------------------------------------

static void handle_cntl_c(int )
{
    done = true;
}


// This handler gets every message, presumably from a stream file
int VRPN_CALLBACK handle_any_print (void * userdata, vrpn_HANDLERPARAM p) 
{
	static unsigned long msg_number = 0 ;
	if( doHead && ( msg_number > msgs_to_print - 1 ) )
	{  return 0;  }
	vrpn_Connection * c = (vrpn_Connection *) userdata;
    struct timeval el;
    if(fcon) {
		fcon->time_since_connection_open(&el);
    } else {
		connection->time_since_connection_open(&el);
    }
	
	printf("Msg %lu \"%s\" from \"%s\" time %ld.%ld timestamp %ld.%ld\n",
			msg_number++, c->message_type_name(p.type), c->sender_name(p.sender),
			static_cast<long>(el.tv_sec),
			static_cast<long>(el.tv_usec),
			static_cast<long>(p.msg_time.tv_sec),
			static_cast<long>(p.msg_time.tv_usec));
	fflush( stdout );
	
	if( doHead && ( msgs_to_print - msg_number - 1 <= 0 ) )
		done = true;
	
	return 0;
}


// Argument handling
void usage(char *program_name)
{
	fprintf(stderr, "Error: bad or incomlete arguments.\n");
	fprintf(stderr, "usage: %s -i in-streamfile [-head num-msgs-to-print]\n", program_name);
	fflush( stderr );
	exit(-1);
}


void parseArguments(int argc, char **argv)
{
	int i;
	if( argc < 3 ) usage( argv[0] );
	
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i"))
		{
			if (++i >= argc) usage(argv[0]);
			device_name = new char[14 + strlen(argv[i])+1];
			sprintf(device_name,"file://%s", argv[i]);
		}
		else if( !strcmp( argv[i], "-head" ) )
		{
			if (++i >= argc) usage(argv[0]);
			if( atoi( argv[i] ) < 0 ) usage( argv[0] );
			msgs_to_print = atoi( argv[i] );
			doHead = true;
		}
		else
		{
			usage(argv[0]);
		}
	}
	printf("Device_name is %s\n", device_name);
}


int	main(int argc, char *argv[])
{
    parseArguments(argc, argv);
    
    // Initialize our connections to the things we are going to control.
    if (device_name == NULL) {  return -1;  }
	
	printf( "device to open:  %s\n", device_name );
	
	connection = vrpn_get_connection_by_name( device_name );
    if( connection == NULL )
	{
		// connection failed. VRPN prints error message.
		return -1;
    }
	
	fcon = connection->get_File_Connection();
	if (fcon==NULL) 
	{
		fprintf(stderr, "Error: expected but didn't get file connection\n");
		exit(-1);
	}
	fcon->set_replay_rate(200.0);
	
	signal(SIGINT, handle_cntl_c);
	
	// DEBUG print all messages, incoming and outgoing, 
	// sent over our VRPN connection.
	connection->register_handler(vrpn_ANY_TYPE, handle_any_print, connection);
	
	while (!done) 
	{
		//------------------------------------------------------------
		// Send/receive message from our vrpn connections.
		connection->mainloop();
		//fprintf(stderr, "After mainloop, fcon is doing %d.\n", fcon->doing_okay());
		if (fcon && fcon->eof()) 
		{
				done = true;
				connection->unregister_handler(vrpn_ANY_TYPE, handle_any_print, connection);
		}
		vrpn_SleepMsecs( 10 );      
	}
	
	if (connection) 
		delete connection; // needed to make stream file write out
	if( device_name )
		delete device_name;
	
	return 0;
}



