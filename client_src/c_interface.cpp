/* Example of providing an interface to VRPN client-side objects through C.
 * Initially made to handle the Tracker and Button objects, for the purpose
 * of connecting to a Spaceball through Fortran.  If you know how to link
 * C and C++ code into Fortran, then you should be able to use it to link.
 * (Note, you may need to add an underscore to the name of the C functions.)
 * Also note, the implementation code is C++ code, but exposes its functions using
 * a C interface.
 */

#include <stddef.h>                     // for NULL
#include <vrpn_Button.h>                // for vrpn_Button_Remote, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "c_interface.h"
#include "vrpn_Configure.h"             // for VRPN_CALLBACK

/* Helper function that receives the callback handler from the C++ Tracker
 * remote, repackages the values, and calls the user callback function. */
void	VRPN_CALLBACK handle_tracker_pos_quat (void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_c_tracker_callback_function f = (vrpn_c_tracker_callback_function)(userdata);
	f(t.sensor, t.pos, t.quat);
};

/* Open a tracker device, returning an opaque pointer to it.  Returns NULL on
 * failure, an opaque pointer to the tracker device on success.  The tracker
 * pointer needs to be passed to the poll and close functions. */
extern "C" void *vrpn_c_open_tracker(const char *device_name, vrpn_c_tracker_callback_function callback)
{
  vrpn_Tracker_Remote *tkr = NULL;
  try {
    tkr = new vrpn_Tracker_Remote(device_name);
    /* Tell the callback handler what function to call. */
    tkr->register_change_handler((void*)callback, handle_tracker_pos_quat);
  }
  catch (...) { }
  return tkr;
};

/* Poll the tracker whose device pointer is passed in.  This will cause the
 * callback handler to be called whenever a new value comes in for
 * a sensor.  Returns false if the device is not working. */
extern "C" vrpn_c_bool vrpn_c_poll_tracker(void *device)
{
	if (device == NULL) { return false; }
	vrpn_Tracker_Remote *tkr = (vrpn_Tracker_Remote *)device;

	tkr->mainloop();
	return true;
};


/* Close the tracker whose device pointer is passed in.  Returns true on success,
 * false on failure. */
extern "C" vrpn_c_bool vrpn_c_close_tracker(void *device)
{
	if (device == NULL) { return false; }
	vrpn_Tracker_Remote *tkr = (vrpn_Tracker_Remote *)device;

        try {
          delete tkr;
        } catch (...) {
          return false;
        }
	return true;
};


/* Helper function that receives the callback handler from the C++ Button
 * remote, repackages the values, and calls the user callback function. */
void	VRPN_CALLBACK handle_button_event (void *userdata, const vrpn_BUTTONCB b)
{
	vrpn_c_button_callback_function f = (vrpn_c_button_callback_function)(userdata);
	f(b.button, (b.state != 0));
};

/* Open a button device, returning an opaque pointer to it.  Returns NULL on
 * failure, an opaque pointer to the button device on success.  The button
 * device pointer needs to be passed to the read and close functions. */
extern "C" void *vrpn_c_open_button(const char *device_name, vrpn_c_button_callback_function callback)
{
  vrpn_Button_Remote *btn = NULL;
  try {
    vrpn_Button_Remote *btn = new vrpn_Button_Remote(device_name);
    /* Tell the callback handler what function to call. */
    btn->register_change_handler((void*)callback, handle_button_event);
  } catch (...) {}
  return btn;
};

/* Poll the button whose device pointer is passed in.  This will cause the
 * callback handler to be called whenever a new value comes in for
 * a button.  Returns false if the device is not working. */
extern "C" vrpn_c_bool vrpn_c_poll_button(void *device)
{
	if (device == NULL) { return false; }
	vrpn_Button_Remote *btn = (vrpn_Button_Remote *)device;

	btn->mainloop();
	return true;
};

/* Close the button whose device pointer is passed in.  Returns true on success,
 * false on failure. */
extern "C" vrpn_c_bool vrpn_c_close_button(void *device)
{
	if (device == NULL) { return false; }
	vrpn_Button_Remote *btn = (vrpn_Button_Remote *)device;

        try {
          delete btn;
        }
        catch (...) {
          return false;
        }
	return true;
};

