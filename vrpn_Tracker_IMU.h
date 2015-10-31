/// @file vrpn_Tracker_IMU.h
/// @author Russ Taylor <russ@reliasolve.com>
/// @license Standard VRPN license.

/// This file contains classes useful in producing tracker reports
/// from inertial-navitation units (IMUs).  Initially, it implements
/// the classes needed to take inputs from a magnetometer (compass),
/// accelerometer (gravity++) and a rate gyroscope output from
/// vrpn_Analog devices and merge them into an estimate of orientation.

#pragma once

#include <quat.h>                       // for q_matrix_type
#include <stdio.h>                      // for NULL
#include <string>
#include "vrpn_Analog.h"                // for vrpn_ANALOGCB, etc
#include "vrpn_Tracker.h"               // for vrpn_Tracker

/// @brief Describes information describing an IMU axis.
/// It describes the analog channel to use to drive the axis
/// as well as the scale (and polarity) of the mapping to that axis.

class vrpn_IMU_Axis_Params {
public:
  VRPN_API vrpn_IMU_Axis_Params(void) {
    for (size_t i = 0; i < 3; i++) {
      channels[i] = 0;
      offsets[i] = 0;
      scales[i] = 1;
    }
  }

  std::string name;	  //< Name of the Analog device driving these axes

  int channels[3];	  //< Which channel to use from the Analog device for each axis
  double offsets[3];  //< Offset to apply to the measurement (applied before scale)
  double scales[3];   //< Scale, including positive or negative axis
};

class vrpn_IMU_Vector {
public:
  VRPN_API vrpn_IMU_Vector (void) { ana = NULL;
      values[0] = values[1] = values[2] = 0.0;
      time.tv_sec = 0; time.tv_usec = 0; };

  vrpn_IMU_Axis_Params  params;     //< Parameters used to construct values
  vrpn_Analog_Remote    *ana;       //< Analog Remote device to listen to
  double                values[3];  //< Vector fo values
  struct timeval        time;       //< Time of the report used to generate value
};

/// @brief Normalizes the three directions for a magnetometer into
/// a unit vector.
///
/// This class acts as an analog-to-analog filter. It is given
/// three axes to manage.  For each axis, it keeps track of the
/// minimum and maximum value it has ever received and maps the
/// current value into the range -1..1 based on that range.
///   It then normalizes the resulting vector, producing a unit
/// vector that should auto-calibrate itself over time to provide
/// a unit direction vector (as a 3-output analog) pointing in
/// the direction of magnetic north.

class vrpn_IMU_Magnetometer : public vrpn_Analog_Server {
public:
  /// @brief Constructor
  /// @param name The name to give to the Analog_Server output device.
  /// @param output_con The connection to report on.
  /// @param x The parameters for the X axis.
  /// @param y The parameters for the Y axis.
  /// @param z The parameters for the Z axis.
  /// @param update_rate How often to send reports.  If report_changes
  ///        is true, it will only report at most this often.  If
  ///        report_changes is false, it will send reports at this
  ///        rate whether it has received new values or not.
  /// @param report_changes If true, only reports values when at least
  ///        one of them a changed.  If false, report values at the
  ///        specified rate whether or not they have changed.
  VRPN_API vrpn_IMU_Magnetometer(std::string name, vrpn_Connection *output_con,
        vrpn_IMU_Axis_Params params, float update_rate,
        bool report_changes = VRPN_FALSE);

  virtual VRPN_API ~vrpn_IMU_Magnetometer();

  /// Override base class function.
  void VRPN_API mainloop();

protected:
  double	    d_update_interval;	//< How long to wait between sends
  struct timeval  d_prevtime;		  //< Time of the previous report
  bool        d_report_changes;   //< Call report_changes() or report()?

  /// Axes to handle gathering and scaling the required data.
  vrpn_IMU_Vector	d_vector;

  /// Minimum, maximum, and current values for each axis.
  double d_mins[3], d_maxes[3];

  int setup_vector(vrpn_IMU_Vector *vector);
  int teardown_vector(vrpn_IMU_Vector *vector);

