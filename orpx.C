#include "windows.h"
#include "orpx.h"

static int iomem;      /* memoire pour se rappeler le bit en sortie i/o */

ORPX_SerialComm::ORPX_SerialComm(char *port)
{
    bit_period = 0;
    frame_period = 20;
    evtmask = EV_RXCHAR;
    inbuf_size = 10000;
    outbuf_size = 10000;

    // open serial port
    hCom = CreateFile(port,
	GENERIC_READ | GENERIC_WRITE,
	0, 
	NULL, 
	OPEN_EXISTING,
	FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
	NULL);

    DWORD status = GetLastError();
    if (hCom == INVALID_HANDLE_VALUE) return;
    dcb.DCBlength = sizeof(DCB);
    dcb.DCBlength=sizeof(DCB);
    dcb.BaudRate=CBR_9600;	/* current baud rate */
    dcb.fBinary=1;		/* Binary Mode (skip EOF check)    */
    dcb.fParity=0;		/* Enable parity checking          */
    dcb.fOutxCtsFlow=0;		/* CTS handshaking on output       */
    dcb.fOutxDsrFlow=0;		/* DSR handshaking on output       */
    dcb.fDtrControl=DTR_CONTROL_DISABLE;/* DTR Flow control  */
    dcb.fDsrSensitivity=0;	/* DSR Sensitivity   */
    dcb.fTXContinueOnXoff=0;	/* Continue TX when Xoff sent */
    dcb.fOutX=0;		/* Enable output X-ON/X-OFF   */
    dcb.fInX=0;			/* Enable input X-ON/X-OFF    */
    dcb.fErrorChar=0;		/* Enable Err Replacement     */
    dcb.fNull=0;		/* Enable Null stripping      */
    dcb.fRtsControl=RTS_CONTROL_DISABLE;// received event character 
    dcb.fAbortOnError=0;	/* Abort all reads and writes on Error */
    dcb.XonLim=0;		/* Transmit X-ON threshold         */
    dcb.XoffLim=0;		/* Transmit X-OFF threshold        */
    dcb.ByteSize=8;		/* Number of bits/byte, 4-8        */
    dcb.Parity=0;		/* 0-4=None,Odd,Even,Mark,Space    */
    dcb.StopBits=0;		/* 0,1,2 = 1, 1.5, 2               */
	
    COMMTIMEOUTS commtimeouts;
    GetCommTimeouts(hCom, &commtimeouts);
    commtimeouts.ReadIntervalTimeout = MAXDWORD;
    commtimeouts.ReadTotalTimeoutMultiplier = 0;
    commtimeouts.ReadTotalTimeoutConstant = 0;
    commtimeouts.WriteTotalTimeoutMultiplier = 10;
    commtimeouts.WriteTotalTimeoutConstant = 5000;
    SetCommTimeouts(hCom, &commtimeouts); 

    SetCommState(hCom, &dcb);
    SetCommMask(hCom,evtmask);
    SetupComm(hCom,inbuf_size,outbuf_size);
    PurgeComm(hCom,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

}

ORPX_SerialComm::~ORPX_SerialComm()
{
	CloseHandle(hCom);
}

void ORPX_SerialComm::reset_orpx(void)
{
    EscapeCommFunction(hCom,CLRDTR);
    EscapeCommFunction(hCom,SETDTR);
    EscapeCommFunction(hCom,CLRDTR);
    EscapeCommFunction(hCom,SETDTR);
    EscapeCommFunction(hCom,CLRRTS);
    EscapeCommFunction(hCom,SETRTS);
    EscapeCommFunction(hCom,CLRRTS);
    EscapeCommFunction(hCom,SETRTS);
    Sleep(500);
}

// This is basically the definition for the communications protocol used by the
// ORPX2 ohmmeter.
// Reading and writing to the device is done in this function. 
// The computer tells the ohmmeter what channel, voltage, range, and filter
// to switch to for the next measurement and the last measurement is sent
// in exchange.
// saturated - indicates whether the last measurement was saturated
//		this may be the result of excess noise or an incorrect range
//		setting
// measurement - this is an integer in the range 0..65535
//		resistance = range_min*65536/measurement
//		where range_min is the minimum resistance measureable in the
//			range sent to the ohmmeter the last time this function
//			was called 
int ORPX_SerialComm::read_and_write(double &resistance, double &error,
				orpx_params_t &params)
{
    int doing_okay = 1;
    int satrg; // dummy variable to store regulation saturation
    int vrf;	// voltage, range, filter parameter (9 bits)
    vrf = params.Vmeas + 8*params.range + 64*params.filter;

    outp(C1D1);
    outp(C1D0);				/* 7 fois io(3);io(1);  */
    outp(C1D1);
    outp(C1D0);
    outp(C1D1);
    outp(C1D0);
    outp(C1D1);
    outp(C1D0);
    outp(C1D1);
    outp(C1D0);
    outp(C1D1);
    outp(C1D0);
    outp(C1D1);
    outp(C1D0); 
    if  (inp()&1)	doing_okay=0;	/* io(1)  reponse  0    */
    outp(C1D1); 
    if ((inp()&1)==0)	doing_okay=0;	/* io(3)  reponse  1    */
    outp(C0D1); 
    if ((inp()&1)==0)	doing_okay=0;	/* io(2)  reponse  1    */
    outp(C0D0); 
    if  (inp()&1)	doing_okay=0;	/* io(0)  reponse  0    */
    outp(C0D1); 
    if ((inp()&1)==0)	doing_okay=0;  /* io(2)  reponse  1    */
    outp(C1D1); 
    if ((inp()&1)==0)	doing_okay=0;  /* io(3)  reponse  1    */
    outp(C1D0); 
    if ((inp()&1)==0)	doing_okay=0;  /* io(1)  reponse  1    */
    outp(C0D0); 
    if  (inp()&1)	doing_okay=0;  /* io(0)  reponse  0    */
    if (doing_okay){
		iomem = 0;
		bit2(0); bit2(0); // regulation stuff we aren't using
		bit9(vrf);
		bit3(params.channel);
		outp(C1D1);outp(C1D0);
		outp(C1D1);
		params.saturated = 1-(inp()&1);
		outp(C1D0);
		iomem = 0;
		params.measurement = bit16(0);  // pass 0 for regulation current
		outp(C1D1);outp(C1D0);
		outp(C1D1);
		satrg = 1-(inp()&1);	// regulation saturation dummy variable
		outp(C1D0);
		outp(C1D1);outp(C1D0);
    }

    Sleep(frame_period);
    if (doing_okay){
	// compute resistance and error using last_range
	resistance = get_resistance(params);
	error = get_error(params);
	last_range = params.range;
    }

    return doing_okay;
}

void ORPX_SerialComm::outp(int val)
{
    if (hCom==NULL)	return;

    switch (val){
	case C0D0:
		EscapeCommFunction(hCom,CLRDTR);
		EscapeCommFunction(hCom,CLRRTS);
		break;
	case C0D1:
		EscapeCommFunction(hCom,CLRDTR);
		EscapeCommFunction(hCom,SETRTS);
		break;
	case C1D0:
		EscapeCommFunction(hCom,SETDTR);
		EscapeCommFunction(hCom,CLRRTS);
		break;
	case C1D1:
		EscapeCommFunction(hCom,SETDTR);
		EscapeCommFunction(hCom,SETRTS);
		break;
    }
}

int  ORPX_SerialComm::inp()
{
    DWORD modemStat;
	unsigned int i;
    if (hCom==NULL)	return 0;
    for (i = 0; i < bit_period; i++); // XXX - copied from the French
    GetCommModemStatus(hCom,&modemStat);
    return ((modemStat&MS_CTS_ON)!=0);
}

void ORPX_SerialComm::bit9(int x)								/* sort 9 bits x (0-511)*/
{
    bit3(x>>6);
    bit3(x>>3);
    bit3(x);
}

void ORPX_SerialComm::bit3(int x)              /* sort relais x (0-3)  */
{
    outp(iomem);
    iomem=32-(x<<3 & 32);          /* prep. bit*16 pour S2  */
    outp(iomem);outp(iomem+16);    /* avec S1=0 puis S1=1  */
    outp(iomem);
    iomem=32-(x<<4 & 32);          /* prep. bit*16 pour S2  */
    outp(iomem);outp(iomem+16);    /* avec S1=0 puis S1=1  */
    outp(iomem);
    iomem=32-(x<<5 & 32);          /* prep. bit*16 pour S2  */
    outp(iomem);outp(iomem+16);    /* avec S1=0 puis S1=1  */
}

void ORPX_SerialComm::bit2(int x)             /* sort relais x (0-3)  */
{
    outp(iomem);
    iomem=32-(x<<4 & 32);          /* prep. bit*16 pour S2  */
    outp(iomem);outp(iomem+16);    /* avec S1=0 puis S1=1  */
    outp(iomem);
    iomem=32-(x<<5 & 32);          /* prep. bit*16 pour S2  */
    outp(iomem);outp(iomem+16);    /* avec S1=0 puis S1=1  */
}


unsigned int ORPX_SerialComm::bit16(unsigned int reg)
{
    int i;
    unsigned int mes=0;
    for (i=1;i<=16;i++)
    {
	outp(iomem);
	mes=(mes<<1) + (inp()&1);
	iomem=reg>>10 & 32 ;	 /* msb en bit 2  (*16) */
		outp(iomem);outp(iomem+16);
		reg=reg<<1;
	}
    return(mes);
}

double ORPX_SerialComm::get_resistance(orpx_params_t &params)
{
    double range_min = orpx_ranges[params.range];
    double resistance = 0;
    if (params.measurement == 0) resistance = 0;
    else resistance = range_min*65536.0/(double)params.measurement;
    return resistance;
}

double ORPX_SerialComm::get_error(orpx_params_t &params)
{
    return 0; // XXX replace this with real error when you read the specs
}
