/* Example of providing an interface to VRPN client-side objects through C.
 * Initially made to handle the Tracker and Button objects, for the purpose
 * of connecting to a Spaceball through Fortran.  If you know how to link
 * C and C++ code into Fortran, then you should be able to use it to link.
 * (Note, you may need to add an underscore to the name of the C functions.)
 * Also note, the implementation code is C++ code, but exposes its functions using
 * a C interface.
 */

#ifdef __cplusplus
extern "C" {
#endif


typedef char vrpn_c_bool;

/* Function prototype for the function that will be called whenever a tracker
 * report comes in. */
typedef void (*vrpn_c_tracker_callback_function)(unsigned sensor, const double pos[3], const double quat[4]);

/* Open a tracker device, returning an opaque pointer to it.  Returns NULL on
 * failure, an opaque pointer to the tracker device on success.  The tracker
 * pointer needs to be passed to the poll and close functions. */
void *vrpn_c_open_tracker(const char *device_name, vrpn_c_tracker_callback_function callback);

/* Poll the tracker whose device pointer is passed in.  This will cause the
 * callback handler to be called whenever a new value comes in for
 * a sensor.  Returns false if the device is not working. */
vrpn_c_bool vrpn_c_poll_tracker(void *device);

/* Close the tracker whose device pointer is passed in.  Returns true on success,
 * false on failure. */
vrpn_c_bool vrpn_c_close_tracker(void *device);


/* Function prototype for the function that will be called whenever a button
 * report comes in. */
typedef void (*vrpn_c_button_callback_function)(const unsigned button, const vrpn_c_bool value);

/* Open a button device, returning an opaque pointer to it.  Returns NULL on
 * failure, an opaque pointer to the button device on success.  The button
 * device pointer needs to be passed to the read and close functions. */
void *vrpn_c_open_button(const char *device_name, vrpn_c_button_callback_function callback);

/* Poll the button whose device pointer is passed in.  This will cause the
 * callback handler to be called whenever a new value comes in for
 * a button.  Returns false if the device is not working. */
vrpn_c_bool vrpn_c_poll_button(void *device);

/* Close the button whose device pointer is passed in.  Returns true on success,
 * false on failure. */
vrpn_c_bool vrpn_c_close_button(void *device);

#ifdef __cplusplus
} /* end of extern "C" */
#endif
