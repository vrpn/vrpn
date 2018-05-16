#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Poser.h"
#include "vrpn_PoserRemote.h"


jclass jclass_vrpn_PoserRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_Poser_Remote( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_PoserRemote native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn PoserRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.PoserRemote
  jclass cls = env->FindClass( "vrpn/PoserRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn PoserRemote native library "
            "while trying to find java class vrpn.PoserRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_PoserRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_PoserRemote == NULL )
  {
    printf( "Error loading vrpn PoserRemote native library "
            "while setting up class vrpn.PoserRemote.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_Poser_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn PoserRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.poserRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_PoserRemote );
}


// end DLL functions
/////////////////////////////


/*
 * Class:     vrpn_PoserRemote
 * Method:    mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_PoserRemote_mainloop( JNIEnv* env, jobject jobj )
{
  vrpn_Poser_Remote* po 
	  = (vrpn_Poser_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

  if( po )
	po->mainloop( );
}


/*
 * Class:     vrpn_PoserRemote
 * Method:    shutdownPoser
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_PoserRemote_shutdownPoser( JNIEnv* env, jobject jobj )
{
  // get the poser pointers
  vrpn_Poser_Remote* po 
	  = (vrpn_Poser_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );

  if( po )
  {
	po->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
	delete po;
  }
   
  // set the poser pointers to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );
}


/*
 * Class:     vrpn_PoserRemote
 * Method:    init
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_PoserRemote_init( JNIEnv* env, jobject jobj, jstring jname, 
						    jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
							jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
	// make a global reference to the Java PoserRemote
	// object, so it can be referenced by native code
	jobj = env->NewGlobalRef( jobj );
	
	// create the poser
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
	vrpn_Poser_Remote* po = new vrpn_Poser_Remote( name, conn );
	env->ReleaseStringUTFChars( jname, name );
	env->ReleaseStringUTFChars( jlocalInLogfileName, local_in_logfile_name );
	env->ReleaseStringUTFChars( jlocalOutLogfileName, local_out_logfile_name );
	env->ReleaseStringUTFChars( jremoteInLogfileName, remote_in_logfile_name );
	env->ReleaseStringUTFChars( jremoteOutLogfileName, remote_out_logfile_name );
	
	// now stash 'po' in the jobj's 'native_device field
	jlong jpo = (jlong) po;
	env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, jpo );
	
	return true;
}


/*
 * Class:     vrpn_PoserRemote
 * Method:    requestPose_native
 * Signature: (JJ[D[D)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_PoserRemote_requestPose_1native( JNIEnv* env, jobject jobj, 
										   jlong jsecs, jlong jusecs, 
										   jdoubleArray jposition, jdoubleArray jquat )
{
  // get the poser pointer
  vrpn_Poser_Remote* po 
	  = (vrpn_Poser_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( po == NULL )  // this poser is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestPose(long,long,double[],double[])\":  "
			"the poser is uninitialized or has been shut down.\n" );
    return false;
  }

  // check the array lengths
  if( env->GetArrayLength( jposition ) != 3 )
  {
    printf( "Error in native method \"requestPose(long,long,double[],double[])\":  "
			"position array was the wrong length.\n" );
	return false;
  }
  if( env->GetArrayLength( jquat ) != 4 )
  {
    printf( "Error in native method \"requestPose(long,long,double[],double[])\":  "
			"quaternion array was the wrong length.\n" );
	return false;
  }

  // eventual return value
  bool retval = true;

  // get the array
  double* position = env->GetDoubleArrayElements( jposition, NULL );
  if( position == NULL )
  {
    printf( "Error in native method \"requestPose(long,long,double[],double[])\":  "
			"couldn't get position in native form.\n" );
	retval = false;
  }
  double* quat = env->GetDoubleArrayElements( jquat, NULL );
  if( quat == NULL )
  {
    printf( "Error in native method \"requestPose(long,long,double[],double[])\":  "
			"couldn't get quaternion in native form.\n" );
	retval = false;
  }

  timeval t;
  t.tv_sec = (long) jsecs;
  t.tv_usec = (long) jusecs;

  if( retval == true )
	  retval = ( 0 != po->request_pose( t, position, quat ) );
  env->ReleaseDoubleArrayElements( jposition, position, JNI_ABORT /*mode*/ );
  env->ReleaseDoubleArrayElements( jquat, quat, JNI_ABORT /*mode*/ );
  return retval;
}


/*
 * Class:     vrpn_PoserRemote
 * Method:    requestPoseRelative_native
 * Signature: (JJ[D[D)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_PoserRemote_requestPoseRelative_1native( JNIEnv* env, jobject jobj, 
										   jlong jsecs, jlong jusecs, 
										   jdoubleArray jpositionDelta, jdoubleArray jquat )

{
  // get the poser pointer
  vrpn_Poser_Remote* po 
	  = (vrpn_Poser_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( po == NULL )  // this poser is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestPoseRelative(long,long,double[],double[])\":  "
			"the poser is uninitialized or has been shut down.\n" );
    return false;
  }

  // check the array lengths
  if( env->GetArrayLength( jpositionDelta ) != 3 )
  {
    printf( "Error in native method \"requestPoseRelative(long,long,double[],double[])\":  "
			"position array was the wrong length.\n" );
	return false;
  }
  if( env->GetArrayLength( jquat ) != 4 )
  {
    printf( "Error in native method \"requestPoseRelative(long,long,double[],double[])\":  "
			"quaternion array was the wrong length.\n" );
	return false;
  }

  // eventual return value
  bool retval = true;

  // get the array
  double* position = env->GetDoubleArrayElements( jpositionDelta, NULL );
  if( position == NULL )
  {
    printf( "Error in native method \"requestPoseRelative(long,long,double[],double[])\":  "
			"couldn't get position in native form.\n" );
	retval = false;
  }
  double* quat = env->GetDoubleArrayElements( jquat, NULL );
  if( quat == NULL )
  {
    printf( "Error in native method \"requestPoseRelative(long,long,double[],double[])\":  "
			"couldn't get quaternion in native form.\n" );
	retval = false;
  }

  timeval t;
  t.tv_sec = (long) jsecs;
  t.tv_usec = (long) jusecs;

  if( retval == true )
	  retval = ( 0 != po->request_pose_relative( t, position, quat ) );
  env->ReleaseDoubleArrayElements( jpositionDelta, position, JNI_ABORT /*mode*/ );
  env->ReleaseDoubleArrayElements( jquat, quat, JNI_ABORT /*mode*/ );
  return retval;
}


