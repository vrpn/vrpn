#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Button.h"
#include "vrpn_ButtonRemote.h"

jclass jclass_vrpn_ButtonRemote = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_Button_Remote( JavaVM* jvm, void* reserved )
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
    printf( "Error loading vrpn ButtonRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.ButtonRemote
  jclass cls = env->FindClass( "vrpn/ButtonRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn ButtonRemote native library "
            "while trying to find class vrpn.ButtonRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_ButtonRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_ButtonRemote == NULL )
  {
    printf( "Error loading vrpn ButtonRemote native library "
            "while setting up class vrpn.ButtonRemote.\n" );
    return JNI_ERR;
  }

  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_Button_Remote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn ButtonRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.ButtonRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_ButtonRemote );
}


// This is the callback for vrpn to notify us of a new button message
void VRPN_CALLBACK handle_button_change( void* userdata, const vrpn_BUTTONCB info )
{
  if( jvm == NULL )
    return;

  /*  
  printf( "button change (C):  time:  %d.%d;  button:  %d;\n"
          "\tstate: %d;\n", info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.button, info.state );
  */

  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleButtonChange", "(JJII)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_ButtonRemote library was unable to find the "
            "Java method \'handleButtonChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jint) info.button, (jint) info.state );

}


JNIEXPORT void JNICALL 
Java_vrpn_ButtonRemote_mainloop( JNIEnv* env, jobject jobj )
{
  vrpn_Button_Remote* t = (vrpn_Button_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( t == NULL )  // this button is uninitialized or has been shut down already
    return;

  // now call mainloop
  t->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_ButtonRemote_init( JNIEnv* env, jobject jobj, jstring jname, 
							 jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
							 jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java ButtonRemote
  // object, so that it can be referenced in the function
  // handle_button_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the button
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
  vrpn_Button_Remote* t = new vrpn_Button_Remote( name, conn );
  t->register_change_handler( jobj, handle_button_change );
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



// this is only supposed to be called when the ButtonRemote
// instance is finalized for garbage collection
JNIEXPORT void JNICALL 
Java_vrpn_ButtonRemote_shutdownButton( JNIEnv* env, jobject jobj )
{
  // get the button pointer
  vrpn_Button_Remote* t = (vrpn_Button_Remote*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  
  // unregister a handler and destroy the button
  if( t )
  {
    t->unregister_change_handler( jobj, handle_button_change );
	t->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
    delete t;
  }

  // set the button pointer to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );


}




