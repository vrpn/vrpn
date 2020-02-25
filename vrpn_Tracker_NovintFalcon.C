// -*- c++ -*-
// This file provides an interface to a Novint Falcon.
// http://home.novint.com/products/novint_falcon.php
// It uses libnifalcon to communicate with the device.
// http://libnifalcon.nonpolynomial.org/
//
// File:        vrpn_Tracker_NovintFalcon.C
// Author:      Axel Kohlmeyer akohlmey@gmail.com
// Date:        2010-08-12
// Copyright:   (C) 2010 Axel Kohlmeyer
// License:     Boost Software License 1.0
// depends:     libnifalcon-1.0.1+, libusb-1.0, boost 1.39, VRPN 07_27
// tested on:   Linux x86_64 w/ gcc 4.4.1

#include "vrpn_Tracker_NovintFalcon.h"

#ifdef VRPN_USE_LIBNIFALCON
#include <vector>
#include <array>
#include <memory>
#include "falcon/core/FalconDevice.h"
#include "falcon/firmware/FalconFirmwareNovintSDK.h"
#include "falcon/grip/FalconGripFourButton.h"
#include "falcon/kinematic/FalconKinematicStamper.h"
#include "falcon/util/FalconFirmwareBinaryNvent.h"

/**************************************************************************/
// number of retries for the I/O loop.
#define FALCON_NUM_RETRIES 10

// define to activate additional messages about
// what the driver is currently trying to do.
#undef VERBOSE

// define for detailed status tracking. very verbose.
#undef VERBOSE2
/**************************************************************************/

