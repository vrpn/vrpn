/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2004 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  vrpn_atmellib_tester.C                                                           */
/*  project    :  atmel-avango                                                                     */
/*  description:  functions to test the state                                                      */
/*                                                                                                 */
/***************************************************************************************************/

/* include system headers */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

/* include i/f header */

#include "vrpn_atmellib.h"
#include "vrpn_atmellib_helper.h"

/***************************************************************************************************/
/* internal (static) variables */

/* define a invalid handle */
const static int              invalid_handle = -1;


/***************************************************************************************************/
/* test if the given handler is a valid handle */
/***************************************************************************************************/
/* extern */ error_t
handle_invalid (handle_t handle)
{
  return (invalid_handle == handle);
}

/***************************************************************************************************/
/* test if the register is valid - if it exists on the mc */
/***************************************************************************************************/
/* extern */ error_t
register_invalid (unsigned char Register)
{
  if (Register > 63) {
    
    return ATMELLIB_ERROR_GENERAL;
  }

  return ATMELLIB_NOERROR;
}


/* file static for <ident> or <what> */

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma set woff 1174
#endif

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma reset woff 1174
#endif

