/** @file	vrpn_Laputa.h
    @brief	Header for Laputa VR devices.
    @date	2016

  @author Georgiy Frolov georgiy@sensics.com
  @license Standard VRPN license.
*/

// Based on vrpn_Oculus driver

#pragma once

#include "vrpn_HumanInterface.h"
#include "vrpn_Analog.h"

#if defined(VRPN_USE_HID)

/// @brief Laputa VR head-mounted display base class
class VRPN_API vrpn_Laputa : public vrpn_Analog, protected vrpn_HidInterface {
public:
    /**
    *
    * @param name Name of the device.
    * @param c Optional vrpn_Connection pointer to use as our connection.
    */
    vrpn_Laputa(const char *name, vrpn_Connection *c = NULL);

    /**
     * @brief Destructor.
     */
    virtual ~vrpn_Laputa();

    virtual void mainloop();

protected:
    //-------------------------------------------------------------
    // Parsers for different report types.
    // Override to define more parsers
    virtual bool parse_message(std::size_t bytes, vrpn_uint8 *buffer);

    //-------------------------------------------------------------
    // Parsers for different report types.  The Laputa Hero sends type-1
    // reports in response to inertial-only keep-alive messages
    //
    /// Parse and send reports for type-1 message
    void parse_message_type_1(std::size_t bytes, vrpn_uint8 *buffer);

    /// Extracts the sensor values from each report and calls the
    /// appropriate parser.
    void on_data_received(std::size_t bytes, vrpn_uint8 *buffer);

    /// Timestamp updated during mainloop()
    struct timeval d_timestamp;
};

#endif // VRPN_USE_HID
