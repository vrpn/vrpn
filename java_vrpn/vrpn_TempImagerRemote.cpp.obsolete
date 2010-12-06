
#include "stdio.h"
#include "jni.h"
#include "vrpn_TempImager.h"

#include "java_vrpn.h"
#include "vrpn_TempImagerRemote.h"


jclass jclass_vrpn_TempImagerRemote = NULL;
jfieldID jfid_vrpn_TempImagerRemote_native_tempImager = NULL;


//////////////////////////
//  DLL functions


// This is called when the Java Virtual Machine loads this library
//  and sets some global references that are used elsewhere.
JNIEXPORT jint JNICALL JNI_OnLoad_TempImagerRemote( JavaVM* jvm, void* reserved )
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
    printf( "Error loading vrpn TempImagerRemote native library.\n" );
    return JNI_ERR;
  }
  
  ///////////////
  // get the jclass reference to vrpn.TempImagerRemote
  jclass cls = env->FindClass( "vrpn/TempImagerRemote" );
  if( cls == NULL )
  {
    printf( "Error loading vrpn TempImagerRemote native library "
            "while trying to find class vrpn.TempImagerRemote.\n" );
    return JNI_ERR;
  }

  // make a global reference so the class can be referenced later.
  // this must be a _WEAK_ global reference, so the class will be
  // garbage collected, and so the JNI_OnUnLoad handler will be called
  jclass_vrpn_TempImagerRemote = (jclass) env->NewWeakGlobalRef( cls );
  if( jclass_vrpn_TempImagerRemote == NULL )
  {
    printf( "Error loading vrpn TempImagerRemote native library "
            "while setting up class vrpn.TempImagerRemote.\n" );
    return JNI_ERR;
  }

  ////////////////
  // get a jfid field id reference to the "native_tempImager" 
  // field of class vrpn.TempImagerRemote.
  // field ids do not have to be made into global references.
  jfid_vrpn_TempImagerRemote_native_tempImager 
    = env->GetFieldID( jclass_vrpn_TempImagerRemote, "native_tempImager", "I" );
  if( jfid_vrpn_TempImagerRemote_native_tempImager == NULL )
  {
    printf( "Error loading vrpn TempImagerRemote native library "
            "while looking into class vrpn.TempImagerRemote.\n" );
    return JNI_ERR;
  }
  
  return JAVA_VRPN_JNI_VERSION;
} // end JNI_OnLoad



JNIEXPORT void JNICALL JNI_OnUnload_TempImagerRemote( JavaVM* jvm, void* reserved )
{
  // get the JNIEnv pointer
  JNIEnv* env;
  if( jvm->GetEnv( (void**) &env, JAVA_VRPN_JNI_VERSION ) != JNI_OK )
  {
    printf( "Error unloading vrpn TempImagerRemote native library.\n" );
    return;
  }

  // delete the global reference to the vrpn.TempImagerRemote class
  env->DeleteWeakGlobalRef( jclass_vrpn_TempImagerRemote );
}

// end DLL functions
/////////////////////////////


////////////////////////////
// dll utility functions


