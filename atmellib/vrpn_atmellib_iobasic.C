/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2004 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  vrpn_atmel_iobasic.C                                                             */
/*  project    :  atmel-avango                                                                     */
/*  description:  basic functions for serial io access on the tty                                  */
/*                                                                                                 */
/***************************************************************************************************/

//#define VRPN_ATMELLIB_VERBOSE_OUTBIT
//#define VRPN_ATMELLIB_VERBOSE

/* include system headers */

#include <errno.h>                      // for errno, ENXIO, error_t
#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for fprintf, stderr, NULL, etc
#include <string.h>                     // for strerror
#include <sys/select.h>                 // for select, FD_SET, FD_ZERO, etc
#include <termios.h>                    // for tcflush, TCIOFLUSH
#include <unistd.h>                     // for read, write

/* include i/f header */

#include "vrpn_Shared.h"                // for timeval
#include "vrpn_atmellib.h"              // for command_t, handle_t, etc
#include "vrpn_atmellib_errno.h"        // for ATMELLIB_ERROR_NORESPVAL, etc

/***************************************************************************************************/
/* read data from the tty */
/* returns the bytes read or one of the system wide error codes returned by read */
/***************************************************************************************************/
static int
read_wrapper (handle_t fd, void* buf, size_t len)
{
  return read (fd, buf, len);
}

/***************************************************************************************************/
/* write data to the tty: return the byte written or one of the system
   wide error codes returned by write */
/***************************************************************************************************/
static int
write_wrapper ( handle_t fd , const void* buf , size_t len )
{
  return write (fd , buf , len );
}

/***************************************************************************************************/
/* make a select for lib-wide time and if this is successfully read one byte */
/***************************************************************************************************/
static int 
select_read_wrapper(handle_t fd , struct timeval * time)
{	
  fd_set rdfs;
  unsigned char byte;
  int ret;
  
  FD_ZERO( &rdfs );
  FD_SET(fd , &rdfs);
 
  ret = select( fd+1, &rdfs, NULL, NULL, time);
  if (ret<0) {
	  
    fprintf(stderr, "select_read_wrapper:: error during select: %s (%i)\n",
             strerror(errno) ,errno);  
    return ATMELLIB_ERROR_NORESPVAL;
  }
  else if (ret==0) {
  
    // time expired
    fprintf(stderr, "vrpn_atmellib::select_read_wrapper: select timed out\n" );  
    return ATMELLIB_ERROR_NORESPVAL;
  }   
   
  // successful select -> read out one command
  if ((ret = read_wrapper(fd , (void *) (&byte), 1)) <=0 )
    return ret;

#ifdef VRPN_ATMELLIB_VERBOSE_OUTBIT
  outBit(byte);
#endif
  
  return byte;
}

/***************************************************************************************************/
/* make a select for lib-wide time and if this is successfully read one byte */
/***************************************************************************************************/
static int 
select_write_wrapper(handle_t fd , struct timeval * time, unsigned char * data , int len)
{	
  fd_set wrfs;
  int ret;
  
  FD_ZERO( &wrfs );
  FD_SET(fd , &wrfs);
 
  ret = select( fd+1, NULL, &wrfs, NULL, time);
  if (ret<0) {
	  
    fprintf(stderr, 
      "vrpn_atmellib::select_write_wrapper::error during waiting for writing permission: %s (%i)\n",
      strerror(errno) ,errno);  
    
    return ATMELLIB_ERROR_NORESPVAL;
  }
  else if (ret==0) {
  
    // time expired
    fprintf(stderr, "vrpn_atmellib::select_write_wrapper: timed out in wrapper\n" );  
    return ATMELLIB_ERROR_NORESPVAL;
  }   
  
  // successful select -> write down command
  // write twice to ensure that the atmel receives it
  //write_wrapper(fd , (void *) data, len); 
  return write_wrapper(fd , (void *) data, len); 
}

