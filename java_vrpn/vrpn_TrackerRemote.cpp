
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Tracker.h"
#include "vrpn_TrackerRemote.h"

jclass jclass_vrpn_TrackerRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_Tracker_Remote( JavaVM* jvm, void* reserved )
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
    printf( "Error loading vrpn TrackerRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.TrackerRemote
  jclass cls = env->FindClass( "vrpn/TrackerRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn TrackerRemote native library "
            "while trying to find class vrpn.TrackerRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_TrackerRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_TrackerRemote == NULL )
  {
    printf( "Error loading vrpn TrackerRemote native library "
            "while setting up class vrpn.TrackerRemote.\n" );
    return JNI_ERR;
  }

 
  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_Tracker_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn TrackerRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.TrackerRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_TrackerRemote );
}


// end DLL functions
/////////////////////////////


////////////////////////////
// dll utility functions


// This is the callback for vrpn to notify us of a new tracker message
void VRPN_CALLBACK handle_tracker_change( void* userdata, const vrpn_TRACKERCB info )
{
  if( jvm == NULL )
    return;

  /*
  printf( "tracker change (C):  time:  %d.%d;  sensor:  %d;\n"
          "\tpos: %f %f %f;\n"
          "\tquat:  %f %f %f %f\n", info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.sensor, info.pos[0], info.pos[1], info.pos[2],
          info.quat[0], info.quat[1], info.quat[2], info.quat[3] );
  */

  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleTrackerChange", "(JJIDDDDDDD)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_TrackerRemote library was unable to find the "
            "Java method \'handleTrackerChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jint) info.sensor, (jdouble) info.pos[0], (jdouble) info.pos[1], 
                       (jdouble) info.pos[2], (jdouble) info.quat[0], (jdouble) info.quat[1], 
                       (jdouble) info.quat[2], (jdouble) info.quat[3] );

}


// This is the callback for vrpn to notify us of a new velocity message
void VRPN_CALLBACK handle_velocity_change( void* userdata, const vrpn_TRACKERVELCB info )
{
  if( jvm == NULL )
    return;

  /*
  printf( "velocityb change (C):  time:  %d.%d;  sensor:  %d;\n"
          "\tvel: %f %f %f;\n"
          "\tquat:  %f %f %f %f  dt:  %f\n", info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.sensor, info.vel[0], info.vel[1], info.vel[2],
          info.vel_quat[0], info.vel_quat[1], info.vel_quat[2], info.vel_quat[3], info.vel_quat_dt );
  */

  
  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleVelocityChange", "(JJIDDDDDDDD)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_TrackerRemote library was unable to find the "
            "Java method \'handleVelocityChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jint) info.sensor, (jdouble) info.vel[0], (jdouble) info.vel[1], 
                       (jdouble) info.vel[2], (jdouble) info.vel_quat[0], (jdouble) info.vel_quat[1], 
                       (jdouble) info.vel_quat[2], (jdouble) info.vel_quat[3], (jdouble) info.vel_quat_dt );
  
}


// This is the callback for vrpn to notify us of a new tracker message
void VRPN_CALLBACK handle_acceleration_change( void* userdata, const vrpn_TRACKERACCCB info )
{
  if( jvm == NULL )
    return;

  /*
  printf( "tracker change (C):  time:  %d.%d;  sensor:  %d;\n"
          "\tacc: %f %f %f;\n"
          "\tquat:  %f %f %f %f  dt:  %f\n", info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.sensor, info.acc[0], info.acc[1], info.acc[2],
          info.acc_quat[0], info.acc_quat[1], info.acc_quat[2], info.acc_quat[3], info.acc_quat_dt );
  */

  
  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleAccelerationChange", "(JJIDDDDDDDD)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_TrackerRemote library was unable to find the "
            "Java method \'handleAccelerationChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jint) info.sensor, (jdouble) info.acc[0], (jdouble) info.acc[1], 
                       (jdouble) info.acc[2], (jdouble) info.acc_quat[0], (jdouble) info.acc_quat[1], 
                       (jdouble) info.acc_quat[2], (jdouble) info.acc_quat[3], (jdouble) info.acc_quat_dt );
  
}

// end dll utility function
//////////////////////////


/////////////////////////
// native java functions

JNIEXPORT jint JNICALL 
Java_vrpn_TrackerRemote_setUpdateRate( JNIEnv* env, jobject jobj, jdouble updateRate )
{
  vrpn_Tracker_Remote* t = (vrpn_Tracker_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( t == NULL )
    return -1;
  
  // now set the update rate
  return t->set_update_rate( updateRate );

}


JNIEXPORT void JNICALL 
Java_vrpn_TrackerRemote_mainloop( JNIEnv* env, jobject jobj )
{
  vrpn_Tracker_Remote* t = (vrpn_Tracker_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( t == NULL )  // this tracker is uninitialized or has been shut down already
    return;

  // now call mainloop
  t->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_TrackerRemote_init( JNIEnv* env, jobject jobj, jstring jname,
							  jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
							  jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java TrackerRemote
  // object, so that it can be referenced in the function
  // handle_tracker_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the tracker
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
  vrpn_Tracker_Remote* t = new vrpn_Tracker_Remote( name, conn );
  t->register_change_handler( jobj, handle_tracker_change );
  t->register_change_handler( jobj, handle_velocity_change );
  t->register_change_handler( jobj, handle_acceleration_change );
  env->ReleaseStringUTFChars( jname, name );
  env->ReleaseStringUTFChars( jlocalInLogfileName, local_in_logfile_name );
  env->ReleaseStringUTFChars( jlocalOutLogfileName, local_out_logfile_name );
  env->ReleaseStringUTFChars( jremoteInLogfileName, remote_in_logfile_name );
  env->ReleaseStringUTFChars( jremoteOutLogfileName, remote_out_logfile_name );
  
  // now stash 't' in the jobj's 'native_device' field
  jlong jt = (jlong) t;
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, jt );
  
  return true;
}



// this is only supposed to be called when the TrackerRemote
// instance is finalized for garbage collection
JNIEXPORT void JNICALL 
Java_vrpn_TrackerRemote_shutdownTracker( JNIEnv* env, jobject jobj )
{
  // get the tracker pointer
  vrpn_Tracker_Remote* t = (vrpn_Tracker_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  
  // unregister a handler and destroy the tracker
  if( t )
  {
    t->unregister_change_handler( jobj, handle_tracker_change );
    t->unregister_change_handler( jobj, handle_velocity_change );
    t->unregister_change_handler( jobj, handle_acceleration_change );
	t->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
    delete t;
  }

  // set the tracker pointer to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );


}

// end native java functions
////////////////////////////
