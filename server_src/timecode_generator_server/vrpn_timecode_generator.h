#ifndef VRPN_TIMECODE_GENERATOR_H
#define VRPN_TIMECODE_GENERATOR_H

#include "vrpn_Analog.h"
#ifdef	VRPN_INCLUDE_TIMECODE_SERVER

/* This library is C code. When programming in C++, there may be a problem with
   decorated names of the library, C++ can't recognize the C names in a library. */
extern "C"{
	#include "aecinttc.h"    // PROTOTYPES OF THE AECPC-TC.DLL FUNCTIONS
}

class vrpn_Timecode_Generator : public vrpn_Analog_Server {
public:
	vrpn_Timecode_Generator(const char * name, vrpn_Connection * c);
	virtual ~vrpn_Timecode_Generator(void);

	void mainloop(void);

	typedef struct _AECBrdFnd
	{
		WORD	BrdAddr;
		int		BrdCkType;
		WORD	BrdModel;
		BOOL	BrdReads;
		PVOID	PhysAddr;
	} AECBrdFnd, *PAECBrdFnd;

private:
	int FindAllBoards(void);	
	void RecordPci(int Cnt, WORD BrdAddr);
	void RecordSmart(int Cnt, WORD BrdAddr);
	void do_poll(void);

	int	 boardNum, boardsFound;			// number of boards found in registry
	AECBrdFnd	*AecBoard;
	char	hhmmssff[12];	    //data string to hold formated unpacked dataC
	char	hhmmssffx[30];

};

#endif

#endif
