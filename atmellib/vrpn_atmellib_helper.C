/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2004 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     : vrpn_atmellib_helper.C                                                            */
/*  project    : atmel-avango                                                                      */
/*  description: part of the serial lib for the Atmel                                              */
/*               miscellaneous functions                                                           */
/*                                                                                                 */
/***************************************************************************************************/

/* include system headers */

#include <errno.h>                      // for error_t
#include <stdio.h>                      // for printf

/* include i/f header */

#include "vrpn_atmellib.h"              // for command_t, getCmd, setCmd, etc
#include "vrpn_atmellib_errno.h"        // for ATMELLIB_NOERROR, etc
#include "vrpn_atmellib_helper.h"

/***************************************************************************************************/
/* set the defined bit in the command to the expected value and leave the rest like it is */
/***************************************************************************************************/
error_t
setBitCmd( struct command_t * Cmd , const unsigned char Pos , unsigned char Val )
{
  /* check if the parameter are valid */
  if ((Val != 0) && (Val != 1))
    return ATMELLIB_ERROR_VALINV;
  if (Pos > 7)
    return ATMELLIB_ERROR_VALINV;


  /* check if the value for "our" pin is already correct else change it */
  if (Pos == 7) {
                                                          /* bit seven is in byte one */
    if (((*Cmd).addr) & 0x01) {                     
      if (Val == 0)                                   
        (*Cmd).addr -= 1;                             /* it's 1 and it has to be 0*/  
    }
    else {
      if (Val == 1)                                     /* it's 0 and it has to be 1 */
        (*Cmd).addr += 1;
    }
  }
  else {                                                    /* the bits in the second byte */
    unsigned char Reference = 0x01 << Pos;
    if ( ((*Cmd).value) & Reference ) {
      if (Val == 0)
        (*Cmd).value -= PowToTwo(Pos);
    }    
    else {                                                            /* change necessary */
      if (Val == 1)
        (*Cmd).value += PowToTwo(Pos);
     }
   }

  return ATMELLIB_NOERROR;
}

/***************************************************************************************************/
/* change only one bit of the value of the specified register    */
/***************************************************************************************************/
error_t
setBitReg( const handle_t Hd , const unsigned char Reg , \
                     const unsigned char Pos , const unsigned char Val )
{
  struct command_t Cmd;
  error_t         R;
  unsigned char   i;


  /* check if the parameters are valid */
  if ((Val != 0) && (Val != 1))
    return ATMELLIB_ERROR_VALINV;

  /* set the correct address for the register */
  /* check if the first bit is already set or if it has to be done here */
  if (Reg & 0x80)
    Cmd.addr = Reg;
  else {
    Cmd.addr = Reg + 64;
    Cmd.addr <<= 1;
  }

  Cmd.value = 0x00;

  /* get the current setting of the register on the microcontroller */
  /* if you get sth invalid try again */
  for (i=0 ; i<10 ; ++i) {

    if ( (R = getCmd( Hd , &Cmd )) != ATMELLIB_NOERROR )
      return R;
 
    if ( Cmd.value<128)
      break;
  }
  if (i==10) 
    return ATMELLIB_ERROR_NOSTATEVAL;

  if ( (R = setBitCmd( &Cmd , Pos , Val )) != ATMELLIB_NOERROR)
    return R;

  /* write the corrected value back to the microcontroller */
  if ( (R = setCmd( Hd , &Cmd )) != ATMELLIB_NOERROR )
    return R;

  return ATMELLIB_NOERROR;
}

/***************************************************************************************************/
/* 2^Exponent */
/***************************************************************************************************/
unsigned int
PowToTwo( unsigned int Exponent )
{
  unsigned int i;
  unsigned int R = 1;

  for( i=0 ; i<Exponent ; i++)
     R *= 2;

  return R;
}


/***************************************************************************************************/
/* valid address byte to the specified register                   */
/* the MSB data bit is set to zero                               */
/***************************************************************************************************/
unsigned char
getAddress( unsigned char Reg )
{
  Reg += 64;
  Reg <<= 1;
  return Reg;
}


/***************************************************************************************************/
/* set the value in a two byte command */
/***************************************************************************************************/
void
setValue( struct command_t * Cmd , unsigned char Val)
{
  if (Val>127) {
  
    Val -= 128;
    if ( ! ((*Cmd).addr & 0x01))
      (*Cmd).addr +=1;
  }
  else
    if ((*Cmd).addr & 0x01)
      (*Cmd).addr -=1;

  (*Cmd).value = Val;
}


/***************************************************************************************************/
/* helper function */
/***************************************************************************************************/
/* extern */ void
outBit( unsigned char Arg )
{
  unsigned char Ref;

  for ( Ref=0x80 ; Ref != 0x00 ; Ref >>= 1 ) {

    if (Ref & Arg)
      printf("1");
    else
      printf("0");
  }

  printf("\n");
}


   
#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma set woff 1174
#endif

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#  pragma reset woff 1174
#endif

