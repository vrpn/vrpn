/* -*- Mode:C -*- */

/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2003 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  vrpn_atmellib.h                                                                  */
/*  project    :  vrpn_avango                                                                      */
/*  description:  part of vrpn_avango_atmel_server                                                 */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*                                                                                                 */
/***************************************************************************************************/

#if !defined(VRPN_AVANGO_ATMELLIB_SERIAL_H)
#define VRPN_AVANGO_ATMELLIB_SERIAL_H

/* includes, system */
/* #include <> */

/* includes, project */
#include "vrpn_atmellib_errno.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*************************************************************************************************/
/* constants you have to remember */

/* maiximum time to wait for a new byte from the atmel */
#define VRPN_ATMELLIB_SELECT_WAIT_SEC       0 
#define VRPN_ATMELLIB_SELECT_WAIT_USEC      150000

/*************************************************************************************************/
/* types, exported (enum, struct, union, typedef) */
/*************************************************************************************************/

/* the 2 bytes which defines a message from/to the microcontroller */
struct command_t {
  unsigned char addr;
  unsigned char value;
};

/*************************************************************************************************/

typedef int error_t;
typedef int handle_t;
  
/*************************************************************************************************/

/* for development purpose mainly 
   displays a value as Bits on the screen */
 extern void outBit( unsigned char );

/*************************************************************************************************/
/* BASICS */
/*************************************************************************************************/

/* opens the the specified port (whole path needed) and sets the parameters
for the communication with the microcontroller 
CAUTION: the return value is a program specific file handler and not the
      system file handler */
extern handle_t openPort(const char*i, const int baud, struct termios * init_param);

/* close the specified port and reset all parameters */
extern error_t closePort (handle_t, struct termios * init_param);

/* checks if the given file handler is valid: returns 1 (true) if the handle is invalid otherwise
return 0 (false) */ 
extern error_t handle_invalid (int); 

/* checks if the register is valid */
extern error_t register_invalid( unsigned char );


/*************************************************************************************************/
/* LOW LEVEL INTERFACE */
/*************************************************************************************************/

/* send one/more command(s) to the microcontroller which don't change the state of it 
the return value of the command - i.e. the value of a register - is returned in the 
second calling variable */  
extern int getCmd (handle_t, struct command_t*);

/* send one/more command(s) to the microcontroller which change the state of the microcontrolle */
extern int setCmd (int, struct command_t*);

/* set the value of the specified register */
extern    error_t    
setRegister( handle_t Hd , const unsigned char Register , const unsigned char Val);

/* get the value of the specified register */
extern    int getRegister( handle_t Hd , const unsigned char Register);

#if defined(__cplusplus)
} /* extern "C" { */
#endif

#endif /* #if !defined(VRPN_AVANGO_ATMELLIB_SERIAL_H) */

