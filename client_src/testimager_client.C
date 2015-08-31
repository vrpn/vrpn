//----------------------------------------------------------------------------
// Example program to read pixels from a vrpn_Imager server and display
// them in an OpenGL window.  It assumes that the size of the imager does
// not change during the run.  It asks for unsigned 8-bit pixels.

#include <stdio.h>                      // for printf, NULL, fprintf, etc
#include <stdlib.h>                     // for exit
#include <string.h>                     // for strcmp

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include <vrpn_Connection.h>            // for vrpn_Connection, etc
#include <vrpn_FileConnection.h>
#include <vrpn_Imager.h>                // for vrpn_Imager_Remote, etc

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/OpenGL.h>
#else
#include <GL/gl.h>                      // for glClear, glClearColor, etc
#include <GL/glut.h>                    // for glutCreateWindow, etc   // IWYU pragma: keep
#endif

//----------------------------------------------------------------------------
// Glut insists on taking over the whole application, leaving us to use
// global variables to pass information between the various callback
// handlers.

bool			g_quit = false;	//< Set to true when time to quit
vrpn_Connection	*g_connection;	//< Set if logging is enabled.
vrpn_Imager_Remote	*g_imager;	//< Imager client object
bool g_got_dimensions = false;		//< Heard image dimensions from server?
int			g_Xdim, g_Ydim;	//< Dimensions in X and Y
bool g_ready_for_region = false;	//< Everything set up to handle a region?
unsigned char *g_image = NULL;		//< Pointer to the storage for the image
bool g_already_posted = false;		//< Posted redisplay since the last display?
bool g_autoscale = false;           //< Should we auto-scale the brightness and contrast?

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
    // it into that channel.
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
    // logging and because the program is killed to exit.
    if (g_connection) { g_connection->save_log_so_far(); }

    // We do not post a redisplay here, because we want to do that only
    // when we've gotten the end of a frame.  It is done in the
    // end_of_frame message handler.
}

void  VRPN_CALLBACK handle_end_of_frame(void *,const struct _vrpn_IMAGERENDFRAMECB)
{
    // Tell Glut it is time to draw.  Make sure that we don't post the redisplay
    // operation more than once by checking to make sure that it has been handled
    // since the last time we posted it.  If we don't do this check, it gums
    // up the works with tons of redisplay requests and the program won't
    // even handle windows events.

    // NOTE: This exposes a race condition.  If more video messages arrive
    // before the frame-draw is executed, then we'll end up drawing some of
    // the new frame along with this one.  To make really sure there is not tearing,
    // double buffer: fill partial frames into one buffer and draw from the
    // most recent full frames in another buffer.  You could use an OpenGL texture
    // as the second buffer, sending each full frame into texture memory and
    // rendering a textured polygon.

    if (!g_already_posted) {
      g_already_posted = true;
      glutPostRedisplay();
    }
}

//----------------------------------------------------------------------------
// Capture timing information and print out how many frames per second
// are being drawn.  Remove this function if you don't want timing info.
void print_timing_info(void)
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

//----------------------------------------------------------------------------
// Auto-scale the image so that the darkest pixel is 0 and the brightest
// is 255.  This is to provide a function that someone using this program
// to watch a microscope video wanted -- we're starting down the slippery
// slope of turning this from an example program into an application...

