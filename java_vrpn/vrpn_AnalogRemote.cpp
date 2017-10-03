

#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Analog.h"
#include "vrpn_Analog_Output.h"
#include "vrpn_AnalogRemote.h"


jclass jclass_vrpn_AnalogRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_Analog_Remote( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_TrackerRemote native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn AnalogRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.AnalogRemote
  jclass cls = env->FindClass( "vrpn/AnalogRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn AnalogRemote native library "
            "while trying to find class vrpn.AnalogRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_AnalogRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_AnalogRemote == NULL )
  {
    printf( "Error loading vrpn AnalogRemote native library "
            "while setting up class vrpn.AnalogRemote.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_Analog_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn AnalogRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.analogRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_AnalogRemote );
}


// This is the callback for vrpn to notify us of a new analog message
void VRPN_CALLBACK handle_analog_change( void* userdata, const vrpn_ANALOGCB info )
{
  if( jvm == NULL )
    return;

  /*
  printf( "analog change (C):  time:  %d.%d;  num_channels:  %d;\n\t", 
          info.msg_time.tv_sec, info.msg_time.tv_usec, info.num_channel );
  for( int i = 0; i <= info.num_channel - 1; i++ )
    printf( "%d  ", info.channel[i] );
  printf( "\n" );
  */

  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleAnalogChange", "(JJ[D)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_AnalogRemote library was unable to find the "
            "Java method \'handleAnalogChange\'.  This may indicate a version mismatch.\n" );
    return;
  }

  // create an array of the appropriate length
  jdoubleArray jchannels = env->NewDoubleArray( info.num_channel );
  jchannels = (jdoubleArray) env->NewWeakGlobalRef( jchannels );
  if( jchannels == NULL )
  {
    printf( "Error:  vrpn AnalogRemote native library was unable to create a \'channels\' array.\n" );
    return;
  }

  env->SetDoubleArrayRegion( jchannels, 0, info.num_channel, (double*) info.channel );


  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jdoubleArray) jchannels /* something here about the channels array */ );

}


JNIEXPORT void JNICALL 
Java_vrpn_AnalogRemote_shutdownAnalog( JNIEnv* env, jobject jobj )
{
  // get the analog pointers
  vrpn_Analog_Remote* a = (vrpn_Analog_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

  // unregister a handler and destroy the analogs
  if( a )
  {
    a->unregister_change_handler( jobj, handle_analog_change );
	a->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
    delete a;
  }
   
  // set the analog pointers to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );

}


JNIEXPORT void JNICALL 
Java_vrpn_AnalogRemote_mainloop( JNIEnv *env, jobject jobj )
{

  vrpn_Analog_Remote* a = (vrpn_Analog_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

  if( a )  // this analog is still alive
    a->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AnalogRemote_init( JNIEnv *env, jobject jobj, jstring jname, 
							 jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
							 jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java AnalogRemote
  // object, so that it can be referenced in the function
  // handle_analog_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the analog
  const char* name = env->GetStringUTFChars( jname, NULL );
  const char* local_in_logfile_name = jlocalInLogfileName == NULL ? NULL :
	  env->GetStringUTFChars( jlocalInLogfileName, NULL );
  const char* local_out_logfile_name = jlocalOutLogfileName == NULL ? NULL :
	  env->GetStringUTFChars( jlocalOutLogfileName, NULL );
  const char* remote_in_logfile_name = jremoteInLogfileName == NULL ? NULL :
	  env->GetStringUTFChars( jremoteInLogfileName, NULL );
  const char* remote_out_logfile_name = jremoteOutLogfileName == NULL ? NULL :
	  env->GetStringUTFChars( jremoteOutLogfileName, NULL );
  vrpn_Connection* conn 
	  = vrpn_get_connection_by_name( name, local_in_logfile_name, local_out_logfile_name,
									 remote_in_logfile_name, remote_out_logfile_name );
  vrpn_Analog_Remote* a = new vrpn_Analog_Remote( name, conn );
  a->register_change_handler( jobj, handle_analog_change );
  env->ReleaseStringUTFChars( jname, name );
  env->ReleaseStringUTFChars( jlocalInLogfileName, local_in_logfile_name );
  env->ReleaseStringUTFChars( jlocalOutLogfileName, local_out_logfile_name );
  env->ReleaseStringUTFChars( jremoteInLogfileName, remote_in_logfile_name );
  env->ReleaseStringUTFChars( jremoteOutLogfileName, remote_out_logfile_name );
 
  // now stash 'a' in the jobj's 'native_device' field
  jlong ja = (jlong) a;
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, ja );
  
  return true;
}


JNIEXPORT jint JNICALL Java_vrpn_AnalogRemote_getNumActiveChannels( JNIEnv* env, jobject jobj )
{
  // get the analog pointer
  vrpn_Analog_Remote* a = (vrpn_Analog_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( a == NULL )  // this analog is uninitialized or has been shut down already
  {
    printf( "Error in native method \"getNumActiveChannels\":  the analog is "
            "uninitialized or has been shut down.\n" );
    return 0;
  }

  return a->getNumChannels( );
}


