
#include <jni.h>
#include <vrpn_FunctionGenerator.h>

#include "java_vrpn.h"
#include "vrpn_FunctionGeneratorRemote.h"


jclass jclass_vrpn_FunctionGeneratorRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


//////////////////////////
//  DLL functions

// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_FunctionGenerator_Remote( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_FunctionGeneratorRemote native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn FunctionGeneratorRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.FunctionGeneratorRemote
  jclass cls = env->FindClass( "vrpn/FunctionGeneratorRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn FunctionGeneratorRemote native library "
            "while trying to find class vrpn.FunctionGeneratorRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_FunctionGeneratorRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_FunctionGeneratorRemote == NULL )
  {
    printf( "Error loading vrpn FunctionGeneratorRemote native library "
            "while setting up class vrpn.FunctionGeneratorRemote.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad



JNIEXPORT void JNICALL JNI_OnUnload_FunctionGenerator_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn FunctionGeneratorRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.ForceDeviceRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_FunctionGeneratorRemote );
}

// end DLL functions
/////////////////////////////


/////////////////////////////
// java_vrpn utility funcitons

void VRPN_CALLBACK handle_channel_reply( void *userdata,
					  const vrpn_FUNCTION_CHANNEL_REPLY_CB info )
{
  if( jvm == NULL )
  {
    printf( "Error in handle_channel_reply:  uninitialized jvm.\n" );
    return;
  }

  /*
  printf( "fg channel change (C):  time:  %d.%d;\n"
          info.msg_time.tv_sec, info.msg_time.tv_usec );
  */
  
  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  
  //xxx
  jmethodID jmid = env->GetMethodID( jcls, "handleForceChange", "(JJDDD)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_ForceDeviceRemote library was unable to find the "
            "Java method \'handleForceChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jdouble) info.force[0], (jdouble) info.force[1], (jdouble) info.force[2] );

}


void VRPN_CALLBACK handle_start( void *userdata,
					     const vrpn_FUNCTION_START_REPLY_CB info )
{

}


void VRPN_CALLBACK handle_stop( void *userdata,
					     const vrpn_FUNCTION_STOP_REPLY_CB info )
{

}


void VRPN_CALLBACK handle_sample_rate( void *userdata,
					     const vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB info )
{

}


void VRPN_CALLBACK handle_interpreter_description( void *userdata,
					     const vrpn_FUNCTION_INTERPRETER_REPLY_CB info )
{

}


/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    init
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_init( JNIEnv* env, jobject jobj, jstring jname, 
										jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
										jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java FunctionGeneratorRemote
  // object, so that it can be referenced in the functions
  // handle_*_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the function generator
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
  vrpn_FunctionGenerator_Remote* f = new vrpn_FunctionGenerator_Remote( name, conn );
  f->register_channel_reply_handler( jobj, handle_channel_reply );
  f->register_start_reply_handler( jobj, handle_start );
  f->register_stop_reply_handler( jobj, handle_stop );
  f->register_sample_rate_reply_handler( jobj, handle_sample_rate );
  f->register_interpreter_reply_handler( jobj, handle_interpreter_description );
  env->ReleaseStringUTFChars( jname, name );
  env->ReleaseStringUTFChars( jlocalInLogfileName, local_in_logfile_name );
  env->ReleaseStringUTFChars( jlocalOutLogfileName, local_out_logfile_name );
  env->ReleaseStringUTFChars( jremoteInLogfileName, remote_in_logfile_name );
  env->ReleaseStringUTFChars( jremoteOutLogfileName, remote_out_logfile_name );
  
  // now stash 'f' in the jobj's 'native_device' field
  jlong jf = (jlong) f;
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, jf );
  
  return true;
}


/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    shutdownFunctionGenerator
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_FunctionGeneratorRemote_shutdownFunctionGenerator( JNIEnv* env, jobject jobj )
{
	// get the function generator pointer
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

	// unregister a handler and destroy the force device
	if( f > 0 )
	{
		f->unregister_channel_reply_handler( jobj, handle_channel_reply );
		f->unregister_start_reply_handler( jobj, handle_start );
		f->unregister_stop_reply_handler( jobj, handle_stop );
		f->unregister_sample_rate_reply_handler( jobj, handle_sample_rate );
		f->unregister_interpreter_reply_handler( jobj, handle_interpreter_description );
		f->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
		delete f;
	}

	// set the force device pointer to -1
	env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

	// delete global reference to object (that was created in init)
	env->DeleteGlobalRef( jobj );
}


/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_FunctionGeneratorRemote_mainloop( JNIEnv* env, jobject jobj )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f <= 0 )  // this force device is uninitialized or has been shut down already
		return;

	// now call mainloop
	f->mainloop( );
}

// end java_vrpn utility functions
////////////////////////////////////


////////////////////////////////////
// native functions from java class ForceDeviceRemote



/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    setChannel_native
 * Signature: (ILvrpn/FunctionGeneratorRemote$Channel;)I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_setChannel_1native
  (JNIEnv *, jobject, jint, jobject);

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    requestChannel_native
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_requestChannel_1native
  (JNIEnv *, jobject, jint);

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    requestAllChannels_native
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_requestAllChannels_1native
  (JNIEnv *, jobject);

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    requestStart_native
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_requestStart_1native
  (JNIEnv *, jobject);

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    requestStop_native
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_requestStop_1native
  (JNIEnv *, jobject);

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    requestSampleRate_native
 * Signature: (F)I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_requestSampleRate_1native
  (JNIEnv *, jobject, jfloat);

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    requestInterpreterDescription_native
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_vrpn_FunctionGeneratorRemote_requestInterpreterDescription_1native
  (JNIEnv *, jobject);

// end native functions for java class ForceDeviceRemote
///////////////////////////////////////////


