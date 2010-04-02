
#include <jni.h>
#include "java_vrpn.h"


JavaVM* jvm = NULL;


JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* jvm, void* reserved )
{
  ::jvm = jvm;
  
  if( JNI_OnLoad_VRPNDevice( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_AnalogOutput_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_Analog_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_AuxiliaryLogger_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_Button_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_ForceDevice_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_FunctionGenerator_Remote( jvm, reserved) == JNI_ERR ) return JNI_ERR;
  //if( JNI_OnLoad_TempImagerRemote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_Tracker_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_Poser_Remote( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_Text_Receiver( jvm, reserved ) == JNI_ERR ) return JNI_ERR;
  if( JNI_OnLoad_Text_Sender( jvm, reserved ) == JNI_ERR ) return JNI_ERR;

  return JAVA_VRPN_JNI_VERSION;
}



JNIEXPORT void JNICALL JNI_OnUnload( JavaVM* jvm, void* reserved )
{
  JNI_OnUnload_AnalogOutput_Remote( jvm, reserved );
  JNI_OnUnload_Analog_Remote( jvm, reserved );
  JNI_OnUnload_AuxiliaryLogger_Remote( jvm, reserved );
  JNI_OnUnload_Button_Remote( jvm, reserved );
  JNI_OnUnload_ForceDevice_Remote( jvm, reserved );
  JNI_OnUnload_FunctionGenerator_Remote( jvm, reserved );
  //JNI_OnUnload_TempImagerRemote( jvm, reserved );
  JNI_OnUnload_Tracker_Remote( jvm, reserved );
  JNI_OnUnload_Poser_Remote( jvm, reserved );
  JNI_OnUnload_Text_Receiver( jvm, reserved );
  JNI_OnUnload_Text_Sender( jvm, reserved );
  JNI_OnUnload_VRPNDevice( jvm, reserved );
}