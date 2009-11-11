#ifndef __TRACKER_3DMOUSE_H
#define __TRACKER_3DMOUSE_H

#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

// This parameter is passed to the constructor for the AnalogFly; it describes
// the channel mapping and parameters of that mapping, as well as the button
// that will be used to reset the tracker when it is pushed. Any entry which
// has a NULL pointer for the name is disabled.

class VRPN_API vrpn_TWH_wiimote {

  public:

    vrpn_TWH_wiimote (void)
      { name = NULL; };
};

class VRPN_API vrpn_Tracker_WiimoteHeadParam {

  public:

    vrpn_Tracker_WiimoteHeadParam (void) {
        x.name = y.name = z.name =
        sx.name = sy.name = sz.name = reset_name = clutch_name = NULL;
    }

    /// Translation along each of these three axes
    vrpn_TAF_axis x, y, z;

    /// Rotation in the positive direction about the three axes
    vrpn_TAF_axis sx, sy, sz;

    /// Button device that is used to reset the matrix to the origin

    char * reset_name;
    int reset_which;

    /// Clutch device that is used to enable relative motion over
    // large distances

    char * clutch_name;
    int clutch_which;
};

class VRPN_API vrpn_Tracker_WiimoteHead;	// Forward reference

class VRPN_API vrpn_TWH_fullaxis {
public:
        vrpn_TAF_fullaxis (void) { ana = NULL; value = 0.0; wh = NULL; };

	vrpn_TAF_axis axis;
        vrpn_Analog_Remote * ana;
	vrpn_Tracker_WiimoteHead * wh;
        double value;
};

class VRPN_API vrpn_Tracker_WiimoteHead : public vrpn_Tracker {
  public:
    vrpn_Tracker_WiimoteHead (const char * name, vrpn_Connection * trackercon,
			    vrpn_Tracker_WiimoteHeadParam * params,
                            float update_rate, vrpn_bool absolute = vrpn_FALSE,
                            vrpn_bool reportChanges = VRPN_FALSE);

    virtual ~vrpn_Tracker_WiimoteHead (void);

    virtual void mainloop ();
    virtual void reset (void);

    void update (q_matrix_type &);

    static void VRPN_CALLBACK handle_joystick (void *, const vrpn_ANALOGCB);
    static int VRPN_CALLBACK handle_newConnection (void *, vrpn_HANDLERPARAM);

  protected:

    double	    d_update_interval;	//< How long to wait between sends
    struct timeval  d_prevtime;		//< Time of the previous report
    vrpn_bool	    d_absolute;		//< Report absolute (vs. differential)?
    vrpn_bool       d_reportChanges;

    vrpn_TAF_fullaxis	d_x, d_y, d_z, d_sx, d_sy, d_sz;
    vrpn_Button_Remote	* d_reset_button;
    int			d_which_button;

    vrpn_Button_Remote	* d_clutch_button;
    int			d_clutch_which;
    bool                d_clutch_engaged;

    q_matrix_type d_initMatrix, d_currentMatrix, d_clutchMatrix;

    void    update_matrix_based_on_values (double time_interval);
    void    convert_matrix_to_tracker (void);

    vrpn_bool shouldReport (double elapsedInterval) const;

    int setup_channel (vrpn_TAF_fullaxis * full);
    int teardown_channel (vrpn_TAF_fullaxis * full);

    static void	VRPN_CALLBACK handle_analog_update (void * userdata,
                                      const vrpn_ANALOGCB info);
    static void VRPN_CALLBACK handle_reset_press (void * userdata, const vrpn_BUTTONCB info);
    static void VRPN_CALLBACK handle_clutch_press (void * userdata, const vrpn_BUTTONCB info);
};

#endif
