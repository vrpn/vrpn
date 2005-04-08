// XXX It gets so busy reading from the network that it never updates
// the display.  This doesn't happen when it is a local connection.

//----------------------------------------------------------------------------
// Example program to read pixels from a vrpn_Imager server and display
// them in an OpenGL window.  It assumes that the size of the imager does
// not change during the run.  It asks for unsigned 8-bit pixels.

#include <stdio.h>
#include <stdlib.h>
#ifdef	_WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <glut.h>
#include <vrpn_Connection.h>
#include <vrpn_Imager.h>

//----------------------------------------------------------------------------
// Glut insists on taking over the whole application, leaving us to use
// global variables to pass information between the various callback
// handlers.

bool			g_quit = false;	//< Set to true when time to quit
vrpn_Connection		*g_connection;	//< Set if logging is enabled.
vrpn_Imager_Remote	*g_imager;	//< Imager client object
bool g_got_dimensions = false;		//< Heard image dimensions from server?
int			g_Xdim, g_Ydim;	//< Dimensions in X and Y
bool g_ready_for_region = false;	//< Everything set up to handle a region?
unsigned char *g_image = NULL;		//< Pointer to the storage for the image
bool g_already_posted = false;		//< Posted redisplay since the last display?

//----------------------------------------------------------------------------
// Imager callback handlers.

void  VRPN_CALLBACK handle_discarded_frames(void *, const vrpn_IMAGERDISCARDEDFRAMESCB info)
{
  printf("Server discarded %d frames\n", (int)(info.count));
}

void  VRPN_CALLBACK handle_description_message(void *, const struct timeval)
{
  // This method is different from other VRPN callbacks because it simply
  // reports that values have been filled in on the Imager_Remote class.
  // It does not report what the new values are, only the time at which
  // they changed.

  // If we have already heard the dimensions, then check and make sure they
  // have not changed.  Also ensure that there is at least one channel.  If
  // not, then print an error and exit.
  if (g_got_dimensions) {
    if ( (g_Xdim != g_imager->nCols()) || (g_Ydim != g_imager->nRows()) ) {
      fprintf(stderr,"Error -- different image dimensions reported\n");
      exit(-1);
    }
    if (g_imager->nChannels() <= 0) {
      fprintf(stderr,"Error -- No channels to display!\n");
      exit(-1);
    }
  }

  // Record that the dimensions are filled in.  Fill in the globals needed
  // to store them.
  g_Xdim = g_imager->nCols();
  g_Ydim = g_imager->nRows();
  g_got_dimensions = true;
}

void myDisplayFunc();
float fps[2]={0,0};
DWORD lastCallTime[2]={0,0};
DWORD ReportInterval=5000;

// New pixels coming: fill them into the image and tell Glut to redraw.
void  VRPN_CALLBACK handle_region_change(void *userdata, const vrpn_IMAGERREGIONCB info)
{
    const vrpn_Imager_Region  *region=info.region;
    const vrpn_Imager_Remote  *imager = (const vrpn_Imager_Remote *)userdata;

    // Just leave things alone if we haven't set up the drawable things
    // yet.
    if (!g_ready_for_region) { return; }

    // Copy pixels into the image buffer.
    // Flip the image over in Y so that the image coordinates
    // display correctly in OpenGL.
    // Figure out which color to put the data in depending on the name associated
    // with the channel index.  If it is one of "red", "green", or "blue" then put
    // it into that channel.  If it is not one of these, put it into all channels.
    if (strcmp(imager->channel(region->d_chanIndex)->name, "red") == 0) {
      region->decode_unscaled_region_using_base_pointer(g_image+0, 3, 3*g_Xdim, 0, g_Ydim, true);
    } else if (strcmp(imager->channel(region->d_chanIndex)->name, "green") == 0) {
      region->decode_unscaled_region_using_base_pointer(g_image+1, 3, 3*g_Xdim, 0, g_Ydim, true);
    } else if (strcmp(imager->channel(region->d_chanIndex)->name, "blue") == 0) {
      region->decode_unscaled_region_using_base_pointer(g_image+2, 3, 3*g_Xdim, 0, g_Ydim, true);
    } else {
      // This uses a repeat count of three to put the data into all channels.
      // NOTE: This copies each channel into all buffers, rather
      // than just a specific one (overwriting all with the last one written).  A real
      // application will probably want to provide a selector to choose which
      // is drawn.  It can check region->d_chanIndex to determine which channel
      // is being reported for each callback.
      region->decode_unscaled_region_using_base_pointer(g_image, 3, 3*g_Xdim, 0, g_Ydim, true, 3);
    }

    // If we're logging, save to disk.  This is needed to keep up with
    // logging and because to program is killed to exit it.
    if (g_connection) { g_connection->save_log_so_far(); }

    // Tell Glut it is time to draw.  Make sure that we don't post the redisplay
    // operation more than once by checking to make sure that it has been handled
    // since the last time we posted it.  If we don't do this check, it gums
    // up the works with tons of redisplay requests and the program won't
    // even handle windows events.

    // NOTE: This will show intermediate frames, where perhaps only one color has
    // been loaded or a fraction of some have been loaded.  Use the end-of-frame
    // callback to determine when a full frame has been filled if you want to
    // ensure that no tearing is visible.

    if (!g_already_posted) {
      g_already_posted = true;
      glutPostRedisplay();
    }
}

