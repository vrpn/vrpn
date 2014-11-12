/**
    @file
    @brief Implementation

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

// Internal Includes
#include "vrpn_SerialPort.h"
#include "vrpn_Serial.h"

// Library/third-party includes
// - none

// Standard includes
#include <stdio.h>
#include <vector>
#include <exception>
#include <algorithm>
#include <limits>

typedef std::vector<unsigned char> DynamicBufferType;

vrpn_SerialPort::vrpn_SerialPort(const char *portname, long baud, int charsize,
                                 vrpn_SER_PARITY parity)
    : _comm(vrpn_open_commport(portname, baud, charsize, parity))
    , _rts_status(false)
{
    if (!is_open()) {
        throw OpenFailure();
    }
}

vrpn_SerialPort::vrpn_SerialPort()
    : _comm(-1)
    , _rts_status(false)
{
}

vrpn_SerialPort::~vrpn_SerialPort()
{
    if (is_open()) {
        try {
            close();
        }
        catch (std::exception &) {
            fprintf(stderr, "Error when closing serial port in destructor.\n");
        }
    }
}

void vrpn_SerialPort::open(const char *portname, long baud, int charsize,
                           vrpn_SER_PARITY parity)
{
    if (is_open()) {
        throw AlreadyOpen();
    }
    _comm = vrpn_open_commport(portname, baud, charsize, parity);
    if (!is_open()) {

        throw OpenFailure();
    }
}

void vrpn_SerialPort::close()
{
    requiresOpen();
    int ret = vrpn_close_commport(_comm);
    if (ret != 0) {
        throw CloseFailure();
    }
}

int vrpn_SerialPort::write(std::string const &buffer)
{
    if (buffer.size() > 0) {
        DynamicBufferType buf(buffer.c_str(), buffer.c_str() + buffer.size());
        return write(&(buf[0]), static_cast<int>(buffer.size()));
    }
    return 0;
}

int vrpn_SerialPort::write(const unsigned char *buffer, int bytes)
{
    requiresOpen();
    int ret = vrpn_write_characters(_comm, buffer, bytes);
    if (ret == -1) {
        throw WriteFailure();
    }
    return ret;
}

int vrpn_SerialPort::read_available_characters(unsigned char *buffer, int count)
{
    requiresOpen();
    int ret = vrpn_read_available_characters(_comm, buffer, count);
    if (ret == -1) {
        throw ReadFailure();
    }

    return ret;
}

std::string vrpn_SerialPort::read_available_characters(int count)
{
    std::string retString;
    unsigned int numRead = 0;
    unsigned int thisRead = 0;
    static const unsigned int BUFSIZE = 256;
    unsigned char buf[BUFSIZE];
    unsigned int needed = BUFSIZE - 1;
    do {
        if (count > -1) {
            needed = (std::min)(count - numRead, BUFSIZE - 1);
        }
        thisRead = read_available_characters(buf, needed);
        if (thisRead != 0) {
            retString.append(&(buf[0]), &(buf[0]) + thisRead);
            numRead += thisRead;
        }
    } while (thisRead != 0 &&
             (static_cast<int>(numRead) < count || count == -1));
    return retString;
}

int vrpn_SerialPort::read_available_characters(unsigned char *buffer, int count,
                                               struct timeval &timeout)
{
    requiresOpen();
    int ret = vrpn_read_available_characters(_comm, buffer, count, &timeout);
    if (ret == -1) {
        throw ReadFailure();
    }
    return ret;
}

std::string vrpn_SerialPort::read_available_characters(int count,
                                                       struct timeval &timeout)
{
    if (count == std::numeric_limits<int>::max()) {
        /// overflow safety
        throw ReadFailure();
    }
    DynamicBufferType buf(count + 1);
    int ret = read_available_characters(&(buf[0]), count, timeout);
    return std::string(&(buf[0]), &(buf[0]) + ret);
}

void vrpn_SerialPort::flush_input_buffer()
{
    requiresOpen();
    int ret = vrpn_flush_input_buffer(_comm);
    if (ret == -1) {
        throw FlushFailure();
    }
}

void vrpn_SerialPort::flush_output_buffer()
{
    requiresOpen();
    int ret = vrpn_flush_output_buffer(_comm);
    if (ret == -1) {
        throw FlushFailure();
    }
}

void vrpn_SerialPort::drain_output_buffer()
{
    requiresOpen();
    int ret = vrpn_drain_output_buffer(_comm);
    if (ret == -1) {
        throw DrainFailure();
    }
}

void vrpn_SerialPort::set_rts()
{
    requiresOpen();
    int ret = vrpn_set_rts(_comm);
    if (ret == -1) {
        throw RTSFailure();
    }
}

void vrpn_SerialPort::clear_rts()
{
    requiresOpen();
    int ret = vrpn_clear_rts(_comm);
    if (ret == -1) {
        throw RTSFailure();
    }
}
