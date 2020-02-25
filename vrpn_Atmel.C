/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2003 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  vrpn_Atmel                                                                       */
/*  project    :  vrpn_Avango                                                                      */
/*  description:  server for microcontroller based on Atmel chip                                   */
/*                hardware developed by Albotronic: www.albotronic.de                              */
/*                                                                                                 */
/***************************************************************************************************/

#ifndef _WIN32

#include <errno.h>                      // for errno
#include <stdio.h>                      // for fprintf, stderr, printf, etc
#include <stdlib.h>                     // for exit
#include <sys/select.h>                 // for select, FD_SET, FD_ZERO, etc
#include <vrpn_Shared.h>                // for vrpn_gettimeofday

#include "vrpn_Atmel.h"
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Types.h"                 // for vrpn_float64
#include "vrpn_atmellib.h"              // for getRegister, closePort, etc
#include "vrpn_atmellib_errno.h"        // for ATMELLIB_NOERROR

#include <termios.h>                    // for tcflush, TCIOFLUSH, termios

#include <string.h>                     // for strerror

/***************************************************************************************************/
/***************************************************************************************************/
namespace {

struct termios init_params;
struct timeval wait;

/***************************************************************************************************/
// check if there if the Atmel is connected to the serial 
/***************************************************************************************************/
bool
check_serial(int fd)
{
  int ret=0;
	  
  // make a new fd set to watch
  fd_set  rfds;

  // clear the set
  FD_ZERO( &rfds );
  // ad the current fd to the set
  FD_SET( fd, &rfds );
    
  wait.tv_sec = 5;
  wait.tv_usec = 0;
    
  ret = select( fd+1 , &rfds , 0 , 0 , &wait);
    
  if (ret == 0) {

    printf("Atmel not connected to the specified port.\n");
    printf("Connect and try again.\n");

    return false;	  
  }
  else if (ret<0) {

    printf("Error while checking if Atmel is connected.\n");
    printf("Error: %s (%i)\n", strerror(errno), errno );

    return false;
  }

  printf("Atmel started.\n\n");
     
  return true; 
}

}  // end of namespace

/***************************************************************************************************/
/* factory */
/***************************************************************************************************/
/* static */ vrpn_Atmel *
vrpn_Atmel::Create(char* name, vrpn_Connection *c,
                    const char *port, long baud,
		    int channel_count ,
		    int * channel_mode)
{	
  int fd;

#ifdef VRPN_ATMEL_SERIAL_VRPN

  if ( (fd=vrpn_open_commport(port, baud)) == -1) {
    
    fprintf(stderr,"vrpn_Atmel: can't Open serial port\n");
  
    return NULL; 
  }
#else
  // Open the serial port
  if ( (fd=openPort(port, baud, &init_params)) == -1) {
    
    fprintf(stderr,"vrpn_Atmel: can't Open serial port\n");
  
    return NULL; 
  }
  
  // look if the atmel is connected
  if (! check_serial(fd) ) { 

    return NULL; 
  }
#endif 
 
  vrpn_Atmel * self = NULL;
  try { self = new vrpn_Atmel(name, c, fd); }
  catch (...) { return NULL; }
      
  if ( (self->vrpn_Analog_Server::setNumChannels(channel_count) != channel_count) 
     || (self->vrpn_Analog_Output_Server::setNumChannels(channel_count) != channel_count) ) {
  
      fprintf(stderr,"vrpn_Atmel: the requested number of channels is not available\n");
      try {
        delete self;
      } catch (...) {
        fprintf(stderr, "vrpn_Atmel::Create(): delete failed\n");
        return NULL;
      }

      return NULL;
  }
  

  // init the channels based on the infos given from the config file
  self->init_channel_mode(channel_mode);
 
  self->_status = VRPN_ATMEL_STATUS_WAITING_FOR_CONNECTION; 

  return self;
}



/***************************************************************************************************/
/* constructor */
/***************************************************************************************************/
vrpn_Atmel::vrpn_Atmel(char* name, vrpn_Connection *c, int fd) 
  : vrpn_Analog_Server(name, c), vrpn_Analog_Output_Server(name, c),
    _status(VRPN_ATMEL_STATUS_ERROR), 
    serial_fd(fd)
{	
  // find out what time it is - needed?
  vrpn_gettimeofday(&timestamp, 0);
  vrpn_gettimeofday(&_time_alive, 0);
  vrpn_Analog::timestamp = timestamp;
}


