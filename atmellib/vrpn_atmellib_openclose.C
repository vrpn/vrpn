/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2004 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  vrpn_atmellib_openclose.C                                                        */
/*  project    :  atmel-avango                                                                     */
/*  description:  handle open, close and initalization of the tty                                  */
/*                                                                                                 */
/***************************************************************************************************/

/* include system headers */

#include <errno.h>                      // for ENXIO, error_t
#include <fcntl.h>                      // for open, O_NOCTTY, O_RDWR
#include <termios.h>                    // for termios, cfsetispeed, etc
#include <unistd.h>                     // for close, isatty

/* include i/f header */

#include "vrpn_atmellib.h"              // for handle_t, handle_invalid
#include "vrpn_atmellib_errno.h"        // for ATMELLIB_NOERROR


/***************************************************************************************************/
/* internal (static) functions */

/*****************************************************************/
/* set the parameter to communicate with the microcontroller */
/*****************************************************************/
static int
Set_Params( handle_t fd , const int baud, struct termios * init_param)
{
  struct termios Params;

  /* get the actual parameters of the tty */
  tcgetattr( fd , &Params );

  /* save the init values of the tty */
  (*init_param) = Params;

  /* wait time for new data */
  Params.c_cc[VTIME] = 0;
  Params.c_cc[VMIN] = 1;

  if (baud==38400) {
	  
    Params.c_cflag = B38400 | CSIZE | CS8 | CREAD | CLOCAL | HUPCL;
    cfsetospeed( &Params , B38400 );
    cfsetispeed( &Params , B38400 );
  }
  else {

    // 9600 baud is default
    Params.c_cflag = B9600 | CSIZE | CS8 | CREAD | CLOCAL | HUPCL;
    cfsetospeed( &Params , B9600 );
    cfsetispeed( &Params , B9600 );
  }
    
  Params.c_iflag = (IGNBRK | IGNPAR);
  Params.c_lflag = 0;
  Params.c_oflag = 0;

  /* activate the new settings */
  tcsetattr( fd , TCSANOW , &Params );

  return ATMELLIB_NOERROR;
}

/***************************************************************************************************/
/* PART OF THE BASIC INTERFACE: for explainations to the functions see the header file */
/***************************************************************************************************/


/***************************************************************************************************/
/* open the specified serial port and set the parameter that it's ready to communicate with the mc*/
/***************************************************************************************************/
/* extern */ handle_t
openPort (const char* tty , const int baud, struct termios * init_param)
{
  handle_t  fd; 
  
  /* try to open thespecified device */
  if ( (fd = open( tty , O_RDWR | O_NOCTTY )) < 0) 
    return fd;

  /* check if the open file is a tty */
  if (!isatty( fd )) {
    close (fd);   
    return -1;
  }

  /* set the correct serial parameter to communicate with the microcontroller */
  if ( Set_Params( fd , baud, init_param ) < 0 ) 
  {
    close (fd);   
    return -1;
  }

  return fd;
}

/***************************************************************************************************/
/* close the specified port and reset the parameter to the initial values */
/***************************************************************************************************/
/* extern */ error_t
closePort (handle_t fd , struct termios * init_param)
{
  if (handle_invalid(fd)) 
    return ENXIO;

  /* first reset the parameters and then close the port */
  tcsetattr( fd , TCSADRAIN, init_param );
  close( fd );
   
  return ATMELLIB_NOERROR;
}


/* file static for <ident> or <what> */

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma set woff 1174
#endif

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma reset woff 1174
#endif

