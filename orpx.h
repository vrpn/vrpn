#ifndef _ORPX_H
#define _ORPX_H
// This class takes care of serial port communications for an ORPX2 ohmmeter
// connection to a PC running Win95 or WinNT. Also the constants at the
// beginning of this header file may be useful for client applications that
// are connected to an ORPX remotely. Because the settings are discrete for
// the ORPX while the vrpn_Ohmmeter class uses floats for the corresponding
// parameters, we must jump to the next appropriate value (e.g. by rounding).
// The client can use the following values to show the user what they
// can actually select. Perhaps this should be taken care of in vrpn_Ohmmeter
// but it seemed like it would be too complicated - especially since we only
// want to use one ohmmeter at the moment. If needed, additional messages could
// be added later to communicate which discrete values are allowed and the
// client application could get them that way.

#define NUM_ORPX_SETTINGS 6
#define NUM_ORPX_CHANNELS 4
// device specific values corresponding to index sent out serial port (0..5)
// voltage used for measurement: units = [millivolts]:
const double orpx_voltages[NUM_ORPX_SETTINGS] =
                                {0.003, 0.010, 0.030, 0.100, 0.300, 1.0};
// range for measurement: (example: 0.5 = (0.5-5)), units = [ohms]
const double orpx_ranges[NUM_ORPX_SETTINGS] = {0.5, 5, 50, 500, 5000, 50000};
// filter time constants: units = [seconds]
const double orpx_filters[NUM_ORPX_SETTINGS] = {0.1, 0.3, 1.0, 3.0, 10.0, 30.0};

#ifdef _WIN32
// C = CLK, D = DataIn, values passed to outp()
typedef enum {C0D0 = 0, C1D0 = 16, C0D1 = 32, C1D1 = 48} output_t;

// This struct contains all the information that passes to and from orpx in a
// single call to read_and_write()
typedef struct _channel_params_t {
	// input parameters
	int channel;	// which channel to measure (0.. NUM_ORPX_CHANNELS-1)
        int Vmeas;      // ="tension",which measurement voltage to use: 0..5 =
                        //      0..5 = 3,10,30,100,300,1000mV
        int range;      // ="gamme", which measurement range:
                        //      0..5 = 0.5,5,50,500,5k,50k ohms
        int filter;     // ="filtre", which noise filter:
                        //      0..5 = 0.1, 0.3, 1, 3, 10, 30 seconds

        // output parameters
        int measurement;	// 16 bit measurement
        int saturated;		// binary flag
} orpx_params_t;

class ORPX_SerialComm {
  public:
    ORPX_SerialComm(char *port = "COM1");
    ~ORPX_SerialComm();
    void reset_orpx(void);
    int read_and_write(double &res, double &err, orpx_params_t &params);
    double get_resistance(orpx_params_t &params);
    double get_error(orpx_params_t &params);
  protected:
    void outp(int val);
    int inp(void);
    void bit9(int x);
    void bit3(int x);
    void bit2(int x);
    unsigned int bit16(unsigned int reg);

    HANDLE hCom;
    DCB dcb;
    DWORD dwmask;
	DWORD evtmask;
	DWORD inbuf_size;
	DWORD outbuf_size;

    int last_range;

    // orpx communication parameters:
    unsigned int bit_period;	// = iPeriode, sets calling frequency for inp()
    unsigned int frame_period;	// = iSommeil, sets calling frequency for 
				// read_and_write()

};

#endif	// _WIN32
#endif
