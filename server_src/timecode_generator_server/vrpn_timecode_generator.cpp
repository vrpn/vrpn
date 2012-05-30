#include "vrpn_timecode_generator.h"

#ifdef	VRPN_INCLUDE_TIMECODE_SERVER


/* This library is C code. When programming in C++, there may be a problem with
   decorated names of the library, C++ can't recognize the C names in a library. */
#ifndef VRPN_ADRIENNE_INCLUDE_FILENAME
// If we don't have nice customized stuff from CMake, fall back to
// original include style.
# define VRPN_ADRIENNE_INCLUDE_FILENAME "aecinttc.h"
#endif

#ifndef VRPN_ADRIENNE_INCLUDE_HAS_EXTERN_C
extern "C"{
#endif

	// PROTOTYPES OF THE AECPC-TC.DLL FUNCTIONS
	#include VRPN_ADRIENNE_INCLUDE_FILENAME

#ifndef VRPN_ADRIENNE_INCLUDE_HAS_EXTERN_C
}
#endif

/*
//Function Prototypes
int		FindAllBoards(void);
void	RecordSmart(int, WORD);
void	RecordPci(int, WORD);
*/


vrpn_Timecode_Generator::vrpn_Timecode_Generator(const char * name, vrpn_Connection * c) :
	vrpn_Analog_Server(name, c) {

//	numBoards = AEC_PCTC_GET_NUMBER_OF_DEVICES ();	//# use when dll
//	time_gen_server->setNumChannels(numBoards);

	boardsFound = AEC_PCTC_GET_NUMBER_OF_DEVICES ();

//	boardsFound = FindAllBoards();

	AecBoard = new AECBrdFnd[boardsFound];
	FindAllBoards();
	setNumChannels(boardsFound);
	for(vrpn_int32 i=0; i < boardsFound; i++) {
		channel[i] = last[i] = -1.0;	// initialize all channel values for each board found
	}
}

vrpn_Timecode_Generator::~vrpn_Timecode_Generator(void) {
	delete [] AecBoard;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Call the 'Check_board' function for each of the possible addresses
//	that a time code board could be at and then store the board Type and
//	determine if the board has reading ability (some Smart boards are
//	generate only).  
// Returns the number of boards that where found.
//
//	PC-LTC/IOR BOARDS HAVE STANDARD ADDRESS JUMPER SETTINGS OF...
//	(port mapped)	200h, 220h, 280h, 2A0h, 300, 320h, 380h, & 3A0h
//	PC-LTC/MMR ... C0(00)h, C4(00)h, C8(00)h, CC(00)h, D0(00)h, D4(00)h
//	(memory mapped)		D8(00)h, & DC(00)h.
//	PC-LTC/(RDR, GEN, RG1) & ALL PC-VITC/XXX & ALL PC-VLTC/XXX HAVE
//		ADDRESS... CC00h, CD00h, CE00h, CF00h, DC00h, DD00h, DE00h, & DF00h
// The 'actual' address that the memory mapped boards are 'physically' 
//	mapped to is four bits higher than the jumper's lable. 
//	(ie: CC00 -> CC000h address
// PCI-LTC/RDR, all other PCI- TC models, are accessed by calling the
//  DLL with address parameter values of 0x1000 and up (increased by 0x0100)
//  to 0x2100 (total 18 different addresses). These address are arbitrary
//  (defined in the DLL). The first PCI board "found" by scanning of
//  the PCI bus by the driver/enumerator will be assigned the 0x1000
//  address "code" by the DLL. Each board after will be assigned the
//  next address up to MAX.
int	  vrpn_Timecode_Generator::FindAllBoards(void)
{
	WORD	BrdAddr;
	int		CkType, FoundCnt;

//look for and IOR return value at the possible IOR addresses
	FoundCnt = 0x00;
	BrdAddr = 0x200;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);	// returns a 0(none), 
												// 1(IOR), 2(MMR), 3(Smart)		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x220;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x280;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x2A0;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x300;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x320;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x380;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}
	BrdAddr = 0x3A0;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		AecBoard[FoundCnt].BrdReads = TRUE;
		FoundCnt++;
	}

//look for and MMR/Smart return value at the possible memory map addresses
	BrdAddr = 0xC000;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xC400;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xC800;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xCC00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xD000;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xD400;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xD800;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xDC00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xCD00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xCE00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xCF00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xDD00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xDE00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}
	BrdAddr = 0xDF00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		if (CkType == 2 )
		{
			AecBoard[FoundCnt].BrdReads = TRUE;
			FoundCnt++;
		}
		else if (CkType == 3 )
		{
			RecordSmart(FoundCnt, BrdAddr);
			FoundCnt++;
		}	
	}

	//now see if there are any PCI boards
	BrdAddr = 0x1000;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}

	BrdAddr = 0x1100;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1200;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1300;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1400;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1500;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1600;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1700;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1800;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1900;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1A00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1B00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1C00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1D00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1E00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x1F00;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x2000;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	BrdAddr = 0x2100;
	CkType = AEC_PCTC_CHECK_BOARD (&BrdAddr);		
	if	(CkType != 0 )
	{
		AecBoard[FoundCnt].BrdAddr = BrdAddr;
		AecBoard[FoundCnt].BrdCkType = CkType;
		RecordPci(FoundCnt, BrdAddr);
		FoundCnt++;
	}
	return (boardsFound = FoundCnt);