void VRPN_CALLBACK java_vrpn_handle_region_change( void * userdata, const vrpn_IMAGERREGIONCB info )
{
  if( jvm == NULL )
    return;
   
  //printf( "region change (C):  time:  %d.%d;\n"
          //"\tchannel:  %d\n"
          //"\trows [%d,%d]  cols[%d,%d]\n",
          //info.msg_time.tv_sec, info.msg_time.tv_usec,
          //info.region->chanIndex, info.region->rMin, info.region->rMax,
          //info.region->cMin, info.region->cMax );

  if( info.region->d_valType != vrpn_IMAGER_VALTYPE_UINT16 )
  {
	  printf( "Error in java_vrpn java_vrpn_handle_region_change:  this has "
			  "only been written for 16-bit image data, and someone's sending "
			  "something different.\n" );
	  return;
  }


  JNIEnv*env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  jmethodID jmid_handler = env->GetMethodID( jcls, "handleRegionChange", "(JJIIIII)V" );
  if( jmid_handler == NULL )
  {
    printf( "Warning:  vrpn_TempImageRemote library was unable to find the "
            "Java method \'handleRegionChange\'.  This may indicate a version mismatch.\n" );
    return;
  }

  // set the 'val' array
  jfieldID jvalfid = env->GetFieldID( jcls, "regionValues", "[S" );
  if( jvalfid == NULL )
  {
    printf( "Warning:  vrpn_TempImageRemote library was unable to find the "
            "Java field ID for \'regionValues\'.  This may indicate a version mismatch.\n" );
    return;
  }
  jshortArray jvals = (jshortArray) env->GetObjectField( jobj, jvalfid );
  if( jvals == NULL )
  {
	  printf( "Warning:  vrpn_TempImageRemote library was unable to find the "
            "Java field \'regionValues\'.  This may indicate a version mismatch.\n" );
    return;
  }
  env->SetShortArrayRegion( jvals, 0, info.region->getNumVals(), 
							(short*) info.region->d_valBuf );


  // now call the handler method
  env->CallVoidMethod( jobj, jmid_handler, (jlong) info.msg_time.tv_sec, 
                       (jlong) info.msg_time.tv_usec, (jint) info.region->d_chanIndex,
                       (jint) info.region->d_cMin, 
                       (jint) info.region->d_cMax, 
                       (jint) info.region->d_rMin, (jint) info.region->d_rMax );

} // end java_vrpn_handle_region_change(...)


void VRPN_CALLBACK java_vrpn_handle_description_change( void * userdata, const struct timeval msg_time )
{
  
  if( jvm == NULL )
    return;
   
  printf( "description change (C):  time:  %d.%d;\n",
          msg_time.tv_sec, msg_time.tv_usec );

  JNIEnv*env;
  jvm->AttachCurrentThread( (void**) &env, NULL );
  
  // go get all the various java entities that we'll need
  jobject jobj = (jobject) userdata;
  jclass jcls = env->GetObjectClass( jobj );
  
  jmethodID jmid_handler = env->GetMethodID( jcls, "handleDescriptionChange", "(JJ)V" );
  if( jmid_handler == NULL )
  {
    printf( "Warning:  vrpn_TempImageRemote library was unable to find the "
            "Java method \'handleDescriptionChange\'.  This may indicate a version mismatch.\n" );
    return;
  }
  
  jmethodID jmid_setupDescription 
	  = env->GetMethodID( jcls, "setupTempImagerDescription", "(IIFFFFI)V" );
  if( jmid_setupDescription == NULL )
  {
    printf( "Warning:  vrpn_TempImagerRemote library was unable to find the "
            "Java method \'setupTempImagerDescription\'.  This may indicate a version mismatch.\n" );
    return;
  }
  
  jmethodID jmid_setChannel = env->GetMethodID( jcls, "setChannel", 
												"(ILjava/lang/String;Ljava/lang/String;FFFF)V" );
  if( jmid_setChannel == NULL )
  {
    printf( "Warning:  vrpn_TempImageRemote library was unable to find the "
            "Java method \'setChannel\'.  This may indicate a version mismatch.\n" );
    return;
  }
  
  jfieldID jfid_imagerptr = env->GetFieldID( jcls, "native_tempImager", "I" );
  if( jfid_imagerptr == NULL )
  {
    printf( "Warning:  vrpn_TempImagerRemote library was unable to ID the "
            "native TempImager in \'java_vrpn_handle_description_change\'.  "
			"This may indicate a version mismatch.\n" );
    return;
  }
  vrpn_TempImager_Remote* t = (vrpn_TempImager_Remote*) env->GetIntField( jobj, jfid_imagerptr );
  if( t <= 0 )
  {
    printf( "Warning:  the native vrpn TempImager was uninitialized or had already "
            "been shut down in \'java_vrpn_handle_description_change\'.\n" );
    return;
  }

  // end getting all the java entities
  
  // set up all the new description
  int numChannels = t->nChannels();
  env->CallVoidMethod( jobj, jmid_setupDescription, (jint) t->nRows(), (jint) t->nCols(),
					   (jfloat) t->minX(), (jfloat) t->maxX(), (jfloat) t->minY(),
					   (jfloat) t->maxY(), (jint) numChannels );

  for( int i = 0; i<= numChannels - 1; i++ )
  {
	  const vrpn_TempImager_Channel* channel = t->channel(i);
	  if( channel == NULL )
	  {
		  printf( "Warning:  invalid channel %d in "
			  "native TempImager in \'java_vrpn_handle_description_change\'.\n" );
		  continue;
	  }
	  env->CallVoidMethod( jobj, jmid_setChannel, (jint) i, 
						   env->NewStringUTF( channel->name ), 
						   env->NewStringUTF( channel->units ),
						   (jfloat) t->channel(i)->minVal, (jfloat) t->channel(i)->maxVal,
						   (jfloat) t->channel(i)->offset, (jfloat) t->channel(i)->scale );
  }

  // now call the java handler
   env->CallVoidMethod( jobj, jmid_handler, (jlong) msg_time.tv_sec, 
                       (jlong) msg_time.tv_usec );

} // end java_vrpn_handle_description_change(...)


