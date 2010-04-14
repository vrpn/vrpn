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

#include "vrpn_Tracker_NovintFalcon.h"

#if defined(VRPN_USE_LIBNIFALCON)
#include "boost/shared_ptr.hpp"
#include "boost/array.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "falcon/core/FalconDevice.h"
#include "falcon/grip/FalconGripFourButton.h"
#include "falcon/firmware/FalconFirmwareNovintSDK.h"
#include "falcon/kinematic/FalconKinematicStamper.h"
#include "falcon/util/FalconFirmwareBinaryNvent.h"

/**************************************************************************/
// define to activate additional messages about
// what the driver is currently trying to do.
#undef VERBOSE

// define for detailed status tracking. very verbose.
#undef VERBOSE2

/**************************************************************************/

/// save some typing
typedef boost::array<double, 3> d_vector;

/// allow to add two vectors
static d_vector operator+(const d_vector &a,const d_vector &b) 
{
    d_vector ret;
    ret[0] = a[0] + b[0];
    ret[1] = a[1] + b[1];
    ret[2] = a[2] + b[2];
    return ret;
}

/// difference of two vectors
static d_vector operator-(const d_vector &a,const d_vector &b) 
{
    d_vector ret;
    ret[0] = a[0] + b[0];
    ret[1] = a[1] + b[1];
    ret[2] = a[2] + b[2];
    return ret;
}
    
/// length of a vector
static double d_length(const d_vector &a) 
{
    double ret;
    ret  = a[0] * a[0];
    ret += a[1] * a[1];
    ret += a[2] * a[2];
    return sqrt(ret);
}

/*************************************************************************/
#if defined(_MSC_VER)
#include <windows.h>
#else
#include <sys/time.h>
#endif

// get the time of day from the system clock, and store it (in seconds)
static double hires_time(void) {
#if defined(_MSC_VER)
  double t;
 
  t = GetTickCount(); 
  t = t / 1000.0;

  return t;
#else
  struct timeval tm;

  gettimeofday(&tm, NULL);
  return((double)(tm.tv_sec) + (double)(tm.tv_usec)/1000000.0);
#endif
}

/*************************************************************************/    

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
#ifdef VERBOSE
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

#ifdef VERBOSE 
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
#ifdef VERBOSE
            fprintf(stderr, "Loading Falcon Firmware\n");
