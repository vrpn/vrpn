//----------------------------------------------------------------------------
// Example program to read pixels from a vrpn_TempImager server and display
// them in an OpenGL window.  It assumes that the size of the imager does
// not change during the run.  It assumes that the pixels are actually 8-bit
// values (it will clip if they are larger).

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
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

// New pixels coming; fill them into the image and tell Glut to redraw.
void  handle_region_change(void *, const vrpn_IMAGERREGIONCB info)
{
  int r,c, ir;	  //< Row, Column, Inverted Row
  vrpn_uint16 uns_pix;

  if (!g_ready_for_region) { return; }

  // Copy pixels into the image buffer.  Flip the image over in
  // Y so that the image coordinates display correctly in OpenGL.
  for (r = info.region->rMin; r <= info.region->rMax; r++) {
    ir = g_ti->nRows() - r - 1;
    for (c = info.region->cMin; c <= info.region->cMax; c++) {
      if (!info.region->read_unscaled_pixel(c, r, uns_pix)) {
	fprintf(stderr, "Cannot read pixel from region\n");
	exit(-1);
      }
      
      // This assumes that the pixels are actually 8-bit values
      // and will clip if they go above this.  It also writes pixels
      // from all regions into the image, which is similar to
      // assuming that there is only one channel.  It also does
      // not scale or offset the pixels to get them into the
      // units for the region.
      g_image[0 + 3 * (c + g_ti->nCols() * ir)] = uns_pix;
      g_image[1 + 3 * (c + g_ti->nCols() * ir)] = uns_pix;
      g_image[2 + 3 * (c + g_ti->nCols() * ir)] = uns_pix;
    }
  }

  // Tell Glut it is time to draw
  glutPostRedisplay();
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
}

void myIdleFunc(void)
{
  // See if there are any more messages from the server and then sleep
  // a little while so that we don't eat the whole CPU.
  g_ti->mainloop();
  vrpn_SleepMsecs(1);
}

int main(int argc, char **argv)
{
  char	*device_name = "TestImage@radon-cs:4511";

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
  }
  if ( (g_image = new unsigned char[g_ti->nCols() * g_ti->nRows() * 3]) == NULL) {
    fprintf(stderr,"Out of memory when allocating image!\n");
    return -1;
  }
  g_ready_for_region = true;

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