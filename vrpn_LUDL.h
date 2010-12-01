#ifndef VRPN_LUDL_H
#define VRPN_LUDL_H

#include "vrpn_HumanInterface.h"
#include "vrpn_Analog.h"
#include "vrpn_Analog_Output.h"

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__APPLE__) || defined(VRPN_USE_LIBHID)

// Device drivers for the LUDL family of translation stage controllers.
// XXX Problem: This device is not a HID device, even though it is USB.
// This means that the device is not detected as part of the HID device
// identification, so we can't control it using the HID interface.  We'll
// need to write a lower-level USB driver for it.

// The only device currently implemented is the USBMAC6000 controller in its
// two-axis configuration controlling the stage for the Panoptes system
// at UNC Chapel Hill.  This driver uses the vrpn_HumanInterface driver (HID
// interface) to control the device.  It exposes the vrpn_Analog and the
// vrpn_Analog_Output interfaces, to report and set the stage position.
//
// Analog/Analog_Output channel 0 is X (0 to maximum #ticks, in ticks).
// Analog/Analog_Output channel 1 is Y (0 to maximum #ticks, in ticks).

class vrpn_LUDL_USBMAC6000 :
  public vrpn_Analog, 
  public vrpn_Analog_Output,
  protected vrpn_HidInterface
{
public:
  vrpn_LUDL_USBMAC6000(const char *name, vrpn_Connection *c = 0, bool do_recenter = false);
  virtual ~vrpn_LUDL_USBMAC6000();

  virtual void mainloop();

protected:
  struct timeval _timestamp;
  vrpn_HidAcceptor *_filter;
  unsigned  _endpoint;        // Which endpoint to use to communicate with the device

  // When we get data, we store it into a growing buffer.  Once we've stopped
  // getting data, we call the parse routine on the buffer.  The data-received
  // function adds it onto the end of the buffer, dropping characters if it
  // overflows.  The interpret function reads the characters in the buffer and
  // handles them.  The interpret function reads one ASCII response from the
  // buffer and reports the value that it returned; it does not adjust the
  // buffer pointer.
  static const unsigned _INBUFFER_SIZE = 1024;
  vrpn_uint8 _inbuffer[_INBUFFER_SIZE]; // MUST CHANGE the sizeof() code if this becomes not an array.
  unsigned _incount;
  void on_data_received(size_t bytes, vrpn_uint8 *buffer);
  bool interpret_usbmac_ascii_response(const vrpn_uint8 *buffer, int *value_return);

  // XXX I think we're using ASCII send and response because Ryan had some kind of trouble
  // with the binary send/response.  Not sure about this, though.  It sure would be
  // faster to read and easier to parse the binary ones, which all have the same length.

  // vrpn_Analog overridden methods.
  // Send report iff changed
  void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE);
  // Send report whether or not changed
  void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE);

  // No actual types to register, derived classes will be analogs and analog_outputs
  int register_types(void) { return 0; }

  // Handlers for the Analog_Output messages.
  /// Responds to a request to change one of the values by
  /// setting the channel to that value.
  static int VRPN_CALLBACK handle_request_message(void *userdata, vrpn_HANDLERPARAM p);

  /// Responds to a request to change multiple channels at once.
  static int VRPN_CALLBACK handle_request_channels_message(void *userdata, vrpn_HANDLERPARAM p);

  /// Responds to a connection request with a report of the values
  static int VRPN_CALLBACK handle_connect_message(void *userdata, vrpn_HANDLERPARAM p);

private:
  // Helper functions for communication with the stage
  void  flush_input_from_ludl(void);
  bool  send_usbmac_command(unsigned device, unsigned command, unsigned index, int value);
  bool  recenter(void);
  bool  ludl_axis_moving(unsigned axis);  // Returns true if the axis is still moving
  bool  move_axis_to_position(int axis, int position);

  // Stores whether we think each axis is moving and where we think each axis is
  // going if it is moving.  These are set in the move_axis_to_position() routine
  // and used in the mainloop() routine to decide if it is time to report that we
  // have gotten where we want to be.
  bool    *_axis_moving;
  vrpn_float64  *_axis_destination;
};

// end of OS selection
#endif

// end of VRPN_LUDL_H
#endif

