/// @file vrpn_Tracker_IMU.h
/// @copyright 2015 ReliaSolve.com
/// @author ReliaSolve.com russ@reliasolve.com
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
  VRPN_API vrpn_IMU_Magnetometer(std::string const &name, vrpn_Connection *output_con,
        vrpn_IMU_Axis_Params params, float update_rate,
        bool report_changes = VRPN_FALSE);

  virtual VRPN_API ~vrpn_IMU_Magnetometer();

  /// Override base class function.
  virtual VRPN_API void mainloop();

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

class vrpn_Tracker_IMU_Params {
public:
  VRPN_API vrpn_Tracker_IMU_Params(void) {}

  vrpn_IMU_Axis_Params d_acceleration;     //< Acceleration input to use
  vrpn_IMU_Axis_Params d_rotational_vel;  //< Rotational velocity input to use
  std::string d_magnetometer_name;        //< Magnetometer to use (Empty if none)
};

/// This class will turn set of two or three analog devices into a tracker by
// interpreting them as inertial-measurement report vectors.  The two required
// inputs are an acceleration vector and a rotational velocity measurement.  There
// is an optional magnetometer.
//  The accelerometer scale parameter should be set to produce values that
// are in meters/second/second.  The rotational input scale parameters should
// be set to produce values that are in radians/second.  The magnetometer
// scale should be set to produce a unit normal vector.
//  NOTE: The coordinate system of the HMD has X to the right as the device
// is worn, Y facing up, and Z pointing out the back of the wearer's head.
//  The time reported by is as of the last report received from any device.
//  If reportChanges is TRUE, updates are ONLY sent if there has been a
// change since the last update, in which case they are generated no faster
// than update_rate. 

class vrpn_IMU_SimpleCombiner : public vrpn_Tracker {
  public:
    VRPN_API vrpn_IMU_SimpleCombiner(const char *name, vrpn_Connection *trackercon,
      vrpn_Tracker_IMU_Params *params,
      float update_rate, bool report_changes = VRPN_FALSE);

    virtual VRPN_API ~vrpn_IMU_SimpleCombiner(void);

    virtual VRPN_API void mainloop ();

  protected:
    double	    d_update_interval;	//< How long to wait between sends
    struct timeval  d_prevtime;	  	//< Time of the previous report
    bool       d_report_changes;    //< Report only changes, or always?

    vrpn_IMU_Vector d_acceleration;     //< Analog input for accelerometer
    vrpn_IMU_Vector d_rotational_vel;   //< Analog input for rotational velocity
    vrpn_IMU_Vector d_magnetometer;     //< Analog input for magnetometer, if present

    double  d_gravity_restore_rate;     //< Radians/second to restore gravity vector
    double  d_north_restore_rate;       //< Radians/second to restore North vector

    struct timeval d_prev_update_time;  //< Time of previous integration update
    void    update_matrix_based_on_values(double time_interval);

    int setup_vector(vrpn_IMU_Vector *vector, vrpn_ANALOGCHANGEHANDLER f);
    int teardown_vector(vrpn_IMU_Vector *vector, vrpn_ANALOGCHANGEHANDLER f);

    static void	VRPN_CALLBACK handle_analog_update(void *userdata,
      const vrpn_ANALOGCB info);
};