void do_autoscale(void)
{
  // Find the minimum and maximum value of all pixels of all colors
  // in the image.
  unsigned char min_val = g_image[0];
  unsigned char max_val = min_val;
  int x,y,c;
  for (x = 0; x < g_Xdim; x++) {
    for (y = 0; y < g_Ydim; y++) {
      for (c = 0; c < 3; c++) {
        unsigned char val = g_image[c + 3 * ( x + g_Xdim * ( y ) )];
        if (val < min_val) { min_val = val; }
        if (val > max_val) { max_val = val; }
      }
    }
  }

  // Compute the scale and offset to apply to map the minimum value to
  // zero and the maximum value to 255.
  float offset = min_val;
  float scale;
  if (max_val == min_val) {
    scale = 1;
  } else {
    scale = 255.0 / (max_val - min_val);
  }

  // Apply this scaling to each pixel.
  for (x = 0; x < g_Xdim; x++) {
    for (y = 0; y < g_Ydim; y++) {
      for (c = 0; c < 3; c++) {
        float val = g_image[c + 3 * ( x + g_Xdim * ( y ) )];
        val = (val - offset) * scale;
        g_image[c + 3 * ( x + g_Xdim * ( y ) )] = static_cast<unsigned char>(val);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Glut callback handlers.

void myDisplayFunc(void)
{
  // Clear the window and prepare to draw in the back buffer.
  // This is not strictly necessary, because we're going to overwrite
  // the entire window without Z buffering turned on.
  glDrawBuffer(GL_BACK);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  // If we're auto-scaling the image, do so now.
  if (g_autoscale) {
    do_autoscale();
  }

  // Store the pixels from the image into the frame buffer
  // so that they cover the entire image (starting from lower-left
  // corner, which is at (-1,-1)).
  glRasterPos2f(-1, -1);
  glDrawPixels(g_imager->nCols(),g_imager->nRows(), GL_RGB, GL_UNSIGNED_BYTE, g_image);

  // Swap buffers so we can see it.
  glutSwapBuffers();

  // Capture timing information and print out how many frames per second
  // are being drawn.
  print_timing_info();

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
    if (g_image) { delete [] g_image; g_image = NULL; };
    exit(0);
  }
}

void myKeyboardFunc(unsigned char key, int /*x*/, int /*y*/)
{
  switch (key) {
  case 27:  // Escape
  case 'q':
  case 'Q':
    g_quit = 1;
    break;

  case 'a':
  case 'A':
    g_autoscale = !g_autoscale;
    printf("Turning autoscaling %s\n", g_autoscale?"on":"off" );
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
  char  default_imager[] = "TestImage@localhost";
  char	*device_name = default_imager;
  char	*logfile_name = NULL;
  int i;

  // Parse the command line.  If there is one argument, it is the device
  // name.  If there is a second, it is a logfile name.
  if (argc >= 2) { device_name = argv[1]; }
  if (argc >= 3) { logfile_name = argv[2]; }
  if (argc > 3) { fprintf(stderr, "Usage: %s [device_name [logfile_name]]\n", argv[0]); exit(-1); }

  // In case we end up loading an video from a file, make sure we
  // neither pre-load nor accumulate the images.
  vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD = false;
  vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE = false;

  // Say that we've posted a redisplay so that the callback handler
  // for endframe doesn't try to post one before glut is open.
  g_already_posted = true;

  // Create a log file of the video if we've been asked to do logging.
  // This has the side effect of having the imager also use this same
  // connection, because VRPN maps the same connection name to the
  // same one rather than creating a new one.
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
  g_imager->register_end_frame_handler(g_imager, handle_end_of_frame);

  printf("Waiting to hear the image dimensions...\n");
  while (!g_got_dimensions) {
    g_imager->mainloop();
    vrpn_SleepMsecs(1);
  }

  // Because the vrpn_Imager server doesn't follow "The VRPN Way" in that
  // it will continue to attempt to flood the network with more data than
  // can be sent, we need to tell the client's connection to stop handling
  // incoming messages after some finite number, to avoid getting stuck down
  // in the imager's mainloop() and never returning control to the main
  // program.  This strange-named function does this for us.  If the camera
  // is not sending too many messages for the network, this should not have
  // any effect.
  g_imager->connectionPtr()->Jane_stop_this_crazy_thing(50);

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
  printf("Press '0'-'9' in OpenGL window to throttle incoming images.\n");
  printf("Press '-' to disable throttling.\n");
  printf("Press 'a' to enable/disable autoscaling of brightness.\n");
  printf("Press 'q' or 'Q' or ESC to quit.\n");

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
