

#include <stdio.h>
#include <jni.h>
#include "java_vrpn.h"
#include "vrpn_Analog.h"
#include "vrpn_AnalogRemote.h"


JavaVM* jvm = NULL;
jclass jclass_vrpn_AnalogRemote = NULL;
jfieldID jfid_vrpn_AnalogRemote_native_analog = NULL;

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
    printf( "Error loading vrpn AnalogRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.AnalogRemote
  jclass cls = env->FindClass( "vrpn/AnalogRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn AnalogRemote native library "
            "while trying to find class vrpn.AnalogRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_AnalogRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_AnalogRemote == NULL )
  {
    printf( "Error loading vrpn AnalogRemote native library "
            "while setting up class vrpn.AnalogRemote.\n" );
    return JNI_ERR;
  }

  ////////////////
  // get a jfid field id reference to the "native_analog" 
  // field of class vrpn.AnalogRemote.
  // field ids do not have to be made into global references.
  jfid_vrpn_AnalogRemote_native_analog
    = env->GetFieldID( jclass_vrpn_AnalogRemote, "native_analog", "I" );
  if( jfid_vrpn_AnalogRemote_native_analog == NULL )
  {
    printf( "Error loading vrpn AnalogRemote native library "
            "while looking into class vrpn.AnalogRemote.\n" );
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
    printf( "Error unloading vrpn AnalogRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.analogRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_AnalogRemote );
}


// This is the callback for vprn to notify us of a new analog message
void handle_analog_change( void* userdata, const vrpn_ANALOGCB info )
{
  if( jvm == NULL )
    return;

  /*
  printf( "analog change (C):  time:  %d.%d;  num_channels:  %d;\n\t", 
          info.msg_time.tv_sec, info.msg_time.tv_usec, info.num_channel );
  for( int i = 0; i <= info.num_channel - 1; i++ )
    printf( "%d  ", info.channel[i] );
  printf( "\n" );
  */

  JNIEnv* env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid = env->GetMethodID( jcls, "handleAnalogChange", "(JJ[D)V" );
  if( jmid == NULL )
  {
    printf( "Warning:  vrpn_AnalogRemote library was unable to find the "
            "Java method \'handleAnalogChange\'.  This may indicate a version mismatch.\n" );
    return;
  }

  // create an array of the appropriate length
  jdoubleArray jchannels = env->NewDoubleArray( info.num_channel );
  jchannels = (jdoubleArray) env->NewWeakGlobalRef( jchannels );
  if( jchannels == NULL )
  {
    printf( "Error:  vrpn AnalogRemote native library was unable to create a \'channels\' array.\n" );
    return;
  }

  // fill in the array
  if( info.channel == NULL )
  {
    printf( "Error:  vrpn AnalogRemote native library:  we were handed a bad update from vrpn.\n" );
    return;
  }
  env->SetDoubleArrayRegion( jchannels, 0, info.num_channel, (double*) info.channel );


  env->CallVoidMethod( jobj, jmid, (jlong) info.msg_time.tv_sec, (jlong) info.msg_time.tv_usec,
                       (jdoubleArray) jchannels /* something here about the channels array */ );

}



JNIEXPORT jboolean JNICALL 
Java_vrpn_AnalogRemote_requestValueChange( JNIEnv* env, jobject jobj, 
                                           jint jchannel, jdouble jvalue )
{
  if( jchannel < 0 ) 
  {
    return false;
  }
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_analog", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"requestValueChange\":  unable to ID native analog field.\n" );
    return false;
  }

  // get the analog pointer
  vrpn_Analog_Remote* a = (vrpn_Analog_Remote*) env->GetIntField( jobj, jfid );
  if( a <= 0 )  // this analog is uninitialized or has been shut down already
  {
    printf( "Error in native method \"requestValueChange\":  the analog is uninitialized or "
            "has been shut down.\n" );
    return false;
  }

  return a->request_change_channel_value( jchannel, jvalue );
}



JNIEXPORT void JNICALL 
Java_vrpn_AnalogRemote_shutdownAnalog( JNIEnv* env, jobject jobj )
{
  // look up where to store the analog pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_analog", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"shutdownAnalog\":  unable to ID native analog field.\n" );
    return;
  }

  // get the analog pointer
  vrpn_Analog_Remote* a = (vrpn_Analog_Remote*) env->GetIntField( jobj, jfid );
  
  // unregister a handler and destroy the analog
  if( a > 0 )
  {
    a->unregister_change_handler( jobj, handle_analog_change );
   delete a;
  }

  // set the analog pointer to -1
  env->SetIntField( jobj, jfid, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );

}


JNIEXPORT void JNICALL 
Java_vrpn_AnalogRemote_mainloop( JNIEnv *env, jobject jobj )
{
  // look up the analog pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_analog", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"mainloop\":  unable to ID native analog field.\n" );
    return;
  }
  vrpn_Analog_Remote* a = (vrpn_Analog_Remote*) env->GetIntField( jobj, jfid );
  if( a <= 0 )  // this analog is uninitialized or has been shut down already
    return;

  // now call mainloop
  a->mainloop( );
}


JNIEXPORT jboolean JNICALL 
Java_vrpn_AnalogRemote_init( JNIEnv *env, jobject jobj, jstring jname )
{
  printf( "in Java_AnalogRemote_init(...)\n" );

  // look up where to store the analog pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_analog", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"init\":  unable to ID native analog field.\n" );
    return false;
  }

  // make a global reference to the Java AnalogRemote
  // object, so that it can be referenced in the function
  // handle_analog_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the analog
  const char* name = env->GetStringUTFChars( jname, NULL );
  vrpn_Analog_Remote* a = new vrpn_Analog_Remote( name );
  a->register_change_handler( jobj, handle_analog_change );
  env->ReleaseStringUTFChars( jname, name );
  
  // now stash 'a' in the jobj's 'native_analog' field
  jint ja = (jint) a;
  env->SetIntField( jobj, jfid, ja );
  
  return true;
}