#endif
            int i;
            // 10 chances to load the firmware.
            for (i=0; i<10; ++i) {
                if(!m_falconDevice->getFalconFirmware()->loadFirmware(false, libnifalcon::NOVINT_FALCON_NVENT_FIRMWARE_SIZE, const_cast<uint8_t*>(libnifalcon::NOVINT_FALCON_NVENT_FIRMWARE)))
                {
                    fprintf(stderr, "Firmware loading attempt %d failed.\n", i);
                    // Completely close and reopen device and try again
                    m_falconDevice->close();
                    if(!m_falconDevice->open(devidx))
                    {
                        fprintf(stderr, "Cannot open falcon device %d - Lib Error Code: %d - Device Error Code: %d\n",
                                devidx, m_falconDevice->getErrorCode(), m_falconDevice->getFalconComm()->getDeviceErrorCode());
                        return false;
                    }
                } else {
#ifdef VERBOSE
                    fprintf(stderr, "Falcon firmware successfully loaded.\n");
#endif
                    break;
                }
            }
        } else {
#ifdef VERBOSE
            fprintf(stderr, "Falcon Firmware already loaded.\n");
#endif
        }
        
        bool message = false;
        boost::shared_ptr<libnifalcon::FalconFirmware> f;
        f=m_falconDevice->getFalconFirmware();
        m_falconDevice->runIOLoop();
        while(1) { // XXX: add timeout to declare device dead after a while.
            int i;
            f->setHomingMode(true);
            for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
            if(!f->isHomed()) {
                f->setLEDStatus(libnifalcon::FalconFirmware::RED_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
                if (!message) {
                    fprintf(stderr, "Falcon not currently calibrated. Move control all the way out then push straight all the way in.\n");
                    message = true;
                }
            } else {
                f->setLEDStatus(libnifalcon::FalconFirmware::BLUE_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
#ifdef VERBOSE
                fprintf(stderr, "Falcon calibrated successfully.\n");
#endif
                break;
            }
        }
        
        message = false;
        while(1) { // XXX: add timeout to declare device dead after a while.
            int i;
            
            for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
            d_vector pos = m_falconDevice->getPosition();
            m_oldpos = pos;
            m_oldtime = hires_time();
            
            if (!message) {
                fprintf(stderr, "Move control all the way out to activate device.\n");
                message = true;
            } 
            if (pos[2] > 0.170) { // XXX: value taken from libnifalcon test example
                f->setLEDStatus(libnifalcon::FalconFirmware::GREEN_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < 10; ++i) continue;
#ifdef VERBOSE
                fprintf(stderr, "Falcon activated successfully.\n");
#endif
                break;
            }
        }
        return true;
    };

    // query status of device.
    bool get_status(vrpn_float64 *pos, vrpn_float64 *vel, 
                    vrpn_float64 *quat, vrpn_float64 *vel_quat,
                    vrpn_float64 *vel_dt, unsigned char *buttons) {
        if (!m_falconDevice) 
            return false;

        if(!m_falconDevice->runIOLoop())
            return false;

        // we have no orientation of the effector.
        // so we just pick one. to tell them apart
        // more easily, we just give each a different
        // orientation.
        if (quat) {
            switch (m_flags & MASK_DEVICEIDX) {
              case 0:
                  quat[0] =  0.0;
                  quat[1] =  0.0;
                  quat[2] =  1.0;
                  quat[3] =  0.0;
                  break;
              case 1:
                  quat[0] =  1.0;
                  quat[1] =  0.0;
                  quat[2] =  0.0;
                  quat[3] =  0.0;
                  break;
              default:
                  quat[0] =  1.0;
                  quat[1] =  0.0;
                  quat[2] =  0.0;
                  quat[3] =  0.0;
                  break;
            }
        }
        
        if (vel_quat) {
            vel_quat[0] =  0.0;
            vel_quat[1] =  0.0;
            vel_quat[2] =  0.0;
            vel_quat[3] =  0.0;
        }

        // update position information
        d_vector my_pos = m_falconDevice->getPosition();
        const double convert_pos = 1.0; // empirical. need to measure properly.
        pos[0] = convert_pos * my_pos[0];
        pos[1] = convert_pos * my_pos[1];
        pos[2] = convert_pos * (my_pos[2]-0.125); // apply offset to make z axis data centered.
#ifdef VERBOSE2
        fprintf(stderr, "position %5.3f %5.3f %5.3f\n", pos[0],pos[1],pos[2]);
#endif
        if (vel) {
            double deltat = hires_time() - m_oldtime;
            *vel_dt= deltat;
            if (deltat > 0) {
                vel[0] = convert_pos * (my_pos[0] - m_oldpos[0]) / deltat;
                vel[1] = convert_pos * (my_pos[1] - m_oldpos[1]) / deltat;
                vel[2] = convert_pos * (my_pos[2] - m_oldpos[2]) / deltat;
            } else {
                vel[0] = 0.0;
                vel[1] = 0.0;
                vel[2] = 0.0;
            }
#ifdef VERBOSE2
            fprintf(stderr, "velocity %5.3f %5.3f %5.3f\n", vel[0],vel[1],vel[2]);
#endif
            m_oldpos = my_pos;
            m_oldtime = hires_time();
        }
        
        // update button information
        unsigned int my_buttons = m_falconDevice->getFalconGrip()->getDigitalInputs();
        if (m_flags & GRIP_FOURBUTTON) {
            buttons[0] = (my_buttons & libnifalcon::FalconGripFourButton::CENTER_BUTTON)  ? 1 : 0;
            buttons[1] = (my_buttons & libnifalcon::FalconGripFourButton::PLUS_BUTTON)    ? 1 : 0;
            buttons[2] = (my_buttons & libnifalcon::FalconGripFourButton::MINUS_BUTTON)   ? 1 : 0;
            buttons[3] = (my_buttons & libnifalcon::FalconGripFourButton::FORWARD_BUTTON) ? 1 : 0;
        }
        return true;
    };
    
    // set feedback force
    bool set_force(const d_vector &force) {
        if (!m_falconDevice) 
            return false;

        // update position information
        m_falconDevice->setForce(force);
        if(!m_falconDevice->runIOLoop())
            return false;
    };
    
protected:
    int m_flags;
    libnifalcon::FalconDevice *m_falconDevice;
    d_vector m_oldpos;
    double m_oldtime;

private:
    /// default constructor is disabled
    vrpn_NovintFalcon_Device() {};
    /// copy constructor is disabled
    vrpn_NovintFalcon_Device(const vrpn_NovintFalcon_Device &dev) {};   

public:
    /// constants to parse device flags.
    enum falconflags {
        NONE            = 0x0000, //< empty
        MASK_DEVICEIDX  = 0x000f, //< max. 15 falcons
        KINE_STAMPER    = 0x0010, //< stamper kinematics model
        GRIP_FOURBUTTON = 0x0100  //< 4-button grip (the default)
    };
};

