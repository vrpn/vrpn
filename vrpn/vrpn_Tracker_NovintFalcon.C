// -*- c++ -*-
// This file provides an interface to a Novint Falcon. 
// http://home.novint.com/products/novint_falcon.php
// It uses libnifalcon to communicate with the device.
// http://libnifalcon.nonpolynomial.org/
// 
// file:        vrpn_Tracker_NovintFalcon.C
// author:      Axel Kohlmeyer akohlmey@gmail.com 2010-04-02
// copyright:   (C) 2010 Axel Kohlmeyer
// license:     Released to the Public Domain.
// depends:     libnifalcon-1.0.1, libusb-1.0, VRPN 07_26
// tested on:   Linux x86_64 w/ gcc 4.4.1

#include <ctype.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#define M_PI 3.14159265358979323846
#endif

#include "vrpn_Tracker_NovintFalcon.h"

#if defined(VRPN_USE_LIBNIFALCON)
#include "boost/shared_ptr.hpp"
#include "falcon/core/FalconDevice.h"
#include "falcon/grip/FalconGripFourButton.h"
#include "falcon/firmware/FalconFirmwareNovintSDK.h"
#include "falcon/kinematic/FalconKinematicStamper.h"
#include "falcon/util/FalconFirmwareBinaryNvent.h"
#define FALCON_DEBUG 1

class vrpn_NovintFalcon_Device 
{
public:
    vrpn_NovintFalcon_Device(int flags) 
            : m_flags(flags)
        {
            if (m_flags < 0) {
                m_falconDevice = NULL;
                return;
            } else {
                m_falconDevice = new libnifalcon::FalconDevice;
                m_falconDevice->setFalconFirmware<libnifalcon::FalconFirmwareNovintSDK>();
            }

            if (m_flags & KINE_STAMPER) {
                m_falconDevice->setFalconKinematic<libnifalcon::FalconKinematicStamper>();
            } else {
                delete m_falconDevice;
                m_falconDevice=NULL;
                return;
            }
            
            if (m_flags & GRIP_FOURBUTTON) {
                m_falconDevice->setFalconGrip<libnifalcon::FalconGripFourButton>();
            } else {
                delete m_falconDevice;
                m_falconDevice=NULL;
                return;
            }
        }

