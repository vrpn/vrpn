// -*- c++ -*-
// This file provides an interface to a Novint Falcon. 
// http://home.novint.com/products/novint_falcon.php
// It uses libnifalcon to communicate with the device.
// http://libnifalcon.nonpolynomial.org/
// 
// file:        vrpn_Tracker_NovintFalcon.h
// author:      Axel Kohlmeyer akohlmey@gmail.com 2010-04-02
// copyright:   (C) 2010 Axel Kohlmeyer
// license:     Released to the Public Domain.
// depends:     libnifalcon-1.0.1, libusb-1.0, VRPN 07_26
// tested on:   Linux x86_64 w/ gcc 4.4.1

#ifndef __TRACKER_NOVINTFALCON_H
#define __TRACKER_NOVINTFALCON_H

#include "vrpn_Configure.h"

#if defined(VRPN_USE_LIBNIFALCON)

#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

// Forward declaration for proxy class that wraps 
// the device management of the falcon.
class vrpn_NovintFalcon_Device;


class VRPN_API vrpn_Tracker_NovintFalcon // XXX: add force feedback
    : public vrpn_Tracker, public vrpn_Button {
    
public:
    /// custom constructor
    vrpn_Tracker_NovintFalcon(const char *name, 
                              vrpn_Connection *c = NULL,
                              const int devidx = 0);

    /// destructor
    ~vrpn_Tracker_NovintFalcon();

    /// Called once through each main loop iteration to handle updates.
    virtual void mainloop();

    
protected:

    virtual void reset();
    virtual int get_report(void);
    virtual void clear_values(void);
    
    vrpn_NovintFalcon_Device *_dev; //< device handle
    
    int           _numbuttons;
};

#endif /* defined(VRPN_USE_LIBNIFALCON) */
#endif
