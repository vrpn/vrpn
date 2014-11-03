#include <stdio.h>                      // for fprintf, stderr, NULL
#include <stdlib.h>                     // for getenv

#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_int32
#include "vrpn_Wanda.h"

// This will cause the value to be set when the program is run, before
// main() is called.  It will be the same for all vrpn_Wanda instances.
int vrpn_Wanda::dbug_wanda = getenv("DBUG_WANDA") ? 1 : 0;

// Returns time in seconds in a double.
inline double the_time()
{
    struct timeval ts;
    vrpn_gettimeofday(&ts, NULL);
    return (double)(ts.tv_sec + ts.tv_usec/1e6);
}

void
print_bits( char *buf, int num_bytes )
{
  for(int i=0;i<num_bytes;i++) {
    for(int b=7;b>=0;b--)
      fprintf(stderr,"%d ", (((1 << b) & ((int)buf[i])) ? 1 : 0));
  }
  fprintf(stderr,"\n");
}

void print_bits(unsigned char *buf, int n) { print_bits((char *)buf, n); }

// 
// Note: Wanda works at 1200 baud, 1 stopbit, CS7 (character size), and no parity
// 
vrpn_Wanda::vrpn_Wanda(char * name, 
		    vrpn_Connection * c, char * portname,int baud, 
			     vrpn_float64 update_rate):
      vrpn_Serial_Analog(name, c, portname, baud, 7), vrpn_Button_Filter(name, c),
      bytesread(0),
      first(1),
      index(0)
{ 
  num_buttons = 3;  // Wanda has 3 buttons
  num_channel = 2;
  for(int i=0; i<num_channel; i++) resetval[i] = -1;
  if (update_rate != 0) 
    MAX_TIME_INTERVAL = (long)(1000000/update_rate);
  else MAX_TIME_INTERVAL = -1;
  status = vrpn_ANALOG_RESETTING;

  // reset buttons & channels
  buttons[0] = buttons[1] = buttons[2] = 0;
  channel[0] = channel[1] = 0;

  // Set the time
  last_val_timestamp = the_time();
}

void
vrpn_Wanda::report_new_button_info()
{
	if (dbug_wanda) {
		fprintf(stderr, "buttons = %d %d %d\n", int(buttons[0]), int(buttons[1]), int(buttons[2]));
	}
	vrpn_Button::report_changes(); // report any button event;
}


void
vrpn_Wanda::report_new_valuator_info()
{
   last_val_timestamp = the_time();
   if (dbug_wanda) {
	   fprintf(stderr, "vals = %lf %lf\n", channel[0], channel[1]);
   }

   // Send the message on the connection;
   if (vrpn_Analog::d_connection) {
     char      msgbuf[1000];
     vrpn_int32        len = vrpn_Analog::encode_to(msgbuf);
#ifdef VERBOSE
     vrpn_Analog::print();
#endif
     if (vrpn_Analog::d_connection->pack_message(len, vrpn_Analog::timestamp,
                                  channel_m_id, vrpn_Analog::d_sender_id, msgbuf,
                                  vrpn_CONNECTION_LOW_LATENCY)) {
       fprintf(stderr,"Tracker: cannot write message: tossing\n");
     }
   } else {
         fprintf(stderr,"Tracker Fastrak: No valid connection\n");
   }
}

