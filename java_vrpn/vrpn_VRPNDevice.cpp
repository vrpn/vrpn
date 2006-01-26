
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_VRPNDevice.h"
#include "vrpn_Connection.h"
#include "vrpn_FileConnection.h"

jclass jclass_vrpn_VRPNDevice = NULL;
jfieldID jfid_vrpn_VRPNDevice_native_device = NULL;


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_VRPNDevice( JavaVM* jvm, void* reserved )
{

  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_VRPNDevice native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn_VRPNDevice native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.VRPNDevice
  jclass cls = env->FindClass( "vrpn/VRPNDevice" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn_VRPNDevice native library "
            "while trying to find class vrpn.VRPNDevice.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_VRPNDevice = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_VRPNDevice == NULL )
  {
    printf( "Error loading vrpn VRPNDevice native library "
            "while setting up class vrpn.VRPNDevice.\n" );
    return JNI_ERR;
  }

  ////////////////
  // get a jfid field id reference to the "native_device" 
  // field of class vrpn.VRPNDevice.
  // field ids do not have to be made into global references.
  jfid_vrpn_VRPNDevice_native_device
    = env->GetFieldID( jclass_vrpn_VRPNDevice, "native_device", "I" );
  if( jfid_vrpn_VRPNDevice_native_device == NULL )
  {
    printf( "Error loading vrpn native library "
            "while looking into class vrpn.VRPNDevice.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;  
}


JNIEXPORT void JNICALL JNI_OnUnload_VRPNDevice( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn VRPNDevice native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.analogRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_VRPNDevice );
}