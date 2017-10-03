
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_VRPNDevice.h"
#include "vrpn_BaseClass.h"
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
    = env->GetFieldID( jclass_vrpn_VRPNDevice, "native_device", "J" );
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



/*
 * Class:     vrpn_VRPNDevice
 * Method:    doingOkay_native
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_doingOkay_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )  return false;
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL ) return false;
	return ( conn->doing_okay() != 0 );
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    getElapsedTimeSecs_native
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL 
Java_vrpn_VRPNDevice_getElapsedTimeSecs_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == 0 )
	{
	    printf( "Error in native method \"getElapsedTimeSecs\":  "
				"the device is uninitialized or has been shut down.\n" );
		return -1;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"getElapsedTimeSecs\":  "
				"no connection\n." );
		return -1;
	}
	timeval elapsed;
	conn->time_since_connection_open( &elapsed );
	return elapsed.tv_sec;
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    getTime_native
 * Signature: (Ljava/util/Date;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_getTime_1native( JNIEnv* env, jobject jobj, jobject jdate )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == 0 )
	{
	    printf( "Error in native method \"getTime\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"getTime\":  "
				"no connection\n." );
		return false;
	}
	timeval t = conn->get_time( );
	jclass jcls = env->GetObjectClass( jdate );
	jmethodID jmid = env->GetMethodID( jcls, "setTime", "(J)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_VRPNDevice (getTime) library was unable to find the "
            "Java method \'Date::setTime\'.\n" );
		return false;
	}
	env->CallVoidMethod( jdate, jmid, ( (jlong) t.tv_sec ) * 1000 + ( (jlong) t.tv_usec ) / 1000 );
	return true;
}


  
/*
 * Class:     vrpn_VRPNDevice
 * Method:    isConnected_native
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_isConnected_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"isConnected\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"isConnected\":  "
				"no connection\n." );
		return false;
	}
	return (jboolean) conn->connected();
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    isLive_native
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_isLive_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"isLive\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"isLive\":  "
				"no connection\n." );
		return false;
	}
	return ( conn->get_File_Connection( ) == NULL );
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    playToElapsedTime_native
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_playToElapsedTime_1native( JNIEnv* env, jobject jobj, jlong time )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"playToElapsedTime\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"playToElapsedTime\":  "
				"no connection\n." );
		return false;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"playToElapsedTime\":  "
				"no file connection -- not replay\n." );
		return false;
	}
	return ( fc->play_to_time( (double) time ) == 0 );
	// note:  the compiler claims the cast "(double) time" could possibly lose data
	// (casting from __int64 to double)
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    getEarliestTime_native
 * Signature: (Ljava/util/Date;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_getEarliestTime_1native( JNIEnv* env, jobject jobj, jobject jdate)
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"getEarliestTime\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"getEarliestTime\":  "
				"no connection\n." );
		return false;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"getEarliestTime\":  "
				"no file connection -- not replay\n." );
		return false;
	}
	timeval t = fc->get_lowest_user_timestamp( );
	jclass jcls = env->GetObjectClass( jdate );
	jmethodID jmid = env->GetMethodID( jcls, "setTime", "(J)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_VRPNDevice (getEarliestTime) library was unable to find the "
            "Java method \'Date::setTime\'.\n" );
		return false;
	}
	env->CallVoidMethod( jdate, jmid, ( (jlong) t.tv_sec ) * 1000 + ( (jlong) t.tv_usec ) / 1000 );
	return true;
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    getLatestTime_native
 * Signature: (Ljava/util/Date;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_getLatestTime_1native( JNIEnv* env, jobject jobj, jobject jdate )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"getLatestTime\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"getLatestTime\":  "
				"no connection\n." );
		return false;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"getLatestTime\":  "
				"no file connection -- not replay\n." );
		return false;
	}
	timeval t = fc->get_highest_user_timestamp( );
	jclass jcls = env->GetObjectClass( jdate );
	jmethodID jmid = env->GetMethodID( jcls, "setTime", "(J)V" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_VRPNDevice (getLatestTime) library was unable to find the "
            "Java method \'Date::setTime\'.\n" );
		return false;
	}
	env->CallVoidMethod( jdate, jmid, ( (jlong) t.tv_sec ) * 1000 + ( (jlong) t.tv_usec ) / 1000 );
	return true;
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    eof_native
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_eof_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"eof\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"eof\":  "
				"no connection\n." );
		return false;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"eof\":  "
				"no file connection -- not replay\n." );
		return false;
	}
	return fc->eof( );
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    getLengthSecs_native
 * Signature: ()D
 */
