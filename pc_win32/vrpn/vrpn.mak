# Microsoft Developer Studio Generated NMAKE File, Based on vrpn.dsp
!IF "$(CFG)" == ""
CFG=vrpn - Win32 Debug
!MESSAGE No configuration specified. Defaulting to vrpn - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vrpn - Win32 Release" && "$(CFG)" != "vrpn - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vrpn.mak" CFG="vrpn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vrpn - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vrpn - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe

!IF  "$(CFG)" == "vrpn - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\vrpn.lib"

!ELSE 

ALL : "$(OUTDIR)\vrpn.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vrpn_Analog.obj"
	-@erase "$(INTDIR)\vrpn_Button.obj"
	-@erase "$(INTDIR)\vrpn_Clock.obj"
	-@erase "$(INTDIR)\vrpn_Connection.obj"
	-@erase "$(INTDIR)\vrpn_FileConnection.obj"
	-@erase "$(INTDIR)\vrpn_FileController.obj"
	-@erase "$(INTDIR)\vrpn_ForceDevice.obj"
	-@erase "$(INTDIR)\vrpn_Forwarder.obj"
	-@erase "$(INTDIR)\vrpn_ForwarderController.obj"
	-@erase "$(INTDIR)\vrpn_Ohmmeter.obj"
	-@erase "$(INTDIR)\vrpn_Serial.obj"
	-@erase "$(INTDIR)\vrpn_Shared.obj"
	-@erase "$(INTDIR)\vrpn_Sound.obj"
	-@erase "$(INTDIR)\vrpn_Text.obj"
	-@erase "$(INTDIR)\vrpn_Tracker.obj"
	-@erase "$(OUTDIR)\vrpn.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\vrpn.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vrpn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\vrpn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\vrpn_Analog.obj" \
	"$(INTDIR)\vrpn_Button.obj" \
	"$(INTDIR)\vrpn_Clock.obj" \
	"$(INTDIR)\vrpn_Connection.obj" \
	"$(INTDIR)\vrpn_FileConnection.obj" \
	"$(INTDIR)\vrpn_FileController.obj" \
	"$(INTDIR)\vrpn_ForceDevice.obj" \
	"$(INTDIR)\vrpn_Forwarder.obj" \
	"$(INTDIR)\vrpn_ForwarderController.obj" \
	"$(INTDIR)\vrpn_Ohmmeter.obj" \
	"$(INTDIR)\vrpn_Serial.obj" \
	"$(INTDIR)\vrpn_Shared.obj" \
	"$(INTDIR)\vrpn_Sound.obj" \
	"$(INTDIR)\vrpn_Text.obj" \
	"$(INTDIR)\vrpn_Tracker.obj"

"$(OUTDIR)\vrpn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\vrpn.lib"

!ELSE 

ALL : "$(OUTDIR)\vrpn.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vrpn_Analog.obj"
	-@erase "$(INTDIR)\vrpn_Button.obj"
	-@erase "$(INTDIR)\vrpn_Clock.obj"
	-@erase "$(INTDIR)\vrpn_Connection.obj"
	-@erase "$(INTDIR)\vrpn_FileConnection.obj"
	-@erase "$(INTDIR)\vrpn_FileController.obj"
	-@erase "$(INTDIR)\vrpn_ForceDevice.obj"
	-@erase "$(INTDIR)\vrpn_Forwarder.obj"
	-@erase "$(INTDIR)\vrpn_ForwarderController.obj"
	-@erase "$(INTDIR)\vrpn_Ohmmeter.obj"
	-@erase "$(INTDIR)\vrpn_Serial.obj"
	-@erase "$(INTDIR)\vrpn_Shared.obj"
	-@erase "$(INTDIR)\vrpn_Sound.obj"
	-@erase "$(INTDIR)\vrpn_Text.obj"
	-@erase "$(INTDIR)\vrpn_Tracker.obj"
	-@erase "$(OUTDIR)\vrpn.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /GX /Z7 /Od /I "..\.." /I "..\..\..\quat" /D "WIN32"\
 /D "_DEBUG" /D "_WINDOWS" /D "VRPN_CLIENT_ONLY" /Fp"$(INTDIR)\vrpn.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c /Tp 
CPP_OBJS=.\Debug/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vrpn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\vrpn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\vrpn_Analog.obj" \
	"$(INTDIR)\vrpn_Button.obj" \
	"$(INTDIR)\vrpn_Clock.obj" \
	"$(INTDIR)\vrpn_Connection.obj" \
	"$(INTDIR)\vrpn_FileConnection.obj" \
	"$(INTDIR)\vrpn_FileController.obj" \
	"$(INTDIR)\vrpn_ForceDevice.obj" \
	"$(INTDIR)\vrpn_Forwarder.obj" \
	"$(INTDIR)\vrpn_ForwarderController.obj" \
	"$(INTDIR)\vrpn_Ohmmeter.obj" \
	"$(INTDIR)\vrpn_Serial.obj" \
	"$(INTDIR)\vrpn_Shared.obj" \
	"$(INTDIR)\vrpn_Sound.obj" \
	"$(INTDIR)\vrpn_Text.obj" \
	"$(INTDIR)\vrpn_Tracker.obj"

