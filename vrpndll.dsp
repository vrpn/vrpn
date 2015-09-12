# Microsoft Developer Studio Project File - Name="vrpndll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vrpndll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vrpndll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vrpndll.mak" CFG="vrpndll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vrpndll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vrpndll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vrpndll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "pc_win32/DLL/Release"
# PROP Intermediate_Dir "pc_win32/DLL/Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VRPNDLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./atmellib" /I "../quat" /I "../isense" /I "../Dtrack" /I "$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include" /I "$(SYSTEMDRIVE)\sdk\cpp" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VRPNDLL_EXPORTS" /YX /FD /c /Tp
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "vrpndll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "pc_win32/DLL/Debug"
# PROP Intermediate_Dir "pc_win32/DLL/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VRPNDLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "./atmellib" /I "../quat" /I "../isense" /I "../Dtrack" /I "$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include" /I "$(SYSTEMDRIVE)\sdk\cpp" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VRPNDLL_EXPORTS" /FD /GZ /c /Tp
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "vrpndll - Win32 Release"
# Name "vrpndll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\vrpn_3DConnexion.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_3DMicroscribe.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_3Space.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_5DT16.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ADBox.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_5dt.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_Output.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_Radamec_SPI.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_USDigital_A2.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Auxiliary_Logger.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseClass.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button_NI_DIO24.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button_USB.cpp
# End Source File
# Begin Source File

SOURCE=.\vrpn_CerealBox.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Connection.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dial.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_DirectXFFJoystick.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_DirectXRumblePad.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dyna.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Event.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Event_Analog.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Event_Mouse.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileConnection.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileController.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock_Parallel.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDevice.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDeviceServer.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Forwarder.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForwarderController.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_GlobalHapticsOrb.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_HumanInterface.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Imager.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Imager_Stream_Buffer.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_ImmersionBox.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_inertiamouse.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Joylin.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Joywin32.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Keyboard.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_LamportClock.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Magellan.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Mouse.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Mutex.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_NationalInstruments.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_nikon_controls.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Poser.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Poser_Analog.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Poser_Tek4662.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_raw_sgibox.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_RedundantTransmission.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Serial.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Shared.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_SharedObject.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Sound.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Spaceball.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Text.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Streaming_Arduino.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tng3.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_CHProducts_Controller_Raw.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_3DMouse.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_AnalogFly.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_ButtonFly.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Crossbow.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_DTrack.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Fastrak.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_isense.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Isotrak.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Liberty.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_MotionNode.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_NDI_Polaris.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_PhaseSpace.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_UNC_Joystick.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_VPJoystick.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Wanda.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Xkeys.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_YEI_3Space.C
# End Source File
# Begin Source File

SOURCE=.\vrpn_Zaber.C
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\isense\isense.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_3DConnexion.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_3DMicroscribe.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_3Space.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_5DT16.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ADBox.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_5dt.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_Output.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_Radamec_SPI.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Analog_USDigital_A2.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Auxiliary_Logger.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_BaseClass.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button_NI_DIO24.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Button_USB.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_CerealBox.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Configure.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Connection.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dial.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_DirectXFFJoystick.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_DirectXRumblePad.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Dyna.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Event.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Event_Analog.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Event_Mouse.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileConnection.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_FileController.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Flock_Parallel.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForceDevice.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Forwarder.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ForwarderController.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_GlobalHapticsOrb.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_HumanInterface.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Imager.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Imager_Stream_Buffer.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_ImmersionBox.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_inertiamouse.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Joylin.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Joywin32.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Keyboard.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_LamportClock.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Magellan.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Mouse.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Mutex.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_NationalInstruments.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_nikon_controls.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Poser.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Poser_Analog.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Poser_Tek4662.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_raw_sgibox.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_RedundantTransmission.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Serial.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Shared.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_SharedObject.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Sound.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Spaceball.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Text.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Streaming_Arduino.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tng3.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_CHProducts_Controller_Raw.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_3DMouse.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_AnalogFly.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_ButtonFly.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Crossbow.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_DTrack.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Fastrak.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_isense.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Isotrak.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_Liberty.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_MotionNode.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_NDI_Polaris.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Tracker_PhaseSpace.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Types.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_UNC_Joystick.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_VPJoystick.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Wanda.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Xkeys.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_YEI_3Space.h
# End Source File
# Begin Source File

SOURCE=.\vrpn_Zaber.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
