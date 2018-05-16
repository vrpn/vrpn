
#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Text.h"
#include "vrpn_TextSender.h"


jclass jclass_vrpn_TextSender = NULL;
extern jfieldID jfid_vrpn_VRPNDevice_native_device;


//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_Text_Sender( JavaVM* jvm, void* reserved )
{
  ///////////////
  // make sure the general library code set jvm
  if( jvm == NULL )
  {
	printf( "vrpn_TextSender native:  no jvm.\n" );
    return JNI_ERR;
  }

  ///////////////
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error loading vrpn TextSender native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.TextSender
  jclass cls = env->FindClass( "vrpn/TextSender" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn TextSender native library "
            "while trying to find class vrpn.TextSender.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_TextSender = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_TextSender == NULL )
  {
    printf( "Error loading vrpn TextSender native library "
            "while setting up class vrpn.TextSender.\n" );
    return JNI_ERR;
  }

 
  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad


JNIEXPORT void JNICALL JNI_OnUnload_Text_Sender( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn TextSender native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.TrackerRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_TextSender );
}


// end DLL functions
/////////////////////////////


/*
 * Class:     vrpn_TextSender
 * Method:    sendMessage_native
 * Signature: (Ljava/lang/String;IIJ)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_TextSender_sendMessage_1native( JNIEnv* env, jobject jobj, jstring jmsg, jint jtype, 
										  jint jlevel, jlong jmsecs )
{
	// get the analog pointer
	vrpn_Text_Sender* s = (vrpn_Text_Sender*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
	if( s == NULL )  // this analog is uninitialized or has been shut down already
	{
		printf( "Error in native method \"sendMessage(...)\":  the text sender is "
            "uninitialized or has been shut down.\n" );
		return false;
	}
	const char* msg = env->GetStringUTFChars( jmsg, NULL );
	timeval time;
	time.tv_sec = (long) jmsecs / 1000;
	time.tv_usec = (long) ( jmsecs % 1000 ) * 1000;
	
	int retval =  s->send_message( msg, vrpn_TEXT_SEVERITY(jtype), jlevel, time );
	
	env->ReleaseStringUTFChars( jmsg, msg );
	return retval;
}


/*
 * Class:     vrpn_TextSender
 * Method:    mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_TextSender_mainloop( JNIEnv* env, jobject jobj )
{
  vrpn_Text_Sender* t = (vrpn_Text_Sender*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  if( t == NULL )  // this text sender is uninitialized or has been shut down already
    return;

  // now call mainloop (since text sender is really a server object, call the connection's mainloop)
  t->mainloop( );
  t->connectionPtr()->mainloop();
}


/*
 * Class:     vrpn_TextSender
 * Method:    init
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_TextSender_init( JNIEnv* env, jobject jobj, jstring jname,
							  jstring jlocalInLogfileName, jstring jlocalOutLogfileName,
							  jstring jremoteInLogfileName, jstring jremoteOutLogfileName )
{
  // make a global reference to the Java TextSender
  // object, so that it can be referenced in the function
  // handle_text_message(...)
  jobj = env->NewGlobalRef( jobj );

  // create the text Sender
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
	  = vrpn_create_server_connection( vrpn_DEFAULT_LISTEN_PORT_NO, local_in_logfile_name, local_out_logfile_name );
  vrpn_Text_Sender* t = new vrpn_Text_Sender( name, conn );

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


// this is only supposed to be called when the TextSender
// instance is finalized for garbage collection
JNIEXPORT void JNICALL 
Java_vrpn_TextSender_shutdownTextSender( JNIEnv* env, jobject jobj )
{
  // get the tracker pointer
  vrpn_Text_Sender* t = (vrpn_Text_Sender*) env->GetLongField( jobj, jfid_vrpn_VRPNDevice_native_device );
  
  // destroy the text sender
  if( t )
  {
	t->connectionPtr()->removeReference(); // because we called vrpn_get_connection_by_name
    delete t;
  }

  // set the tracker pointer to -1
  env->SetLongField( jobj, jfid_vrpn_VRPNDevice_native_device, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );


}

