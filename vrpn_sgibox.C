#ifdef sgi

#include "vrpn_sgibox.h"
#include <stdio.h>

// all for gethostbyname();
#include <unistd.h>
static int sgibox_con_cb(void * userdata, vrpn_HANDLERPARAM p);
static int sgibox_alert_handler(void * userdata, vrpn_HANDLERPARAM);


vrpn_SGIBox::vrpn_SGIBox(char * name, vrpn_Connection * c):
  vrpn_Analog(name, c), vrpn_Button_Filter(name, c) {
    int ret;
    char hn[128];
    sid  = -1;
    winid =  -1;

    if (winid != -1) {
      printf("Closing previous windows Winid=%d, sid = %d:\n",winid, sid);
      winclose(winid);
      //dglclose(sid);
      dglclose(-1); // close all dgl connection;
    }

    ret = gethostname(hn, 100);
    if (ret < 0) {
      fprintf(stderr, "vrpn_SGIBox: error in gethostname()\n");
      return;
    }
    sid = ret = dglopen(hn,DGLLOCAL);
    if (ret < 0) {
      fprintf(stderr, "vrpn_SGIBox: error in dglopen()\n");
      return;
    }
    noport();
    winid = winopen("");
    reset();
    num_channel = NUM_DIALS;
    num_buttons = NUM_BUTTONS;
    
    register_autodeleted_handler(c->register_message_type( vrpn_got_first_connection), sgibox_con_cb, this);
    register_autodeleted_handler(alert_message_id,sgibox_alert_handler, this);
    set_alerts(1);	//turn on alerts from toggle filter class to notify
			//local sgibox that lights should be turned on/off
}

void vrpn_SGIBox::reset() {  /* Button/Dial box setup */
  int i;
  
  for (i=0; i<NUM_DIALS; i++) devs[i] = DIAL0+i;
  for (i=0; i<NUM_BUTTONS; i++) devs[i+NUM_DIALS] = SW0+i;
  btstat = 0;  /* Set all on/off buttons to off */
  //fprintf(stderr, "vrpn_SGIBox::reset %d\n", __LINE__);
  setdblights(btstat); /* Make the lights reflect this */
  //fprintf(stderr, "vrpn_SGIBox::reset %d\n", __LINE__);
  getdev(NUMDEVS, devs, vals1);  /* Get initial values */

  for (i=0; i<NUM_BUTTONS; i++)
    lastbuttons[i] = vals1[NUM_DIALS+i];

  for (i=0; i<NUM_DIALS; i++) mid_values[i] = vals1[i], last[i] = 0;
}

void vrpn_SGIBox::get_report() {
  int i;
  getdev(NUMDEVS, devs, vals1); /* read button/dial boxes */
  //fprintf(stderr, "Button=");
  for (i=0; i< NUM_BUTTONS; i++) {
    buttons[i] = vals1[NUM_DIALS+i];
  }
  for (i=0; i< NUM_DIALS; i++) {
    int temp = vals1[i] -mid_values[i];
    if (temp > 200) channel[i] = 0.5, mid_values[i] = vals1[i] - 200; 
    else if (temp < -200) channel[i] = -0.5, mid_values[i] = vals1[i]
			    + 200;
    else 
        channel[i] = temp/400.0;
      //fprintf(stderr, " %d", vals1[i]);
  }
  //fprintf(stderr, "\n");

  vrpn_Analog::report_changes();
  vrpn_Button_Filter::report_changes();
}

void vrpn_SGIBox::mainloop() {
  server_mainloop();
  get_report();
}


static int sgibox_alert_handler(void * userdata, vrpn_HANDLERPARAM){
  int i;
  long lights;
  vrpn_SGIBox*me=(vrpn_SGIBox *)userdata;

  lights=0;
  for(i=0;i<NUM_BUTTONS;i++){
    if(me->buttonstate[i]==vrpn_BUTTON_TOGGLE_ON) lights=lights|1<<i;
  }
  setdblights(lights);
  return 0;

}

static int sgibox_con_cb(void * userdata, vrpn_HANDLERPARAM)
{
     
  printf("vrpn_SGIBox::Get first new connection, reset the box\n");
  ((vrpn_SGIBox *)userdata) ->reset();
  return 0;
}

#endif  // sgi


