# Microsoft Developer Studio Project File - Name="java_vrpn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=java_vrpn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "java_vrpn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "java_vrpn.mak" CFG="java_vrpn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "java_vrpn - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "java_vrpn - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "java_vrpn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "bin/Release"
# PROP Intermediate_Dir "bin/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JAVA_VRPN_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\Program Files\Java\jdk1.6.0_01\include" /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JAVA_VRPN_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib vrpn.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /libpath:"..\pc_win32\Release\\"

!ELSEIF  "$(CFG)" == "java_vrpn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "bin/Debug"
# PROP Intermediate_Dir "bin/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JAVA_VRPN_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\Program Files\Java\jdk1.6.0_01\include" /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JAVA_VRPN_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib vrpn.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\pc_win32\Debug\\"

!ENDIF 

# Begin Target

# Name "java_vrpn - Win32 Release"
# Name "java_vrpn - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\java_vrpn.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_AnalogOutputRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_AnalogRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_AuxiliaryLoggerRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_ButtonRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDeviceRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_PoserRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_TextReceiver.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_TextSender.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_TrackerRemote.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_VRPNDevice.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\java_vrpn.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_AnalogOutputRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_AnalogRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_AuxiliaryLoggerRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ButtonRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDeviceRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_PoserRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_TextReceiver.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_TextSender.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_TrackerRemote.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_VRPNDevice.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
