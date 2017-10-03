
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Text.h"
#include "vrpn_TextReceiver.h"


jclass jclass_vrpn_TextReceiver = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_Text_Receiver( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_TextReceiver native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn TextReceiver native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.TextReceiver
  jclass cls = env->FindClass( "vrpn/TextReceiver" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn TextReceiver native library "
            "while trying to find class vrpn.TextReceiver.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_TextReceiver = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_TextReceiver == NULL )
  {
    printf( "Error loading vrpn TextReceiver native library "
            "while setting up class vrpn.TextReceiver.\n" );
    return JNI_ERR;
  }

 
  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_Text_Receiver( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn TextReceiver native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.TrackerRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_TextReceiver );
}


// end DLL functions
/////////////////////////////


////////////////////////////
// dll utility functions


// This is the callback for vrpn to notify us of a new tracker message
void VRPN_CALLBACK handle_text_message( void* userdata, const vrpn_TEXTCB info )
{
  if( jvm == NULL )
    return;

  /*
  printf( "text received (C):  time:  %d.%d;  severity:  %d;  level:  %d\n"
          "\tmsg: %s\n", info.msg_time.tv_sec, info.msg_time.tv_usec,
          info.type, info.level, info.message );
  */

  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleTextMessage", "(JJIILjava/lang/String;)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_TextReceiver library was unable to find the "
            "Java method \'handleTextMessage\'.  This may indicate a version mismatch.\n" );
    return;
  }

  // get a string to hold the message
  jstring jmsg = env->NewStringUTF( info.message );

  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jint) info.type, (jint) info.level, jmsg );

}



/*
 * Class:     vrpn_TextReceiver
 * Method:    mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_TextReceiver_mainloop( JNIEnv* env, jobject jobj )
{
  vrpn_Text_Receiver* t = (vrpn_Text_Receiver*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( t == NULL )  // this text receiver is uninitialized or has been shut down already
    return;
  
  // now call mainloop
  t->mainloop( );
}


/*
 * Class:     vrpn_TextReceiver
 * Method:    init
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_TextReceiver_init( JNIEnv* env, jobject jobj, jstring jname,
							  jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
							  jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{

  // make a global reference to the Java TextReceiver
  // object, so that it can be referenced in the function
  // handle_text_message(...)
  jobj = env->NewGlobalRef( jobj );

  // create the text receiver
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
  vrpn_Text_Receiver* t = new vrpn_Text_Receiver( name, conn );
  t->register_message_handler( jobj, handle_text_message );

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


// this is only supposed to be called when the TextReceiver
// instance is finalized for garbage collection
JNIEXPORT void JNICALL 
Java_vrpn_TextReceiver_shutdownTextReceiver( JNIEnv* env, jobject jobj )
{
  // get the tracker pointer
  vrpn_Text_Receiver* t = (vrpn_Text_Receiver*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  
  // unregister a handler and destroy the text receiver
  if( t )
  {
    t->unregister_message_handler( jobj, handle_text_message );
	t->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
    delete t;
  }

  // set the tracker pointer to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );


}

