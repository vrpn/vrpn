LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := vrpn

LOCAL_SRC_FILES := \
	../../../vrpn_Analog.c \
	../../../vrpn_Button.c \
	../../../vrpn_BaseClass.c \
	../../../vrpn_Connection.c \
	../../../vrpn_FileConnection.c \
	../../../vrpn_Serial.c \
	../../../vrpn_Shared.c \
	../../../vrpn_Android.c \
	../../../jni_layer.cpp \
	
include $(BUILD_SHARED_LIBRARY)