JNIEXPORT jdouble JNICALL 
Java_vrpn_VRPNDevice_getLengthSecs_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"getLengthSecs\":  "
				"the device is uninitialized or has been shut down.\n" );
		return -1;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"getLengthSecs\":  "
				"no connection\n." );
		return -1;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"getLengthSecs\":  "
				"no file connection -- not replay\n." );
		return -1;
	}
	return (jdouble) fc->get_length_secs( );
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    setReplayRate_native
 * Signature: (F)V
 */
JNIEXPORT void JNICALL 
Java_vrpn_VRPNDevice_setReplayRate_1native( JNIEnv* env, jobject jobj, jfloat jrate )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"setReplayRate\":  "
				"the device is uninitialized or has been shut down.\n" );
		return;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"setReplayRate\":  "
				"no connection\n." );
		return;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"setReplayRate\":  "
				"no file connection -- not replay\n." );
		return;
	}
	fc->set_replay_rate( jrate );
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    getReplayRate_native
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL 
Java_vrpn_VRPNDevice_getReplayRate_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"setReplayRate\":  "
				"the device is uninitialized or has been shut down.\n" );
		return -1;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"setReplayRate\":  "
				"no connection\n." );
		return -1;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"setReplayRate\":  "
				"no file connection -- not replay\n." );
		return -1;
	}
	return fc->get_replay_rate( );
}


  
 /*
 * Class:     vrpn_VRPNDevice
 * Method:    reset_native
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_reset_1native( JNIEnv* env, jobject jobj )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"reset\":  "
				"the device is uninitialized or has been shut down.\n" );
		return -1;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"reset\":  "
				"no connection\n." );
		return -1;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"reset\":  "
				"no file connection -- not replay\n." );
		return -1;
	}
	return ( fc->reset( ) == 0 );
}


/*
 * Class:     vrpn_VRPNDevice
 * Method:    playToWallTime_native
 * Signature: (Ljava/util/Date;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_VRPNDevice_playToWallTime_1native( JNIEnv* env, jobject jobj, jobject jdate )
{
	vrpn_BaseClass* device 
		= (vrpn_BaseClass*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( device == NULL )
	{
	    printf( "Error in native method \"playToElapsedTime\":  "
				"the device is uninitialized or has been shut down.\n" );
		return false;
	}
	vrpn_Connection* conn = device->connectionPtr( );
	if( conn == NULL )
	{
	    printf( "Error in native method \"playToElapsedTime\":  "
				"no connection\n." );
		return false;
	}
	vrpn_File_Connection* fc = conn->get_File_Connection( );
	if( fc == NULL )
	{
	    printf( "Error in native method \"playToElapsedTime\":  "
				"no file connection -- not replay\n." );
		return false;
	}
	timeval t;
	long int msecs;
	jclass jcls = env->GetObjectClass( jdate );
	jmethodID jmid = env->GetMethodID( jcls, "getTime", "()J" );
	if( jmid == NULL )
	{
		printf( "Warning:  vrpn_VRPNDevice (playToWallTime) library was unable to find the "
            "Java method \'Date::getTime\'.\n" );
		return false;
	}
	msecs = (long) env->CallLongMethod( jdate, jmid );
	t.tv_sec = msecs / 1000;
	t.tv_usec = ( msecs - t.tv_sec * 1000 ) * 1000;
	
	return ( fc->play_to_filetime( t ) == 0 );
}
