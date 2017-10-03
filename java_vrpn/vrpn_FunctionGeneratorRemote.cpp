
#include <typeinfo>
#include <iostream>
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

	//determine what type of channel change this is
	const vrpn_FunctionGenerator_function* fxn = info.channel->getFunction();
	jmethodID jmid = NULL;
	if( typeid( *fxn ) == typeid( vrpn_FunctionGenerator_function_NULL ) )
	{
		jmid = env->GetMethodID( jcls, "handleChannelChange_NULL", "(JJI)V" );
		if( jmid == NULL )
		{
			printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
					"Java method \'handleChannelChange_NULL\'.  This may indicate a version mismatch.\n" );
			return;
		}
		env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
							 (jint) info.channelNum );
	}
	else if( typeid( *fxn ) == typeid( vrpn_FunctionGenerator_function_script ) )
	{
		jmid = env->GetMethodID( jcls, "handleChannelChange_Script", "(JJILjava/lang/String;)V" );
		if( jmid == NULL )
		{
			printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
					"Java method \'handleChannelChange_Script\'.  This may indicate a version mismatch.\n" );
			return;
		}
		jstring jscript 
			= env->NewStringUTF( (dynamic_cast<const vrpn_FunctionGenerator_function_script*>(fxn))->getScript() );
		if( jscript == NULL ) // out of memory
		{
			printf( "Error:  vrpn_FunctionGeneratorRemote library (handle_channel_reply) "
				"was unable to create the script string.\n" );
			return;
		}
		env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
							 (jint) info.channelNum, jscript );
	}
	else 
	{
		printf( "Error:  vrpn_FunctionGeneratorRemote library (handle_channel_reply):  unknown function type\n" );
		std::cout << "handle_channel_reply:  function type " << typeid( *fxn ).name() << std::endl;
		return;
	}
}


void VRPN_CALLBACK handle_start( void *userdata,
								const vrpn_FUNCTION_START_REPLY_CB info )
{
	if( jvm == NULL )
	{
		printf( "Error in handle_start:  uninitialized jvm.\n" );
		return;
	}
	/*
	printf( "fg start (C):  time:  %d.%d;\n"
	info.msg_time.tv_sec, info.msg_time.tv_usec );
	*/

	JNIEnv* env;
	jvm->AttachCurrentThread( (void**) &env, NULL );

	jobject jobj = (jobject) userdata;
	jclass jcls = env->GetObjectClass( jobj );

	jmethodID jmid = env->GetMethodID( jcls, "handleStartReply", "(JJZ)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
			"Java method \'handleStartReply\'.  This may indicate a version mismatch.\n" );
		return;
	}
	env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
		(jboolean) info.isStarted );
}


void VRPN_CALLBACK handle_stop( void *userdata,
							   const vrpn_FUNCTION_STOP_REPLY_CB info )
{
	if( jvm == NULL )
	{
		printf( "Error in handle_stop:  uninitialized jvm.\n" );
		return;
	}
	/*
	printf( "fg stop (C):  time:  %d.%d;\n"
	info.msg_time.tv_sec, info.msg_time.tv_usec );
	*/

	JNIEnv* env;
	jvm->AttachCurrentThread( (void**) &env, NULL );

	jobject jobj = (jobject) userdata;
	jclass jcls = env->GetObjectClass( jobj );

	jmethodID jmid = env->GetMethodID( jcls, "handleStopReply", "(JJZ)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
			"Java method \'handleStopReply\'.  This may indicate a version mismatch.\n" );
		return;
	}
	env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
		(jboolean) info.isStopped );
}


void VRPN_CALLBACK handle_sample_rate( void *userdata,
									  const vrpn_FUNCTION_SAMPLE_RATE_REPLY_CB info )
{
	if( jvm == NULL )
	{
		printf( "Error in handle_sample_rate:  uninitialized jvm.\n" );
		return;
	}
	/*
	printf( "fg sample rate (C):  time:  %d.%d;\n"
	info.msg_time.tv_sec, info.msg_time.tv_usec );
	*/

	JNIEnv* env;
	jvm->AttachCurrentThread( (void**) &env, NULL );

	jobject jobj = (jobject) userdata;
	jclass jcls = env->GetObjectClass( jobj );

	jmethodID jmid = env->GetMethodID( jcls, "handleSampleRateReply", "(JJD)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
			"Java method \'handleSampleRateReply\'.  This may indicate a version mismatch.\n" );
		return;
	}
	env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
		(jdouble) info.sampleRate );
}


void VRPN_CALLBACK handle_interpreter_description( void *userdata,
												  const vrpn_FUNCTION_INTERPRETER_REPLY_CB info )
{
	if( jvm == NULL )
	{
		printf( "Error in handle_interpreter_description:  uninitialized jvm.\n" );
		return;
	}
	/*
	printf( "fg interpreter description (C):  time:  %d.%d;\n"
	info.msg_time.tv_sec, info.msg_time.tv_usec );
	*/

	JNIEnv* env;
	jvm->AttachCurrentThread( (void**) &env, NULL );

	jobject jobj = (jobject) userdata;
	jclass jcls = env->GetObjectClass( jobj );

	jmethodID jmid = env->GetMethodID( jcls, "handleInterpreterDescription", "(JJLjava/lang/String;)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
			"Java method \'handleInterpreterDescription\'.  This may indicate a version mismatch.\n" );
		return;
	}

	jstring jdesc = env->NewStringUTF( info.description );
	if( jdesc == NULL ) // out of memory
	{
		printf( "Warning:  vrpn_FunctionGeneratorRemote library (handle_interpreter_description) "
			"was unable to create the description string.\n" );
		return;
	}
	env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec, jdesc );
}


