#ifndef _CM_C_MACROS_H
#define _CM_C_MACROS_H

/* This is inspired by Sun's <c_varieties.h> header.
 * Revised         : Erik Erikson 4/29/92 - Added UNC_
 * Initial Version : Jon Leech 4/8/92
 *
 * EXTERN_FUNCTION - declare a function with argument types
 * Usage:
 *
 *  EXTERN_FUNCTION(int function_with_no_arguments, (_VOID_));
 *  EXTERN_FUNCTION(double normal_function, (int arg1, double arg2));
 *  EXTERN_FUNCTION(void varargs_function, (char *, DOTDOTDOT));
 *
 * FUNCTION_POINTER - declare a function pointer with argument types
 * Usage:
 *
 *  typedef struct {
 *	FUNCTION_POINTER(int (*insidefunc),(float f[3]));
 *  } obj;
 *
 *  EXTERN_FUNCTION(int read,(int fd, const char *buf, int len));
 *  FUNCTION_POINTER(int (*readwriteptr),(int,const char *,int)) = read;
 *
 */

#if defined(__cplusplus)
/* C++ */
#define CM_FUNCTION_POINTER(ptr,args) ptr args
#define CM_EXTERN_FUNCTION(func,args) extern "C" { func args; }
#define CM_DOTDOTDOT ...
#define CM__VOID_ void

#elif defined(__STDC__)
/* ANSI C */
#define CM_FUNCTION_POINTER(ptr,args) ptr args
#define CM_EXTERN_FUNCTION(func,args) func args
#define CM_DOTDOTDOT
#define CM__VOID_ void

#else
/* K&R C */
#define CM_FUNCTION_POINTER(ptr,args) ptr()
#define CM_EXTERN_FUNCTION(func,args) func()
#define CM_DOTDOTDOT
#define CM__VOID_

#endif

#endif /*_CM_C_MACROS_H*/





