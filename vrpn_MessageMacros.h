/** @file
    @brief Header containing macros formerly duplicated in a lot of
   implementation files.

    For use only in implementation files for vrpn devices. This is the "old way"
    of doing things: just unified here to reduce code duplication. The
    new way of simplifying message sending is in
   vrpn_SendTextMessageStreamProxy.h.

    @date 2013

    @author
    Ryan Pavlik
    <rpavlik@iastate.edu> and <abiryan@ryand.net>
    http://academic.cleardefinition.com/
    Iowa State University Virtual Reality Applications Center
    Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef INCLUDED_vrpn_MessageMacros_h_GUID_289adc6a_78da_4d50_8166_4611d0d911e8
#define INCLUDED_vrpn_MessageMacros_h_GUID_289adc6a_78da_4d50_8166_4611d0d911e8

#ifndef VRPN_TIMESTAMP_MEMBER
#define VRPN_TIMESTAMP_MEMBER timestamp
#endif

#define VRPN_MSG_INFO(msg)                                                     \
    {                                                                          \
        send_text_message(msg, VRPN_TIMESTAMP_MEMBER, vrpn_TEXT_NORMAL);       \
        if (d_connection && d_connection->connected())                         \
            d_connection->send_pending_reports();                              \
    }

#define VRPN_MSG_WARNING(msg)                                                  \
    {                                                                          \
        send_text_message(msg, VRPN_TIMESTAMP_MEMBER, vrpn_TEXT_WARNING);      \
        if (d_connection && d_connection->connected())                         \
            d_connection->send_pending_reports();                              \
    }

#define VRPN_MSG_ERROR(msg)                                                    \
    {                                                                          \
        send_text_message(msg, VRPN_TIMESTAMP_MEMBER, vrpn_TEXT_ERROR);        \
        if (d_connection && d_connection->connected())                         \
            d_connection->send_pending_reports();                              \
    }

#endif // INCLUDED_vrpn_MessageMacros_h_GUID_289adc6a_78da_4d50_8166_4611d0d911e8