"$(OUTDIR)\vrpn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(CFG)" == "vrpn - Win32 Release" || "$(CFG)" == "vrpn - Win32 Debug"
SOURCE=..\..\vrpn_Analog.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_=\
	"..\..\vrpn_Analog.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	

"$(INTDIR)\vrpn_Analog.obj" : $(SOURCE) $(DEP_CPP_VRPN_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_=\
	"..\..\vrpn_Analog.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Analog.obj" : $(SOURCE) $(DEP_CPP_VRPN_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Button.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_B=\
	"..\..\vrpn_Button.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"gl\gl.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Button.obj" : $(SOURCE) $(DEP_CPP_VRPN_B) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_B=\
	"..\..\vrpn_Button.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Button.obj" : $(SOURCE) $(DEP_CPP_VRPN_B) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Clock.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_C=\
	"..\..\vrpn_Clock.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Clock.obj" : $(SOURCE) $(DEP_CPP_VRPN_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_C=\
	"..\..\vrpn_Clock.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Clock.obj" : $(SOURCE) $(DEP_CPP_VRPN_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Connection.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_CO=\
	"..\..\vrpn_Clock.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_FileConnection.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Connection.obj" : $(SOURCE) $(DEP_CPP_VRPN_CO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_CO=\
	"..\..\vrpn_Clock.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_FileConnection.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Connection.obj" : $(SOURCE) $(DEP_CPP_VRPN_CO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_FileConnection.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_F=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_FileConnection.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_FileConnection.obj" : $(SOURCE) $(DEP_CPP_VRPN_F) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_F=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_FileConnection.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_FileConnection.obj" : $(SOURCE) $(DEP_CPP_VRPN_F) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_FileController.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_FI=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_FileController.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	

"$(INTDIR)\vrpn_FileController.obj" : $(SOURCE) $(DEP_CPP_VRPN_FI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_FI=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_FileController.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_FileController.obj" : $(SOURCE) $(DEP_CPP_VRPN_FI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_ForceDevice.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_FO=\
	"..\..\vrpn_Button.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_ForceDevice.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Tracker.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_ForceDevice.obj" : $(SOURCE) $(DEP_CPP_VRPN_FO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_FO=\
	"..\..\vrpn_Button.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_ForceDevice.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Tracker.h"\
	

"$(INTDIR)\vrpn_ForceDevice.obj" : $(SOURCE) $(DEP_CPP_VRPN_FO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Forwarder.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_FOR=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Forwarder.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	

"$(INTDIR)\vrpn_Forwarder.obj" : $(SOURCE) $(DEP_CPP_VRPN_FOR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_FOR=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Forwarder.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Forwarder.obj" : $(SOURCE) $(DEP_CPP_VRPN_FOR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_ForwarderController.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_FORW=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Forwarder.h"\
	"..\..\vrpn_ForwarderController.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_ForwarderController.obj" : $(SOURCE) $(DEP_CPP_VRPN_FORW)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_FORW=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Forwarder.h"\
	"..\..\vrpn_ForwarderController.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_ForwarderController.obj" : $(SOURCE) $(DEP_CPP_VRPN_FORW)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Ohmmeter.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_O=\
	"..\..\orpx.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Ohmmeter.h"\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Ohmmeter.obj" : $(SOURCE) $(DEP_CPP_VRPN_O) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_O=\
	"..\..\orpx.h"\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Ohmmeter.h"\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Ohmmeter.obj" : $(SOURCE) $(DEP_CPP_VRPN_O) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Serial.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_S=\
	"..\..\vrpn_Serial.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Serial.obj" : $(SOURCE) $(DEP_CPP_VRPN_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_S=\
	"..\..\vrpn_Serial.h"\
	

"$(INTDIR)\vrpn_Serial.obj" : $(SOURCE) $(DEP_CPP_VRPN_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Shared.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_SH=\
	"..\..\vrpn_Shared.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	

"$(INTDIR)\vrpn_Shared.obj" : $(SOURCE) $(DEP_CPP_VRPN_SH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_SH=\
	"..\..\vrpn_Shared.h"\
	

"$(INTDIR)\vrpn_Shared.obj" : $(SOURCE) $(DEP_CPP_VRPN_SH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Sound.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_SO=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Sound.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Sound.obj" : $(SOURCE) $(DEP_CPP_VRPN_SO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_SO=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Sound.h"\
	

"$(INTDIR)\vrpn_Sound.obj" : $(SOURCE) $(DEP_CPP_VRPN_SO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Text.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_T=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Text.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	

"$(INTDIR)\vrpn_Text.obj" : $(SOURCE) $(DEP_CPP_VRPN_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_T=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Text.h"\
	

"$(INTDIR)\vrpn_Text.obj" : $(SOURCE) $(DEP_CPP_VRPN_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vrpn_Tracker.C

!IF  "$(CFG)" == "vrpn - Win32 Release"

DEP_CPP_VRPN_TR=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Serial.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Tracker.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	

"$(INTDIR)\vrpn_Tracker.obj" : $(SOURCE) $(DEP_CPP_VRPN_TR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

DEP_CPP_VRPN_TR=\
	"..\..\vrpn_Connection.h"\
	"..\..\vrpn_Serial.h"\
	"..\..\vrpn_Shared.h"\
	"..\..\vrpn_Tracker.h"\
	

"$(INTDIR)\vrpn_Tracker.obj" : $(SOURCE) $(DEP_CPP_VRPN_TR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