//	return FoundCnt;
}


void vrpn_Timecode_Generator::RecordPci(int Cnt, WORD BrdAddr)
{
	WORD		Addr, Offset08, OffsetEF;
	BYTE		bId, bType; // bytes from 3E0, 3E1, 3E2, 3EF.
	BYTE		bCom, bRet;
	BOOL		ok;
	
	Addr =  BrdAddr;
	Offset08 = 0x08;
	OffsetEF = 0x3EF;

	
	// read the smart board type id byte ( 0x50 VLTC, 0x93 VITC, 0x98 LTC)
	//This offset value is NOT IN THE PCI ADDRESS RANGE, BUT THE DLL MAPS
	// this value to a "created" value based upon the data byte at offset
	// 0x08 of the PCI device.
	if (ok = AEC_PCTC_READ_1BYTE (&bType, &OffsetEF, &Addr))	
		AecBoard[Cnt].BrdModel = (WORD)bType;	//3ef

	if (ok = AEC_PCTC_READ_1BYTE (&bId, &Offset08, &Addr))	
	{
		if ((bId & 0x50) != 0x00)	//if LTC and/or VITC reader bits4&6 set
		{
			AecBoard[Cnt].BrdReads = TRUE;
			// now make sure that the board is in reader mode
			//By sending the "dual" reader mode command, the LTC
			// and VITC models will have their available (only) reader enabled
			// and the return "bRet" will NOT match, but will be a
			// 0x21 (LTC), 0x22 (VITC) or 0x23 (VLTC).
			bCom = 0x23; // dual reader mode command vltc board
			ok = AEC_PCTC_SEND_COMMAND(&bCom, &bRet, &Addr);
		}
		else AecBoard[Cnt].BrdReads = FALSE;
	}
	else AecBoard[Cnt].BrdReads = FALSE;
	//no return for 'void'
}


void vrpn_Timecode_Generator::RecordSmart(int Cnt, WORD BrdAddr)
{
	WORD		Addr, OffsetE0, OffsetE1, OffsetE2, OffsetEF;
	BYTE		bId0, bId1, bId2, bType; // bytes from 3E0, 3E1, 3E2, 3EF.
	BYTE		bCom, bRet;
	BOOL		ok;
	
	Addr =  BrdAddr;
	OffsetE0 = 0x3E0;
	OffsetE1 = 0x3E1;
	OffsetE2 = 0x3E2;
	OffsetEF = 0x3EF;
	
	// read the smart board type id byte ( 0x50 VLTC, 0x93 VITC, 0x98 LTC)
	if (ok = AEC_PCTC_READ_1BYTE (&bType, &OffsetEF, &Addr))	
		AecBoard[Cnt].BrdModel = (WORD)bType;

	// read the board id bytes to see if this board reads tc.
	ok = AEC_PCTC_READ_1BYTE (&bId0, &OffsetE0, &Addr);
	ok = AEC_PCTC_READ_1BYTE (&bId1, &OffsetE1, &Addr);
	ok = AEC_PCTC_READ_1BYTE (&bId2, &OffsetE2, &Addr);
		
	if ((bId0 == 'R' ) || (bId1 == 'R' ) || (bId2 == 'R' ))
	{
		AecBoard[Cnt].BrdReads = TRUE;
		// now make sure that the board is in reader mode
		bCom = 0x16; // idle command  ltc/vitc/vltc
		ok = AEC_PCTC_SEND_COMMAND(&bCom, &bRet, &Addr);
		if (bType ==0x50)
			bCom = 0x23; // dual reader mode command vltc board
		else bCom = 0x10; // reader mode command ltc/vitc boards
			
		ok = AEC_PCTC_SEND_COMMAND(&bCom, &bRet, &Addr);

	}
	else AecBoard[Cnt].BrdReads = FALSE;
	//no return for 'void'
}

void vrpn_Timecode_Generator::mainloop() {
	do_poll();
	report_changes();
}

void vrpn_Timecode_Generator::do_poll(void) {
	unsigned long tVal;
	vrpn_int32	aecint;

	// this should read a value from the board, call server_mainloop() and note
	// whether the value has changed or not
	this->server_mainloop();

	for (vrpn_int32 i=0; i < boardsFound; i++) {
		if (AecBoard[i].BrdReads == TRUE) {
			aecint = AEC_PCTC_READ_TB(&tVal, &AecBoard[i].BrdAddr);
			channel[i] = (vrpn_float64) tVal;
		}
//		printf("brd %d: %f\n",i, channel[i]);
	}
}

#endif
