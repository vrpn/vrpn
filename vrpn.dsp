# Microsoft Developer Studio Project File - Name="vrpn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
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
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I "ghostlib" /I "../quat" /I "server_src/quat" /I "../server_src/quat" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c /Tp
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
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "D:\Program Files\PHANToM\Ghost\lib" /I "C:\Program Files\PHANToM\Ghost\lib" /I "..\quat" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c /Tp
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
# Begin Source File

SOURCE=.\plane.C
# End Source File
# Begin Source File

SOURCE=.\plane.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button.h
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

SOURCE=.\vrpn_ForceDevice.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDevice.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Shared.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Shared.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Sound.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker.h
# End Source File
# End Target
# End Project
