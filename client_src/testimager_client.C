//----------------------------------------------------------------------------
// Example program to read pixels from a vrpn_TempImager server and display
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
#include <vrpn_TempImager.h>

//----------------------------------------------------------------------------
// Glut insists on taking over the whole application, leaving us to use
// global variables to pass information between the various callback
// handlers.

vrpn_Connection		*g_connection;	//< Set if logging is enabled.
vrpn_TempImager_Remote  *g_ti;		//< TempImager client object
bool g_got_dimensions = false;		//< Heard image dimensions from server?
bool g_ready_for_region = false;	//< Everything set up to handle a region?
unsigned char *g_image = NULL;		//< Pointer to the storage for the image
bool g_already_posted = false;		//< Posted redisplay since the last display?

//----------------------------------------------------------------------------
// TempImager callback handlers.

void  handle_description_message(void *, const struct timeval)
{
  // This assumes that the size of the image does not change -- application
  // should allocate a new image array and get a new window size whenever it
  // does change.  Here, we just record that the entries in the imager client
  // are valid.
  g_got_dimensions = true;
}

void myDisplayFunc();
float fps[2]={0,0};
DWORD lastCallTime[2]={0,0};
DWORD ReportInterval=5000;

// New pixels coming; fill them into the image and tell Glut to redraw.
void  handle_region_change(void *, const vrpn_IMAGERREGIONCB info)
{
    const vrpn_TempImager_Region  *region=info.region;

    vrpn_int32 nCols=g_ti->nCols();
    vrpn_int32 nRows=g_ti->nRows();

    if (!g_ready_for_region) { return; }

    // Copy pixels into the image buffer.
    // Flip the image over in Y so that the image coordinates
    // display correctly in OpenGL.
    region->decode_unscaled_region_using_base_pointer(g_image, 3, 3*nCols, nRows, true, 3);

    // If we're logging, save to disk.  This is needed to keep up with
    // logging and because to program is killed to exit it.
    if (g_connection) { g_connection->save_log_so_far(); }

    // Tell Glut it is time to draw.  Make sure that we don't post the redisplay
    // operation more than once by checking to make sure that it has been handled
    // since the last time we posted it.  If we don't do this check, it gums
    // up the works with tons of redisplay requests and the program won't
    // even handle windows events.
    if (!g_already_posted) {
      glutPostRedisplay();
      g_already_posted = true;
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
  glDrawPixels(g_ti->nCols(),g_ti->nRows(), GL_RGB, GL_UNSIGNED_BYTE, g_image);

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

  // Have no longer posted redisplay since the last display.
  g_already_posted = false;
}

void myIdleFunc(void)
{
  // See if there are any more messages from the server and then sleep
  // a little while so that we don't eat the whole CPU.
  g_ti->mainloop();
  vrpn_SleepMsecs(5);
}

int main(int argc, char **argv)
{
  char	*device_name = "TestImage@localhost:4511";
  char	*logfile_name = NULL;

  // Parse the command line.  If there is one argument, it is the device
  // name.  If there is a second, it is a logfile name.
  if (argc >= 2) { device_name = argv[1]; }
  if (argc >= 3) { logfile_name = argv[2]; }
  if (argc > 3) { fprintf(stderr, "Usage: %s [device_name [logfile_name]]\n", argv[0]); exit(-1); }

  // Create a log file of the video if we've been asked to
  if (logfile_name) {
    g_connection = vrpn_get_connection_by_name(device_name, logfile_name);
  }

  // Open the TempImager client and set the callback
  // for new data and for information about the size of
  // the image.
  printf("Opening %s\n", device_name);
  g_ti = new vrpn_TempImager_Remote(device_name);
  g_ti->register_description_handler(NULL, handle_description_message);
  g_ti->register_region_handler(NULL, handle_region_change);

  printf("Waiting to hear the image dimensions...\n");
  while (!g_got_dimensions) {
    g_ti->mainloop();
    vrpn_SleepMsecs(1);
  }
  if ( (g_image = new unsigned char[g_ti->nCols() * g_ti->nRows() * 3]) == NULL) {
    fprintf(stderr,"Out of memory when allocating image!\n");
    return -1;
  }
  g_ready_for_region = true;
  printf("Receiving images at size %dx%d\n", g_ti->nCols(), g_ti->nRows());

  // Initialize GLUT and create the window that will display the
  // video -- name the window after the device that has been
  // opened in VRPN.
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(g_ti->nCols(), g_ti->nRows());
  glutInitWindowPosition(100, 100);
  glutCreateWindow(vrpn_copy_service_name(device_name));

  // Set the display function and idle function for GLUT (they
  // will do all the work) and then give control over to GLUT.
  glutDisplayFunc(myDisplayFunc);
  glutIdleFunc(myIdleFunc);
  glutMainLoop();

  // Clean up objects and return.
  delete g_ti;
  if (g_image) { delete [] g_image; };
  return 0;
}
