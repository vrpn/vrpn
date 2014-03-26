#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for printf, fprintf, puts, NULL, etc

#include "vrpn_Configure.h"             // for VRPN_USE_HID
#include "vrpn_HumanInterface.h"        // for vrpn_HidAcceptor (ptr only), etc
#include "vrpn_Types.h"                 // for vrpn_uint8, vrpn_uint16
#ifdef _WIN32
#include <conio.h>
#endif
#include <sstream>                      // for istringstream, basic_ios, etc
#include <string>                       // for operator==, string, etc

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
#ifdef TEST
	for (size_t i = 0; i < (bytes / 2); i++) {
		if ((i != 0) && ((i % 20) == 0))
		{
			printf("\n");
		}
		printf("%02X ", buffer[i]);
	}
#else
	printf("%d bytes: ", static_cast<int>(bytes));
	for (size_t i = 0; i < bytes; i++) {
		printf("%02X ", buffer[i]);
	}
#endif // TEST
	puts("");
}
#endif

int usage(char * argv0) {
	printf("Usage:\n\n"
		"%s -h|--help\n"
		"	Display this help text.\n\n"

		"%s [N]\n"
		"	Open HID device number N (default to 0)\n\n"

		"%s VEND PROD [N]\n"
		"	Open HID device number N (default to 0) that matches\n"
		"	vendor VEND and product PROD, in _decimal_\n\n"


#ifdef  _WIN32
		"During runtime:\n"
		"	Press ESC to exit\n"
		"	Press r to reconnect\n\n"
#endif
		,
		argv0, argv0, argv0);
	return 1;
}

int failedOnArgument(int argNum, const char * expected, char * argv[]) {
	fprintf(stderr, "Failed to interpret argument %d: expected %s, got '%s' - usage help follows.\n\n", argNum, expected, argv[argNum]);
	return usage(argv[0]);
}

int main(int argc, char * argv[]) {

#if defined(VRPN_USE_HID)
	if (argc > 1 && (std::string("-h") == argv[1] || std::string("--help") == argv[1])) {
		return usage(argv[0]);
	}
	vrpn_HidAcceptor * acceptor = NULL;
	unsigned N = 0; // Which device to open?
	if (argc >= 3) {
		vrpn_uint16 vend;
		std::istringstream vendS(argv[1]);
		if (!(vendS >> vend)) {
			return failedOnArgument(1, "a decimal vendor ID", argv);
		}

		vrpn_uint16 prod;
		std::istringstream prodS(argv[2]);
		if (!(prodS >> prod)) {
			return failedOnArgument(2, "a decimal product ID", argv);
		}

		if (argc >= 4) {
			std::istringstream nS(argv[3]);
			if (!(nS >> N)) {
				return failedOnArgument(3, "a number indicating which matching device to pick, or nothing for the default '0'", argv);
			}
		}
		printf("Will accept HID device number %u that has vendor:product %04x:%04x\n", N, vend, prod);
		acceptor = new vrpn_HidProductAcceptor(vend, prod);
	} else {
		if (argc == 2) {
			std::istringstream nS(argv[1]);
			if (!(nS >> N)) {
				return failedOnArgument(1, "a number indicating which device to pick, or nothing for the default '0'", argv);
			}
		}
		printf("Will accept HID device number %u\n", N);
		acceptor =  new vrpn_HidAlwaysAcceptor;
	}

	HidDebug hid(new vrpn_HidNthMatchAcceptor(N, acceptor));
	printf("HID initialized.\n");
	if (hid.connected()) {
		printf("Connected: HID device vendor ID %u, product ID %u, aka %04x:%04x\n",
		       static_cast<unsigned>(hid.vendor()), static_cast<unsigned>(hid.product()),
		       static_cast<unsigned>(hid.vendor()), static_cast<unsigned>(hid.product()));
	} else {
		printf("Could not connect.\n");
		return 1;
	}

	bool go = true;
	printf("Entering update loop.\n");
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
