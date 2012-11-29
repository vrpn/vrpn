#include "NIUtil.h"

const short Max_NIDAQ_Devices(10);

// This array keeps track of whether a particular device has been
// returned by the findDevice() function.  It prevents the same
// device from being found twice, enabling the program to open more
// than one device of the same type on the same computer

static	bool  opened_already[Max_NIDAQ_Devices];

namespace NIUtil
{
   const char *nameCodeToString
   (
      int code
   )
   {
      const char *name = NULL;


      switch (code)
      {
         // The folling table comes from:
         // National Instruments(TM)
         // NI-DAQ(TM) Function Reference Manual for PC Compatibles
         // Version 6.6
         // August 1999 Edition
         // Part Number 321645E-01
         // Chapter 2 Function Reference
         // Init_DA_Brds : pages 2-274 - 2-277
	 // Entries were added based on later versions of the manual.

         case  -1: name = "Not a National Instruments DAQ device"; break;
         case   7: name = "PC-DIO-24"; break;
         case   8: name = "AT-DIO-32F"; break;
         case  12: name = "PC-DIO-96"; break;
         case  13: name = "PC-LPM-16"; break;
         case  14: name = "PC-TIO-10"; break;
         case  15: name = "AT-AO-6"; break;
         case  25: name = "AT-MIO-16E-2"; break;
         case  26: name = "AT-AO-10"; break;
         case  27: name = "AT-A2150C"; break;
         case  28: name = "Lab-PC+"; break;
         case  30: name = "SCXI-1200"; break;
         case  31: name = "DAQCard-700"; break;
         case  32: name = "NEC-MIO-16E-4"; break;
         case  33: name = "DAQPad-1200"; break;
         case  35: name = "DAQCard-DIO-24"; break;
         case  36: name = "AT-MIO-16E-10"; break;
         case  37: name = "AT-MIO-16DE-10"; break;
         case  38: name = "AT-MIO-64E-3"; break;
         case  39: name = "AT-MIO-16XE-50"; break;
         case  40: name = "NEC-AI-16E-4"; break;
         case  41: name = "NEC-MIO-16XE-50"; break;
         case  42: name = "NEC-AI-16XE-50"; break;
         case  43: name = "DAQPad-MIO-16XE-50"; break;
         case  44: name = "AT-MIO-16E-1"; break;
         case  45: name = "PC-OPDIO-16"; break;
         case  46: name = "PC-AO-2DC"; break;
         case  47: name = "DAQCard-AO-2DC"; break;
         case  48: name = "DAQCard-1200"; break;
         case  49: name = "DAQCard-500"; break;
         case  50: name = "AT-MIO-16XE-10"; break;
         case  51: name = "AT-AI-16XE-10"; break;
         case  52: name = "DAQCard-AI-16XE-50"; break;
         case  53: name = "DAQCard-AI-16E-4"; break;
         case  54: name = "DAQCard-516"; break;
         case  55: name = "PC-516"; break;
         case  56: name = "PC-LPM-16PnP"; break;
         case  57: name = "Lab-PC-1200"; break;
         case  58: name = "Lab-PC-1200AI"; break;
         case  59: name = "VXI-MIO-64E-1"; break;
         case  60: name = "VXI-MIO-64XE-10"; break;
         case  61: name = "VXI-AO-48XDC"; break;
         case  62: name = "VXI-DIO-128"; break;
         case  65: name = "PC-DIO-24PnP"; break;
         case  66: name = "PC-DIO-96PnP"; break;
         case  67: name = "AT-DIO-32HS"; break;
         case  68: name = "PXI-6533"; break;
         case  75: name = "DAQPad-6507/6508"; break;
         case  76: name = "DAQPad-6020E for USB"; break;
         case  88: name = "DAQCard-6062E"; break;
         case  90: name = "DAQCard-6023E"; break;
         case  91: name = "DAQCard-6024E"; break;
         case 200: name = "PCI-DIO-96"; break;
         case 201: name = "PCI-1200"; break;
         case 202: name = "PCI-MIO-16XE-50"; break;
         case 204: name = "PCI-MIO-16XE-10"; break;
         case 205: name = "PCI-MIO-16E-1"; break;
         case 206: name = "PCI-MIO-16E-4"; break;
         case 207: name = "PXI-6070E"; break;
         case 208: name = "PXI-6040E"; break;
         case 209: name = "PXI-6030E"; break;
         case 210: name = "PXI-6011E"; break;
         case 211: name = "PCI-DIO-32HS"; break;
         case 215: name = "DAQCard-6533"; break;
         case 220: name = "PCI-6031E (MIO-64XE-10)"; break;
         case 221: name = "PCI-6032E (AI-16XE-10)"; break;
         case 222: name = "PCI-6033E (AI-64XE-10)"; break;
         case 223: name = "PCI-6071E (MIO-64E-1)"; break;
         case 232: name = "PCI-6602"; break;
         case 233: name = "PCI-4451"; break;
         case 234: name = "PCI-4452"; break;
         case 235: name = "NI 4551"; break;
         case 236: name = "NI 4552"; break;
         case 237: name = "PXI-6602"; break;
         case 240: name = "PXI-6508"; break;
         case 241: name = "PCI-6110E"; break;
         case 244: name = "PCI-6111E"; break;
         case 256: name = "PCI-6503"; break;
         case 261: name = "PCI-6711"; break;
         case 262: name = "PXI-6711"; break;
         case 263: name = "PCI-6713"; break;
         case 264: name = "PXI-6713"; break;
         case 265: name = "PCI-6704"; break;
         case 266: name = "PXI-6704"; break;
         case 267: name = "PCI-6023E"; break;
         case 268: name = "PXI-6023E"; break;
         case 269: name = "PCI-6024E"; break;
         case 270: name = "PXI-6024E"; break;
         case 271: name = "PCI-6025E"; break;
         case 272: name = "PXI-6025E"; break;
         case 273: name = "PCI-6052E"; break;
         case 274: name = "PXI-6052E"; break;
         case 275: name = "DAQPad-6070E"; break;
         case 276: name = "DAQPad-6052E"; break;
         case 285: name = "PCI-6527"; break;
         case 286: name = "PXI-6527"; break;
         case 308: name = "PCI-6601"; break;
         case 311: name = "PCI-6703"; break;
         case 314: name = "PCI-6034E"; break;
         case 315: name = "PXI-6034E"; break;
         case 316: name = "PCI-6035E"; break;
         case 317: name = "PXI-6035E"; break;
         case 318: name = "PXI-6703"; break;
         case 319: name = "PXI-6608"; break;
         case 320: name = "PCI-4453"; break;
         case 321: name = "PCI-4454"; break;
	 case 348: name = "DAQCard-6036E"; break;
	 case 350: name = "PCI-6733"; break;
         default :
	    fprintf(stderr, "NIUtil::nameCodeToString: Unknown code (%d)!\n", code);
            name = "Unknown";
            break;
      }

      return name;
   }

