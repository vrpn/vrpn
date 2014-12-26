#ifndef VRPN_YEI_3SPACE_H
#define VRPN_YEI_3SPACE_H

#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

/** @brief Base class with routines for YEI 3Space units.
*/
class VRPN_API vrpn_YEI_3Space
  : public vrpn_Tracker_Server
  , public vrpn_Analog
  , public vrpn_Button_Filter
{
public:
  /** @brief Constructor.
	@param name Name for the device
	@param c Connection to use.
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
  vrpn_YEI_3Space (const char * name,
    vrpn_Connection * c,
    bool calibrate_gyros_on_setup = false,
    bool tare_on_setup = false,
    double frames_per_second = 50,
    double red_LED_color = 0,
    double green_LED_color = 0,
    double blue_LED_color = 0,
    int LED_mode = 1,
    const char *reset_commands[] = NULL);

  /// Destructor.
  virtual ~vrpn_YEI_3Space();

  /// Called once through each main loop iteration to handle updates.
  virtual void mainloop ();
	
protected:
  /// Initialization that would normally happen in the constructor, but
  /// we need to wait for the derived classes to open a communications port
  /// before we can do this.  Derived classes should call this at the end
  /// of their constructors.
  void init(bool calibrate_gyros_on_setup
            , bool tare_on_setup
            , double frames_per_second
            , double red_LED_color
            , double green_LED_color
            , double blue_LED_color
            , int LED_mode
            , const char *reset_commands[]);

  /// Flush any incoming characters in the communications channel.
  virtual void flush_input(void) = 0;

  // A list of ASCII commands that are sent to the device after
  // the other reset commands have been sent, each time the device
  // is reset.  This enables arbitrary configuration.
  char **d_reset_commands;        //< Commands to send on reset
  int d_reset_command_count;      //< How many reset commands

  // Status and handlers for different states
  int     d_status;               //< What are we currently up to?
  virtual int reset(void);        //< Set device back to starting config
  virtual void get_report(void) = 0;  //< Try to read and handle a report from the device
  virtual void handle_report(unsigned char *report);  //< Parse and handle a complete streaming report

  double  d_frames_per_second;    //< How many frames/second do we want?
  int     d_LED_mode;             //< LED mode we read from the device.
  vrpn_float32  d_LED_color[3];   //< LED color we read from the device.

  /// Compute the CRC for the message, append it, and send message.
  /// Returns true on success, false on failure.
 virtual bool send_binary_command(const unsigned char *cmd, int len) = 0;

  /// Put a ':' character at the front and '\n' at the end and then
  /// send the resulting command as an ASCII command.
  /// Returns true on success, false on failure.
  virtual bool send_ascii_command(const char *cmd) = 0;

  /// Read and parse the response to an LED-state request command.
  /// NULL timeout pointer means wait forever.
  /// Returns true on success and false on failure.
  virtual bool receive_LED_mode_response(struct timeval *timeout = NULL) = 0;

  /// Read and parse the response to an LED-values request command.
  /// NULL timeout pointer means wait forever.
  /// Returns true on success and false on failure.
  virtual bool receive_LED_values_response(struct timeval *timeout = NULL) = 0;

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

/** @brief Class to support reading data from a wired YEI 3Space unit.
*/
class VRPN_API vrpn_YEI_3Space_Sensor
  : public vrpn_YEI_3Space
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

protected:
  int d_serial_fd;    //< Serial port to read from.

  /// Flush any incoming characters in the communications channel.
  virtual void flush_input(void);

  virtual void get_report(void);  //< Try to read and handle a report from the device

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
};

// Consider two constructors, one that handles opening the serial port
// and setting the state on the dongle and another that takes the
// serial_fd and uses it to communicate.  All of them will share the
// communications port, and each will configure their own device through
// it.

// Dongle slot command to configure a dongle to a particular slot
//    Pan ID 16-bit number (default 1), Channel 11-26 (default 26), 
//    Slot/logical ID 0-14, Address (ser #)
//    0xf1, 0xf2: Turn off mouse and joystick HID streaming

// Streaming mode: 0xb0 sets it, we probably want manual-release (0).
//    0xb4 is manual flush for a single channel, which is probably what we want
//    0xd1 sets the serial number at a given slot on the wireless dongle.
//    0xdb configures the wireless response header bitfield (we want it empty...)

// Binary Packet:
//   Starts with 0xF8 and then has the Logical ID address, then a
// checksum "over all bytes except the first".

// Binary Response:
//   Starts with 0 for success or nonzero for failure).  Then the
// logical ID.  Then a data-length byte.  Then the response itself
// (which may be zero bytes) -- all messages get a response.
//   In case of failure, the logical ID will end the packet.
//   For commands without a return, we get 00 <ID> 00.

// Ascii packet:
//  >ADDRESS,Command[,Data]'\n'

// Ascii response:
//  Starts with 0 or 1 (0 on success)
// then ',' and logical ID
// then ',' and data length (unless there is failure)
// then [,Data]
// then '\n'  (Example table has \r before \n, but protocol description doesn't)

#endif
