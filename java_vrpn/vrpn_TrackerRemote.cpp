
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Tracker.h"
#include "vrpn_TrackerRemote.h"

JavaVM* jvm = NULL;
jclass jclass_vrpn_TrackerRemote = NULL;
jfieldID jfid_vrpn_TrackerRemote_native_tracker = NULL;

// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* jvm, void* reserved )
{
  //////////////
  // set the jvm reference
  ::jvm = jvm;

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

  ////////////////
  // get a jfid field id reference to the "native_tracker" 
  // field of class vrpn.TrackerRemote.
  // field ids do not have to be made into global references.
  jfid_vrpn_TrackerRemote_native_tracker 
    = env->GetFieldID( jclass_vrpn_TrackerRemote, "native_tracker", "I" );
  if( jfid_vrpn_TrackerRemote_native_tracker == NULL )
  {
    printf( "Error loading vrpn TrackerRemote native library "
            "while looking into class vrpn.TrackerRemote.\n" );
    return JNI_ERR;
  }
  
  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload( JavaVM* jvm, void* reserved )
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


// This is the callback for vprn to notify us of a new tracker message
void handle_tracker_change( void* userdata, const vrpn_TRACKERCB info )
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


// This is the callback for vprn to notify us of a new velocity message
void handle_velocity_change( void* userdata, const vrpn_TRACKERVELCB info )
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


// This is the callback for vprn to notify us of a new tracker message
void handle_acceleration_change( void* userdata, const vrpn_TRACKERACCCB info )
{
  if( jvm == NULL )
    return;

  
  printf( "tracker change (C):  time:  %d.%d;  sensor:  %d;\n"
          "\tacc: %f %f %f;\n"
          "\tquat:  %f %f %f %f  dt:  %f\n", info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.sensor, info.acc[0], info.acc[1], info.acc[2],
          info.acc_quat[0], info.acc_quat[1], info.acc_quat[2], info.acc_quat[3], info.acc_quat_dt );
  

  
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


JNIEXPORT jint JNICALL 
Java_vrpn_TrackerRemote_setUpdateRate( JNIEnv* env, jobject jobj, jdouble updateRate )
{
  // look up the tracker pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tracker", "I" );
  if( jfid == NULL )
    return -1;
  vrpn_Tracker_Remote* t = (vrpn_Tracker_Remote*) env->GetIntField( jobj, jfid );
  if( t <= 0 )
    return -1;
  
  // now set the update rate
  return t->set_update_rate( updateRate );

}


JNIEXPORT void JNICALL 
Java_vrpn_TrackerRemote_mainloop( JNIEnv* env, jobject jobj )
{
  // look up the tracker pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tracker", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"mainloop\":  unable to ID native tracker field.\n" );
    return;
  }
  vrpn_Tracker_Remote* t = (vrpn_Tracker_Remote*) env->GetIntField( jobj, jfid );
  if( t <= 0 )  // this tracker is uninitialized or has been shtu down already
    return;

  // now call mainloop
  t->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_TrackerRemote_init( JNIEnv* env, jobject jobj, jstring jname )
{
  printf( "in Java_TrackerRemote_init(...)\n" );

  // look up where to store the tracker pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tracker", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"init\":  unable to ID native tracker field.\n" );
    return false;
  }

  // make a global reference to the Java TrackerRemote
  // object, so that it can be referenced in the function
  // handle_tracker_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the tracker
  const char* name = env->GetStringUTFChars( jname, NULL );
  vrpn_Tracker_Remote* t = new vrpn_Tracker_Remote( name );
  t->register_change_handler( jobj, handle_tracker_change );
  t->register_change_handler( jobj, handle_velocity_change );
  t->register_change_handler( jobj, handle_acceleration_change );
  env->ReleaseStringUTFChars( jname, name );
  
  // now stash 't' in the jobj's 'native_tracker' field
  jint jt = (jint) t;
  env->SetIntField( jobj, jfid, jt );
  
  return true;
}



// this is only supposed to be called when the TrackerRemote
// instance is finalized for garbage collection
JNIEXPORT void JNICALL 
Java_vrpn_TrackerRemote_shutdownTracker( JNIEnv* env, jobject jobj )
{

  // look up where to store the tracker pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tracker", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"shutdownTracker\":  unable to ID native tracker field.\n" );
    return;
  }

  // get the tracker pointer
  vrpn_Tracker_Remote* t = (vrpn_Tracker_Remote*) env->GetIntField( jobj, jfid );
  
  // unregister a handler and destroy the tracker
  if( t > 0 )
  {
    t->unregister_change_handler( jobj, handle_tracker_change );
    t->unregister_change_handler( jobj, handle_velocity_change );
    t->unregister_change_handler( jobj, handle_acceleration_change );
    delete t;
  }

  // set the tracker pointer to -1
  env->SetIntField( jobj, jfid, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj /* is this the right object? */ );


}

