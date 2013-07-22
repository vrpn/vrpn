# Microsoft Developer Studio Project File - Name="vrpn_server" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=vrpn_server - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vrpn_server.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vrpn_server.mak" CFG="vrpn_server - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vrpn_server - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "vrpn_server - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vrpn_server - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../pc_win32/server_src/vrpn_server/Release"
# PROP Intermediate_Dir "../pc_win32/server_src/vrpn_server/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../Adrienne/AEC_DLL" /I "$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include" /I "$(SYSTEMDRIVE)/Program Files/SensAble/3DTouch/include" /I "$(SYSTEMDRIVE)/Program Files/SensAble/3DTouch/utilities/include" /I "../../isense" /I ".." /I "../../quat" /I "../../Dtrack" /D "_CONSOLE" /D "NDEBUG" /D "_MBCS" /D "WIN32" /FR /YX /FD /c /Tp
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /libpath:"../pc_win32/Release" /libpath:"../pc_win32/DLL/Release"

!ELSEIF  "$(CFG)" == "vrpn_server - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../pc_win32/server_src/vrpn_server/Debug"
# PROP Intermediate_Dir "../pc_win32/server_src/vrpn_server/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../Adrienne/AEC_DLL" /I "$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include" /I "$(SYSTEMDRIVE)/Program Files/SensAble/3DTouch/include" /I "$(SYSTEMDRIVE)/Program Files/SensAble/3DTouch/utilities/include" /I "../../isense" /I ".." /I "../../quat" /I "../../Dtrack" /D "_CONSOLE" /D "_DEBUG" /D "_MBCS" /D "WIN32" /FR /YX /FD /TP /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"../pc_win32/Debug" /libpath:"../pc_win32/DLL/Debug"
# SUBTRACT LINK32 /verbose

!ENDIF 

# Begin Target

# Name "vrpn_server - Win32 Release"
# Name "vrpn_server - Win32 Debug"
# Begin Source File

SOURCE=vrpn.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Generic_server_object.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Generic_server_object.h
# End Source File
# End Target
# End Project
