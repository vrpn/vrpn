/*                               -*- Mode: C -*- 
 * 
 * 
 * Author          : Ruigang Yang
 * Created On      : Thu Apr 30 11:08:23 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Wed May  6 13:48:17 1998
 * Update Count    : 28
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_sgibox.C,v $
 * $Date: 1998/05/06 18:00:40 $
 * $Author: ryang $
 * $Revision: 1.1 $
 * 
 * $Log: vrpn_sgibox.C,v $
 * Revision 1.1  1998/05/06 18:00:40  ryang
 * v0.1 of vrpn_sgibox
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

static char rcsid[] = "$Id: vrpn_sgibox.C,v 1.1 1998/05/06 18:00:40 ryang Exp $";

#include "vrpn_sgibox.h"
#include <stdio.h>

// all for gethostbyname();
#include <unistd.h>




vrpn_SGIBox::vrpn_SGIBox(char * name, vrpn_Connection * c):
  vrpn_Analog(name, c), vrpn_Button(name, c) {
    int ret;
    char hn[100];
    
    fprintf(stderr, "CTOR of vrpn_SGIBox\n");
    ret = gethostname(hn, 100);
    if (ret < 0) {
      fprintf(stderr, "vrpn_SGIBox: error in gethostname()\n");
      return;
    }
    ret = dglopen(hn,DGLLOCAL);
    if (ret < 0) {
      fprintf(stderr, "vrpn_SGIBox: error in dglopen()\n");
      return;
    }
    noport();
    winopen("");
    reset();
    num_channel = NUM_DIALS;
    num_buttons = NUM_BUTTONS;
}

void vrpn_SGIBox::reset() {  /* Button/Dial box setup */
  int i;
  

  for (i=0; i<NUM_DIALS; i++) devs[i] = DIAL0+i;
  for (i=0; i<NUM_BUTTONS; i++) devs[i+NUM_DIALS] = SW0+i;
  btstat = 0;  /* Set all on/off buttons to off */
  fprintf(stderr, "vrpn_SGIBox::reset %d\n", __LINE__);
  setdblights(btstat); /* Make the lights reflect this */
  fprintf(stderr, "vrpn_SGIBox::reset %d\n", __LINE__);
  getdev(NUMDEVS, devs, vals1);  /* Get initial values */

  for (i=0; i<NUM_BUTTONS; i++)
    lastbuttons[i] = vals1[NUM_DIALS+i];

  for (i=0; i<NUM_DIALS; i++) last[i] = vals1[i];
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
      channel[i] = vals1[i];
      //fprintf(stderr, " %d", vals1[i]);
  }
  //fprintf(stderr, "\n");

  vrpn_Analog::report_changes();
  vrpn_Button::report_changes();
}

void vrpn_SGIBox::mainloop() {
  get_report();
}










