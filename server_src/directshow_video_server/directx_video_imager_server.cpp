#include  <math.h>
#include  <stdlib.h>
#include  <signal.h>
#include  <string.h>
#include  <stdio.h>
#include  <vrpn_Connection.h>
#include  <vrpn_Imager.h>
#include  "directx_camera_server.h"

const int MAJOR_VERSION = 3;
const int MINOR_VERSION = 2;

// sprintf() is okay in our usage.
#ifdef _MSC_VER
#pragma warning( disable : 4995 )
#endif

//-----------------------------------------------------------------
// This section contains code to initialize the camera and read its
// values.  The server code reads values from the camera and sends
// them over the network.

directx_camera_server  *g_camera;	    //< The camera we're going to read from
unsigned	    g_width = 0, g_height = 0;  //< Resolution for DirectX cameras, take the default
int                 g_numchannels = 1;  //< How many channels to send (3 for RGB cameras, 1 otherwise)
int                 g_maxval = 4095;    //< Maximum value available in a channel for this device

/// Open the camera we want to use (the type is based on the name passed in)
bool  init_camera_code(const char *type, int which = 1)
{
  if (!strcmp(type, "directx")) {
    printf("Opening DirectX Camera %d\n", which);
    g_camera = new directx_camera_server(which, g_width, g_height);
    g_numchannels = 3; // Send RGB
    g_maxval = 255;
    printf("Making sure camera is working\n");
    if (!g_camera->working()) {
      fprintf(stderr,"init_camera_code(): Can't open DirectX camera server\n");
      return false;
    }
  } else if (!strcmp(type, "directx640x480")) {
    printf("Opening DirectX Camera %d\n", which);
    g_camera = new directx_camera_server(which, 640, 480);
    g_numchannels = 3; // Send RGB
    g_maxval = 255;
    printf("Making sure camera is working\n");
    if (!g_camera->working()) {
      fprintf(stderr,"init_camera_code(): Can't open DirectX camera server\n");
      return false;
    }
  } else {
    fprintf(stderr,"init_camera_code(): Unknown camera type (%s)\n", type);
    return false;
  }

  return true;
}

void  teardown_camera_code(void)
{
  if (g_camera) { delete g_camera; };
}

//-----------------------------------------------------------------
// This section contains code that does what the server should do

vrpn_Connection               *svrcon;	//< Connection for server to talk on
vrpn_Imager_Server	      *svr;	//< Image server to be used to send
int			      svrchan;	//< Server channel index for image data

bool  init_server_code(const char *outgoing_logfile_name)
{
  int port = vrpn_DEFAULT_LISTEN_PORT_NO;
  if ( (svrcon = vrpn_create_server_connection(port, NULL, outgoing_logfile_name)) == NULL) {
    fprintf(stderr, "Could not open server connection\n");
    return false;
  }
  if ( (svr = new vrpn_Imager_Server("TestImage", svrcon,
    g_camera->get_num_columns(), g_camera->get_num_rows())) == NULL) {
    fprintf(stderr, "Could not open Imager Server\n");
    return false;
  }
  if ( (svrchan = svr->add_channel("red", "unknown", 0, (float)(g_maxval) )) == -1) {
    fprintf(stderr, "Could not add channel to server image\n");
    return false;
  }
  // XXX This relies on VRPN to hand us sequential channel numbers.  This might be
  // dangerous.
  if ( (svr->add_channel("green", "unknown", 0, (float)(g_maxval) )) == -1) {
    fprintf(stderr, "Could not add channel to server image\n");
    return false;
  }
  if ( (svr->add_channel("blue", "unknown", 0, (float)(g_maxval) )) == -1) {
    fprintf(stderr, "Could not add channel to server image\n");
    return false;
  }
  printf("Waiting for video connections on port %d\n", port);

  return true;
}

void  teardown_server_code(void)
{
  if (svr) { delete svr; svr = NULL; };
  if (svrcon) { delete svrcon; svrcon = NULL; };
}

//-----------------------------------------------------------------
// g_done gets set when the user presses ^C to exit the program.

static	bool  g_done = false;	//< Becomes true when time to exit

// WARNING: On Windows systems, this handler is called in a separate
// thread from the main program (this differs from Unix).  To avoid all
// sorts of chaos as the main program continues to handle packets, we
// set a done flag here and let the main program shut down in its own
// thread by calling shutdown() to do all of the stuff we used to do in
// this handler.

void handle_cntl_c(int) {
  g_done = true;
}

void  Usage(const char *s)
{
  fprintf(stderr,"Usage: %s [-res x y] [devicename] [devicenum [logfilename]]]\n",s);
  fprintf(stderr,"       -res: Resolution in x and y (default 320 200)\n");
  fprintf(stderr,"       devicename: directx or directx640x480 (default is directx)\n");
  fprintf(stderr,"       devicenum: Which (starting with 1) if there are multiple (default 1)\n");
  fprintf(stderr,"       logfilename: Name of file to store outgoing log in (default NULL)\n");
  exit(-1);
}

//-----------------------------------------------------------------
// Mostly just calls the above functions; split into client and
// server parts is done clearly to help people who want to use this
// as example code.  You could pull the above functions down into
// the main() program body without trouble (except for the callback
// handler, of course).

int main(int argc, char *argv[])
{
  int	i, realparams;		  // How many non-flag command-line arguments
  char	*devicename = "directx";  // Name of the device to open
  int	devicenum = 1;		  // Which, if there are more than one, to open
  char	*logfilename = NULL;	  // Outgoing log file name.

  realparams = 0;
  for (i = 1; i < argc; i++) {
    if (!strncmp(argv[i], "-res", strlen("-res"))) {
      if (++i > argc) { Usage(argv[0]); }
      g_width = atoi(argv[i]);
      if ( (g_width < 1) || (g_width > 1600) ) {
	fprintf(stderr,"Invalid width (1-1600 allowed, %f entered)\n", g_width);
	exit(-1);
      }
      if (++i > argc) { Usage(argv[0]); }
      g_height = atoi(argv[i]);
      if ( (g_height < 1) || (g_height > 1200) ) {
	fprintf(stderr,"Invalid height (1-1200 allowed, %f entered)\n", g_height);
	exit(-1);
      }
    } else {
      switch (++realparams) {
      case 1:
	devicename = argv[i];
	break;
      case 2:
	devicenum = atoi(argv[i]);
	break;
      case 3:
	logfilename = argv[i];
	break;
      default:
	Usage(argv[0]);
      }
    }
  }
    
  // Set up handler for all these signals to set done
  signal(SIGINT, handle_cntl_c);

  printf("vrpn_DirectShow_Imager_server version %02d.%02d\n", MAJOR_VERSION, MINOR_VERSION);

  if (!init_camera_code(devicename, devicenum)) { return -1; }
  printf("Opened camera\n");
  if (!init_server_code(logfilename)) { return -1; }

  while (!g_done) {
    g_camera->read_image_to_memory(0,0,0,0,1);
    g_camera->send_vrpn_image(svr,svrcon,1.0,svrchan, g_numchannels);
    svr->mainloop();
    svrcon->mainloop();
    svrcon->save_log_so_far();
  }

  printf("Deleting camera and connection objects\n");
  teardown_server_code();
  teardown_camera_code();
  return 0;
}