void vrpn_Wanda::mainloop(void)
{
   server_mainloop();

   if (first) {
      bytesread += vrpn_read_available_characters(serial_fd,buffer+bytesread,1024-bytesread);

      if (bytesread < 2) return;

      if (bytesread > 2) {
		  fprintf(stderr,"wanda huh?  expected 2 characters on opening (got %d)\n", bytesread);
	  } else {
         if (buffer[0] == 'M' && buffer[1] == '3') {
            fprintf(stderr,"Read init message from wanda\n");
         } else {
            fprintf(stderr,"vrpn_Wanda:  ERROR, expected 'M3' from wanda...\n");
         }
      }
      bytesread = 0;
      first = 0;
      return;
   }

   int new_button_info = 0;
   int new_valuator_info = 0;

   // read available characters into end of buffer
//   bytesread += vrpn_read_available_characters(serial_fd,buffer+bytesread,1024-bytesread);
   int num_read = vrpn_read_available_characters(serial_fd,buffer+bytesread,1024-bytesread);
#if 1
   if (dbug_wanda)
      if (num_read > 0)
         print_bits(buffer+bytesread, num_read);
#endif
   bytesread += num_read;

   // handling synching
   while( index == 0 && bytesread > 0 && !(buffer[0] & (1<<6)) ) {
      fprintf(stderr,"synching wanda\n");
      for(int i=0;i<bytesread-1;i++)
         buffer[i] = buffer[i+1];

      bytesread--;
   }

#if 1
   double curtime = the_time();
   // XXX - hack.  why doesn't wanda send a record when the user
   // XXX - isn't pushing the joystick ball?  we weren't getting
   // XXX - the last record
//   if (bytesread == 3 && index == 3 && curtime - last_val_timestamp > 0.2) {
   if (curtime - last_val_timestamp > 0.2) {
      int new_valuator_info = 0;

      if (channel[0] != 0) {
         channel[0] = 0;
         new_valuator_info = 1;
      }
      if (channel[1] != 0) {
         channel[1] = 0;
         new_valuator_info = 1;
      }

      if (new_valuator_info) {
		  if (dbug_wanda) {
            fprintf(stderr, "timeout:  %lf\n", curtime - last_val_timestamp);
		  }
		  report_new_valuator_info();
      }
   }
#endif

   // process all data in buffer
#if 1
   if (dbug_wanda)
      if (num_read > 0)
         fprintf(stderr, "\t(bytesread = %d)\n", bytesread);
#endif

   while( bytesread >= 3 && index < bytesread ) {
      // as soon as 3 bytes are available, report them...
      if (index == 0 && bytesread >= 3) {
         // update valuators & buttons #0 & #2

         signed char x  = static_cast<char>((buffer[1] | ((buffer[0]&3)  << 6)));
         signed char y  = static_cast<char>((buffer[2] | ((buffer[0]&12) << 4)));
         double xd = -((double)x) / 34.0;  // XXX - what is max range? 34?
         double yd =  ((double)y) / 34.0;

         if (xd > 1.0) xd = 1.0;
         if (xd <-1.0) xd =-1.0;
         if (yd > 1.0) yd = 1.0;
         if (yd <-1.0) yd =-1.0;

         if (channel[0] != xd) {
            channel[0] = xd;
            new_valuator_info = 1;
         }
         if (channel[1] != yd) {
            channel[1] = yd;
            new_valuator_info = 1;
         }


         // decode button data
         int blue_val = (buffer[0] & (1<<4)) ? 1 : 0; // blue button
         int red_val  = (buffer[0] & (1<<5)) ? 1 : 0; // red button

         if (blue_val != buttons[0]) {
            buttons[0] = static_cast<unsigned char>(blue_val);
            new_button_info = 1;
         }
         if (red_val != buttons[2]) {
            buttons[2] = static_cast<unsigned char>(red_val);
            new_button_info = 1;
         }


         index = 3;
      }

      // if index is at 3 & there are more bytes & the next byte
      // isn't the start of a new record, then process button info
      if (index == 3 && bytesread > 3 && !(buffer[3] & (1<<6))) {
         int new_val = (buffer[3]) ? 1 : 0; // yellow button

         if (new_val != buttons[1]) {
            buttons[1] = static_cast<unsigned char>(new_val);
            new_button_info = 1;
         }
   
         index = 4;
      } 

      if (new_button_info)
         report_new_button_info();

      if (new_valuator_info)
         report_new_valuator_info();

      // if next byte is start of new record, shift bytes
      if (index == 4 || (index == 3 && bytesread > 3 && (buffer[3] & (1<<6)))) {
         for(int i=index;i<bytesread;i++)
            buffer[i - index] = buffer[i];
         bytesread -= index;
         index = 0;
      }
   }
}