// save us some typing
typedef std::array<double, 3> d_vector;

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
// compute time difference in microseconds.
static double timediff(struct timeval t1, struct timeval t2) {
    return (t1.tv_usec - t2.tv_usec)*1.0 + 1000000.0 * (t1.tv_sec - t2.tv_sec);
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
                try {
                  delete m_falconDevice;
                } catch (...) {
                  fprintf(stderr, "vrpn_NovintFalcon_Device::vrpn_NovintFalcon_Device(): delete failed\n");
                  return;
                }
                m_falconDevice=NULL;
                return;
            }

            if (m_flags & GRIP_FOURBUTTON) {
                m_falconDevice->setFalconGrip<libnifalcon::FalconGripFourButton>();
            } else {
                try {
                  delete m_falconDevice;
                } catch (...) {
                  fprintf(stderr, "vrpn_NovintFalcon_Device::vrpn_NovintFalcon_Device(): delete failed\n");
                  return;
                }
                m_falconDevice=NULL;
                return;
            }
        }

    ~vrpn_NovintFalcon_Device() {
#ifdef VERBOSE
        fprintf(stderr, "Closing Falcon device %d.\n", m_flags & MASK_DEVICEIDX);
#endif
        if (m_falconDevice) {
            std::shared_ptr<libnifalcon::FalconFirmware> f;
            f=m_falconDevice->getFalconFirmware();
            if(f) {
                f->setLEDStatus(libnifalcon::FalconFirmware::RED_LED |
                                libnifalcon::FalconFirmware::BLUE_LED |
                                libnifalcon::FalconFirmware::GREEN_LED);
                for (int i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
            }
            m_falconDevice->close();
        }
        try {
          delete m_falconDevice;
        } catch (...) {
          fprintf(stderr, "vrpn_NovintFalcon_Device::~vrpn_NovintFalcon_Device(): delete failed\n");
          return;
        }
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
        fprintf(stderr, "Trying to open Falcon device %d/%d.\n", devidx, count);
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
            for (i=0; i<FALCON_NUM_RETRIES; ++i) {
                if(!m_falconDevice->getFalconFirmware()->loadFirmware(true, libnifalcon::NOVINT_FALCON_NVENT_FIRMWARE_SIZE, const_cast<uint8_t*>(libnifalcon::NOVINT_FALCON_NVENT_FIRMWARE)))
                {
                    fprintf(stderr, "Firmware loading attempt %d failed.\n", i);
                    if(i==FALCON_NUM_RETRIES-1){
                        fprintf(stderr, "Cannot load falcon device %d firmware - Device Error Code: %d - Comm Lib Error Code: %d\n",
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

        int i;
        bool message = false;
        std::shared_ptr<libnifalcon::FalconFirmware> f;
        f=m_falconDevice->getFalconFirmware();
        for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
        while(1) { // XXX: add timeout to declare device dead after a while.
            f->setHomingMode(true);
            for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
            if(!f->isHomed()) {
                f->setLEDStatus(libnifalcon::FalconFirmware::RED_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
                if (!message) {
                    fprintf(stderr, "Falcon not currently calibrated. Move control all the way out then push straight all the way in.\n");
                    message = true;
                }
            } else {
                f->setLEDStatus(libnifalcon::FalconFirmware::BLUE_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
#ifdef VERBOSE
                fprintf(stderr, "Falcon calibrated successfully.\n");
#endif
                break;
            }
        }

        message = false;
        while(1) { // XXX: add timeout to declare device dead after a while.
            int i;

            for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
            d_vector pos = m_falconDevice->getPosition();
            m_oldpos = pos;
            vrpn_gettimeofday(&m_oldtime, NULL);

            if (!message) {
                fprintf(stderr, "Move control all the way out until led turns green to activate device.\n");
                message = true;
            }
            if (pos[2] > 0.170) { // XXX: value taken from libnifalcon test example
                f->setLEDStatus(libnifalcon::FalconFirmware::GREEN_LED);
                for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
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

        int i;
        for (i=0; !m_falconDevice->runIOLoop() && i < FALCON_NUM_RETRIES; ++i) continue;
               if ( i == FALCON_NUM_RETRIES )
                       return false;

        // we have no orientation of the effector.
        // so we just pick the identity quaternion.
        if (quat) {
            quat[0] =  0.0;
            quat[1] =  0.0;
            quat[2] =  0.0;
            quat[3] =  1.0;
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
            struct timeval current_time;
            vrpn_gettimeofday(&current_time, NULL);
            double deltat = timediff(current_time, m_oldtime);
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
            m_oldtime.tv_sec = current_time.tv_sec;
            m_oldtime.tv_usec = current_time.tv_usec;
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

        return true;
    };

protected:
    int m_flags;
    libnifalcon::FalconDevice *m_falconDevice;
    d_vector m_oldpos;
    struct timeval m_oldtime;

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
    ForceFieldEffect() : m_active(false), m_time(0), m_cutoff(0.0), m_damping(0.9)
    {
        int i,j;
        for (i=0; i < 3; i++) {
            m_origin[i] = 0.0;
            m_addforce[i]  = 0.0;
            for (j=0; j < 3; j++) {
                m_jacobian[i][j] = 0.0;
            }
        }
    };

    /// destructor
    ~ForceFieldEffect() {}

public:
    /// sets member variables to specificed values
    void setForce(vrpn_float32 ori[3], vrpn_float32 frc[3], vrpn_float32 jac[3][3], vrpn_float32 rad) {
        int i,j;
        for (i=0; i < 3; i++) {
            m_neworig[i] = ori[i];
            m_newadd[i]  = frc[i];
            for (j=0; j < 3; j++) {
                m_newjacob[i][j] = jac[i][j];
            }
        }
        m_newcut = rad;
    }

    /// updates damping factor
    void setDamping(double damp) {
        m_damping = damp;
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
        force.fill(0.0);
        if (m_active) {
            // apply damping to effect values
            const double mix = 1.0 - m_damping;
            int i,j;
            for (i=0; i < 3; i++) {
                m_origin[i]    = m_damping*m_origin[i] + mix*m_neworig[i];
                m_addforce[i]  = m_damping*m_addforce[i] + mix*m_newadd[i];
                for (j=0; j < 3; j++) {
                    m_jacobian[i][j] = m_damping*m_jacobian[i][j] + mix*m_newjacob[i][j];
                }
            }
            m_cutoff = m_damping*m_cutoff + mix*m_newcut;

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
    double m_jacobian[3][3];    /// describes increase in force away from origin in different directions.
    double m_newcut;            /// new force cutoff radius
    d_vector m_neworig;         /// new effect origin handed over at update
    d_vector m_newadd;          /// new additional constant force
    double m_newjacob[3][3];    /// new jacobian handed over at update
    double m_damping;           /// damping coefficient for updates
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
    std::vector<ForceFieldEffect*> m_FFEffects;

protected:
    d_vector m_curforce; //< collected force value
    d_vector m_curpos;   //< position for force calculation
    d_vector m_curvel;   //< velocity for force calculation

public:
    /// constructor
    vrpn_NovintFalcon_ForceObjects() {
            m_curforce.fill(0.0);
            m_curpos.fill(0.0);
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
            m_curforce = m_curforce + m_FFEffects[i]->calcForce (m_curpos);
        }
        return m_curforce;
    };
};

/// custom contructor for falcon device tracker. handles a single device.
vrpn_Tracker_NovintFalcon::vrpn_Tracker_NovintFalcon(const char *name,
                                                     vrpn_Connection *c,
                                                     const int devidx,
                                                     const char *grip,
                                                     const char *kine,
                                                     const char *damp)
        : vrpn_Tracker(name, c), vrpn_Button_Filter(name, c),
          vrpn_ForceDevice(name, c), m_dev(NULL), m_obj(NULL), m_update_rate(1000.0), m_damp(0.9)
{
    m_devflags=vrpn_NovintFalcon_Device::MASK_DEVICEIDX & devidx;
    if (grip != NULL) {
        if (0 == strcmp(grip,"4-button")) {
            m_devflags |= vrpn_NovintFalcon_Device::GRIP_FOURBUTTON;
            vrpn_Button::num_buttons = 4;
        } else {
            fprintf(stderr, "WARNING: Unknown grip for Novint Falcon #%d: %s \n", devidx, grip);
            m_devflags = -1;
            return;
        }
    }

    if (kine != NULL) {
        if (0 == strcmp(kine,"stamper")) {
            m_devflags |= vrpn_NovintFalcon_Device::KINE_STAMPER;
        } else {
            fprintf(stderr, "WARNING: Unknown kinematic model for Novint Falcon #%d: %s \n", devidx, kine);
            m_devflags = -1;
            return;
        }
    }

    if (damp != NULL) {
        vrpn_float64 val= atof(damp);
        if (val >= 1.0 && val <= 10000.0) { 
            m_damp = 1.0 - 1.0/val;
        } else {
            fprintf(stderr, "WARNING: Ignoring illegal force effect damping factor: %g \n", val);
        }
    }
    clear_values();

    if (register_autodeleted_handler(forcefield_message_id,
                                     handle_forcefield_change_message, this, vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr,"vrpn_Tracker_NovintFalcon:can't register force handler\n");
        return;
    }
    vrpn_gettimeofday(&m_timestamp, NULL);
    status = vrpn_TRACKER_RESETTING;
}

void vrpn_Tracker_NovintFalcon::clear_values()
{
    // nothing to do
    if (m_devflags < 0) return;

    int i;
    for (i=0; i <vrpn_Button::num_buttons; i++)
        vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;

    if (m_obj) {
      try {
        delete m_obj;
      } catch (...) {
        fprintf(stderr, "vrpn_Tracker_NovintFalcon::clear_values(): delete failed\n");
        return;
      }
    }
    try { m_obj = new vrpn_NovintFalcon_ForceObjects; }
    catch (...) { m_obj = NULL; return; }

    // add dummy effect object
    ForceFieldEffect *ffe;
    try { ffe = new ForceFieldEffect; }
    catch (...) { return; }
    ffe->setDamping(m_damp);
    ffe->stop();
    m_obj->m_FFEffects.push_back(ffe);
}

vrpn_Tracker_NovintFalcon::~vrpn_Tracker_NovintFalcon()
{
  try {
    if (m_dev) delete m_dev;
    if (m_obj) delete m_obj;
  } catch (...) {
    fprintf(stderr, "vrpn_Tracker_NovintFalcon::~vrpn_Tracker_NovintFalcon(): delete failed\n");
    return;
  }
}

void vrpn_Tracker_NovintFalcon::reset()
{

    // nothing to do
    if (m_devflags < 0) return;

    int ret, i;
    clear_values();

    fprintf(stderr, "Resetting the NovintFalcon #%d\n",
        vrpn_NovintFalcon_Device::MASK_DEVICEIDX & m_devflags);

    if (m_dev) {
      try {
        delete m_dev;
      } catch (...) {
        fprintf(stderr, "vrpn_Tracker_NovintFalcon::reset(): delete failed\n");
        return;
      }
    }

    try { m_dev = new vrpn_NovintFalcon_Device(m_devflags); }
    catch (...) {
#ifdef VERBOSE
        fprintf(stderr, "Device constructor failed!\n");
#endif
        status = vrpn_TRACKER_FAIL;
        m_dev = NULL;
        return;
    }

    if (!m_dev->init()) {
#ifdef VERBOSE
                fprintf(stderr, "Device init failed!\n");
#endif
        status = vrpn_TRACKER_FAIL;
        return;
    }

    fprintf(stderr, "Reset Completed.\n");
    status = vrpn_TRACKER_SYNCING;    // We're trying for a new reading
}

int vrpn_Tracker_NovintFalcon::get_report(void)
{
    if (!m_dev)
        return 0;

    if (status == vrpn_TRACKER_SYNCING) {
        if (m_dev->get_status(pos, vel, d_quat, vel_quat, &vel_quat_dt, buttons)) {
            // if all buttons are pressed. we force a reset.
            int i,j;
            j=0;
            for (i=0; i < num_buttons; i++)
                j += buttons[i];
            // all buttons pressed
            if (j == num_buttons) {
                status = vrpn_TRACKER_FAIL;
                return 0;
            }
        } else {
            status = vrpn_TRACKER_FAIL;
            return 0;
        }
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
        if (d_connection->pack_message(len, m_timestamp, position_m_id, d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
        }
        len = vrpn_Tracker::encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, m_timestamp, velocity_m_id, d_sender_id, msgbuf,
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
    m_obj->m_FFEffects[0]->start();
    m_obj->m_FFEffects[0]->setForce(center, force, jacobian, radius);
    return 0;
}


void vrpn_Tracker_NovintFalcon::mainloop()
{
    struct timeval current_time;
    server_mainloop();

    // no need to report more often than we can poll the device
    vrpn_gettimeofday(&current_time, NULL);
    if ( timediff(current_time, m_timestamp) >= 1000000.0/m_update_rate) {

        // Update the time
        m_timestamp.tv_sec = current_time.tv_sec;
        m_timestamp.tv_usec = current_time.tv_usec;
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
              break;

          default:
              fprintf(stderr, "NovintFalcon #%d , unknown status message: %d)\n",
                  vrpn_NovintFalcon_Device::MASK_DEVICEIDX & m_devflags, status);
              break;
        }
    }
}

#endif
