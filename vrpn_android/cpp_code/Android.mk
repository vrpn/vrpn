LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := vrpn

LOCAL_SRC_FILES := \
	vrpn_Connection.c \
	vrpn_BaseClass.c \
	vrpn_Button.c \
	vrpn_Event.c \
	vrpn_Shared.c \
	vrpn_Analog.c \
	vrpn_Tracker.c \
	vrpn_Analog_Output.c \
	vrpn_Imager.c \
	vrpn_FileConnection.c \
	vrpn_Serial.c \
	vrpn_Android.c \
	jni_layer.cpp \
	
include $(BUILD_SHARED_LIBRARY)