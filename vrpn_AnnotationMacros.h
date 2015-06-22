/** @file
    @brief Header defining macros used to annotate code for static
    analysis and optimization.

    Must be c-safe!

    @date 2014

    @author
    Ryan Pavlik
    <ryan@sensics.com>
    <http://sensics.com>

*/

/*
// Copyright 2014 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef INCLUDED_vrpn_AnnotationMacros_h_GUID_B7A15ED0_6EC8_41A8_E49B_38DBCC69D378
#define INCLUDED_vrpn_AnnotationMacros_h_GUID_B7A15ED0_6EC8_41A8_E49B_38DBCC69D378


#ifndef VRPN_DISABLE_ANALYSIS

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
/* Visual C++ (2012 and newer) */
/* Using SAL attribute format:
 * http://msdn.microsoft.com/en-us/library/ms182032(v=vs.120).aspx */

#include <CodeAnalysis/SourceAnnotations.h>

#define VRPN_IN _In_
#define VRPN_IN_PTR _In_
#define VRPN_IN_OPT _In_opt_
#define VRPN_IN_STRZ _In_z_
#define VRPN_IN_READS(NUM_ELEMENTS) _In_reads_(NUM_ELEMENTS)

#define VRPN_OUT _Out_
#define VRPN_OUT_PTR _Out_
#define VRPN_OUT_OPT _Out_opt_

#define VRPN_INOUT _Inout_
#define VRPN_INOUT_PTR _Inout_

/* end of msvc section */
#elif defined(__GNUC__)
/* section for GCC and GCC-alikes */

#if defined(__clang__)
/* clang-specific section */
#endif

#define VRPN_FUNC_NONNULL(X) __attribute__((__nonnull__ X))

/* end of gcc section and compiler detection */
#endif

/* end of ndef disable analysis */
#endif

/* Fallback declarations */
/**
@defgroup annotation_macros Static analysis annotation macros
@brief Wrappers for Microsoft's SAL annotations and others

Use of these is optional, but recommended particularly for core APIs,
as well as any methods handling a buffer with a length.
@{
*/
/** @name Parameter annotations

    These indicate the role and valid values for parameters to functions.

    At most one of these should be placed before a parameter's type name in the
   function parameter list, in both the declaration and definition. (They must
   match!)
   @{
*/
#ifndef VRPN_IN
/** @brief Indicates a required function parameter that serves only as input.

    Place before the parameter's type name in a function parameter list.
*/
#define VRPN_IN
#endif

#ifndef VRPN_IN_PTR
/** @brief Indicates a required pointer (non-null) function parameter that
    serves only as input.
*/
#define VRPN_IN_PTR
#endif

#ifndef VRPN_IN_OPT
/** @brief Indicates a function parameter that serves only as input, but is
    optional and might be NULL

    Place before the parameter's type name in a function parameter list.
*/
#define VRPN_IN_OPT
#endif

#ifndef VRPN_IN_STRZ
/** @brief Indicates a null-terminated string function parameter that serves
   only as input.

    Place before the parameter's type name in a function parameter list.
*/
#define VRPN_IN_STRZ
#endif

#ifndef VRPN_IN_READS
/** @brief Indicates a buffer containing input with the specified number of
   elements.

    The specified number of elements is typically the name of another parameter.

    Place before the parameter's type name in a function parameter list.
*/
#define VRPN_IN_READS(NUM_ELEMENTS)
#endif

#ifndef VRPN_OUT
/** @brief Indicates a required function parameter that serves only as output.
    In C code, since this usually means "pointer", you probably want
   VRPN_OUT_PTR.
*/
#define VRPN_OUT
#endif

#ifndef VRPN_OUT_PTR
/** @brief Indicates a required pointer (non-null) function parameter that
    serves only as output.
*/
#define VRPN_OUT_PTR
#endif

#ifndef VRPN_OUT_OPT
/** @brief Indicates a function parameter that serves only as output, but is
    optional and might be NULL

    Place before the parameter's type name in a function parameter list.
*/
#define VRPN_OUT_OPT
#endif

#ifndef VRPN_INOUT
/** @brief Indicates a required function parameter that is both read and written
   to.

    In C code, since this usually means "pointer", you probably want
   VRPN_INOUT_PTR.
*/
#define VRPN_INOUT
#endif

#ifndef VRPN_INOUT_PTR
/** @brief Indicates a required pointer (non-null) function parameter that is
    both read and written to.
*/
#define VRPN_INOUT_PTR
#endif

/* End of parameter annotations. */
/** @} */

/** @name Function annotations

    These indicate particular relevant aspects about a function. Some
    duplicate the effective meaning of parameter annotations: applying both
    allows the fullest extent of static analysis tools to analyze the code.

    These should be placed after a function declaration (but before the
   semicolon). Repeating them in the definition is not needed.
   @{
*/
#ifndef VRPN_FUNC_NONNULL
/** @brief Indicates the parameter(s) that must be non-null.

    @param X A parenthesized list of parameters by number (1-based index)
*/
#define VRPN_FUNC_NONNULL(X)
#endif
/* End of function annotations. */
/** @} */

/* End of annotation group. */
/** @} */

#endif
