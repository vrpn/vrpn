/*                               -*- Mode: C -*- 
 * 
 * 
 * Author          : Ruigang Yang
 * Created On      : Thu Apr 30 11:08:23 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Thu May 14 10:41:07 1998
 * Update Count    : 50
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_sgibox.C,v $
 * $Date: 1998/05/14 14:45:25 $
 * $Author: ryang $
 * $Revision: 1.3 $
 * 
 * $Log: vrpn_sgibox.C,v $
 * Revision 1.3  1998/05/14 14:45:25  ryang
 * modified vrpn_sgibox, so output is clamped to +-0.5, max 2 rounds
 *
 * Revision 1.1  1998/05/06 18:00:40  ryang
 * v0.1 of vrpn_sgibox
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

static char rcsid[] = "$Id: vrpn_sgibox.C,v 1.3 1998/05/14 14:45:25 ryang Exp $";

#include "vrpn_sgibox.h"
#include <stdio.h>

// all for gethostbyname();
#include <unistd.h>
int sgibox_con_cb(void * userdata, vrpn_HANDLERPARAM p);


vrpn_SGIBox::vrpn_SGIBox(char * name, vrpn_Connection * c):
  vrpn_Analog(name, c), vrpn_Button(name, c) {
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
    
    c->register_handler(c->register_message_type( vrpn_got_first_connection),
		      sgibox_con_cb, this);
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
    //fprintf(stderr, " %d", buttons[i]);
  }
  //fprintf(stderr, "\nChannel=");
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
  vrpn_Button::report_changes();
}

void vrpn_SGIBox::mainloop() {
  get_report();
}
static int sgibox_con_cb(void * userdata, vrpn_HANDLERPARAM)
{
     
  printf("vrpn_SGIBox::Get first new connection, reset the box\n");
  ((vrpn_SGIBox *)userdata) ->reset();
  return 0;
}









