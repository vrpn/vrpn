#ifndef _OHMM_H
#define _OHMM_H

#include "vrpn_Connection.h"

#ifdef _WIN32
#include "windows.h"
#include "orpx.h"
#endif

// status values
#define CHANGING        (0)     // in the process of changing parameters
#define COMMUTATION     (1)     // waiting for scanner switching
#define WAITING         (2)     // waiting for wait_time ms
#define MEASURING       (3)     // recording actual measurements
#define M_OVERFLO       (4)     // measurement overflow, meter saturated
#define M_UNDERFLO      (5)     // measurement underflow, range should
				// be decreased for greater precision
#define R_OVERFLO       (6)     // range overflow, range is too small
#define ITF_ERROR       (7)     // error in serial port interface

#define MAX_OHMMETER_CHANNELS	4

class vrpn_Ohmmeter {
  public:
    vrpn_Ohmmeter(char *name, vrpn_Connection *c = NULL);
    virtual void mainloop(void) = 0;	// Handle getting any reports
    virtual ~vrpn_Ohmmeter() {};

  protected:
    vrpn_Connection *connection;	
    long my_id;			// ID of this ohmmeter to connection
    long channelset_m_id;	// ID of message indicating current setting
    long measure_m_id;		// ID of measurement message
    long setchannel_m_id;	// ID of message to change a setting

    // Description of current state of measurement parameters
    long num_channels;
    double range_min[MAX_OHMMETER_CHANNELS]; // [Ohms]
    double voltage[MAX_OHMMETER_CHANNELS];   // [mV]
    double filter[MAX_OHMMETER_CHANNELS];    // [sec]

    // Description of the next report to go out (for a specific channel)
    long channel;	// Current channel
    long enabled[MAX_OHMMETER_CHANNELS];
    struct timeval timestamp;
    // for each channel we have:
    long status[MAX_OHMMETER_CHANNELS];      // most recent status
    double resistance[MAX_OHMMETER_CHANNELS];// most recent resistance [Ohms]
    double error[MAX_OHMMETER_CHANNELS];     // most recent error estimate [%]

    virtual int encode_channelset_to(char *buf);
    virtual int encode_measure_to(char *buf);
    virtual int encode_setchannel_to(char *buf);
};

#ifdef _WIN32

// routine to handle a message telling the ohmmeter to set the parameters
// for a particular channel (for server-side use)
typedef struct {
        struct timeval msg_time;
        long channel_num;
        long enable;
        double voltage;
        double range_min;
        double filter;
} vrpn_SETOHMCB;
typedef void (*vrpn_SETOHMCHANGEHANDLER)(void *userdata,
                                        const vrpn_SETOHMCB &info);

class vrpn_Ohmmeter_ORPX2 : public vrpn_Ohmmeter {
  public:
    vrpn_Ohmmeter_ORPX2(char *name, vrpn_Connection *c, float hz=1.0);
    virtual void mainloop(void);
    virtual int register_change_handler(void *userdata, 
				vrpn_SETOHMCHANGEHANDLER handler);
    virtual int unregister_change_handler(void *userdata,
				vrpn_SETOHMCHANGEHANDLER handler);
    static void handle_channel_set(void *userdata, const vrpn_SETOHMCB &info);
  protected:
    ORPX_SerialComm *theORPX;
    orpx_params_t chan_data[NUM_ORPX_CHANNELS];
	float channel_acquisition_time[NUM_ORPX_CHANNELS]; // seconds
	struct timeval last_channel_switch_time;
    float update_rate;
    virtual void get_measurement_report(void);

    typedef struct vrpn_ORPXCS {
		void *userdata;
		vrpn_SETOHMCHANGEHANDLER handler;
		struct vrpn_ORPXCS *next;
    } vrpn_ORPXCHANGELIST;
	vrpn_ORPXCHANGELIST *change_list;

    static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);

};

#endif // _WIN32

//----------------------------------------------------------
// ************** Users deal with the following *************

// User routine to handle an ohmmeter message indicating that an ohmmeter
// channel's parameters have been set

typedef struct {
	struct timeval msg_time;
	long channel_num;
	long enabled; // channel enabled?
	double voltage;
	double range_min;
	double filter;
} vrpn_OHMSETCB;
typedef void (*vrpn_OHMSETCHANGEHANDLER)(void *userdata, 
						const vrpn_OHMSETCB &info);

// User routine to handle a message giving the a measurement from a certain
// ohmmeter channel
typedef struct {
	struct timeval msg_time;
	long channel_num;
	long status;
	double resistance;
	double error;
} vrpn_OHMMEASUREMENTCB;
typedef void (*vrpn_OHMMEASUREMENTHANDLER)(void *userdata, 
					const vrpn_OHMMEASUREMENTCB &info);

class vrpn_Ohmmeter_Remote : public vrpn_Ohmmeter {
  public:
    vrpn_Ohmmeter_Remote(char *name);
    int set_channel_parameters(int chan, int enable,
		double voltage, double range_min, double filter);
	int register_measurement_handler(void *userdata,
		vrpn_OHMMEASUREMENTHANDLER handler);
	int unregister_measurement_handler(void *userdata,
		vrpn_OHMMEASUREMENTHANDLER handler);
	int register_ohmset_handler(void *userdata,
        vrpn_OHMSETCHANGEHANDLER handler);
	int unregister_ohmset_handler(void *userdata,
        vrpn_OHMSETCHANGEHANDLER handler);
	virtual void mainloop(void);
                                                
  protected:
    typedef struct vrpn_ORSCS {
        void *userdata;
        vrpn_OHMSETCHANGEHANDLER handler;
        struct vrpn_ORSCS *next;
    } vrpn_OHMSETCHANGELIST;
	vrpn_OHMSETCHANGELIST *ohmset_change_list;
    
    typedef struct vrpn_ORMCS {
        void *userdata;
        vrpn_OHMMEASUREMENTHANDLER handler;
        struct vrpn_ORMCS *next;
    } vrpn_OHMMEASCHANGELIST;
	vrpn_OHMMEASCHANGELIST *measure_change_list;

	static int handle_measurement_message(void *userdata,
						vrpn_HANDLERPARAM p);
	static int handle_ohmset_message(void *userdata,vrpn_HANDLERPARAM p);

};

#endif

