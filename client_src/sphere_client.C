//
// sphere.C - simple VRPN client, inspired by Randy Heiland
//	generates a sphere with a radius of 3 cm 
//

#include <math.h>                       // for sqrt
#include <stdio.h>                      // for printf, NULL
#include <stdlib.h>                     // for atof, exit
#include <vrpn_Button.h>                // for vrpn_BUTTONCB, etc
#include <vrpn_ForceDevice.h>           // for vrpn_ForceDevice_Remote, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Types.h"                 // for vrpn_float64

static float xpos,ypos,zpos;


/*****************************************************************************
 *
   Callback handler
 *
 *****************************************************************************/

void    VRPN_CALLBACK handle_force_change(void * /*userdata*/, const vrpn_FORCECB f)
{
  static vrpn_FORCECB lr;        // last report
  static int first_report_done = 0;

  if ((!first_report_done) ||
    ((f.force[0] != lr.force[0]) || (f.force[1] != lr.force[1])
      || (f.force[2] != lr.force[2]))) {
    printf("force is (%f,%f,%f)\n", f.force[0], f.force[1], f.force[2]);
  }

  first_report_done = 1;
  lr = f;
}

vrpn_ForceDevice_Remote *forceDevice;

void    VRPN_CALLBACK handle_tracker_change(void * /*userdata*/, const vrpn_TRACKERCB t)
{
  static vrpn_TRACKERCB lr; // last report
  static float dist_interval_sq = (float)0.004;

  if ((lr.pos[0] - t.pos[0])*(lr.pos[0] - t.pos[0]) +
    (lr.pos[1] - t.pos[1])*(lr.pos[1] - t.pos[1]) +
    (lr.pos[2] - t.pos[2])*(lr.pos[2] - t.pos[2]) > dist_interval_sq){
    printf("Sensor %d is now at (%g,%g,%g)\n", t.sensor,
            t.pos[0], t.pos[1], t.pos[2]);
    lr = t;
  }
  xpos = (float)t.pos[0];
  ypos = (float)t.pos[1];
  zpos = (float)t.pos[2];

    // we may call forceDevice->set_plane(...) followed by
    //      forceDevice->sendSurface() here to change the plane
    // for example: using position information from a tracker we can
    //      compute a plane to approximate a complex surface at that
    //      position and send that approximation 15-30 times per
    //      second to simulate the complex surface

    double norm = sqrt(xpos*xpos + ypos*ypos + zpos*zpos);
    double radius = .03;        // radius is 3 cm
    forceDevice->set_plane(xpos,ypos,zpos, (float)(-radius*norm));

    forceDevice->sendSurface();
}

void	VRPN_CALLBACK handle_button_change(void *userdata, const vrpn_BUTTONCB b)
{
  static int count=0;
  static int buttonstate = 1;
  int done = 0;

  if (b.state != buttonstate) {
    printf("button #%d is in state %d\n", b.button, b.state);
    buttonstate = b.state;
    count++;
  }
  if (count > 4)
    done = 1;

  *(int *)userdata = done;
}

int main(int argc, char *argv[])
{
  int     done = 0;
  vrpn_Tracker_Remote *tracker;
  vrpn_Button_Remote *button;

  if (argc != 4) {
    printf("Usage: %s sFric dFric device_name\n",argv[0]);
    printf("   Example: %s 0.1 0.1 Phantom@myhost.mydomain.edu\n",argv[0]);
    exit(-1);
  }
  float sFric = (float)atof(argv[1]);
  float dFric = (float)atof(argv[2]);
  char *device_name = argv[3];
  printf("Connecting to %s: sFric, dFric= %f %f\n",device_name, sFric,dFric);

  /* initialize the force device */
  forceDevice = new vrpn_ForceDevice_Remote(device_name);
  forceDevice->register_force_change_handler(NULL, handle_force_change);

  /* initialize the tracker */
  tracker = new vrpn_Tracker_Remote(device_name);
  tracker->register_change_handler(NULL, handle_tracker_change);

  /* initialize the button */
  button = new vrpn_Button_Remote(device_name);
  button->register_change_handler(&done, handle_button_change);

  // Wait until we are connected to the server.
  while (!forceDevice->connectionPtr()->connected()) {
      forceDevice->mainloop();
  }

  // Set plane and surface parameters.  Initially, the plane will be
  // far outside the volume so we don't get a sudden jerk at startup.
  forceDevice->set_plane(0.0, 1.0, 0.0, 100.0);

/*-------------------------------------------------------------
correct ranges for these values (from GHOST 1.2 documentation):
dynamic, static friction: 0-1.0
Kspring: 0-1.0
Kdamping: 0-0.005

An additional constraint that I discovered is that dynamic friction must
be smaller than static friction or you will get the same error.  "1.0" means
the maximum stable surface presentable by the device.
--------------------------------------------------------------*/
  forceDevice->setSurfaceKspring(1.0);

  forceDevice->setSurfaceKdamping(0.0);	// damping constant -
                                            // units of dynes*sec/cm

  forceDevice->setSurfaceFstatic(sFric); 	// set static friction
  forceDevice->setSurfaceFdynamic(dFric);	// set dynamic friction

  // texture and buzzing stuff:
  // this turns off buzzing and texture
  forceDevice->setSurfaceBuzzAmplitude(0.0);
  forceDevice->setSurfaceBuzzFrequency(60.0); // Hz
  forceDevice->setSurfaceTextureAmplitude(0.00); // meters!!!
  forceDevice->setSurfaceTextureWavelength((float)0.01); // meters!!!

  forceDevice->setRecoveryTime(10);	// recovery occurs over 10
                                        // force update cycles

  // enable force device and send first surface
  forceDevice->startSurface();

  printf("\n3cm sphere at the origin should be present always\n");
  printf("Press and release the Phantom button 3 times to exit\n");

  // main loop
  while (! done )
  {
    // Let the forceDevice send its planes to remote force device
    forceDevice->mainloop();

    // Let tracker receive position information from remote tracker
    tracker->mainloop();

    // Let button receive button status from remote button
    button->mainloop();
  }

  // shut off force device
  forceDevice->stopSurface();

  // Delete the objects
  delete forceDevice;
  delete button;
  delete tracker;

  return 0;
}   /* main */