/// force field effect for Novint Falcon. 
class ForceFieldEffect 
{
public:
    /// constructor
    ForceFieldEffect() : m_active(false), m_time(0), m_cutoff(0.0) {}
    /// destructor
    ~ForceFieldEffect() {}

public:
    /// sets member variables to specificed values
    void setForce(vrpn_float32 ori[3], vrpn_float32 frc[3], vrpn_float32 jac[3][3], vrpn_float32 rad) {
        int i,j;
        for (i=0; i < 3; i++) {
            m_origin[i] = ori[i];
            m_addforce[i]  = frc[i];
            for (j=0; j < 3; j++) {
                m_jacobian[i][j] = jac[i][j];
            }
        }
        m_cutoff = rad;
    }

    /// start effect
    virtual bool start() {
        m_active = true;
        m_time = 0.0;
        // this will delay the effect until it 
        // is (re-)initialized with setForce()
        m_cutoff = 0.0; 
        return m_active;
    }
    
    /// stop effect
    virtual void stop() {
        m_active = false;
    }
    
    /// query active status
    virtual bool isActive() const { return m_active; }

    /// calculate the effect force
    d_vector calcForce(const d_vector &pos) {
        d_vector force, offset;
        force.assign(0.0);
        if (m_active) {
            offset = pos - m_origin;
            // no force too far away.
            if (d_length(offset) > m_cutoff) {
                return force;
            }
            // Compute the force, which is the constant force plus
            // a force that varies as the effector position deviates
            // from the origin of the force field.  The Jacobian
            // determines how the force changes in different directions
            // away from the origin (generalizing spring forces of different
            // magnitudes that constrain the Phantom to the point of the
            // origin, to a line containing the origin, or to a plane
            // containing the origin).
            force = m_addforce;
            int i,j;
            for (i=0; i<3; ++i) {
                for (j=0; j<3; ++j) {
                    force[i] += offset[j]*m_jacobian[i][j];
                }
            }
        }
        return force;
    }

protected:
    bool   m_active;            /// true if effect is active
    double m_time;              /// time since last update
    double m_cutoff;            /// force cutoff radius
    d_vector m_origin;          /// origin of effect
    d_vector m_addforce;        /// additional constant force 
    double m_jacobian[3][3];    /// describes increase in force away from origin in differnt directions.    
};

