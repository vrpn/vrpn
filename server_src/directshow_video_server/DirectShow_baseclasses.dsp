# Microsoft Developer Studio Project File - Name="DirectShow_baseclasses" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=DirectShow_baseclasses - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DirectShow_baseclasses.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DirectShow_baseclasses.mak" CFG="DirectShow_baseclasses - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DirectShow_baseclasses - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "DirectShow_baseclasses - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DirectShow_baseclasses - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\pc_win32\server_src\directshow_video_server\Release"
# PROP Intermediate_Dir "..\..\pc_win32\server_src\directshow_video_server\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses" /I "$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\include" /I "$(SYSTEMDRIVE)\Program Files\Microsoft DirectX SDK (August 2006)\Include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D COINIT_DISABLE_OLE1DDE=0x4 /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "DirectShow_baseclasses - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DirectShow_baseclasses___Win32_Debug"
# PROP BASE Intermediate_Dir "DirectShow_baseclasses___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\pc_win32\server_src\directshow_video_server\Debug"
# PROP Intermediate_Dir "..\..\pc_win32\server_src\directshow_video_server\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses" /I "$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\include" /I "$(SYSTEMDRIVE)\Program Files\Microsoft DirectX SDK (August 2006)\Include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D COINIT_DISABLE_OLE1DDE=0x4 /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "DirectShow_baseclasses - Win32 Release"
# Name "DirectShow_baseclasses - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\amextra.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\amfilter.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\amvideo.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\combase.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\cprop.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\ctlutil.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\ddmm.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\dllentry.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\dllsetup.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\mtype.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\outputq.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\pstream.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\pullpin.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\refclock.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\renbase.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\schedule.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\seekpt.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\source.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\strmctl.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\sysclock.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\transfrm.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\transip.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\videoctl.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\vtrans.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\winctrl.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\winutil.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\wxdebug.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\wxlist.cpp"
# End Source File
# Begin Source File

SOURCE="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Samples\Multimedia\DirectShow\BaseClasses\wxutil.cpp"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
