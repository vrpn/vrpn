/**	daq_server.C

  This program is a bit backwards from most VRPN progams, in that it
is a "server" but it uses a client connection to do its thing.  Its
purpose is to drive the voltages on a National Instruments' DAQ board
based on the values placed in a vrpn_Analog_Server.
  This program creates a client connection and then connects a
vrpn_Analog_Remote to it.  Whenever values come into the analog, it
puts them channel-for-channel into the D/A board.
  Because of the continually-tried reconnect, you can leave this
server running, and it will continually try to reconnect to a
Server Connection whose name you tell it at startup.

**/

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "NIUtil.h"
#include "vrpn_Connection.h"
#include "vrpn_Analog.h"


/*****************************************************************************
 *
   Variables used to pass information around the program
 *
 *****************************************************************************/

vrpn_Analog_Remote *ana;
vrpn_Connection *c;
int done = 0;
int verbose = 0;		//< Print the values when setting them?
int NI_device_number = -1;	//< National Instruments device to use
int NI_num_channels = 8;	//< Number of channels on the board
double	min_voltage = 0.0;	//< Minimum voltage allowed on a channel
double	max_voltage = 6.0;	//< Maximum voltate allowed on a channel

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_analog (void *, const vrpn_ANALOGCB a)
{
    int i;
    int channels = a.num_channel;

    // Print the values if we're in verbose mode.
    if (verbose) {
	printf("Analogs: ");
	for (i = 0; i < a.num_channel; i++) {
		printf("%4.2f ",a.channel[i]);
	}
	printf("\n");
    }

    // Make sure we have a valid board number
    if (NI_device_number < 0) {
	fprintf(stderr,"handle_analog: Bad NI device number (%g)\n", NI_device_number);
	done = 1;
    }

    // Make sure we don't have too many channels
    if (a.num_channel > NI_num_channels) {
	fprintf(stderr,"handle_analog: Warning... too many channels (truncating to %d)\n", NI_num_channels);
	channels = NI_num_channels;
    }

    // XXX For now, we are setting each channel separately.  In the future, it
    // would be good to set them all in one fell swoop so that they voltages
    // all change at once.
    for (i = 0; i < channels; i++) {

	// Clip the values to within the minimum and maximum.
	double voltage = a.channel[i];
	if (voltage < min_voltage) {
		fprintf(stderr,"Warning: low voltage clipped to %g\n", min_voltage);
		voltage = min_voltage;
	}
	if (voltage > max_voltage) {
		fprintf(stderr,"Warning: high voltage clipped to %g\n", max_voltage);
		voltage = max_voltage;
	}
	AO_VWrite(NI_device_number, i, voltage);
    }
}

int	VRPN_CALLBACK handle_connection_shutdown(void *, vrpn_HANDLERPARAM)
{
    int	i;

    // Make sure we have a valid board number
    if (NI_device_number < 0) {
	fprintf(stderr,"handle_analog: Bad NI device number (%g)\n", NI_device_number);
	done = 1;
    }

    fprintf(stderr, "handle_connection_shutdown: Zeroing the D/As\n");

    // Set all the channels to zero whenever a connection is dropped
    for (i = 0; i < NI_num_channels; i++) {
	AO_VWrite(NI_device_number, i, 0.0);
    }

    return 0;
}

/// Shut down if the user presses ^C.

void handle_cntl_c (int) {
    done = 1;
}


void Usage (const char * s)
{
  fprintf(stderr,"Usage: %s [-board boardname] [-channels num] [-server servername] [-unipolar]\n",s);
  fprintf(stderr,"       [-bipolar] [-range min max] [-internal_ref] [-external_ref]\n");
  fprintf(stderr,"       [-ref_voltage voltage] [-verbose]\n");
  fprintf(stderr,"       -channels: Number of channels (default 8).\n");
  fprintf(stderr,"       -board: Name of the NI board to use (default PCI-6713).\n");
  fprintf(stderr,"       -server: Name of the vrpn_Analog_Server to connect to (default MagnetDrive@nobellium-cs).\n");
  fprintf(stderr,"       -bipolar/-unipolar: Polarity (default unipolar).\n");
  fprintf(stderr,"       -range: Allowable voltage range (default %g to %g).\n", min_voltage, max_voltage);
  fprintf(stderr,"       -internal_ref/-external_ref: Reference source (default internal).\n");
  fprintf(stderr,"       -ref_voltage: Reference voltage (default 0.0).\n");
  fprintf(stderr,"       -verbose: Print each new round of voltages.\n");
  exit(0);
}