//----------------------------------------------------------------------------
// Glut callback handlers.

void myDisplayFunc(void)
{
  // Clear the window and prepare to draw in the back buffer
  glDrawBuffer(GL_BACK);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  // Store the pixels from the image into the frame buffer
  // so that they cover the entire image (starting from lower-left
  // corner, which is at (-1,-1)).
  glRasterPos2f(-1, -1);
  glDrawPixels(g_imager->nCols(),g_imager->nRows(), GL_RGB, GL_UNSIGNED_BYTE, g_image);

  // Swap buffers so we can see it.
  glutSwapBuffers();

  // Capture timing information and print out how many frames per second
  // are being drawn.

  { static struct timeval last_print_time;
    struct timeval now;
    static bool first_time = true;
    static int frame_count = 0;

    if (first_time) {
      vrpn_gettimeofday(&last_print_time, NULL);
      first_time = false;
    } else {
      frame_count++;
      vrpn_gettimeofday(&now, NULL);
      double timesecs = 0.001 * vrpn_TimevalMsecs(vrpn_TimevalDiff(now, last_print_time));
      if (timesecs >= 5) {
	double frames_per_sec = frame_count / timesecs;
	frame_count = 0;
	printf("Displayed frames per second = %lg\n", frames_per_sec);
	last_print_time = now;
      }
    }
  }

  // We've no longer posted redisplay since the last display.
  g_already_posted = false;
}

void myIdleFunc(void)
{
  // See if there are any more messages from the server and then sleep
  // a little while so that we don't eat the whole CPU.
  g_imager->mainloop();
  vrpn_SleepMsecs(5);
  if (g_quit) {
    delete g_imager;
    if (g_image) { delete [] g_image; };
    exit(0);
  }
}

void myKeyboardFunc(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:  // Escape
  case 'q':
  case 'Q':
    g_quit = 1;
    break;

  case '0': // For a number, set the throttle to that number
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    printf("Throttling after %d frames\n", key - '0');
    g_imager->throttle_sender(key - '0');
    break;

  case '-': // For the '-' sign, set the throttle to unlimited
    printf("Turning off frame throttle\n");
    g_imager->throttle_sender(-1);
    break;
  }
}


int main(int argc, char **argv)
{
  char	*device_name = "TestImage@localhost";
  char	*logfile_name = NULL;
  int i;

  // Parse the command line.  If there is one argument, it is the device
  // name.  If there is a second, it is a logfile name.
  if (argc >= 2) { device_name = argv[1]; }
  if (argc >= 3) { logfile_name = argv[2]; }
  if (argc > 3) { fprintf(stderr, "Usage: %s [device_name [logfile_name]]\n", argv[0]); exit(-1); }

  // Create a log file of the video if we've been asked to do logging
  if (logfile_name) {
    g_connection = vrpn_get_connection_by_name(device_name, logfile_name);
  }

  // Open the Imager client and set the callback
  // for new data and for information about the size of
  // the image.
  printf("Opening %s\n", device_name);
  g_imager = new vrpn_Imager_Remote(device_name);
  g_imager->register_description_handler(NULL, handle_description_message);
  g_imager->register_region_handler(g_imager, handle_region_change);
  g_imager->register_discarded_frames_handler(NULL, handle_discarded_frames);

  printf("Waiting to hear the image dimensions...\n");
  while (!g_got_dimensions) {
    g_imager->mainloop();
    vrpn_SleepMsecs(1);
  }

  // Allocate memory for the image and clear it, so that things that
  // don't get filled in will be black.
  if ( (g_image = new unsigned char[g_Xdim * g_Ydim * 3]) == NULL) {
    fprintf(stderr,"Out of memory when allocating image!\n");
    return -1;
  }
  for (i = 0; i < g_Xdim * g_Ydim * 3; i++) {
    g_image[i] = 0;
  }
  g_ready_for_region = true;
  printf("Receiving images at size %dx%d\n", g_Xdim, g_Ydim);

  // Initialize GLUT and create the window that will display the
  // video -- name the window after the device that has been
  // opened in VRPN.
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(g_Xdim, g_Ydim);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(vrpn_copy_service_name(device_name));

  // Set the display function and idle function for GLUT (they
  // will do all the work) and then give control over to GLUT.
  glutDisplayFunc(myDisplayFunc);
  glutIdleFunc(myIdleFunc);
  glutKeyboardFunc(myKeyboardFunc);
  glutMainLoop();

  // Clean up objects and return.  This code is never called because
  // glutMainLoop() never returns.
  return 0;
}