// end dll utility functions
////////////////////////////



/*
 * Class:     vrpn_TempImagerRemote
 * Method:    init
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL 
Java_vrpn_TempImagerRemote_init( JNIEnv* env, jobject jobj, jstring jname)
{

  // look up where to store the tracker pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tempImager", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"init\":  unable to ID native TempImager field.\n" );
    return false;
  }

  // make a global reference to the Java TempImageRemote
  // object, so that it can be referenced in the function
  // handle_tracker_change(...)
  jobj = env->NewGlobalRef( jobj );

  // create the tracker
  const char* name = env->GetStringUTFChars( jname, NULL );
  vrpn_TempImager_Remote* t = new vrpn_TempImager_Remote( name );
  t->register_region_handler( jobj, java_vrpn_handle_region_change );
  t->register_description_handler( jobj, java_vrpn_handle_description_change );
  env->ReleaseStringUTFChars( jname, name );
  
  // now stash 't' in the jobj's 'native_tempImager' field
  jint jt = (jint) t;
  env->SetIntField( jobj, jfid, jt );
  
  return true;
}


/*
 * Class:     vrpn_TempImagerRemote
 * Method:    mainloop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_TempImagerRemote_mainloop( JNIEnv* env, jobject jobj )
{
  // look up the tempImager pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tempImager", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"mainloop\":  unable to ID native tempImager field.\n" );
    return;
  }
  vrpn_TempImager_Remote* t = (vrpn_TempImager_Remote*) env->GetIntField( jobj, jfid );
  if( t <= 0 )  // this tempImager is uninitialized or has been shut down already
    return;

  // now call mainloop
  t->mainloop( );
}


/*
 * Class:     vrpn_TempImagerRemote
 * Method:    shutdownTempImager
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_vrpn_TempImagerRemote_shutdownTempImager( JNIEnv* env, jobject jobj )
{

  // look up where to store the tempImager pointer
  jclass jcls = env->GetObjectClass( jobj );
  jfieldID jfid = env->GetFieldID( jcls, "native_tempImager", "I" );
  if( jfid == NULL )
  {
    printf( "Error in native method \"shutdownTempImager\":  "
            "unable to ID native tempImager field.\n" );
    return;
  }

  // get the tempImager pointer
  vrpn_TempImager_Remote* t = (vrpn_TempImager_Remote*) env->GetIntField( jobj, jfid );
  
  // unregister handlers and destroy the tempImager
  if( t > 0 )
  {
    t->unregister_region_handler( jobj, java_vrpn_handle_region_change );
    t->unregister_description_handler( jobj, java_vrpn_handle_description_change );
    delete t;
  }

  // set the tempImager pointer to -1
  env->SetIntField( jobj, jfid, -1 );

  // delete global reference to object (that was created in init)
  env->DeleteGlobalRef( jobj );


}


