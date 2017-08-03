
#include <stdlib.h>
#include <stdio.h>
#include <vrpn_ForceDevice.h>
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>


#define PHANTOM_SERVER "Tracker0@localhost"

/*****************************************************************************
 *
   Callback handler
 *
 *****************************************************************************/

void    handle_force_change(void *userdata, const vrpn_FORCECB f)
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

void    handle_tracker_change(void *userdata, const vrpn_TRACKERCB t)
{
  static vrpn_TRACKERCB lr; // last report
  static float dist_interval_sq = 0.04;
  float *pos = (float *)userdata;

  if ((lr.pos[0] - t.pos[0])*(lr.pos[0] - t.pos[0]) +
    (lr.pos[1] - t.pos[1])*(lr.pos[1] - t.pos[1]) +
    (lr.pos[2] - t.pos[2])*(lr.pos[2] - t.pos[2]) + dist_interval_sq){
    //printf("Sensor %d is now at (%g,%g,%g)\n", t.sensor,
     //       t.pos[0], t.pos[1], t.pos[2]);
    lr = t;
  }
  pos[0] = t.pos[0];
  pos[1] = t.pos[1];
  pos[2] = t.pos[2];
}

void    handle_button_change(void *userdata, const vrpn_BUTTONCB b)
{
  static int buttonstate = 1;

  if (b.state != buttonstate) {
    printf("button #%ld is in state %ld\n", b.button, b.state);
    buttonstate = b.state;
  }

  *(int *)userdata = buttonstate;
}

int main(int argc, char *argv[]) {
  int     done = 0;
  float pos[3];
  int     forceInEffect = 0;
  int     forceEnabled = 0;
  vrpn_ForceDevice_Remote *forceDevice;
  vrpn_Tracker_Remote *tracker;
  vrpn_Button_Remote *button;
  float t;

  /* initialize the force device */
  forceDevice = new vrpn_ForceDevice_Remote(PHANTOM_SERVER);
  forceDevice->register_force_change_handler(NULL, handle_force_change);

  /* initialize the tracker */
  tracker = new vrpn_Tracker_Remote(PHANTOM_SERVER);
  tracker->register_change_handler((void *)pos, handle_tracker_change);

  /* initialize the button */
  button = new vrpn_Button_Remote(PHANTOM_SERVER);
  button->register_change_handler((void *)&forceEnabled, handle_button_change);

  t=0.0;

  // main loop
  while (!done) {
    t+=0.001f;

    // Let the forceDevice send its messages to remote force device
    forceDevice->mainloop();

    // Let tracker receive position information from remote tracker
    tracker->mainloop();

    // Let button receive button status from remote button
    button->mainloop();

    forceEnabled = 1;

    if (forceEnabled) {
      float py = (0.05 * sin(t + pos[0]*40.0*VRPN_PI) + 0.05);
      if (pos[1] > py) {
        forceDevice->stopForceField();
        forceInEffect = 0;
      } else {
        float fy;

        forceDevice->setFF_Origin(pos[0],pos[1],pos[2]);

        fy = (py - pos[1]) * 14.0;
        // units = dynes
        forceDevice->setFF_Force(0.20 * cos(t + pos[0]*200.0*VRPN_PI),
                                 fy,
                                 0.0 * cos(    pos[2]*200.0*VRPN_PI));
        // set derivatives of force field:
        // units = dynes/meter
        forceDevice->setFF_Jacobian(
            -20.0*sin(t + pos[0]*200.0*VRPN_PI), 0, 0, 
             0, -14.0, 0, 
             0, 0, -0.0*VRPN_PI*sin(pos[2]*200.0*VRPN_PI)
        );
  
        forceDevice->setFF_Radius(0.02); // 2cm radius of validity
        forceDevice->sendForceField();
        forceInEffect = 1;
      }
    }
    else if (forceInEffect){
       forceDevice->stopForceField();
       forceInEffect = 0;
    }
  }
}   /* main */