/***************************************************************************************************/
/* destructor */
/***************************************************************************************************/
vrpn_Atmel::~vrpn_Atmel()
{
#ifdef VRPN_ATMEL_SERIAL_VRPN
  vrpn_close_commport(serial_fd);
#else
  closePort(serial_fd , &init_params);      
#endif
  
  
}

/***************************************************************************************************/
/* mainloop */
/***************************************************************************************************/
void 
vrpn_Atmel::mainloop()
{
  if (_status == VRPN_ATMEL_STATUS_ERROR) {

    return;
  }
	
  // o_num_channel is used as reference against num_channel
  if (o_num_channel != num_channel) {

    num_channel = o_num_channel;
  }

  server_mainloop();
  
  // get the pending messages so that the buffer to write down is updated
  d_connection->mainloop();
  
  if ( ! d_connection->connected()) {

    return;
  }
  
  // it's the first time we are in mainloop after 
  if ( _status == VRPN_ATMEL_STATUS_WAITING_FOR_CONNECTION) {
	  
    if (handle_new_connection()) {
  
      _status = VRPN_ATMEL_STATUS_RUNNING;
    
      fprintf(stderr,"vrpn_Atmel: mainloop()\n");
      fprintf(stderr,"  new connection. status set to RUNNING\n");
    }
    else {
   
      _status = VRPN_ATMEL_STATUS_ERROR;
    
      fprintf(stderr,"vrpn_Atmel: mainloop()\n");
      fprintf(stderr,"  error handling new connection. status set to ERROR\n");
   
      return; 
    }
    
  }

  // do the read/write operations on the serial port   
  if ( ! mainloop_serial_io() ) {
    
    fprintf(stderr,"vrpn_Atmel: mainloop()\n");
    fprintf(stderr,"  error while io operations ERROR\n");

  }
  
  vrpn_Analog::report();
  //vrpn_Analog::report_changes();

  d_connection->mainloop();
}
 
/***************************************************************************************************/
/* check if the connection to the Atmel is still reliable */
/***************************************************************************************************/
bool
vrpn_Atmel::Check_Serial_Alive()
{
  struct timeval look_time;

  // check serial status every second
  if ((timestamp.tv_sec - _time_alive.tv_sec) > VRPN_ATMEL_ALIVE_INTERVAL_SEC) {

    // reset time alive
    vrpn_gettimeofday(&_time_alive,0);

    tcflush(serial_fd, TCIOFLUSH);

    // make a new fd set to watch
    fd_set  rfds;

    // clear the set
    FD_ZERO( &rfds );
    // ad the current fd to the set
    FD_SET( serial_fd, &rfds );

    look_time.tv_sec = VRPN_ATMEL_ALIVE_TIME_LOOK_SEC;
    look_time.tv_usec = VRPN_ATMEL_ALIVE_TIME_LOOK_USEC;

    if ((select( (serial_fd+1) , &rfds , 0 , 0 , &look_time)) <= 0) {

      fprintf(stderr, "\nExiting...\n");

      fprintf(stderr, "vrpn_Atmel::Check_Serial_Alive: connection timed out after (sec,msec):");
      fprintf(stderr, "%i,%i\n", VRPN_ATMEL_ALIVE_TIME_LOOK_SEC, VRPN_ATMEL_ALIVE_TIME_LOOK_USEC);
      fprintf(stderr, "Killing Program!!!\n");

      return false;
    }

#ifdef VRPN_ATMEL_VERBOSE 
    fprintf(stderr, "Connection still available...\n");
#endif
  }

  return true;
}
 