/*
 * Class:     vrpn_PoserRemote
 * Method:    requestPoseVelocity_native
 * Signature: (JJ[D[DD)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_PoserRemote_requestVelocity_1native( JNIEnv* env, jobject jobj,
											   jlong jsecs, jlong jusecs, 
											   jdoubleArray jvelocity, jdoubleArray jquat, 
											   jdouble jinterval )
{
  // get the poser pointer
  vrpn_Poser_Remote* po 
	  = (vrpn_Poser_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( po == NULL )  // this poser is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestVelocity(long,long,double[],double[],double)\":  "
			"the poser is uninitialized or has been shut down.\n" );
    return false;
  }

  // check the array lengths
  if( env->GetArrayLength( jvelocity ) != 3 )
  {
    printf( "Error in native method \"requestVelocity(long,long,double[],double[],double)\":  "
			"position array was the wrong length.\n" );
	return false;
  }
  if( env->GetArrayLength( jquat ) != 4 )
  {
    printf( "Error in native method \"requestVelocity(long,long,double[],double[],double)\":  "
			"quaternion array was the wrong length.\n" );
	return false;
  }

  // eventual return value
  bool retval = true;

  // get the array
  double* position = env->GetDoubleArrayElements( jvelocity, NULL );
  if( position == NULL )
  {
    printf( "Error in native method \"requestVelocity(long,long,double[],double[],double)\":  "
			"couldn't get position in native form.\n" );
	retval = false;
  }
  double* quat = env->GetDoubleArrayElements( jquat, NULL );
  if( quat == NULL )
  {
    printf( "Error in native method \"requestVelocity(long,long,double[],double[],double)\":  "
			"couldn't get quaternion in native form.\n" );
	retval = false;
  }

  timeval t;
  t.tv_sec = (long) jsecs;
  t.tv_usec = (long) jusecs;

  if( retval == true )
	  retval = ( 0 != po->request_pose_velocity( t, position, quat, jinterval ) );
  env->ReleaseDoubleArrayElements( jvelocity, position, JNI_ABORT /*mode*/ );
  env->ReleaseDoubleArrayElements( jquat, quat, JNI_ABORT /*mode*/ );
  return retval;
}


/*
 * Class:     vrpn_PoserRemote
 * Method:    requestVelocityRelative_native
 * Signature: (JJ[D[DD)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_PoserRemote_requestVelocityRelative_1native( JNIEnv* env, jobject jobj,
													   jlong jsecs, jlong jusecs, 
													   jdoubleArray jvelocityDelta, 
													   jdoubleArray jquat, 
													   jdouble jinterval )
{
  // get the poser pointer
  vrpn_Poser_Remote* po 
	  = (vrpn_Poser_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( po == NULL )  // this poser is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestVelocityRelative(long,long,double[],double[],double)\":  "
			"the poser is uninitialized or has been shut down.\n" );
    return false;
  }

  // check the array lengths
  if( env->GetArrayLength( jvelocityDelta ) != 3 )
  {
    printf( "Error in native method \"requestVelocityRelative(long,long,double[],double[],double)\":  "
			"position array was the wrong length.\n" );
	return false;
  }
  if( env->GetArrayLength( jquat ) != 4 )
  {
    printf( "Error in native method \"requestVelocityRelative(long,long,double[],double[],double)\":  "
			"quaternion array was the wrong length.\n" );
	return false;
  }

  // eventual return value
  bool retval = true;

  // get the array
  double* position = env->GetDoubleArrayElements( jvelocityDelta, NULL );
  if( position == NULL )
  {
    printf( "Error in native method \"requestVelocityRelative(long,long,double[],double[],double)\":  "
			"couldn't get position in native form.\n" );
	retval = false;
  }
  double* quat = env->GetDoubleArrayElements( jquat, NULL );
  if( quat == NULL )
  {
    printf( "Error in native method \"requestVelocityRelative(long,long,double[],double[],double)\":  "
			"couldn't get quaternion in native form.\n" );
	retval = false;
  }

  timeval t;
  t.tv_sec = (long) jsecs;
  t.tv_usec = (long) jusecs;

  if( retval == true )
	  retval = ( 0 != po->request_pose_velocity_relative( t, position, quat, jinterval ) );
  env->ReleaseDoubleArrayElements( jvelocityDelta, position, JNI_ABORT /*mode*/ );
  env->ReleaseDoubleArrayElements( jquat, quat, JNI_ABORT /*mode*/ );
  return retval;
}

