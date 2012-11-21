/* -*- Mode:C -*- */

/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2003 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  lib                                                                              */
/*  project    :  atmellib                                                                         */
/*  description:  private part of the header for internal use                                      */
/*                                                                                                 */
/***************************************************************************************************/

/* $Id: vrpn_atmellib_helper.h,v 1.1 2007/06/25 19:56:16 taylorr Exp $ */

#if !defined(ATMELLIB_SERIAL_HELPER_H)
#define ATMELLIB_SERIAL_HELPER_H

/* includes, system */
/* #include <> */

#include <errno.h>                      // for error_t

/* includes, project */
#include "vrpn_atmellib.h"              // for handle_t


#if defined(__cplusplus)
extern "C" {
#endif
  
  /*****************************************************************/
  /* types, variables NOT exported, internal (static) */

  /* internal functions */

  /* set the specified bit in the given Command without interfering the other bits*/
  error_t   setBitCmd( struct command_t * Command , \
                                    const unsigned char Position , unsigned char Value );

  /* set the specified bit in the Register without interfering the other bits */
  error_t   setBitReg( const handle_t Handle , const unsigned char Register , \
                                     const unsigned char Position , const unsigned char Value );

  /* gives you the 2^Exponent */
  unsigned int  PowToTwo( unsigned int Exponent);

  /* valid address byte for the specified Register 
     the MSB data bit is set to zero */
  unsigned char getAddress( unsigned char Register );

  /* set a value in an two byte command*/
  void setValue( struct command_t * Command , unsigned char Value);

 
#if defined(__cplusplus)
} /* extern "C" { */
#endif

#endif /* #if !defined(ATMELLIB_SERIAL_HELPER_H) */

