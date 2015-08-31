/** @file
    @brief Header to minimally include windows.h

    @date 2015

    @author
    Ryan Pavlik
    Sensics, Inc.
    <http://sensics.com/osvr>
*/


// Copyright 2015 Sensics, Inc.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_vrpn_WindowsH_h_GUID_97C90BFD_D6C3_4AB3_3272_A10F7448D165
#define INCLUDED_vrpn_WindowsH_h_GUID_97C90BFD_D6C3_4AB3_3272_A10F7448D165

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define VRPN_WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#define VRPN_NOMINMAX
#endif

#ifndef NOSERVICE
#define NOSERVICE
#define VRPN_NOSERVICE
#endif

#ifndef NOMCX
#define NOMCX
#define VRPN_NOMCX
#endif

#ifndef NOIME
#define NOIME
#define VRPN_NOIME
#endif

#include <windows.h>

#ifdef VRPN_WIN32_LEAN_AND_MEAN
#undef VRPN_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#ifdef VRPN_NOMINMAX
#undef VRPN_NOMINMAX
#undef NOMINMAX
#endif

#ifdef VRPN_NOSERVICE
#undef VRPN_NOSERVICE
#undef NOSERVICE
#endif

#ifdef VRPN_NOMCX
#undef VRPN_NOMCX
#undef NOMCX
#endif

#ifdef VRPN_NOIME
#undef VRPN_NOIME
#undef NOIME
#endif

#endif // _WIN32

#endif // INCLUDED_vrpn_WindowsH_h_GUID_97C90BFD_D6C3_4AB3_3272_A10F7448D165

