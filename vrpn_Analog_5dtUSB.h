#ifndef VRPN_ANALOG_5DTUSB_H
#define VRPN_ANALOG_5DTUSB_H

#include "vrpn_HumanInterface.h"
#include "vrpn_Analog.h"
#include <string>

// Device drivers for the 5th Dimension Technologies (5dt) data gloves
// connecting to them as HID devices (USB). The base class does all the work:
// the inherited classes just create the right filters and input for the base class;

// Exposes Analog channels for the raw values of each sensor.

#if defined(VRPN_USE_HID)
class VRPN_API vrpn_Analog_5dtUSB : public vrpn_Analog, protected vrpn_HidInterface {
	public:
		virtual ~vrpn_Analog_5dtUSB();

		virtual void mainloop();

		/// Returns a string description of the device we've connected to. Used internally,
		/// but also possibly useful externally.
		std::string get_description() const;

		/// Accessor to know if this is a left hand glove.
		bool isLeftHand() const { return _isLeftHand; }

		/// Accessor to know if this is a right hand glove.
		bool isRightHand() const { return !_isLeftHand; }

	protected:
		/// Protected constructor: use a subclass to specify the glove variant to use.
		vrpn_Analog_5dtUSB(vrpn_HidAcceptor *filter, int num_sensors,
		                   bool isLeftHand, const char *name, vrpn_Connection *c = 0);
		// Set up message handlers, etc.
		void on_data_received(size_t bytes, vrpn_uint8 *buffer);

		struct timeval _timestamp;
		vrpn_HidAcceptor *_filter;

		double _rawVals[16];
		bool _isLeftHand;

		/// Send report iff changed
		void report_changes(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
		/// Send report whether or not changed
		void report(vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
};

class VRPN_API vrpn_Analog_5dtUSB_Glove5Left: public vrpn_Analog_5dtUSB {
	public:
		vrpn_Analog_5dtUSB_Glove5Left(const char *name, vrpn_Connection *c = 0);
		virtual ~vrpn_Analog_5dtUSB_Glove5Left() {};

	protected:
};

class VRPN_API vrpn_Analog_5dtUSB_Glove5Right: public vrpn_Analog_5dtUSB {
	public:
		vrpn_Analog_5dtUSB_Glove5Right(const char *name, vrpn_Connection *c = 0);
		virtual ~vrpn_Analog_5dtUSB_Glove5Right() {};

	protected:
};

class VRPN_API vrpn_Analog_5dtUSB_Glove14Left: public vrpn_Analog_5dtUSB {
	public:
		vrpn_Analog_5dtUSB_Glove14Left(const char *name, vrpn_Connection *c = 0);
		virtual ~vrpn_Analog_5dtUSB_Glove14Left() {};

	protected:
};

class VRPN_API vrpn_Analog_5dtUSB_Glove14Right: public vrpn_Analog_5dtUSB {
	public:
		vrpn_Analog_5dtUSB_Glove14Right(const char *name, vrpn_Connection *c = 0);
		virtual ~vrpn_Analog_5dtUSB_Glove14Right() {};

	protected:
};

class VRPN_API vrpn_HidProductMaskAcceptor: public vrpn_HidAcceptor {
	public:
		vrpn_HidProductMaskAcceptor(vrpn_uint16 vendorId, vrpn_uint16 productMask = 0x0000, vrpn_uint16 desiredProduct = 0x0000) :
			vendor(vendorId),
			product(desiredProduct),
			mask(productMask) {}
			
		~vrpn_HidProductMaskAcceptor() {}

		bool accept(const vrpn_HIDDEVINFO &device) {
			return (device.vendor == vendor) && ((device.product & mask) == (product & mask));
		}
	private:
		vrpn_uint16 vendor;
		vrpn_uint16 product;
		vrpn_uint16 mask;
};

#endif // end of ifdef VRPN_USE_HID

// end of VRPN_ANALOG_5DTUSB_H
#endif

