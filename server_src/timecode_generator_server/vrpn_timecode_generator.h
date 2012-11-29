#ifndef VRPN_TIMECODE_GENERATOR_H
#define VRPN_TIMECODE_GENERATOR_H

#include "vrpn_Configure.h" // IWYU pragma: keep
#ifdef	VRPN_INCLUDE_TIMECODE_SERVER

#include "vrpn_Analog.h"

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