  static void	VRPN_CALLBACK handle_analog_update (void *userdata,
                                    const vrpn_ANALOGCB info);
};

# if 0

class vrpn_Tracker_IMU_Params {
public:
  VRPN_API vrpn_Tracker_IMU_Params(void) {
  }

  std::string 

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

class vrpn_TAF_fullaxis {
public:
  VRPN_API vrpn_TAF_fullaxis (void) { ana = NULL; value = 0.0; af = NULL; };

  vrpn_TAF_axis axis;
  vrpn_Analog_Remote * ana;
  double value;
};

/// This class will turn an analog device such as a joystick or a camera
// tracker into a tracker by interpreting the joystick
// positions as either position or velocity inputs and "flying" the user
// around based on analog values.
// The "absolute" parameter tells whether the tracker integrates differential
// changes (the default, with FALSE) or takes the analog values as absolute
// positions or orientations.
// The mapping from analog channels to directions (or orientation changes) is
// described in the vrpn_Tracker_AnalogFlyParam parameter. For translations,
// values above threshold are multiplied by the scale and then taken to the
// power; the result is the number of meters (or meters per second) to move
// in that direction. For rotations, the result is taken as the number of
// revolutions (or revolutions per second) around the given axis.
// Note that the reset button has no effect on an absolute tracker.
// The time reported by absolute trackers is as of the last report they have
// had from their analog devices.  The time reported by differential trackers
// is the local time that the report was generated.  This is to allow a
// gen-locked camera tracker to have its time values passed forward through
// the AnalogFly class.

// If reportChanges is TRUE, updates are ONLY sent if there has been a
// change since the last update, in which case they are generated no faster
// than update_rate. 

// If worldFrame is TRUE, then translations and rotations take place in the
// world frame, rather than the local frame. Useful for a simulated wand
// when doing desktop testing of immersive apps - easier to keep under control.

class VRPN_API vrpn_Tracker_AnalogFly : public vrpn_Tracker {
  public:
    vrpn_Tracker_AnalogFly (const char * name, vrpn_Connection * trackercon,
			    vrpn_Tracker_AnalogFlyParam * params,
                            float update_rate, bool absolute = vrpn_FALSE,
                            bool reportChanges = VRPN_FALSE, bool worldFrame = VRPN_FALSE);

    virtual ~vrpn_Tracker_AnalogFly (void);

    virtual void mainloop ();
    virtual void reset (void);

    void update (q_matrix_type &);

    static void VRPN_CALLBACK handle_joystick (void *, const vrpn_ANALOGCB);
    static int VRPN_CALLBACK handle_newConnection (void *, vrpn_HANDLERPARAM);

  protected:

    double	    d_update_interval;	//< How long to wait between sends
    struct timeval  d_prevtime;		//< Time of the previous report
    bool	    d_absolute;		//< Report absolute (vs. differential)?
    bool       d_reportChanges;
    bool       d_worldFrame;

    vrpn_TAF_fullaxis	d_x, d_y, d_z, d_sx, d_sy, d_sz;
    vrpn_Button_Remote	* d_reset_button;
    int			d_which_button;

    vrpn_Button_Remote	* d_clutch_button;
    int			d_clutch_which;
    bool                d_clutch_engaged;
    bool		d_clutch_was_off;

    q_matrix_type d_initMatrix, d_currentMatrix, d_clutchMatrix;

    void    update_matrix_based_on_values (double time_interval);
    void    convert_matrix_to_tracker (void);

    bool shouldReport (double elapsedInterval) const;

    int setup_channel (vrpn_TAF_fullaxis * full);
    int teardown_channel (vrpn_TAF_fullaxis * full);

    static void	VRPN_CALLBACK handle_analog_update (void * userdata,
                                      const vrpn_ANALOGCB info);
    static void VRPN_CALLBACK handle_reset_press (void * userdata, const vrpn_BUTTONCB info);
    static void VRPN_CALLBACK handle_clutch_press (void * userdata, const vrpn_BUTTONCB info);
};

#endif
