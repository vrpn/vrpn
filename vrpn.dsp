# Microsoft Developer Studio Project File - Name="vrpn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vrpn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vrpn.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vrpn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "PC_WIN32/Release"
# PROP Intermediate_Dir "PC_WIN32/Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "ghostlib" /I "../quat" /I "server_src/quat" /I "../server_src/quat" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c /Tp
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vrpn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "PC_WIN32/DEBUG"
# PROP Intermediate_Dir "PC_WIN32/Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "../quat" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /TP /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "vrpn - Win32 Release"
# Name "vrpn - Win32 Debug"
# Begin Group "new connection stuff"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\vrpn_BaseConnection.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseConnection.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseConnectionController.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseConnectionController.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseMulticast.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseMulticast.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ClientConnectionController.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ConnectionController.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ConnectionController.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ConnectionOldCommonStuff.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ConnectionOldCommonStuff.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_NetConnection.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_NetConnection.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ServerConnectionController.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_ServerConnectionController.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_UnreliableMulticastRecvr.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_UnreliableMulticastRecvr.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_UnreliableMulticastSender.C
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vrpn_UnreliableMulticastSender.h
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\orpx.C
# End Source File
# Begin Source File

SOURCE=.\orpx.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_3Space.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_3Space.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_CerealBox.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_CerealBox.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Clock.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Clock.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Connection.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Connection.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dial.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dial.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dyna.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dyna.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileConnection.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileConnection.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileController.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileController.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock_Parallel.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock_Parallel.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDevice.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDevice.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Forwarder.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Forwarder.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForwarderController.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForwarderController.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Ohmmeter.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Ohmmeter.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_raw_sgibox.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_raw_sgibox.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Serial.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Serial.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Shared.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Shared.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Sound.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Sound.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Text.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Text.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_AnalogFly.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_AnalogFly.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Fastrak.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Fastrak.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Types.h
# End Source File
# End Target
# End Project
