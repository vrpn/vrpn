//----------------------------------------------------------------------------
// Example program to read pixels from a vrpn_TempImager server and display
// them in an OpenGL window.  It assumes that the size of the imager does
// not change during the run.  It assumes that the pixels are actually 8-bit
// values (it will clip if they are larger).

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
    int r,c;	//< Row, Column
    int ir;		//< Inverted Row
    int offset,RegionOffset;
    const vrpn_TempImager_Region* region=info.region;

    int infoLineSize=region->cMax-region->cMin+1;
    vrpn_int32 nCols=g_ti->nCols();
    vrpn_uint16 uns_pix;

    if (!g_ready_for_region) { return; }

    // Copy pixels into the image buffer.  Flip the image over in
    // Y so that the image coordinates display correctly in OpenGL.
    for (r = info.region->rMin,RegionOffset=(r-region->rMin)*infoLineSize; r <= region->rMax; r++,RegionOffset+=infoLineSize) {
		ir = g_ti->nRows() - r - 1;
		for (c = info.region->cMin; c <= region->cMax; c++) {
			offset=3 * (c + ir*nCols);
#if 1
			// This assumes that the pixels are actually 8-bit values
			// and will clip if they go above this.  It also writes pixels
			// from all regions into the image, which is similar to
			// assuming that there is only one channel.  It also does
			// not scale or offset the pixels to get them into the
			// units for the region.
			uns_pix = region->vals[(c-region->cMin) + RegionOffset];
#else
			// This assumes that the pixels are actually 12-bit values
			// and will clip if they go above this.  It also writes pixels
			// from all regions into the image, which is similar to
			// assuming that there is only one channel.  It also does
			// not scale or offset the pixels to get them into the
			// units for the region.
			uns_pix = region->vals[(c-region->cMin) + RegionOffset] >> 4;
#endif
			g_image[0 + offset] = uns_pix;
			g_image[1 + offset] = uns_pix;
			g_image[2 + offset] = uns_pix;
		}
    }

    // Capture timing information and print out how many frames per second
    // are coming across the wire.

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
      gettimeofday(&last_print_time, NULL);
      first_time = false;
    } else {
      frame_count++;
      gettimeofday(&now, NULL);
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