    ~vrpn_NovintFalcon_Device() {
#if FALCON_DEBUG
        fprintf(stderr, "Closing Falcon device %d.\n", m_flags & MASK_DEVICEIDX);
#endif
        if (m_falconDevice) {
            m_falconDevice->close();
        }
        delete m_falconDevice;
        m_flags=-1;
    };

public:
    // open device, load firmware and calibrate.
    bool init() {
        if (!m_falconDevice) 
            return false;
        
        unsigned int count;
        m_falconDevice->getDeviceCount(count);
        int devidx = m_flags & MASK_DEVICEIDX;

#if FALCON_DEBUG
        fprintf(stderr, "Trying to open Falcon device %d/%d\n", devidx, count);
#endif
        if (devidx < count) {
            if (!m_falconDevice->open(devidx)) {
                fprintf(stderr, "Cannot open falcon device %d - Lib Error Code: %d - Device Error Code: %d\n",
                        devidx, m_falconDevice->getErrorCode(), m_falconDevice->getFalconComm()->getDeviceErrorCode());
                return false;
            }
        } else {
            fprintf(stderr, "Trying to open non-existing Novint Falcon device %d\n", devidx);
            return false;
        }

        if (!m_falconDevice->isFirmwareLoaded()) {
#if FALCON_DEBUG            
            fprintf(stderr, "Loading Falcon Firmware\n");
#endif
            int i;
            for (i=0; i<10; ++i) {
                if(!m_falconDevice->getFalconFirmware()->loadFirmware(false, libnifalcon::NOVINT_FALCON_NVENT_FIRMWARE_SIZE, const_cast<uint8_t*>(libnifalcon::NOVINT_FALCON_NVENT_FIRMWARE)))
                {
#if FALCON_DEBUG            
                    fprintf(stderr, "Firmware loading attempt %d failed.\n", i);
#endif
                    //Completely close and reopen
                    m_falconDevice->close();
                    if(!m_falconDevice->open(devidx))
                    {
                        fprintf(stderr, "Cannot open falcon device %d - Lib Error Code: %d - Device Error Code: %d\n",
                                devidx, m_falconDevice->getErrorCode(), m_falconDevice->getFalconComm()->getDeviceErrorCode());
                        return false;
                    }
                } else {
#if FALCON_DEBUG            
                    fprintf(stderr, "Falcon firmware successfully loaded.\n");
#endif
                    break;
                }
            }
        } else {
#if FALCON_DEBUG            
            fprintf(stderr, "Falcon Firmware already loaded.\n");
#endif
        }
        
        bool message = false;
        while(1) { // XXX: add timeout to declare device dead after a while.
            int i;
            
            m_falconDevice->getFalconFirmware()->setHomingMode(true);
            for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
            if(!m_falconDevice->getFalconFirmware()->isHomed()) {
                m_falconDevice->getFalconFirmware()->setLEDStatus(libnifalcon::FalconFirmware::RED_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
                if (!message) {
                    fprintf(stderr, "Falcon not currently calibrated. Move control all the way out then push straight all the way in.\n");
                    message = true;
                }
            } else {
                m_falconDevice->getFalconFirmware()->setLEDStatus(libnifalcon::FalconFirmware::BLUE_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
                fprintf(stderr, "Falcon calibrated successfully.\n");
                break;
            }
        }
        
        message = false;
        
        while(1) { // XXX: add timeout to declare device dead after a while.
            int i;
            
            for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
            boost::array<double, 3> pos = m_falconDevice->getPosition();
            
            if (!message) {
                fprintf(stderr, "Move control all the way out to activate device.\n");
                message = true;
            } 
            if (pos[2] > 0.170) {
                m_falconDevice->getFalconFirmware()->setLEDStatus(libnifalcon::FalconFirmware::GREEN_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
                fprintf(stderr, "Falcon activated successfully.\n");
                break;
            }
        }
        return true;
    };

    // open device, load firmware and calibrate.
    bool get_status(vrpn_float64 *pos, vrpn_float64 *quat, unsigned char *buttons) {
        if (!m_falconDevice) 
            return false;

        if(!m_falconDevice->runIOLoop())
            return false;

        quat[0] = 1.0;
        quat[1] = 0.0;
        quat[2] = 0.0;
        quat[3] = 0.0;

        // update position information
        boost::array<double, 3> my_pos = m_falconDevice->getPosition();
        const double convert_pos = 1.0;
        pos[0] = convert_pos * my_pos[0];
        pos[1] = convert_pos * my_pos[1];
        pos[2] = convert_pos * my_pos[2];
#if FALCON_DEBUG & 0
        fprintf(stderr, "position %5.3f %5.3f %5.3f\n", pos[0],pos[1],pos[2]);
#endif
        
        // update button information
        unsigned int my_buttons = m_falconDevice->getFalconGrip()->getDigitalInputs();
        int num_buttons = m_falconDevice->getFalconGrip()->getNumDigitalInputs();
        int i;
        for (i=0; i < num_buttons; ++i) {
#if FALCON_DEBUG & 0
            fprintf(stderr, "button [%d]: %s\n", i, (my_buttons & 1<<i) ? "on" : "off");
#endif
            buttons[i] = (my_buttons & 1<<i) ? vrpn_BUTTON_TOGGLE_ON : vrpn_BUTTON_TOGGLE_OFF;
        }
        return true;
    };
    
protected:
    int m_flags;
    libnifalcon::FalconDevice *m_falconDevice;

private:
    // disable default and copy constructor
    vrpn_NovintFalcon_Device() {};
    vrpn_NovintFalcon_Device(const vrpn_NovintFalcon_Device &dev) {};   

public:
    enum falconflags {
        NONE            = 0x0000, //< empty
        MASK_DEVICEIDX  = 0x000f, //< max. 15 falcons
        KINE_STAMPER    = 0x0010, //< stamper kinematics model
        GRIP_FOURBUTTON = 0x0100  //< 4-button grip (the default)
    };
};

vrpn_Tracker_NovintFalcon::vrpn_Tracker_NovintFalcon(const char *name, 
                                                     vrpn_Connection *c, 
                                                     const int devidx,
                                                     const char *grip,
                                                     const char *kine)
        : vrpn_Tracker(name, c), vrpn_Button(name, c), m_dev(NULL)
{
    m_devflags=vrpn_NovintFalcon_Device::MASK_DEVICEIDX & devidx;
    if (grip != NULL) {
        if (0 == strcmp(grip,"4-button")) {
            m_devflags |= vrpn_NovintFalcon_Device::GRIP_FOURBUTTON;
            vrpn_Button::num_buttons = 4;
        } else {
            fprintf(stderr, "WARNING: Unknown grip for Novint Falcon: %s \n", grip);
            m_devflags = -1;
            return;
        }
    }

    if (kine != NULL) {
        if (0 == strcmp(kine,"stamper")) {
            m_devflags |= vrpn_NovintFalcon_Device::KINE_STAMPER;
        } else {
            fprintf(stderr, "WARNING: Unknown kinematic model for Novint Falcon: %s \n", kine);
            m_devflags = -1;
            return;
        }
    }
	clear_values();
    status = vrpn_TRACKER_RESETTING;
}

void vrpn_Tracker_NovintFalcon::clear_values()
{
    // nothing to do
    if (m_devflags < 0) return;

	int i;
	for (i=0; i <vrpn_Button::num_buttons; i++)
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
}

vrpn_Tracker_NovintFalcon::~vrpn_Tracker_NovintFalcon()
{
    if (m_dev)
        delete m_dev;
}

void vrpn_Tracker_NovintFalcon::reset()
{

    // nothing to do
    if (m_devflags < 0) return;
    
	static int numResets = 0;	// How many resets have we tried?
	int ret, i;
    numResets++;		  	// We're trying another reset

	clear_values();

	fprintf(stderr, "Resetting the NovintFalcon (attempt %d)\n", numResets);

    if (m_dev)
        delete m_dev;
    
    m_dev = new vrpn_NovintFalcon_Device(m_devflags);
    if (!m_dev) {
		status = vrpn_TRACKER_FAIL;
		return;
	}

    if (!m_dev->init()) {
		status = vrpn_TRACKER_FAIL;
		return;
	}

	fprintf(stderr, "Reset Completed.\n");
	status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
}

int vrpn_Tracker_NovintFalcon::get_report(void)
{
    if (!m_dev) 
        return 0;
    
    if (status == vrpn_TRACKER_SYNCING) {
        if (m_dev->get_status(pos, d_quat, buttons))
            send_report();
            vrpn_Button::report_changes();
    }
    status = vrpn_TRACKER_SYNCING;

#ifdef VERBOSE2
      print_latest_report();
#endif

   return 1;
}

void vrpn_Tracker_NovintFalcon::send_report(void)
{
    if (d_connection) {
        char        msgbuf[1000];
        int len = vrpn_Tracker::encode_to(msgbuf);
        if (d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
        }
    }
}

void vrpn_Tracker_NovintFalcon::mainloop()
{
	server_mainloop();

	switch(status)
	{
		case vrpn_TRACKER_AWAITING_STATION:
		case vrpn_TRACKER_PARTIAL:
		case vrpn_TRACKER_SYNCING:
			if (get_report()) {
                send_report();
            }
			break;
		case vrpn_TRACKER_RESETTING:
			reset();
			break;

		case vrpn_TRACKER_FAIL:
			fprintf(stderr, "NovintFalcon failed, trying to reset (Try power cycle if more than 4 attempts made)\n");
			status = vrpn_TRACKER_RESETTING;
			break;
            
		default:
			break;
	}
}

#endif
