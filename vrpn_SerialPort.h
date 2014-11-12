/** @file
    @brief Header

    @date 2012

    @author
    Ryan Pavlik
    <rpavlik@iastate.edu> and <abiryan@ryand.net>
    http://academic.cleardefinition.com/
    Iowa State University Virtual Reality Applications Center
    Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// Internal Includes
#include "vrpn_Configure.h" // for VRPN_API
#include "vrpn_Serial.h"    // for ::vrpn_SER_PARITY_NONE, etc

// Library/third-party includes
// - none

// Standard includes
#include <stdexcept> // for runtime_error, logic_error
#include <string>    // for string

/// @brief A simple class wrapping the functionality of vrpn_Serial.h with
/// RAII, object-orientation, and optional STL types
class VRPN_API vrpn_SerialPort {
public:
    typedef int file_handle_type;
    /// @brief Construct and open port
    /// @sa vrpn_open_commport
    /// @throws OpenFailure
    vrpn_SerialPort(const char *portname, long baud, int charsize = 8,
                    vrpn_SER_PARITY parity = vrpn_SER_PARITY_NONE);

    /// @brief Construct without opening
    vrpn_SerialPort();

    /// @brief Destructor - closes port if open.
    ~vrpn_SerialPort();

    /// @name Open/Close Methods
    /// @{
    /// @brief Open serial port
    /// @sa vrpn_open_commport
    /// @throws OpenFailure, AlreadyOpen
    void open(const char *portname, long baud, int charsize = 8,
              vrpn_SER_PARITY parity = vrpn_SER_PARITY_NONE);
    bool is_open() const;

    /// @brief Close the serial port.
    /// @throws NotOpen, CloseFailure
    void close();
    /// @}

    /// @name Write
    /// @returns number of bytes written
    /// @throws WriteFailure, NotOpen
    /// @{
    int write(std::string const &buffer);
    int write(const unsigned char *buffer, int bytes);
    /// @}

    /// @name Read
    /// @throws ReadFailure, NotOpen
    /// @{
    /// @brief Read available characters from input buffer, up to indicated
    /// count.
    int read_available_characters(unsigned char *buffer, int count);

    /// @brief Read available characters from input buffer, up to indicated
    /// count (or -1 for no limit)
    std::string read_available_characters(int count = -1);

    /// @brief Read available characters from input buffer, and wait up to the
    /// indicated timeout for those remaining, up to indicated count.
    int read_available_characters(unsigned char *buffer, int count,
                                  struct timeval &timeout);

    /// @brief Read available characters from input buffer, and wait up to the
    /// indicated timeout for those remaining, up to indicated count.
    std::string read_available_characters(int count, struct timeval &timeout);
    /// @}

    /// @name Buffer manipulation
    /// @{

    /// @brief Throw out any characters within the input buffer.
    /// @throws FlushFailure, NotOpen
    void flush_input_buffer();

    /// @brief Throw out any characters (do not send) within the output buffer.
    /// @throws FlushFailure, NotOpen
    void flush_output_buffer();

    /// @brief Wait until all of the characters in the output buffer are sent,
    /// then return.
    void drain_output_buffer();
    /// @}

    /// @name RTS
    /// @brief Set and clear functions for the RTS ("ready to send") hardware
    /// flow- control bit.
    ///
    /// These are used on a port that is already open. Some devices (like the
    /// Ascension Flock of Birds) use this to reset the device.
    /// @throws RTSFailure, NotOpen
    /// @{
    void set_rts();
    void clear_rts();
    void assign_rts(bool set);
    /// @}

    /// @name Serial Port Exceptions
    /// @{
    struct AlreadyOpen;
    struct CloseFailure;
    struct DrainFailure;
    struct FlushFailure;
    struct NotOpen;
    struct OpenFailure;
    struct RTSFailure;
    struct ReadFailure;
    struct WriteFailure;
    /// @}

private:
    void requiresOpen() const;
    /// @name Non-copyable
    /// @{
    vrpn_SerialPort(vrpn_SerialPort const &);
    vrpn_SerialPort const &operator=(vrpn_SerialPort const &);
    /// @}
    file_handle_type _comm;
    bool _rts_status;
};

struct vrpn_SerialPort::AlreadyOpen : std::logic_error {
    AlreadyOpen()
        : std::logic_error("Tried to open a serial port that was already open.")
    {
    }
};

struct vrpn_SerialPort::NotOpen : std::logic_error {
    NotOpen()
        : std::logic_error("Tried to use a serial port that was not yet open.")
    {
    }
};

struct vrpn_SerialPort::OpenFailure : std::runtime_error {
    OpenFailure()
        : std::runtime_error(
              "Received an error when trying to open serial port.")
    {
    }
};

struct vrpn_SerialPort::CloseFailure : std::runtime_error {
    CloseFailure()
        : std::runtime_error(
              "Received an error when trying to close serial port.")
    {
    }
};

struct vrpn_SerialPort::RTSFailure : std::runtime_error {
    RTSFailure()
        : std::runtime_error("Failed to modify serial port RTS status.")
    {
    }
};

struct vrpn_SerialPort::ReadFailure : std::runtime_error {
    ReadFailure()
        : std::runtime_error("Failure on serial port read.")
    {
    }
};

struct vrpn_SerialPort::WriteFailure : std::runtime_error {
    WriteFailure()
        : std::runtime_error("Failure on serial port write.")
    {
    }
};

struct vrpn_SerialPort::FlushFailure : std::runtime_error {
    FlushFailure()
        : std::runtime_error("Failure on serial port flush.")
    {
    }
};

struct vrpn_SerialPort::DrainFailure : std::runtime_error {
    DrainFailure()
        : std::runtime_error("Failure on serial port drain.")
    {
    }
};

inline bool vrpn_SerialPort::is_open() const { return _comm != -1; }

inline void vrpn_SerialPort::assign_rts(bool set)
{
    if (set) {
        set_rts();
    }
    else {
        clear_rts();
    }
}

inline void vrpn_SerialPort::requiresOpen() const
{
    if (!is_open()) {
        throw NotOpen();
    }
}