/// callback for receiving new force field effects.
static int VRPN_CALLBACK handle_forcefield_change_message(void *userdata, vrpn_HANDLERPARAM p)
{
  vrpn_Tracker_NovintFalcon *dev = (vrpn_Tracker_NovintFalcon *)userdata;
  dev->update_forcefield_effect(p);
  return 0;
}

/// class to collect all force generating objects.
class vrpn_NovintFalcon_ForceObjects {
public:
    boost::ptr_vector<ForceFieldEffect> m_FFEffects;
    
    d_vector m_curforce;
    d_vector m_curpos;
    d_vector m_curvel;
    
    /// constructor
    vrpn_NovintFalcon_ForceObjects() {
            m_curforce.assign(0.0);
            m_curpos.assign(0.0);
    };
    /// destructor
    ~vrpn_NovintFalcon_ForceObjects() {};
public:
    /// compute the resulting force of all force field objects.
    d_vector getObjForce(vrpn_float64 *pos, vrpn_float64 *vel) {
        int i;
        
        for (i=0; i<3; ++i) {
            m_curforce[i]=0.0;
            m_curpos[i]=pos[i];
            m_curvel[i]=vel[i];
        }

        // force field objects
        int nobj = m_FFEffects.size();
        for (i=0; i<nobj; ++i) {
            m_curforce = m_curforce + m_FFEffects[i].calcForce (m_curpos);
        }
        return m_curforce;
    };
};

/// custom contructor for falcon device tracker. handles a single device.
vrpn_Tracker_NovintFalcon::vrpn_Tracker_NovintFalcon(const char *name, 
                                                     vrpn_Connection *c, 
                                                     const int devidx,
                                                     const char *grip,
                                                     const char *kine)
        : vrpn_Tracker(name, c), vrpn_Button(name, c), 
          vrpn_ForceDevice(name, c), m_dev(NULL), m_obj(NULL)
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

    if (register_autodeleted_handler(forcefield_message_id, 
                                     handle_forcefield_change_message, this, vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr,"vrpn_Tracker_NovintFalcon:can't register force handler\n");
        return;
    }
    status = vrpn_TRACKER_RESETTING;
}

void vrpn_Tracker_NovintFalcon::clear_values()
{
    // nothing to do
    if (m_devflags < 0) return;

	int i;
	for (i=0; i <vrpn_Button::num_buttons; i++)
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;

    if (m_obj) delete m_obj;
    m_obj = new vrpn_NovintFalcon_ForceObjects;

    // add dummy effect object
    ForceFieldEffect *ffe = new ForceFieldEffect;
    ffe->stop();
    m_obj->m_FFEffects.push_back(ffe);
}

vrpn_Tracker_NovintFalcon::~vrpn_Tracker_NovintFalcon()
{
    if (m_dev)
        delete m_dev;
    if (m_obj)
        delete m_obj;
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
        if (!m_dev->get_status(pos, vel, d_quat, vel_quat, &vel_quat_dt, buttons))
            return 0;
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
        len = vrpn_Tracker::encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, vrpn_Tracker::timestamp, velocity_m_id, d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
        }
    }
}

void vrpn_Tracker_NovintFalcon::handle_forces(void)
{
    if (!m_dev) return;
    if (!m_obj) return;

    // we have just updated our published position and can use that
    d_vector force= m_obj->getObjForce(pos,vel);
    m_dev->set_force(force);
}


int vrpn_Tracker_NovintFalcon::update_forcefield_effect(vrpn_HANDLERPARAM p)
{
    if (!m_obj) return 1;

    vrpn_float32 center[3];
    vrpn_float32 force[3];
    vrpn_float32 jacobian[3][3];
    vrpn_float32 radius;

    decode_forcefield(p.buffer, p.payload_len, center, force, jacobian, &radius);
    // XXX: only one force field effect. sufficient for VMD.
    // we have just updated our published position and can use that
    m_obj->m_FFEffects[0].start();
    m_obj->m_FFEffects[0].setForce(center, force, jacobian, radius);
    return 0;
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
                vrpn_Button::report_changes();
                handle_forces();
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
