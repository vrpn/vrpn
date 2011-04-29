#include "vrpn_HumanInterface.h"
#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#endif

#if defined(VRPN_USE_HID)
class HidDebug: public vrpn_HidInterface {
	public:
		HidDebug(vrpn_HidAcceptor *a);
		~HidDebug() { }
	protected:
		void on_data_received(size_t bytes, vrpn_uint8 *buffer);
};

HidDebug::HidDebug(vrpn_HidAcceptor *a): vrpn_HidInterface(a) { }

void HidDebug::on_data_received(size_t bytes, vrpn_uint8 *buffer) {
	printf("%d bytes: ", static_cast<int>(bytes));
	for (size_t i = 0; i < bytes; i++) {
		printf("%02X ", buffer[i]);
	}
	puts("");
}
#endif

int main() {

#if defined(VRPN_USE_HID)
	unsigned N = 0; // Which device to open?
	HidDebug hid(new vrpn_HidNthMatchAcceptor(N, new vrpn_HidAlwaysAcceptor));
	printf("HID initialized.\n");
	if (hid.connected()) {
		printf("HID device vendor ID %u, product ID %u\n",
		       static_cast<unsigned>(hid.vendor()), static_cast<unsigned>(hid.product()));
	} else {
		return 1;
	}

	bool go = true;
	while (go) {
		hid.update();
#ifdef  _WIN32
		if (_kbhit())
			switch (_getch()) {
				case 27:
					go = false;
					break;
				case 'r':
				case 'R':
					hid.reconnect();
					break;
			}
#endif // _WIN32
	}

	return 0;
#else
	printf("HID support not included.\n");
	return 0;
#endif // defined(VRPN_USE_HID)
}
