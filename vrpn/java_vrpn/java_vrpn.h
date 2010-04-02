
#include <jni.h>

// this is the java/jni version that we're expecting
#define JAVA_VRPN_JNI_VERSION JNI_VERSION_1_4


// this is the version of the java_vrpn library
#define JAVA_VRPN_VERSION "1.0"



extern JavaVM* jvm;

// VRPNDevice Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_VRPNDevice( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_VRPNDevice( JavaVM* jvm, void* reserved );

// AnalogOutput_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_AnalogOutput_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_AnalogOutput_Remote( JavaVM* jvm, void* reserved );

// Analog_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_Analog_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_Analog_Remote( JavaVM* jvm, void* reserved );

// AuxiliaryLogger_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_AuxiliaryLogger_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_AuxiliaryLogger_Remote( JavaVM* jvm, void* reserved );

// Button_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_Button_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_Button_Remote( JavaVM* jvm, void* reserved );

// ForceDevice_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_ForceDevice_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_ForceDevice_Remote( JavaVM* jvm, void* reserved );

// FunctionGenerator_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_FunctionGenerator_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_FunctionGenerator_Remote( JavaVM* jvm, void* reserved );

// TempImage_Remote Load/Unload
//JNIEXPORT jint JNICALL JNI_OnLoad_TempImagerRemote( JavaVM* jvm, void* reserved );
//JNIEXPORT void JNICALL JNI_OnUnload_TempImagerRemote( JavaVM* jvm, void* reserved );

// Tracker_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_Tracker_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_Tracker_Remote( JavaVM* jvm, void* reserved );

// Poser_Remote Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_Poser_Remote( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_Poser_Remote( JavaVM* jvm, void* reserved );

// TextReceiver Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_Text_Receiver( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_Text_Receiver( JavaVM* jvm, void* reserved );

// TextSender Load/Unload
JNIEXPORT jint JNICALL JNI_OnLoad_Text_Sender( JavaVM* jvm, void* reserved );
JNIEXPORT void JNICALL JNI_OnUnload_Text_Sender( JavaVM* jvm, void* reserved );