void VRPN_CALLBACK handle_error( void *userdata,
								 const vrpn_FUNCTION_ERROR_CB info )
{
	if( jvm == NULL )
	{
		printf( "Error in handle_error:  uninitialized jvm.\n" );
		return;
	}
	/*
	printf( "fg error (C):  time:  %d.%d;\n"
	info.msg_time.tv_sec, info.msg_time.tv_usec );
	*/

	JNIEnv* env;
	jvm->AttachCurrentThread( (void**) &env, NULL );

	jobject jobj = (jobject) userdata;
	jclass jcls = env->GetObjectClass( jobj );

	jmethodID jmid = env->GetMethodID( jcls, "handleErrorReport", "(JJII)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_FunctionGeneratorRemote library was unable to find the "
			"Java method \'handleErrorReport\'.  This may indicate a version mismatch.\n" );
		return;
	}

	env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec, 
						 (jint) info.err, (jint) info.channel );
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
	f->register_error_handler( jobj, handle_error );
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
	if( f )
	{
		f->unregister_channel_reply_handler( jobj, handle_channel_reply );
		f->unregister_start_reply_handler( jobj, handle_start );
		f->unregister_stop_reply_handler( jobj, handle_stop );
		f->unregister_sample_rate_reply_handler( jobj, handle_sample_rate );
		f->unregister_interpreter_reply_handler( jobj, handle_interpreter_description );
		f->unregister_error_handler( jobj, handle_error );
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
	if( f == NULL )  // this force device is uninitialized or has been shut down already
		return;

	// now call mainloop
	f->mainloop( );
}

// end java_vrpn utility functions
////////////////////////////////////


////////////////////////////////////
// native functions from java class FunctionGeneratorRemote

/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    setChannelNULL_native
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_setChannelNULL_1native( JNIEnv* env, jobject jobj, jint jchannelNum )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( jchannelNum < 0 )
		return false;
	vrpn_FunctionGenerator_channel c;
	vrpn_FunctionGenerator_function_NULL func;
	c.setFunction( &func );
	if( f->setChannel( jchannelNum, &c ) < 0 )
		return false;
	return true;
}


/*
 * Class:     vrpn_FunctionGeneratorRemote
 * Method:    setChannelScript_native
 * Signature: (ILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_setChannelScript_1native( JNIEnv* env, jobject jobj, jint jchannelNum, jstring jscript )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( jchannelNum < 0 )
		return false;
	vrpn_FunctionGenerator_channel c;
	const char* msg = env->GetStringUTFChars( jscript, NULL );
	vrpn_FunctionGenerator_function_script func( msg );
	env->ReleaseStringUTFChars( jscript, msg );
	c.setFunction( &func );
	if( f->setChannel( jchannelNum, &c ) < 0 )
		return false;
	return true;
}


/*
* Class:     vrpn_FunctionGeneratorRemote
* Method:    requestChannel_native
* Signature: (I)I
*/
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_requestChannel_1native( JNIEnv* env, jobject jobj, jint jchannelNum )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( jchannelNum < 0 )
		return false;
	if( f->requestChannel( jchannelNum ) < 0 )
		return false;
	return true;
}


/*
* Class:     vrpn_FunctionGeneratorRemote
* Method:    requestAllChannels_native
* Signature: ()I
*/
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_requestAllChannels_1native( JNIEnv* env, jobject jobj )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( f->requestAllChannels( ) < 0 )
		return false;
	return true;
}


/*
* Class:     vrpn_FunctionGeneratorRemote
* Method:    requestStart_native
* Signature: ()I
*/
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_requestStart_1native( JNIEnv* env, jobject jobj )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( f->requestStart( ) < 0 )
		return false;
	return true;
}


/*
* Class:     vrpn_FunctionGeneratorRemote
* Method:    requestStop_native
* Signature: ()I
*/
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_requestStop_1native( JNIEnv* env, jobject jobj )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( f->requestStop( ) < 0 )
		return false;
	return true;
}


/*
* Class:     vrpn_FunctionGeneratorRemote
* Method:    requestSampleRate_native
* Signature: (F)I
*/
JNIEXPORT jboolean JNICALL \
Java_vrpn_FunctionGeneratorRemote_requestSampleRate_1native( JNIEnv* env, jobject jobj, jfloat jrate)
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( jrate < 0 )
		return false;
	if( f->requestSampleRate( jrate ) < 0 )
		return false;
	return true;
}


/*
* Class:     vrpn_FunctionGeneratorRemote
* Method:    requestInterpreterDescription_native
* Signature: ()I
*/
JNIEXPORT jboolean JNICALL 
Java_vrpn_FunctionGeneratorRemote_requestInterpreterDescription_1native( JNIEnv* env, jobject jobj )
{
	vrpn_FunctionGenerator_Remote* f = (vrpn_FunctionGenerator_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;
	if( f->requestInterpreterDescription( ) < 0 )
		return false;
	return true;
}


// end native functions for java class ForceDeviceRemote
///////////////////////////////////////////


