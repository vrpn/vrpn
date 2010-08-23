/* -*- Mode:C -*- */

/***************************************************************************************************/
/*                                                                                                 */
/* Copyright (C) 2003 Bauhaus University Weimar                                                    */
/* Released into the public domain on 6/23/2007 as part of the VRPN project                        */
/* by Jan P. Springer.                                                                             */
/*                                                                                                 */
/***************************************************************************************************/
/*                                                                                                 */
/*  module     :  atmel_serial_errno.h                                                             */
/*  project    :  atmellib                                                                         */
/*  description:  error numbers of the lib                                                         */
/*                                                                                                 */
/***************************************************************************************************/

#if !defined(ATMELLIB_SERIAL_ERRNO_H)

#define ATMELLIB_SERIAL_ERRNO_H


/* program specific error codes */

#define ATMELLIB_NOERROR                   1      /* no error */
#define ATMELLIB_ERROR_GENERAL            -1      /* general error */
#define ATMELLIB_ERROR_SEQTO              -5      /* sequence to small */ 
#define ATMELLIB_ERROR_NORESPVAL          -6      /* the mc didn't confirm the command */ 
#define ATMELLIB_ERROR_NOCOMVAL           -7      /* no valid command (must be bigger than 127 ) */
#define ATMELLIB_ERROR_REGINV            -10      /* the register is invalid */
#define ATMELLIB_ERROR_VALINV            -11      /* the value register is invalid */
#define ATMELLIB_ERROR_RESINV            -12      /* hardware ressource invalid */
#define ATMELLIB_ERROR_NOTINIT           -13      /* the prequists are not ok: i.e. to put a
                                                       light on the pin must be in output mode */
#define ATMELLIB_ERROR_PARAMINV          -14      /* functioncall with invalid parameter*/
#define ATMELLIB_ERROR_NOSTATEVAL        -15      /* no valid state or not able to read it out */


#endif /* #if !defined(ATMELLIB_SERIAL_ERRNO_H) */

