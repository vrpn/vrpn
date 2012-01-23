#include "jni_layer.h"
#include "vrpn_Android.h"

/*
 * This is JNI code for interaction between the Java and C++ components.
 * The JniLayer Java class stores a pointer to a vrpn_Android_Server instance
 * in C++ in the form of a long integer.  This long is passed to each of these
 * functions, in addition to any necessary parameters.  For the most part, the
 * functions here correspond with functions in vrpn_Android_Server, so this 
 * code simply interprets the long as a pointer to the vrpn_Android_Server and
 * calls the corresponding function.
 */

/*
 * Class:     jni_JniLayer
 * Method:    jni_layer_initialize
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_jni_JniLayer_jni_1layer_1initialize
  (JNIEnv * env, jobject, jintArray analogs, jint num_buttons, jint port)
{
	// convert jintArray into a c++ int array
	int num_analogs = env->GetArrayLength(analogs);
	int * analog_sizes = env->GetIntArrayElements(analogs, 0);
	vrpn_Android_Server * theServer = new vrpn_Android_Server(num_analogs, analog_sizes, num_buttons, port);	
	return (long) theServer;
}

/*
 * Class:     jni_JniLayer
 * Method:    jni_layer_mainloop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_jni_JniLayer_jni_1layer_1mainloop
  (JNIEnv *, jobject, jlong ptr)
{
	((vrpn_Android_Server *) ptr)->mainloop();
}

/*
 * Class:     jni_JniLayer
 * Method:    jni_layer_update_button
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_jni_JniLayer_jni_1layer_1set_1button
  (JNIEnv *, jobject, jlong ptr, jint button_id, jint state)
{
	((vrpn_Android_Server *) ptr)->set_button(button_id, state);
}

/*
 * Class:     jni_JniLayer
 * Method:    jni_layer_set_analog
 * Signature: (JIF)V
 */
JNIEXPORT void JNICALL Java_jni_JniLayer_jni_1layer_1set_1analog
  (JNIEnv *, jobject, jlong ptr, jint analog_id, jint channel, jfloat val)
{
	((vrpn_Android_Server *) ptr)->set_analog(analog_id, channel, val);
}

JNIEXPORT void JNICALL Java_jni_JniLayer_jni_1layer_1kill
  (JNIEnv *, jobject, jlong ptr)
{
	delete ((vrpn_Android_Server *) ptr);
}

/*
 * Class:     jni_JniLayer
 * Method:    jni_layer_report_analog_chg
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_jni_JniLayer_jni_1layer_1report_1analog_1chg
  (JNIEnv *, jobject, jlong ptr, jint analog_id)
{
	((vrpn_Android_Server *) ptr)->report_analog_chg(analog_id);
}
