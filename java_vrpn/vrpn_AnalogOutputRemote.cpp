

#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Analog_Output.h"
#include "vrpn_AnalogOutputRemote.h"


jclass jclass_vrpn_AnalogOutputRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;

//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_AnalogOutput_Remote( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_AnalogOutputRemote native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn AnalogOutputRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.AnalogOutputRemote
  jclass cls = env->FindClass( "vrpn/AnalogOutputRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn AnalogOutputRemote native library "
            "while trying to find class vrpn.AnalogOutputRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_AnalogOutputRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_AnalogOutputRemote == NULL )
  {
    printf( "Error loading vrpn AnalogOutputRemote native library "
            "while setting up class vrpn.AnalogOutputRemote.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_AnalogOutput_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn AnalogOutputRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.analogRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_AnalogOutputRemote );
}



// end DLL functions
/////////////////////////////


////////////////////////////
// dll utility functions



JNIEXPORT jboolean JNICALL 
Java_vrpn_AnalogOutputRemote_requestValueChange_1native__ID( JNIEnv* env, jobject jobj, 
															 jint jchannel, jdouble jvalue )
{
  if( jchannel < 0 ) 
  {
    return false;
  }

  // get the analog pointer
  vrpn_Analog_Output_Remote* ao = (vrpn_Analog_Output_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( ao == NULL )  // this analog is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestValueChange(double)\":  the analog output is "
            "uninitialized or has been shut down.\n" );
    return false;
  }

  return ao->request_change_channel_value( jchannel, jvalue );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AnalogOutputRemote_requestValueChange_1native___3D( JNIEnv* env, jobject jobj, 
															  jdoubleArray jvalues )
{
  // get the analog pointer
  vrpn_Analog_Output_Remote* ao = (vrpn_Analog_Output_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( ao == NULL )  // this analog is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestValueChange(double[])\":  the analog output is "
            "uninitialized or has been shut down.\n" );
    return false;
  }

  // check the array length
  int length = env->GetArrayLength( jvalues );
  if( length > ao->getNumChannels( ) )
  {
	printf( "Error in native method \"requestValueChange(double[])\":  someone tried "
			"to use an array that was too long.\n" );
	return false;
  }
  if( length == 0 )
  {
	return true;
  }

  // get the array
  double* values = env->GetDoubleArrayElements( jvalues, NULL );
  if( values == NULL )
  {
	printf( "Error in native method \"requestValueChange(double[])\":  couldn't "
			"get the array in native form.\n" );
	env->ReleaseDoubleArrayElements( jvalues, values, JNI_ABORT /*mode*/ );
	return false;
  }

  bool retval = ao->request_change_channels( length, values );
  env->ReleaseDoubleArrayElements( jvalues, values, JNI_ABORT /*mode*/ );
  return retval;
}


JNIEXPORT jint JNICALL 
Java_vrpn_AnalogOutputRemote_getNumActiveChannels( JNIEnv* env, jobject jobj )
{
  // get the analog pointer
  vrpn_Analog_Output_Remote* ao = (vrpn_Analog_Output_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( ao == NULL )  // this analog is uninitialized or has been shut down already
  {
    printf( "Error in native method \"getNumActiveChannels\":  the analog output is "
            "uninitialized or has been shut down.\n" );
    return 0;
  }

  return ao->getNumChannels( );
}


JNIEXPORT void JNICALL 
Java_vrpn_AnalogOutputRemote_shutdownAnalogOutput( JNIEnv* env, jobject jobj )
{
  // get the analog pointers
  vrpn_Analog_Output_Remote* ao 
	  = (vrpn_Analog_Output_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

  if( ao )
  {
	ao->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
	delete ao;
  }
   
  // set the analog pointers to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );
}


JNIEXPORT void JNICALL 
Java_vrpn_AnalogOutputRemote_mainloop( JNIEnv *env, jobject jobj )
{

  vrpn_Analog_Output_Remote* ao 
	  = (vrpn_Analog_Output_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

  if( ao )
	ao->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AnalogOutputRemote_init( JNIEnv *env, jobject jobj, jstring jname,
								  jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
								  jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java AnalogOutputRemote
  // object, so that it can be referenced in the function
  // handle_analog_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the analog & output
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
  vrpn_Analog_Output_Remote* ao = new vrpn_Analog_Output_Remote( name, conn );
  env->ReleaseStringUTFChars( jname, name );
  env->ReleaseStringUTFChars( jlocalInLogfileName, local_in_logfile_name );
  env->ReleaseStringUTFChars( jlocalOutLogfileName, local_out_logfile_name );
  env->ReleaseStringUTFChars( jremoteInLogfileName, remote_in_logfile_name );
  env->ReleaseStringUTFChars( jremoteOutLogfileName, remote_out_logfile_name );
  

  // now stash 'ao' in the jobj's 'native_device' field
  jlong jao = (jlong) ao;
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, jao );
  
  return true;
}