/***************************************************************************************************/
/* PART OF THE BASIC INTERFACE: for explainations to the functions see the header file */
/***************************************************************************************************/


/***************************************************************************************************/
/* get the value of one register                                                                   */
/***************************************************************************************************/
/* extern */ error_t 
getCmd (handle_t fd, struct command_t* Cmd)
{
  struct timeval time_wait;  
  int ret;

#ifdef VRPN_ATMEL_SERIAL_VRPN
  unsigned char read_val;
#endif
  
  // some adds to measure time for reading out  
#ifdef VRPN_ATMELLIB_TIME_MEASURE
  struct timeval start;
  struct timeval end;
  int sec, usec; 
  vrpn_gettimeofday( &start , 0);
#endif

  unsigned char PossibilityOne;
  unsigned char PossibilityTwo;
  unsigned char Reference;

  /* check if the given parameters are valid */
  if (handle_invalid(fd)) 
     return ENXIO;
  if ((*Cmd).addr < 128)
     return ATMELLIB_ERROR_NOCOMVAL;
  
  PossibilityOne = (*Cmd).addr;
  PossibilityTwo = (*Cmd).addr;
  Reference = 0x01;

  if (Reference & PossibilityTwo)
     /* the LSB of the Address is 1 -> make 0 -> use XOR operator */
     PossibilityTwo ^= Reference;
  else
     /* the LSB of the address is 0 -> make 1 -> use OR operator */
     PossibilityTwo |= Reference;

#ifdef VRPN_ATMEL_SERIAL_VRPN  
  vrpn_flush_input_buffer(fd);
  vrpn_flush_output_buffer(fd);
#else 
  tcflush( fd , TCIOFLUSH );
#endif

  time_wait.tv_sec = VRPN_ATMELLIB_SELECT_WAIT_SEC;
  time_wait.tv_usec = VRPN_ATMELLIB_SELECT_WAIT_USEC;
 
#ifdef VRPN_ATMEL_SERIAL_VRPN  
  vrpn_write_characters(fd, (&((*Cmd).addr)) , 1);
#else
  /* you have to send the address first */
  if ( (ret = select_write_wrapper( fd , 
                                    &(time_wait),
				    (&((*Cmd).addr)) , 
                                    1 )) 
       != 1 ) {
	  
      fprintf(stderr, "\n vrpn_atmellib::getCmd: Error while writing down. error=%i\n",
                     ret);
	  
    return ret;
  }
#endif  
 
  while (time_wait.tv_usec!=0) {
   
#ifdef VRPN_ATMEL_SERIAL_VRPN
    if (( vrpn_read_available_characters(fd, &(read_val), 1, &time_wait)) != 1) {

      fprintf(stderr, "vrpn_atmellib::getCmd: Error vrpn_read_available_characters for first byte\n");
      break;
    }

    // else
    ret = read_val;
#else 
    if ((ret = select_read_wrapper(fd, &time_wait)) < 0) {

      fprintf(stderr, "vrpn_atmellib::getCmd:\
                       Error select_read_wrapper for first byte: %i\n" , ret);
      break;
    }
#endif
    
    // found expected first byte
    if ((ret==PossibilityOne) || (ret==PossibilityTwo )) {
       
      (*Cmd).addr = ret;

#ifdef VRPN_ATMEL_SERIAL_VRPN
      if (( vrpn_read_available_characters(fd, &(read_val), 1, &time_wait)) != 1) {

        fprintf(stderr, "vrpn_atmellib::getCmd: Error vrpn_read_available_characters.\n");;
        break;
      }
      
      //else
      ret = read_val;
#else      
      ret = select_read_wrapper(fd, &time_wait); 
#endif      

      if ((ret < 0) || (ret > 128)) {

           fprintf(stderr, "vrpn_atmellib::getCmd: Error reading second byte: %i\n\n" , ret);
           break;
       }
  
       (*Cmd).value   = ret;
      
#ifdef VRPN_ATMELLIB_TIME_MEASURE 
       // display time for
       vrpn_gettimeofday( &end , 0);
       sec=end.tv_sec-start.tv_sec; 
       usec=end.tv_usec-start.tv_usec; 
       printf("Time for reading out: sec=%i , usec=%i\n", sec, usec);
#endif
       
       return ATMELLIB_NOERROR;
    }
     
  }  
  
  return ATMELLIB_ERROR_NORESPVAL;
}
 

