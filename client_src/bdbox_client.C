// test client for sgi button/dial box

/*
button layout:

    00 01 02 03
 04 05 06 07 08 09
 10 11 12 13 14 15
 16 17 18 19 20 21
 22 23 24 25 26 27
    28 29 30 31

dial layout:

 06 07
 04 05
 02 03
 00 01

*/

#include <stdio.h>                      // for printf, NULL
#include <vrpn_Analog.h>                // for vrpn_ANALOGCB, etc
#include <vrpn_Button.h>                // for vrpn_BUTTONCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Types.h"                 // for vrpn_float64
/*  
// this is the one in the PIT area
#define BDBOX_SERVER "sgibox0@x-vrsh://152.2.128.163/~seeger/src/vrpn/server_src/sgi_irix/vrpn_server,-f,~seeger/src/config/sgibox.cfg,-q" 
*/

#define BDBOX_SERVER "sgibox0@x-vrsh://nano//net/nano/nano1/bin/vrpn_server,-f,/net/nano/nano1/config/sgibdbox.cfg,-q,4501,>/dev/null"

/*****************************************************************************
 *
   Callback handler
 *
 *****************************************************************************/


void VRPN_CALLBACK	handle_button_change(void *userdata, const vrpn_BUTTONCB b)
{
	static int count=0;
	static int buttonstate = 1;

	if (b.state != buttonstate) {
	     printf("button #%d is in state %d\n", b.button, b.state);
	     buttonstate = b.state;
	     count++;
	}
	if (count > 20) {
		*(int*)userdata = 1;
	}
}

void VRPN_CALLBACK	handle_dial_change(void *, const vrpn_ANALOGCB info)
{
    static double channel_values[vrpn_CHANNEL_MAX];
    static bool initialized = false;

    int i;
    if (!initialized) {
      for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
        channel_values[i] = 0.0;
      }
      initialized = true;
    }

    for (i = 0; i < info.num_channel; i++){
      if (channel_values[i] != info.channel[i]) {
	    printf("dial #%d has value %f\n", i, (float)(info.channel[i])); 
      }
	  channel_values[i] = info.channel[i];
    }
}

int main(int, char *[])
{
    int     done = 0;
	vrpn_Analog_Remote *bd_dials;
	vrpn_Button_Remote *bd_buttons;

	printf("Connecting to sgi button/dial box:\n" BDBOX_SERVER "\n");

	// initialize the buttons
	bd_buttons = new vrpn_Button_Remote(BDBOX_SERVER);
	bd_buttons->register_change_handler(&done, handle_button_change);

	// initialize the dials
	bd_dials = new vrpn_Analog_Remote(BDBOX_SERVER);
	bd_dials->register_change_handler(NULL, handle_dial_change);


        // main loop
        while (!done ){
	    bd_dials->mainloop();
	    bd_buttons->mainloop();
        }

}   /* main */
