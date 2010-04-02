#include <stdio.h>
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_ForceDevice.h"

void handle_tracker(void *userdata, const vrpn_TRACKERCB t) {
  printf("Sensor %3d is now at (%11f, %11f, %11f, %11f, %11f, %11f, %11f)           \r", 
         t.sensor, t.pos[0], t.pos[1], t.pos[2],
	 t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
  fflush(stdout);
}

void handle_button(void *userdata, const vrpn_BUTTONCB b) {
  printf("\nButton %3d is in state: %d                      \n", 
         b.button, b.state);
  fflush(stdout);  
}

void handle_force(void *userdata, const vrpn_FORCECB f) {
  static vrpn_FORCECB lr;        // last report
  static int first_report_done = 0;

  if ((!first_report_done) || ((f.force[0] != lr.force[0]) || 
      (f.force[1] != lr.force[1]) || (f.force[2] != lr.force[2])))
    printf("\nForce is (%11f, %11f, %11f)                  \n", 
           f.force[0], f.force[1], f.force[2]);

  first_report_done = 1;
  lr = f;
}



int main(int argc, char **argv) {       
  char * server;

  vrpn_Tracker_Remote *tkr;
  vrpn_Button_Remote *btn;
  vrpn_ForceDevice_Remote *fdv;


  if (argc < 2) {
    printf("%s: Invalid parms\n", argv[0]);
    printf("usage: \n");
    printf("  %s Tracker0@host\n", argv[0]);

    return -1;
  }
  server = argv[1];

  printf("Opening: %s ...\n",  server);
  tkr = new vrpn_Tracker_Remote(server);
  tkr->register_change_handler(NULL, handle_tracker);

  btn = new vrpn_Button_Remote(server);
  btn->register_change_handler(NULL, handle_button);

  fdv = new vrpn_ForceDevice_Remote(server);
  fdv->register_force_change_handler(NULL, handle_force);

  tkr->reset_origin();
  /* 
   * main interactive loop
   */
   while (1) {
     // Let the tracker do its thing
     tkr->mainloop();
     btn->mainloop();
     fdv->mainloop();
   }
}   /* main */
