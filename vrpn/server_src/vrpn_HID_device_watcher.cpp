#include "vrpn_HumanInterface.h"
#include "vrpn_Xkeys.h"
#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#endif

class HidDebug: public vrpn_HidInterface {
public:
	HidDebug(vrpn_HidAcceptor *a);
	~HidDebug() { }
protected:
	void on_data_received(size_t bytes, vrpn_uint8 *buffer);
};

HidDebug::HidDebug(vrpn_HidAcceptor *a): vrpn_HidInterface(a) { }

void HidDebug::on_data_received(size_t bytes, vrpn_uint8 *buffer) {
	for (size_t i = 0; i < bytes; i++)
		printf("%02X ", buffer[i]);
	puts("");
}

int main() {
        unsigned N = 0; // Which device to open?
	HidDebug hid(new vrpn_HidNthMatchAcceptor(N, new vrpn_HidAlwaysAcceptor));
	printf("HID initialized.\n");
        if (hid.connected()) {
          printf("HID device vendor ID %u, product ID %u\n",
            static_cast<unsigned>(hid.vendor()), static_cast<unsigned>(hid.product()));
        }

	bool go = true;
	while(go) {
		hid.update();
#ifdef  _WIN32
                if (_kbhit())
			switch(_getch()) {
				case 27: go = false; break;
				case 'r':
				case 'R':
					hid.reconnect(); break;
			}
#endif
        }

        return 0;
}