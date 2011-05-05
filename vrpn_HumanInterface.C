#include "vrpn_HumanInterface.h"

#if defined(VRPN_USE_HID)

// Accessor for USB vendor ID of connected device
vrpn_uint16 vrpn_HidInterface::vendor() const {
	return _vendor;
}

// Accessor for USB product ID of connected device
vrpn_uint16 vrpn_HidInterface::product() const {
	return _product;
}

// Returns true iff everything was working last time we checked
bool vrpn_HidInterface::connected() const {
	return _working;
}


vrpn_HidInterface::vrpn_HidInterface(vrpn_HidAcceptor *acceptor)
	: _acceptor(acceptor)
	, _device(NULL)
	, _working(false)
	, _vendor(0)
	, _product(0)
{
	if (_acceptor == NULL) {
		fprintf(stderr,"vrpn_HidInterface::vrpn_HidInterface(): NULL acceptor\n");
		return;
	}

	// Reset the acceptor and then attempt to connect to a device.
	_acceptor->reset();
	reconnect();
}

vrpn_HidInterface::~vrpn_HidInterface()
{
  if (_device) {
    hid_close(_device);
    _device = NULL;
  }
}

// Reconnects the device I/O for the first acceptable device
// Called automatically by constructor, but userland code can
// use it to reacquire a hotplugged device.
void vrpn_HidInterface::reconnect() {

        // Enumerate all devices and pass each one to the acceptor to see if it is the
        // one that we want.
        struct hid_device_info  *devs = hid_enumerate(0, 0);
        struct hid_device_info  *loop = devs;
        bool found = false;
        const wchar_t *serial;
        while ((loop != NULL) && !found) {
          vrpn_HIDDEVINFO device_info;
          device_info.vendor = loop->vendor_id;
          device_info.product = loop->product_id;
          device_info.serial_number = loop->serial_number;
          device_info.manufacturer_string = loop->manufacturer_string;
          device_info.product_string = loop->product_string;
          //printf("XXX Found vendor %x, product %x\n", (unsigned)(loop->vendor_id), (unsigned)(loop->product_id));

          if (_acceptor->accept(device_info)) {
            _vendor = loop->vendor_id;
            _product = loop->product_id;
            serial = loop->serial_number;
            found = true;
          }
          loop = loop->next;
        }
        if (!found) {
		fprintf(stderr,"vrpn_HidInterface::reconnect(): Device not found\n");
		return;
        }

	// Initialize the HID interface and open the device.
        _device = hid_open(_vendor, _product, const_cast<wchar_t *>(serial));
        if (_device == NULL) {
		fprintf(stderr,"vrpn_HidInterface::reconnect(): Could not open device\n");
#ifdef linux
		fprintf(stderr,"   (Did you remember to run as root?)\n");
#endif
                return;
        }

	// We cannot do this before the call to open because the serial number
	// is a pointer to a string down in there, which forms a race condition.
	// This will be a memory leak if the device fails to open.
        if (devs != NULL) {
          hid_free_enumeration(devs);
          devs = NULL;
        }

        // Set the device to non-blocking mode.
        if (hid_set_nonblocking(_device, 1) != 0) {
		fprintf(stderr,"vrpn_HidInterface::reconnect(): Could not set device to nonblocking\n");
                return;
        }

	_working = true;
}

// Check for incoming characters.  If we get some, pass them on to the handler code.
// This is based on the source code for the hid_interrupt_read() function; it goes
// down to the libusb interface directly to get any available characters.  Because
// we're trying to support a generic device, we can't say in advance how large any
// particular transfer should be.  So we try to one character at a time until we
// don't have any more to read.  This update() routine must be called
// frequently to make sure we don't get partial results, which would mean sending
// truncated reports to the devices derived from us.

void vrpn_HidInterface::update()
{
	if (!_working) {
		//fprintf(stderr,"vrpn_HidInterface::update(): Interface not currently working\n");
		return;
	}

        // Maximum packet size for USB is 512 characters.
	vrpn_uint8 inbuf[512];
	if (inbuf == NULL) {
		fprintf(stderr,"vrpn_HidInterface::update(): Out of memory\n");
		return;
	}

        int ret = hid_read(_device, inbuf, sizeof(inbuf));
        if (ret < 0) {
		fprintf(stderr,"vrpn_HidInterface::update(): Read error\n");
		return;
        }

        // Handle any data we got.  This can include fewer bytes than we
        // asked for.
        if (ret > 0) {
          vrpn_uint8 *data = static_cast<vrpn_uint8 *>(static_cast<void*>(inbuf));
          on_data_received(ret, data);
        }
}

// This is based on sample code from UMinn Duluth at
// http://www.google.com/url?sa=t&source=web&cd=4&ved=0CDUQFjAD&url=http%3A%2F%2Fwww.d.umn.edu%2F~cprince%2FPubRes%2FHardware%2FLinuxUSB%2FPICDEM%2Ftutorial1.c&rct=j&q=hid_interrrupt_read&ei=3C3gTKWeN4GClAeX9qCXAw&usg=AFQjCNHp94pwNFKjZTMqrgPfhV1nk7p5Lg&sig2=rG1A7PL-Up0Yv-sbvEMaCw&cad=rja
// It has not yet been tested.

void vrpn_HidInterface::send_data(size_t bytes, const vrpn_uint8 *buffer)
{
	if (!_working) {
		fprintf(stderr,"vrpn_HidInterface::send_data(): Interface not currently working\n");
		return;
	}
	int ret;
	if ( (ret = hid_write(_device, const_cast<vrpn_uint8 *>(buffer), bytes)) != bytes) {
		fprintf(stderr,"vrpn_HidInterface::send_data(): hid_interrupt_write() failed with code %d\n", ret);
	}
}

#endif // any interface
