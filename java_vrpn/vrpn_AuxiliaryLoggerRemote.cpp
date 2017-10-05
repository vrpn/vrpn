
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Auxiliary_Logger.h"
#include "vrpn_AuxiliaryLoggerRemote.h"

jclass jclass_vrpn_AuxiliaryLoggerRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_AuxiliaryLogger_Remote( JavaVM* jvm, void* reserved )
{
	///////////////
	// make sure the general library code set jvm
	if( jvm == NULL )
	{
		printf( "vrpn_AuxiliaryLoggerRemote native:  no jvm.\n" );
		return JNI_ERR;
	}

	///////////////
	// get the JNIEnv pointer
	JNIEnv* env;
	if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
	{
		printf( "Error loading vrpn AuxiliaryLoggerRemote native library.\n" );
		return JNI_ERR;
	}

	///////////////
	// get the jclass reference to vrpn.AuxiliaryLoggerRemote
	jclass cls = env->FindClass( "vrpn/AuxiliaryLoggerRemote" );
	if( cls == NULL )
	{
		printf( "Error loading vrpn AuxiliaryLoggerRemote native library "
			"while trying to find class vrpn.AuxiliaryLoggerRemote.\n" );
		return JNI_ERR;
	}

	// make a global reference so the class can be referenced later.
	// this must be a _WEAK_ global reference, so the class will be
	// garbage collected, and so the JNI_OnUnLoad handler will be called
	jclass_vrpn_AuxiliaryLoggerRemote = (jclass) env->NewWeakGlobalRef( cls );
	if( jclass_vrpn_AuxiliaryLoggerRemote == NULL )
	{
		printf( "Error loading vrpn AuxiliaryLoggerRemote native library "
			"while setting up class vrpn.AuxiliaryLoggerRemote.\n" );
		return JNI_ERR;
	}

	return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_AuxiliaryLogger_Remote( JavaVM* jvm, void* reserved )
{
	// get the JNIEnv pointer
	JNIEnv* env;
	if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
	{
		printf( "Error unloading vrpn AuxiliaryLoggerRemote native library.\n" );
		return;
	}

	// delete the global reference to the vrpn.AuxiliaryLoggerRemote class
	env->DeleteWeakGlobalRef( jclass_vrpn_AuxiliaryLoggerRemote );
}


void VRPN_CALLBACK 
handle_logging_report( void* userdata, const vrpn_AUXLOGGERCB info )
{
  if( jvm == NULL )
    return;

  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleLoggingReport", "(JJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vropn_AuxiliaryLogger library was unable to find the "
            "Java method \'handleLoggingReport\'.  This may indicate a version mismatch.\n" );
    return;
  }

  // get a string to hold the message
  jstring jLocalIn = env->NewStringUTF( info.local_in_logfile_name );
  jstring jLocalOut = env->NewStringUTF( info.local_out_logfile_name );
  jstring jRemoteIn = env->NewStringUTF( info.remote_in_logfile_name );
  jstring jRemoteOut = env->NewStringUTF( info.remote_out_logfile_name );

  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       jLocalIn, jLocalOut, jRemoteIn, jRemoteOut );

}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AuxiliaryLoggerRemote_sendLoggingRequest( JNIEnv* env, jobject jobj, 
												   jstring jLocalIn, jstring jLocalOut, 
												   jstring jRemoteIn, jstring jRemoteOut )
{
	// get the logger pointer
	vrpn_Auxiliary_Logger_Remote* r = (vrpn_Auxiliary_Logger_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( r == NULL )  // this logger is uninitialized or has been shut down already
	{
		printf( "Error in native method \"sendLoggingRequest(...)\":  the logger is "
			"uninitialized or has been shut down.\n" );
		return false;
	}
	const char* localIn = jLocalIn == NULL ? NULL : env->GetStringUTFChars( jLocalIn, NULL );
	const char* localOut = jLocalOut == NULL ? NULL : env->GetStringUTFChars( jLocalOut, NULL );
	const char* remoteIn = jRemoteIn == NULL ? NULL : env->GetStringUTFChars( jRemoteIn, NULL );
	const char* remoteOut = jRemoteOut == NULL ? NULL : env->GetStringUTFChars( jRemoteOut, NULL );

	bool retval =  r->send_logging_request( localIn, localOut, remoteIn, remoteOut );

	env->ReleaseStringUTFChars( jLocalIn, localIn );
	env->ReleaseStringUTFChars( jLocalOut, localOut );
	env->ReleaseStringUTFChars( jRemoteIn, remoteIn );
	env->ReleaseStringUTFChars( jRemoteOut, remoteOut );
	return retval;
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AuxiliaryLoggerRemote_sendLoggingStatusRequest( JNIEnv* env, jobject jobj )
{
	// get the analog pointer
	vrpn_Auxiliary_Logger_Remote* r = (vrpn_Auxiliary_Logger_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( r == NULL )  // this analog is uninitialized or has been shut down already
	{
		printf( "Error in native method \"sendLoggingStatusRequest(...)\":  the  logger is "
			"uninitialized or has been shut down.\n" );
		return false;
	}

	bool retval =  r->send_logging_status_request( );

	return retval;
}


JNIEXPORT void JNICALL 
Java_vrpn_AuxiliaryLoggerRemote_mainloop( JNIEnv* env, jobject jobj )
{
	vrpn_Auxiliary_Logger_Remote* l = (vrpn_Auxiliary_Logger_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( l == NULL )  // this logger is uninitialized or has been shut down already
		return;

	// now call mainloop
	l->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AuxiliaryLoggerRemote_init( JNIEnv* env, jobject jobj,jstring jname, 
									 jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
									 jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
	// make a global reference to the Java AuxiliaryLoggerRemote
	// object, so that it can be referenced in the function
	// handle_logging_report(...)
	jobj = env->NewGlobalRef( jobj );

	// create the logger
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
	vrpn_Auxiliary_Logger_Remote* r = new vrpn_Auxiliary_Logger_Remote( name, conn );
	r->register_report_handler( jobj, handle_logging_report );
	env->ReleaseStringUTFChars( jname, name );
	env->ReleaseStringUTFChars( jlocalInLogfileName, local_in_logfile_name );
	env->ReleaseStringUTFChars( jlocalOutLogfileName, local_out_logfile_name );
	env->ReleaseStringUTFChars( jremoteInLogfileName, remote_in_logfile_name );
	env->ReleaseStringUTFChars( jremoteOutLogfileName, remote_out_logfile_name );

	// now stash 'r' in the jobj's 'native_device' field
	jlong jr = (jlong) r;
	env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, jr );

	return true;
}


JNIEXPORT void JNICALL 
Java_vrpn_AuxiliaryLoggerRemote_shutdownAuxiliaryLogger( JNIEnv* env, jobject jobj )
{
	// get the logger pointer
	vrpn_Auxiliary_Logger_Remote* r = (vrpn_Auxiliary_Logger_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

	// unregister a handler and destroy the button
	if( r )
	{
		r->unregister_report_handler( jobj, handle_logging_report );
		r->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
		delete r;
	}

	// set the logger pointer to -1
	env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

	// delete global reference to object (that was created in init)
	env->DeleteGlobalRef( jobj );
}


