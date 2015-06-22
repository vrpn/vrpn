#!/usr/bin/env lua
--[[
Script to generate an include header wrapping windows.h, defining
symbols to make the include minimal, and undefining to initial state
when done. -- Ryan Pavlik

Copyright 2015 Sensics, Inc.
Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
         http://www.boost.org/LICENSE_1_0.txt)

]]

symbols = {"WIN32_LEAN_AND_MEAN", "NOMINMAX", "NOSERVICE", "NOMCX", "NOIME"}
template = {
    before = {
        "#ifndef ",
        "\n#define ",
        "\n#define VRPN_",
        "\n#endif\n"
    };
    after = {
        "#ifdef VRPN_",
        "\n#undef VRPN_",
        "\n#undef ",
        "\n#endif\n"
    }
}

print [[/** @file
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
]]

for _, symbol in ipairs(symbols) do
    print(table.concat(template.before, symbol))
end

print [[
#include <windows.h>
]]


for _, symbol in ipairs(symbols) do
    print(table.concat(template.after, symbol))
end

print [[
#endif // _WIN32

#endif // INCLUDED_vrpn_WindowsH_h_GUID_97C90BFD_D6C3_4AB3_3272_A10F7448D165
]]