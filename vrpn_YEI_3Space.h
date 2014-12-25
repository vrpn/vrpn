#ifndef VRPN_YEI_3SPACE_H
#define VRPN_YEI_3SPACE_H

#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

/** @brief Class to support reading data from serial YEI 3Space units.
*/
class VRPN_API vrpn_YEI_3Space_Sensor
  : public vrpn_Tracker_Server
  , public vrpn_Serial_Analog
  , public vrpn_Button_Filter
{
public:
  /** @brief Constructor.
	@param name Name for the device
	@param c Connection to use.
	@param port serial port to connect to
	@param baud Baud rate - 115200 is default.
	@param calibrate_gyros_on_setup - true to cause this to happen
	@param tare_on_setup - true to cause this to happen (usually manufacturing-time operation!)
	@param frames_per_second - How many frames/second to read
	@param red_LED_color - brightness of LED (0-1)
	@param green_LED_color - brightness of LED (0-1)
	@param blue_LED_color - brightness of LED (0-1)
	@param LED_state - 0 = standard, 1 = static
	@param reset_commands - Array of pointers to commands, NULL for end.
                 These will be sent after other commands whenever the tracker
                 is reset.  This will be copied; the caller is responsible for
                 freeing any storage after calling the constructor.
  */
  vrpn_YEI_3Space_Sensor (const char * name,
    vrpn_Connection * c,
    const char * port,
    int baud = 115200,
    bool calibrate_gyros_on_setup = false,
    bool tare_on_setup = false,
    double frames_per_second = 50,
    double red_LED_color = 0,
    double green_LED_color = 0,
    double blue_LED_color = 0,
    int LED_mode = 1,
    const char *reset_commands[] = NULL);

  /// Destructor.
  virtual ~vrpn_YEI_3Space_Sensor();

  /// Called once through each main loop iteration to handle updates.
  virtual void mainloop ();
	
protected:
  // A list of ASCII commands that are sent to the device after
  // the other reset commands have been sent, each time the device
  // is reset.  This enables arbitrary configuration.
  char **d_reset_commands;        //< Commands to send on reset
  int d_reset_command_count;      //< How many reset commands

  // Status and handlers for different states
  int     d_status;               //< What are we currently up to?
  virtual int reset(void);        //< Set device back to starting config
  virtual void get_report(void);  //< Try to read a report from the device

  double  d_frames_per_second;    //< How many frames/second do we want?
  int     d_LED_mode;             //< LED mode we read from the device.
  vrpn_float32  d_LED_color[3];   //< LED color we read from the device.

  /// Compute the CRC for the message, append it, and send message.
  /// Returns true on success, false on failure.
  bool send_binary_command(const unsigned char *cmd, int len);

  /// Put a ':' character at the front and '\n' at the end and then
  /// send the resulting command as an ASCII command.
  /// Returns true on success, false on failure.
  bool send_ascii_command(const char *cmd);

  /// Read and parse the response to an LED-state request command.
  /// NULL timeout pointer means wait forever.
  /// Returns true on success and false on failure.
  bool receive_LED_mode_response(struct timeval *timeout = NULL);

  /// Read and parse the response to an LED-values request command.
  /// NULL timeout pointer means wait forever.
  /// Returns true on success and false on failure.
  bool receive_LED_values_response(struct timeval *timeout = NULL);

  unsigned char d_buffer[128];        //< Buffer to read reports into.
  unsigned d_expected_characters;     //< How many characters we are expecting
  unsigned d_characters_read;         //< How many characters we've read so far

  struct timeval timestamp;   //< Time of the last report from the device

  /// send report iff changed
  virtual void report_changes
                 (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
  /// send report whether or not changed
  virtual void report
                 (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
};

#endif