/***************************************************************************************************/
/* write one command to the mc and wait for confirmation                                           */
/***************************************************************************************************/
/* extern */ error_t
setCmd (handle_t fd , struct command_t * Cmd)
{

  struct timeval time_wait;
  int           ret;
  
#ifdef VRPN_ATMEL_SERIAL_VRPN
  unsigned char read_val;
#endif

  // some adds to measure time for reading out  
#ifdef VRPN_ATMELLIB_TIME_MEASURE
  struct timeval start;
  struct timeval end;
  int sec, usec; 
  vrpn_gettimeofday( &start , 0);
#endif
  
  /* check if the given parameters are valid */
  if (handle_invalid(fd)) 
     return ENXIO;
  if ((*Cmd).addr < 128)
     return ATMELLIB_ERROR_NOCOMVAL;

  time_wait.tv_sec = VRPN_ATMELLIB_SELECT_WAIT_SEC;
  time_wait.tv_usec = VRPN_ATMELLIB_SELECT_WAIT_USEC;

#ifdef VRPN_ATMEL_SERIAL_VRPN
  vrpn_write_characters(fd, (unsigned char*) Cmd, 2);
#else  
  if ( (ret = select_write_wrapper( fd , 
                                    &(time_wait),
				    (unsigned char*) Cmd , 
                                    2 )) 
       != 2 ) {

      fprintf(stderr, "\n vrpn_atmellib::setCmd: Error while writing down. error=%i\n",
                     ret);
    
    return ret;
  }
#endif
 
  
#ifdef VRPN_ATMEL_SERIAL_VRPN  
  vrpn_flush_input_buffer(fd);
  vrpn_flush_output_buffer(fd);
#else
  tcflush( fd , TCIOFLUSH );
#endif
  
  while (time_wait.tv_usec!=0) {

#ifdef VRPN_ATMEL_SERIAL_VRPN
    if (( vrpn_read_available_characters(fd, &(read_val), 1, &time_wait)) != 1) {

      fprintf(stderr, "vrpn_atmellib::setCmd: Error vrpn_read_available_characters.\n");;
      break;
    }
      
    //else
    ret = read_val;
#else      
    if ((ret = select_read_wrapper(fd, &time_wait)) < 0) {

      fprintf(stderr, "vrpn_atmellib::setCmd: Error select_read_wrapper for first byte: %i\n" , ret);
      break;
    } 
#endif

    // found expected first byte
    if (ret==(*Cmd).addr) {
      
#ifdef VRPN_ATMEL_SERIAL_VRPN
      if (( vrpn_read_available_characters(fd, &(read_val), 1, &time_wait)) != 1) {

        printf("Error vrpn_read_available_characters.\n");;
        break;
      }
      
      //else
      ret = read_val;
#else
      ret = select_read_wrapper(fd, &time_wait); 
#endif

      if (ret!=(*Cmd).value) {

           printf("vrpn_atmellib::setCmd: Error select_read_wrapper for second byte: %i\n" , ret);
           break;
       }
  
#ifdef ATMELLIB_TIME_MEASURE 
       // display time for
       vrpn_gettimeofday( &end , 0);
       sec=end.tv_sec-start.tv_sec; 
       usec=end.tv_usec-start.tv_usec; 
       printf("Time for writing down: sec=%i , usec=%i\n", sec, usec);
#endif
       
       return ATMELLIB_NOERROR;
    }
     
  }  
  
  return ATMELLIB_ERROR_NORESPVAL;
}

/* file static for <ident> or <what> */

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma set woff 1174
#endif

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma reset woff 1174
#endif

