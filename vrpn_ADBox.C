// -*- Mode:C++ -*-

/* 
 *  ad-box driver
 *  works with Fraunhofer IMK AD-Box and Fakespace Cubic Mouse
 *
 *  for additional information see:
 *  http://www.imk.fraunhofer.de
 *  http://www.fakespace.com
 *
 *  written by Sascha Scholz <sascha.scholz@imk.fraunhofer.de>
 */

#include <stdio.h>                      // for fprintf, stderr

#include "vrpn_ADBox.h"
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_int32

class VRPN_API vrpn_Connection;

vrpn_ADBox::vrpn_ADBox(char* name, vrpn_Connection *c,
                       const char *port, long baud)
  : vrpn_Analog(name, c), vrpn_Button_Filter(name, c),
    ready(1), serial_fd(0), iNumBytes(0), iNumDigBytes(0), iFilterPos(0)
{
  // Open the serial port
  if ( (serial_fd=vrpn_open_commport(port, baud)) == -1) {
    fprintf(stderr,"vrpn_ADBox: Cannot Open serial port\n");
    ready = 0;
  }
  
  // find out what time it is - needed?
  vrpn_gettimeofday(&timestamp, 0);
  vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;
}


vrpn_ADBox::~vrpn_ADBox()
{
  vrpn_close_commport(serial_fd);
}


void vrpn_ADBox::mainloop()
{
  // Call the generic server mainloop, since we are a server
  server_mainloop();

  struct timeval timeout = {0,200000};
  float fAnalog(0.0f);

  if(!ready) return;
  
  if (iNumBytes == 0)
    {
      vrpn_flush_output_buffer(serial_fd);
      vrpn_flush_input_buffer(serial_fd);

      buffer[0] = 'U';
      vrpn_write_characters(serial_fd, buffer, 1);
      
      while (iNumBytes < 100 &&  vrpn_read_available_characters(serial_fd, buffer, 1,&timeout) == 1)
        iNumBytes++;
      
      switch (iNumBytes)
        {
        case 14:
        case 18: iNumDigBytes = 2; break;
        case 19: iNumDigBytes = 3; break;
        default: iNumBytes = 0; iNumDigBytes = 0; break;
        }
      
      if (iNumBytes != 0)
        {
          num_buttons = iNumDigBytes * 8;
          num_channel = (iNumBytes - iNumDigBytes) / 2;         
 
          fprintf(stderr,
                  "vrpn_ADBox: ad-box with %d digital and %d analog ports detected\n",
                  num_buttons, num_channel);
          
          buffer[0] = 'U';
          vrpn_write_characters(serial_fd, buffer, 1);
          ready = 1;
          
          // initialize the buttons and channels
          for (vrpn_int32 i=0; i<num_buttons; i++)
            buttons[i] = lastbuttons[i] = VRPN_BUTTON_OFF;
          for (vrpn_int32 j=0; j<num_channel; j++) {
                channel[j] = last[j] = 0;
        }

 
        }
      else
        {
          fprintf(stderr,"vrpn_ADBox: trying to detect ad-box\n");
          vrpn_SleepMsecs(1000.0*1);
        }
    }
  else
    {
      buffer[0] = 'U';
      vrpn_write_characters(serial_fd, buffer, 1);

      for (int c = 0; c < iNumBytes; c++)
        {
          if (vrpn_read_available_characters(serial_fd, &buffer[c], 1,&timeout) != 1)
            {
              fprintf(stderr,"vrpn_ADBox: could only read %d chars, %d expected\n",c,iNumBytes+1);
              iNumBytes = 0;
              iNumDigBytes = 0;
              break;
            }
        }

      vrpn_gettimeofday(&timestamp, 0);
      vrpn_Analog::timestamp = timestamp;
      vrpn_Button::timestamp = timestamp;
      
      if (iNumBytes != 0)
        {
          int i(0);
          for (i = 0; i < iNumDigBytes; i++)
            for (int b = 0; b < 8; b++)
              buttons[8 * i + b] = (unsigned char) ((buffer[i] & (1 << b)) != 0 ? VRPN_BUTTON_OFF : VRPN_BUTTON_ON);
          
          for (i = 0; i < (iNumBytes - iNumDigBytes) / 2; i++)
            {
              int iOrg = iFilter[i][iFilterPos] =
                (int(buffer[iNumDigBytes + i * 2]) << 8) | int(buffer[iNumDigBytes + 1 + i * 2]);
              fAnalog = float(iOrg) / 1023.0f;
              
              int p(iFilterPos);
              do p = (p + 29) % 30; while (p != iFilterPos && iFilter[i][p] == iOrg);
              if (p != iFilterPos)
                {
                  int iLast(iFilter[i][p]);
                  if (iOrg - iLast == 1 || iLast - iOrg == 1)
                    {
                      do p = (p + 29) % 30; while (p != iFilterPos && iFilter[i][p] == iLast);
                      if (p != iFilterPos && iFilter[i][p] == iOrg)
                        fAnalog = float(iOrg + iLast) / 2046.0f;
                    }
                }

              channel[i] = fAnalog;
            }
          
          iFilterPos = (iFilterPos + 1) % 30;
          
          vrpn_Button::report_changes();
          vrpn_Analog::report_changes();
        }
    }
}