void main (int argc, char * argv [])
{
    char    *server_name = "MagnetDrive@nobellium-cs";
    char    *board_name = "PCI-6713";
    int	    polarity = 1;	//< Unipolar = 1, bipolar = 0
    int	    ref_source = 0;	//< Reference source, 0 = internal, 1 = external
    double  ref_voltage = 0.0;
    int	    update_mode = 0;	//< Mode to update the board (0 = immediate)

    int i;
    int realparams = 0;	    //< How many non "flag" parameters
    vrpn_Connection *conn;  //< Connection used by the analog client

    // Parse the command line
    i = 1;
    while (i < argc) {
      if (!strcmp(argv[i], "-board")) {
	    if (++i > argc) { Usage(argv[0]); }
	    board_name = argv[i];
      } else if (!strcmp(argv[i], "-channels")) {
	    if (++i > argc) { Usage(argv[0]); }
	    NI_num_channels = atoi(argv[i]);
      } else if (!strcmp(argv[i], "-server")) {
	    if (++i > argc) { Usage(argv[0]); }
	    server_name = argv[i];
      } else if (!strcmp(argv[i], "-unipolar")) {
	    polarity = 1;
      } else if (!strcmp(argv[i], "-bipolar")) {
	    polarity = 0;
      } else if (!strcmp(argv[i], "-internal_ref")) {
	    ref_source = 0;
      } else if (!strcmp(argv[i], "-external_ref")) {
	    ref_source = 1;
      } else if (!strcmp(argv[i], "-ref_voltage")) {
	    if (++i > argc) { Usage(argv[0]); }
	    ref_voltage = atof(argv[i]);
      } else if (!strcmp(argv[i], "-range")) {
	    if (++i > argc) { Usage(argv[0]); }
	    min_voltage = atof(argv[i]);
	    if (++i > argc) { Usage(argv[0]); }
	    max_voltage = atof(argv[i]);
      } else if (!strcmp(argv[i], "-verbose")) {
	    verbose = 1;
      } else if (argv[i][0] == '-') {	// Unknown flag
	    Usage(argv[0]);
      } else switch (realparams) {		// Non-flag parameters
	case 0:
	    Usage(argv[0]);
	    break;
	default:
	    Usage(argv[0]);
      }
      i++;
    }

    // Open the D/A board and set the parameters for each channel.
    NI_device_number = NIUtil::findDevice(board_name);
    if (NI_device_number == -1) {
	fprintf(stderr, "Error opening the D/A board %s\n", board_name);
	exit(-1);
    }
    for (i = 0; i < NI_num_channels; i++) {
	AO_Configure(NI_device_number, i, polarity, ref_source, ref_voltage,
	    update_mode);
    }

    // signal handler so logfiles get closed right
    signal(SIGINT, handle_cntl_c);

    fprintf(stderr, "Analog's name is %s.\n", server_name);
    ana = new vrpn_Analog_Remote (server_name);

    // Set up the callback handlers for analog values, and for
    // when a connection drops.
    printf("Analog update: Analogs: [new values listed]\n");
    ana->register_change_handler(NULL, handle_analog);
    if ( (conn = ana->connectionPtr()) == NULL) {
	fprintf(stderr,"Can't get connection from analog remote!\n");
	exit(-1);
    }
    conn->register_handler( conn->register_message_type(vrpn_dropped_connection),
	    handle_connection_shutdown, NULL);
    conn->register_handler( conn->register_message_type(vrpn_dropped_last_connection),
	    handle_connection_shutdown, NULL);

    /* 
    * main interactive loop
    */
    while ( ! done )
    {
	// Handle any callbacks
	ana->mainloop();

	// Sleep for 1ms so we don't eat the CPU
	vrpn_SleepMsecs(1);
    }

    // Delete the analog device
    if (ana) delete ana;

    // XXX Presumably the device gets closed when the program exits.  Might be
    // better to close it intentionally.

}   /* main */
