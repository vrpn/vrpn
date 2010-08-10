# Microsoft Developer Studio Project File - Name="directXFF_server" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=directXFF_server - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "directXFF_server.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "directXFF_server.mak" CFG="directXFF_server - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "directXFF_server - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "directXFF_server - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "directXFF_server - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../pc_win32/server_src/directXFF_server/Release"
# PROP Intermediate_Dir "../pc_win32/server_src/directXFF_server/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../quat" /I "../../external/pc_win32/include/ghost3.1" /I "../../external/pc_win32/include/stl" /I "../" /D "_CONSOLE" /D "NDEBUG" /D "_MBCS" /D "WIN32" /YX /FD /c /Tp
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:console /machine:I386 /libpath:"../../external/pc_win32/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "directXFF_server - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../pc_win32/server_src/directXFF_server/Debug"
# PROP Intermediate_Dir "../pc_win32/server_src/directXFF_server/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\quat" /I ".." /I "../../external/pc_win32/include/ghost-stl" /I "../../external/pc_win32/include/ghost3.1" /D "VRPN_USE_DIRECTINPUT" /D "_CONSOLE" /D "_DEBUG" /D "_MBCS" /D "WIN32" /Fr /YX /FD /c /Tp
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /pdb:none /map /debug /machine:I386 /nodefaultlib:"libcmtd.lib"

!ENDIF 

# Begin Target

# Name "directXFF_server - Win32 Release"
# Name "directXFF_server - Win32 Debug"
# Begin Group "External Libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\external\pc_win32\lib\GHOST31.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\directXFF.C
# End Source File
# End Target
# End Project
