/** @file
    @brief Implementation of assert handlers
    @date 2015

    @author
    Ryan Pavlik (incorporating some code modified from Boost)
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "vrpn_Configure.h"

// Library/third-party includes
// - none

// Standard includes
#include <stdio.h>

namespace vrpn {
    // implementations based on ALL_ASSERT in vrpn_Shared.C
    VRPN_API void assertion_failed(char const *expr, char const *function,
                          char const *file, long line)
    {
        fprintf(stderr,
                "Assertion (%s) failed\n\tFunction: %s\n\tLocation: %s:%d\n\n",
                expr, function, file, static_cast<int>(line));
    }
    VRPN_API void assertion_failed_msg(char const *expr, char const *msg,
                                     char const *function, char const *file,
                                     long line)
    {

        fprintf(stderr, "Assertion (%s) failed\n\tMessage: %s\n\tFunction: "
                        "%s\n\tLocation: %s:%d\n\n",
                expr, msg, function, file, static_cast<int>(line));
    }
} // namespace vrpn