   const char * getDeviceName
   (
      short deviceNumber
   )
   {
      unsigned long code;


      Get_DAQ_Device_Info(deviceNumber, ND_DEVICE_TYPE_CODE, &code);

      return nameCodeToString(code);
   }

   unsigned long getDeviceCode
   (
      short deviceNumber
   )
   {
      unsigned long code;


      Get_DAQ_Device_Info(deviceNumber, ND_DEVICE_TYPE_CODE, &code);

      return code;
   }

   short findDevice(const char *name)
   {
      static bool first_time = true;
      if (first_time) {
	int i;
	for (i = 0; i < Max_NIDAQ_Devices; i++) {
	  opened_already[i] = false;
	}
	first_time = false;
      }
      short device(1);
      unsigned long code(0);

      while ( (-1 != code) && (device < Max_NIDAQ_Devices) ) {
	 code = getDeviceCode(device);

	 if ( !strcmp(name,nameCodeToString(code)) ) {
	   if (!opened_already[device]) {
	     opened_already[device] = true;
	     return device;
	   }
	 }

	 device++;
      }

      return -1;
   }

   int checkError
   (
      short    status,
      const char *message,
      bool   warn
   ) 
   {
      if (status)
      {
         return NIDAQErrorHandler(status, message, warn);
      }

      return 0;
   }
}
