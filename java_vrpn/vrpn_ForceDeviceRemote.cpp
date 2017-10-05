
#include "stdio.h"
#include "jni.h"
#include "vrpn_ForceDevice.h"

#include "java_vrpn.h"
#include "vrpn_ForceDeviceRemote.h"


jclass jclass_vrpn_ForceDeviceRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_ForceDevice_Remote( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_ForceDevice native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn ForceDeviceRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.ForceDeviceRemote
  jclass cls = env->FindClass( "vrpn/ForceDeviceRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn ForceDeviceRemote native library "
            "while trying to find class vrpn.ForceDeviceRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_ForceDeviceRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_ForceDeviceRemote == NULL )
  {
    printf( "Error loading vrpn ForceDeviceRemote native library "
            "while setting up class vrpn.ForceDeviceRemote.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad



JNIEXPORT void JNICALL JNI_OnUnload_ForceDevice_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn ForceDeviceRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.ForceDeviceRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_ForceDeviceRemote );
}

// end DLL functions
/////////////////////////////


/////////////////////////////
// java_vrpn utility funcitons

void VRPN_CALLBACK handle_force_change( void* userdata, const vrpn_FORCECB info )
{
  if( jvm == NULL )
  {
    printf( "Error in handle_force_change:  uninitialized jvm.\n" );
    return;
  }

  /*
  printf( "force change (C):  time:  %d.%d;\n"
          "\tforce: %f %f %f;\n",
          info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.force[0], info.force[1], info.force[2] );
  */
  
  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleForceChange", "(JJDDD)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_ForceDeviceRemote library was unable to find the "
            "Java method \'handleForceChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jdouble) info.force[0], (jdouble) info.force[1], (jdouble) info.force[2] );

} // end handle_force_change


void VRPN_CALLBACK handle_scp_change( void* userdata, const vrpn_FORCESCPCB info )
{
  if( jvm == NULL )
  {
    printf( "Error in handle_scp_change:  uninitialized jvm.\n" );
    return;
  }

  /*
  printf( "scp change (C):  time:  %d.%d;\n"
          "\pos: %f %f %f;\n\tquat:  %f %f %f\n",
          info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.pos[0], info.pos[1], info.pos[2],
          info.quat[0], info.quat[1], info.quat[2], info.quat[3] );
  */
  
  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleSCPChange", "(JJDDDDDDD)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_ForceDeviceRemote library was unable to find the "
            "Java method \'handleSCPChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jdouble) info.pos[0], (jdouble) info.pos[1], (jdouble) info.pos[2],
                       (jdouble) info.quat[0], (jdouble) info.quat[1], (jdouble) info.quat[2], 
                       (jdouble) info.quat[3] );

} // end handle_scp_change


void VRPN_CALLBACK handle_force_error( void* userdata, const vrpn_FORCEERRORCB info )
{
  if( jvm == NULL )
  {
    printf( "Error in handle_force_error:  uninitialized jvm.\n" );
    return;
  }

  /*
  printf( "force error (C):  time:  %d.%d;  force: %d;\n",
          info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.error_code );
  */
  
  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleForceError", "(JJI)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_ForceDeviceRemote library was unable to find the "
            "Java method \'handleForceError\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jint) info.error_code );

} // end handle_force_error]


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    init
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_ForceDeviceRemote_init( JNIEnv* env, jobject jobj, jstring jname, 
								  jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
								  jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java ForceDeviceRemote
  // object, so that it can be referenced in the functions
  // handle_*_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the force device
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
  vrpn_ForceDevice_Remote* f = new vrpn_ForceDevice_Remote( name, conn );
  f->register_force_change_handler( jobj, handle_force_change );
  f->register_scp_change_handler( jobj, handle_scp_change );
  f->register_error_handler( jobj, handle_force_error );
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
 * Class:     vrpn_ForceDeviceRemote
 * Method:    mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_ForceDeviceRemote_mainloop( JNIEnv* env, jobject jobj )
{
  vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( f == NULL )  // this force device is uninitialized or has been shut down already
    return;

  // now call mainloop
  f->mainloop( );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    shutdownForceDevice
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_ForceDeviceRemote_shutdownForceDevice( JNIEnv* env, jobject jobj )
{
  // get the force device pointer
  vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  
  // unregister a handler and destroy the force device
  if( f )
  {
    f->unregister_force_change_handler( jobj, handle_force_change );
    f->unregister_scp_change_handler( jobj, handle_scp_change );
    f->unregister_error_handler( jobj, handle_force_error );
	f->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
    delete f;
  }

  // set the force device pointer to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );
}


// end java_vrpn utility functions
////////////////////////////////////


////////////////////////////////////
// native functions from java class ForceDeviceRemote

/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    sendForceField
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_ForceDeviceRemote_sendForceField_1native__( JNIEnv* env, jobject jobj )
{
  vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( f == NULL )
    return false;
  
  f->sendForceField( );
  return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    stopForceField
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_ForceDeviceRemote_stopForceField_1native( JNIEnv* env, jobject jobj )
{
  vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( f == NULL )
    return false;
  
  f->stopForceField( );
  return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    sendForceField
 * Signature: ([F[F[[FF)V
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_ForceDeviceRemote_sendForceField_1native___3F_3F_3_3FF( JNIEnv* env, jobject jobj, 
                                                         jfloatArray jorigin, 
                                                         jfloatArray jforce, 
                                                         jobjectArray jjacobian, 
                                                         jfloat jradius )
{
  vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( f == NULL )
    return false;

  // get the arguments
  jfloat origin[3];
  jfloat force[3];
  jfloat jacobian_row0[3];
  jfloat jacobian_row1[3];
  jfloat jacobian_row2[3];
  jfloatArray jacobian_row;

  if( env->GetArrayLength( jorigin ) != 3 )
    return false;
  env->GetFloatArrayRegion( jorigin, 0, 3, origin);

  if( env->GetArrayLength( jforce ) != 3 )
    return false;
  env->GetFloatArrayRegion( jforce, 0, 3, force );
  
  if( env->GetArrayLength( jjacobian ) != 3 )
    return false;
  jacobian_row = (jfloatArray) env->GetObjectArrayElement( jjacobian, 0 );
  if( jacobian_row == NULL )
    return false;
  if( env->GetArrayLength( jacobian_row ) != 3 )
    return false;
  env->GetFloatArrayRegion( jacobian_row, 0, 3, jacobian_row0 );

  jacobian_row = (jfloatArray) env->GetObjectArrayElement( jjacobian, 1 );
  if( jacobian_row == NULL )
    return false;
  if( env->GetArrayLength( jacobian_row ) != 3 )
    return false;
  env->GetFloatArrayRegion( jacobian_row, 0, 3, jacobian_row1 );

  jacobian_row = (jfloatArray) env->GetObjectArrayElement( jjacobian, 2 );
  if( jacobian_row == NULL )
    return false;
  if( env->GetArrayLength( jacobian_row ) != 3 )
    return false;
  env->GetFloatArrayRegion( jacobian_row, 0, 3, jacobian_row2 );

  
  float jacobian[3][3];
  jacobian[0][0] = jacobian_row0[0];
  jacobian[0][1] = jacobian_row0[1];
  jacobian[0][2] = jacobian_row0[2];
  jacobian[1][0] = jacobian_row1[0];
  jacobian[1][1] = jacobian_row1[1];
  jacobian[1][2] = jacobian_row1[2];
  jacobian[2][0] = jacobian_row1[0];
  jacobian[2][1] = jacobian_row1[1];
  jacobian[2][2] = jacobian_row1[2];


  // now call the function
  f->sendForceField( origin, force, jacobian, jradius );
  return true;

}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    enableConstraint
 * Signature: (I)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_enableConstraint_1native
  (JNIEnv *env, jobject jobj, jint on_int)
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

  // now call the function
  f->enableConstraint( on_int );
  return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintMode
 * Signature: (I)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_setConstraintMode_1native
  (JNIEnv *env, jobject jobj, jint mode)
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

  // now call the function
  f->setConstraintMode( (vrpn_ForceDevice::ConstraintGeometry)mode );
  return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintPoint
 * Signature: ([F)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_setConstraintPoint_1native
  (JNIEnv* env, jobject jobj, jfloatArray jpoint)
{
  vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( f == NULL )
    return false;

  // get the arguments
  jfloat point[3];

  if( env->GetArrayLength( jpoint ) != 3 )
    return false;
  env->GetFloatArrayRegion( jpoint, 0, 3, point );
  
  // now call the function
  f->setConstraintPoint( point );
  return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintLinePoint
 * Signature: ([F)V
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_ForceDeviceRemote_setConstraintLinePoint_1native( JNIEnv* env, jobject jobj, jfloatArray jpoint )
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

	// get the arguments
	jfloat point[3];

	if( env->GetArrayLength( jpoint ) != 3 )
		return false;
	env->GetFloatArrayRegion( jpoint, 0, 3, point );
  
	// now call the function
	f->setConstraintLinePoint( point );
	return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintLineDirection
 * Signature: ([F)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_setConstraintLineDirection_1native
  (JNIEnv* env, jobject jobj, jfloatArray jpoint)
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

	// get the arguments
	jfloat point[3];

	if( env->GetArrayLength( jpoint ) != 3 )
		return false;
	env->GetFloatArrayRegion( jpoint, 0, 3, point );
  
	// now call the function
	f->setConstraintLineDirection( point );
	return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintPlaneNormal
 * Signature: ([F)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_setConstraintPlaneNormal_1native
  (JNIEnv* env, jobject jobj, jfloatArray jpoint)
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

	// get the arguments
	jfloat point[3];

	if( env->GetArrayLength( jpoint ) != 3 )
		return false;
	env->GetFloatArrayRegion( jpoint, 0, 3, point );
  
	// now call the function
	f->setConstraintPlaneNormal( point );
	return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintPlanePoint
 * Signature: ([F)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_setConstraintPlanePoint_1native
  (JNIEnv* env, jobject jobj, jfloatArray jpoint)
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

	// get the arguments
	jfloat point[3];

	if( env->GetArrayLength( jpoint ) != 3 )
		return false;
	env->GetFloatArrayRegion( jpoint, 0, 3, point );
  
	// now call the function
	f->setConstraintPlanePoint( point );
	return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setConstraintKSpring
 * Signature: (F)V
 */
JNIEXPORT jboolean JNICALL Java_vrpn_ForceDeviceRemote_setConstraintKSpring_1native
  (JNIEnv* env, jobject jobj, jfloat springConst)
{
	vrpn_ForceDevice_Remote* f = (vrpn_ForceDevice_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( f == NULL )
		return false;

	// now call the function
	f->setConstraintKSpring( springConst );
	return true;
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setTriangle
 * Signature: (IIIIIII)V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_setTriangle_1native
  (JNIEnv *, jobject, jint, jint, jint, jint, jint, jint, jint)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_setTriangle is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    useHcollide
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_useHcollide_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_useHcollide is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    useGhost
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_useGhost_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_useGhost is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setTrimeshTransform
 * Signature: ([F)V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_setTrimeshTransform_1native
  (JNIEnv *, jobject, jfloatArray)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_setTrimeshTransform is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    updateTrimeshChanges
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_updateTrimeshChanges_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_updateTrimeshChanges is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    clearTrimesh
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_clearTrimesh_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_clearTrimesh is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    sendSurface
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_sendSurface_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_sendSurface is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    startSurface
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_startSurface_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_startSurface is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    stopSurface
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_stopSurface_1native
  (JNIEnv *, jobject)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_stopSurface is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setVertex
 * Signature: (IFFF)V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_setVertex_1native
  (JNIEnv *, jobject, jint, jfloat, jfloat, jfloat)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_setVertex is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    setNormal
 * Signature: (IFFF)V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_setNormal_1native
  (JNIEnv *, jobject, jint, jfloat, jfloat, jfloat)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_setNormal is not implemented.\n" );
}


/*
 * Class:     vrpn_ForceDeviceRemote
 * Method:    removeTriangle
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vrpn_ForceDeviceRemote_removeTriangle_1native
  (JNIEnv *, jobject, jint)
{
  printf( "Function Java_vrpn_ForceDeviceRemote_removeTriangle is not implemented.\n" );
}


// end native functions for java class ForceDeviceRemote
///////////////////////////////////////////


