/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2003 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  vrpn_atmellib_register.C                                                         */
/*  project    :  atmel-avango                                                                     */
/*  description:  function of an easy high level interface for access to the registers             */
/*                                                                                                 */
/***************************************************************************************************/

/* include system headers */

#include <errno.h>                      // for ENXIO, error_t

/* include i/f header */

#include "vrpn_atmellib.h"              // for command_t, handle_invalid, etc
#include "vrpn_atmellib_errno.h"        // for ATMELLIB_ERROR_REGINV, etc
#include "vrpn_atmellib_helper.h"       // for getAddress, setValue

/***************************************************************************************************/
/* HIGH LEVEL INTERFACE */ 
/***************************************************************************************************/

/***************************************************************************************************/
/* write the value of the specified register and wait for confirmation */
/***************************************************************************************************/
/* extern */ error_t
setRegister( handle_t Handle , const unsigned char Register , const unsigned char Value)
{
  struct command_t  Cmd;

  /* check if the arguments are valid */
  if (handle_invalid(Handle))
    return ENXIO;
  if (register_invalid( Register ) != ATMELLIB_NOERROR)
    return ATMELLIB_ERROR_REGINV;

  /* combine the parameter to a valid commande */
  Cmd.addr = getAddress( Register );
  setValue( &Cmd , Value );
  
  /* send the command to the microcontroller */ 
  return ( setCmd( Handle , &Cmd ) );
}

/***************************************************************************************************/
/* read out the value of the specified register */
/***************************************************************************************************/
/*extern*/ int 
getRegister( handle_t Handle , const unsigned char Register )
{
  error_t          R;
  struct command_t  Cmd;

  /* check if the arguments are valid */
  if (handle_invalid(Handle))
    return ENXIO;
  if (register_invalid( Register ) == ATMELLIB_ERROR_GENERAL)
    return ATMELLIB_ERROR_REGINV;

  /* make a valid command */
  Cmd.addr = getAddress( Register );

  /* Send the command to the microcontroller */
  if ( (R = getCmd( Handle , &Cmd )) != ATMELLIB_NOERROR)
    return R;  

  /* get the value */
  if ( Cmd.addr & 0x01 ) Cmd.value += 128;

  return (Cmd.value);
}



/* file static for <ident> or <what> */

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma set woff 1174
#endif

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma reset woff 1174
#endif