/***************************************************************************************************/
/* io operations on the serial port in mainloop */
/***************************************************************************************************/
bool
vrpn_Atmel::mainloop_serial_io() 
{
  vrpn_gettimeofday(&timestamp, 0);
  vrpn_Analog::timestamp = timestamp;

  // check if there is still a valid connection to the Chip
  if (!Check_Serial_Alive()) {
      
    // kill the program
    // use e.g. a cron job to init the whole connection mechanism again
    exit(0);
  } 

  // do for all channels
  for(int i=0; i<o_num_channel; ++i) {
	  
    if (o_channel[i] == VRPN_ATMEL_CHANNEL_NOT_VALID) {

      // unused channel
      continue;
    }
	  
    // find out which channels have to been written down to the device
    if (o_channel[i] != channel[i]) {

      // test if channel is writable
      if ( (_channel_mode[i] != VRPN_ATMEL_MODE_RW) 
	    && (_channel_mode[i] != VRPN_ATMEL_MODE_WO) ) {
 
        fprintf(stderr,"vrpn_Atmel: mainloop_serial_io()\n");
        fprintf(stderr,"  channel not writable\n");
     	
        channel[i] = VRPN_ATMEL_ERROR_NOT_WRITABLE;
      
        continue;
      }

      // test if it is a valid value: atmel uses 8 bit
      if ((o_channel[i]<0) || (o_channel[i]>255)) {
 
        fprintf(stderr,"vrpn_Atmel: mainloop_serial_io()\n");
        fprintf(stderr,"  value out of range\n");
     	
        channel[i] = VRPN_ATMEL_ERROR_OUT_OF_RANGE;
      
        continue;
      }
      
       // try to write down
      if (setRegister(serial_fd, i, (unsigned char) o_channel[i])
           != ATMELLIB_NOERROR) {
        
        fprintf(stderr,"vrpn_Atmel: mainloop_serial_io()\n");
        fprintf(stderr,"  error writing down value, channel: %d\n",i);
     	
        channel[i] = VRPN_ATMEL_ERROR_WRITING_DOWN;
      
        continue;
      }
      else {

        fprintf(stderr,"vrpn_Atmel: mainloop_serial_io()\n");
        fprintf(stderr,"  written down value, channel: %d\n",i);
      }
      
      // no error
      channel[i] = o_channel[i];
    
      continue;
    }

    // this values are not requested for change -> read them
   
    if ( (_channel_mode[i] == VRPN_ATMEL_MODE_RO)
	 || (_channel_mode[i] == VRPN_ATMEL_MODE_RW) ) {
	    
      if ( (channel[i] = getRegister(serial_fd, i)) < 0) {
 
#ifdef VRPN_ATMEL_VERBOSE 
        fprintf(stderr, "Channel read: i=%i val=%f\n",i,channel[i]);
        
        fprintf(stderr,"vrpn_Atmel: mainloop_serial_io()\n");
        fprintf(stderr,"  error reading out value. channel=%d\n",i);
#endif
        // reset to the old value - don't send the error
        // in some cases it's propably useful to know the error
        channel[i] = VRPN_ATMEL_ERROR_READING_IN;
        //channel[i] = last[i]; 
       }
    }
    
   } // end of for-loop

  for(int i=0; i<o_num_channel ;++i) {

    o_channel[i] = channel[i];
  }
  
  return true;
}


/***************************************************************************************************/
/* init the io mode of the channels: */
/***************************************************************************************************/
void
vrpn_Atmel::init_channel_mode(int * channel_mode)
{
  int val=0;
	
  for(int i=0; i<o_num_channel; ++i) {
    
    _channel_mode.push_back(channel_mode[i]);
 
    if ( (channel_mode[i] == VRPN_ATMEL_MODE_RO)
        || (_channel_mode[i] == VRPN_ATMEL_MODE_RW) ) {
	
      // read the current value of the register
      val=getRegister(serial_fd , i);
      if (val < 0) {
   
         // error while reading register value
         fprintf(stderr,"vrpn_Atmel: init_channel_mode()\n");
         fprintf(stderr,"  error reading out value: %i\n", val);
	
	 o_channel[i] = VRPN_ATMEL_CHANNEL_NOT_VALID;
	 channel[i] = VRPN_ATMEL_CHANNEL_NOT_VALID;
	}
	else {
          
          channel[i] = (float) val;
	  o_channel[i] = (float) val;
          
#ifdef VRPN_ATMEL_VERBOSE 
	  fprintf(stderr, "vrpn_Atmel: i=%d val=%d\n",i ,val);
#endif

	}
    }
    else {

      // channel is not for reading 	    
      o_channel[i] = VRPN_ATMEL_CHANNEL_NOT_VALID;
      channel[i] = VRPN_ATMEL_CHANNEL_NOT_VALID;
    }

  } // end of for loop
}


/***************************************************************************************************/
/* init the io mode of the channels: */
/***************************************************************************************************/
bool
vrpn_Atmel::handle_new_connection()
{
  // request to send the current status of all registers 	
  vrpn_Analog::report();

  printf("\nconnection received.\n\n");
  
  // really send the current status 
  d_connection->mainloop();

  return true;
}

#endif

