#include "vrpn_Button_USB.h"
#ifdef _WIN32

vrpn_Button_USB::vrpn_Button_USB(const char *name, const char *deviceName,vrpn_Connection *c)
					: vrpn_Button_Filter(name, c)
{
	num_buttons = 16;
	//setup of usb device
	m_hDevice = INVALID_HANDLE_VALUE;
	if ( m_hDevice != INVALID_HANDLE_VALUE )
	{
		fprintf(stderr, "USB device has already been opened." );
		return;
	}

	//--- define the device code
	char PortName[20];
	strcpy( PortName, "\\\\.\\" );
	strcat( PortName, deviceName );

	//--- open the serial port
	m_hDevice = CreateFile( PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL ); 

	if ( m_hDevice == INVALID_HANDLE_VALUE )
	{
		fprintf(stderr, "Could not open USB device." );
		return;
	}

	return;
}

vrpn_Button_USB::~vrpn_Button_USB(void)
{
	if ( m_hDevice != INVALID_HANDLE_VALUE )
		CloseHandle( m_hDevice );
}

void vrpn_Button_USB::read(void)
{
	unsigned long dat=85;
	unsigned long lIn = 65536 * dat;
	USBWrite(lIn);
	bool ret=USBRead(dat,0);
	buttons[0]=!(dat & 1);
	buttons[1]=!(dat & 16);
}

//! writes data to the device
bool vrpn_Button_USB::USBWrite(const unsigned long &data)
{
	unsigned long lOut;
	return USB_IO(data,3,lOut,1);
}

//! reads data from the device
bool vrpn_Button_USB::USBRead(unsigned long &data, int port)
{
	bool res;
	unsigned long lOut;
	unsigned long lIn =  port * 256 + 20;
	res=USB_IO(lIn,2,lOut,2);
	data=(lOut / 256) & 255;
	return res;
}

bool vrpn_Button_USB::USB_IO(unsigned long lIn, int lInSize, unsigned long &lOut, int lOutSize)
{
	unsigned long lSize;
	LPOVERLAPPED gOverlapped=0;
	if(!DeviceIoControl(m_hDevice, 0x4, &lIn, lInSize, &lOut, lOutSize, &lSize, gOverlapped))
	{
		LPVOID lpMsgBuf;
		if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, ::GetLastError(), 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language*/ (LPTSTR) &lpMsgBuf,	0, NULL ))
		{
			// Handle the error.
			return false;
		}
	}
	return true;
}

void vrpn_Button_USB::mainloop()
{
	struct timeval current_time;

	// Call the generic server mainloop, since we are a server
	server_mainloop();

	read();

	vrpn_gettimeofday(&current_time, NULL);
	// Send reports. Stays the same in a real server.
	report_changes();
	
}
#endif
