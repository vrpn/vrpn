#ifndef VRPN_YEI_3SPACE_H
#define VRPN_YEI_3SPACE_H

#include "quat.h"
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
	@param reset_commands - Array of pointers to commands, NULL for end.
                 These will be sent after other commands whenever the tracker
                 is reset.  This will be copied; the caller is responsible for
                 freeing any storage after calling the constructor.
  */
  vrpn_YEI_3Space (const char * name,
    vrpn_Connection * c,
    double frames_per_second = 50,
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
            , double red_LED_color
            , double green_LED_color
            , double blue_LED_color
            , int LED_mode);

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
  virtual bool get_report(void) = 0;  //< Try to read and handle a report from the device
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

  virtual bool get_report(void);  //< Try to read and handle a report from the device

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

/** @brief Class to support reading data from a wireless YEI 3Space unit.
*/
class VRPN_API vrpn_YEI_3Space_Sensor_Wireless
  : public vrpn_YEI_3Space
{
public:
  /** @brief Constructor for the first device, which will open the serial
             port and configure the dongle.  The first device on a wireless
             dongle should call this constructor.  The later ones should call
             the constructor that takes the serial_file_descriptor argument
             after querying the first for that descriptor.
	@param name Name for the device
	@param c Connection to use.
	@param logical_id Which logical ID to use on the wireless unit.
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
  vrpn_YEI_3Space_Sensor_Wireless (const char * name,
    vrpn_Connection * c,
    int logical_id,
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

  /** @brief Constructor for a follow-on device, once a wireless unit has already
             been opened on a given dongle.  The serial_file_descriptor argument
             should be found by querying the first device.
	@param name Name for the device
	@param c Connection to use.
	@param logical_id Which logical ID to use on the wireless unit.
	@param serial_file_descriptor: Pre-opened serial descriptor to use.
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
  vrpn_YEI_3Space_Sensor_Wireless (const char * name,
    vrpn_Connection * c,
    int logical_id,
    int serial_file_descriptor,
    bool calibrate_gyros_on_setup = false,
    bool tare_on_setup = false,
    double frames_per_second = 50,
    double red_LED_color = 0,
    double green_LED_color = 0,
    double blue_LED_color = 0,
    int LED_mode = 1,
    const char *reset_commands[] = NULL);

  /// Destructor.
  virtual ~vrpn_YEI_3Space_Sensor_Wireless();

  /// Reports the serial_fd that was opened, so that additional
  /// sensors that use the same wireless dongle can be opened.
  int get_serial_file_descriptor(void) const { return d_serial_fd; }

protected:
  bool d_i_am_first;  //< Records whether I'm the first sensor (so close the port)
  int d_serial_fd;    //< Serial port to read from.
  vrpn_uint8 d_logical_id;   //< Which logical ID are we assigned to on the dongle?

  /// Configure the dongle (called if we are the first one)
  virtual bool configure_dongle(void);

  /// Insert our serial number into the specified slot.
  virtual bool set_logical_id(vrpn_uint8 logical_id, vrpn_int32 serial_number);

  /// Flush any incoming characters in the communications channel.
  virtual void flush_input(void);

  /// Get and handle a report from the device if one is available.  Return true
  /// if one was available, false if not.
  virtual bool get_report(void);

  /// Compute the CRC for the message, append it, and send message
  /// to the dongle directly (not a wireless command).
  /// Returns true on success, false on failure.
  bool send_binary_command_to_dongle(const unsigned char *cmd, int len);

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

#endif
