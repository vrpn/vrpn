//
// ff_client.C - generates an interesting sinusoidal force field
//

#include <math.h>                       // for cos, sin
#include <stdio.h>                      // for printf, NULL
#include <vrpn_Button.h>                // for vrpn_BUTTONCB, etc
#include <vrpn_ForceDevice.h>           // for vrpn_ForceDevice_Remote, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Types.h"                 // for vrpn_float64

#define PHANTOM_SERVER "Tracker0@localhost"

/*****************************************************************************
 *
   Callback handler
 *
 *****************************************************************************/

void    VRPN_CALLBACK handle_force_change(void *, const vrpn_FORCECB f)
{
  static vrpn_FORCECB lr;        // last report
  static int first_report_done = 0;

  if ((!first_report_done) ||
    ((f.force[0] != lr.force[0]) || (f.force[1] != lr.force[1])
      || (f.force[2] != lr.force[2]))) {
    //printf("force is (%f,%f,%f)\n", f.force[0], f.force[1], f.force[2]);
  }

  first_report_done = 1;
  lr = f;
}

void    VRPN_CALLBACK handle_tracker_change(void *userdata, const vrpn_TRACKERCB t)
{
  static vrpn_TRACKERCB lr; // last report
  static float dist_interval_sq = 0.004f;
  float *pos = (float *)userdata;

  if ((lr.pos[0] - t.pos[0])*(lr.pos[0] - t.pos[0]) +
    (lr.pos[1] - t.pos[1])*(lr.pos[1] - t.pos[1]) +
    (lr.pos[2] - t.pos[2])*(lr.pos[2] - t.pos[2]) > dist_interval_sq){
    //printf("Sensor %d is now at (%g,%g,%g)\n", t.sensor,
     //       t.pos[0], t.pos[1], t.pos[2]);
    lr = t;
  }
  pos[0] = static_cast<float>(t.pos[0]);
  pos[1] = static_cast<float>(t.pos[1]);
  pos[2] = static_cast<float>(t.pos[2]);
}

void	VRPN_CALLBACK handle_button_change(void *userdata, const vrpn_BUTTONCB b)
{
  static int buttonstate = 1;

  if (b.state != buttonstate) {
    printf("button #%d is in state %d\n", b.button, b.state);
    buttonstate = b.state;
  }

  *(int *)userdata = buttonstate;
}

int main(int /* argc */, char * /* argv */ [])
{
  printf("Trying to connect to ForceDevice, Tracker, and Button named:\n");
  printf("%s\n", PHANTOM_SERVER);
  int     done = 0;
  float	pos[3];
  int	  forceInEffect = 0;
  int	  forceEnabled = 0;
  vrpn_ForceDevice_Remote *forceDevice;
  vrpn_Tracker_Remote *tracker;
  vrpn_Button_Remote *button;

  /* initialize the force device */
  forceDevice = new vrpn_ForceDevice_Remote(PHANTOM_SERVER);
  forceDevice->register_force_change_handler(NULL, handle_force_change);

  /* initialize the tracker */
  tracker = new vrpn_Tracker_Remote(PHANTOM_SERVER);
  tracker->register_change_handler((void *)pos, handle_tracker_change);

  /* initialize the button */
  button = new vrpn_Button_Remote(PHANTOM_SERVER);
  button->register_change_handler((void *)&forceEnabled, handle_button_change);

  // main loop
  while (! done )
  {
    // Let the forceDevice send its messages to remote force device
    forceDevice->mainloop();

    // Let tracker receive position information from remote tracker
    tracker->mainloop();

    // Let button receive button status from remote button
    button->mainloop();

    if (forceEnabled) {
       forceDevice->setFF_Origin(pos[0],pos[1],pos[2]);
       // units = dynes
       forceDevice->setFF_Force((float)cos(pos[0]*20.0*VRPN_PI),
			(float)cos(pos[1]*20.0*VRPN_PI),(float)cos(pos[2]*20.0*VRPN_PI));
       // set derivatives of force field:
       // units = dynes/meter
       forceDevice->setFF_Jacobian((float)(-20.0*VRPN_PI*sin(pos[0]*20.0*VRPN_PI)), 0, 0, 
				    0, (float)(-20.0*VRPN_PI*sin(pos[1]*20.0*VRPN_PI)), 0, 
				    0, 0, (float)(-20.0*VRPN_PI*sin(pos[2]*20.0*VRPN_PI)));
       forceDevice->setFF_Radius(0.05f);	// 5cm radius of validity
       forceDevice->sendForceField();
       forceInEffect = 1;
    }
    else if (forceInEffect){
       forceDevice->stopForceField();
       forceInEffect = 0;
    }
  }

}   /* main */
