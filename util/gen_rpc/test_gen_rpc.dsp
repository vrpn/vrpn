# Microsoft Developer Studio Project File - Name="test_gen_rpc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=test_gen_rpc - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "test_gen_rpc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test_gen_rpc.mak" CFG="test_gen_rpc - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "test_gen_rpc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "test_gen_rpc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "test_gen_rpc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../pc_win32/util/test_gen_rpc/Debug"
# PROP Intermediate_Dir "../../pc_win32/util/test_gen_rpc/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../.." /D "_CONSOLE" /D "_DEBUG" /D "_MBCS" /D "WIN32" /FR /YX /FD /GZ /TP /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"../pc_win32/Debug" /libpath:"../pc_win32/DLL/Debug"

!ELSEIF  "$(CFG)" == "test_gen_rpc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../pc_win32/util/test_gen_rpc/Release"
# PROP Intermediate_Dir "../../pc_win32/util/test_gen_rpc/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../.." /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "$(VRPN_COMPILE_DEF)" /YX /FD /GZ /TP /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../.." /D "_CONSOLE" /D "NDEBUG" /D "_MBCS" /D "WIN32" /YX /FD /TP /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"../pc_win32/Release" /libpath:"../pc_win32/DLL/Release"

!ENDIF 

# Begin Target

# Name "test_gen_rpc - Win32 Debug"
# Name "test_gen_rpc - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\main_test.C
# End Source File
# Begin Source File

SOURCE=.\rpc_Test.C
# End Source File
# Begin Source File

SOURCE=.\rpc_Test_Remote.C
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Source File

SOURCE=.\rpc_Test.vrpndef

!IF  "$(CFG)" == "test_gen_rpc - Win32 Debug"

# Begin Custom Build
InputPath=.\rpc_Test.vrpndef

BuildCmds= \
	E:\cygwin\bin\perl.exe gen_vrpn_rpc.pl -h rpc_Test.vrpndef \
	E:\cygwin\bin\perl.exe gen_vrpn_rpc.pl -c rpc_Test.vrpndef \
	

"rpc_Test.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"rpc_Test.C" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "test_gen_rpc - Win32 Release"

# Begin Custom Build
InputPath=.\rpc_Test.vrpndef

BuildCmds= \
	C:\cygwin\bin\perl.exe gen_vrpn_rpc.pl -h rpc_Test.vrpndef \
	C:\cygwin\bin\perl.exe gen_vrpn_rpc.pl -c rpc_Test.vrpndef \
	

"rpc_Test.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"rpc_Test.C" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rpc_Test_Remote.Cdef

!IF  "$(CFG)" == "test_gen_rpc - Win32 Debug"

# Begin Custom Build
InputPath=.\rpc_Test_Remote.Cdef

"rpc_Test_Remote.C" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	E:\cygwin\bin\perl.exe gen_vrpn_rpc.pl rpc_Test_Remote.Cdef

# End Custom Build

!ELSEIF  "$(CFG)" == "test_gen_rpc - Win32 Release"

# Begin Custom Build
InputPath=.\rpc_Test_Remote.Cdef

"rpc_Test_Remote.C" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	C:\cygwin\bin\perl.exe gen_vrpn_rpc.pl rpc_Test_Remote.Cdef

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rpc_Test_Remote.hdef

!IF  "$(CFG)" == "test_gen_rpc - Win32 Debug"

# Begin Custom Build
InputPath=.\rpc_Test_Remote.hdef

"rpc_Test_Remote.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	E:\cygwin\bin\perl.exe gen_vrpn_rpc.pl rpc_Test_Remote.hdef

# End Custom Build

!ELSEIF  "$(CFG)" == "test_gen_rpc - Win32 Release"

# Begin Custom Build
InputPath=.\rpc_Test_Remote.hdef

"rpc_Test_Remote.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	C:\cygwin\bin\perl.exe gen_vrpn_rpc.pl rpc_Test_Remote.hdef

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
