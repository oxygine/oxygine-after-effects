LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := aesdk_static
LOCAL_CFLAGS += -std=c99
SRC := $(LOCAL_PATH)/../../../../../ae-movie/src
LOCAL_SRC_FILES += $(subst $(LOCAL_PATH)/,, $(wildcard $(SRC)/*.c))
LOCAL_C_INCLUDES := $(SRC)/../include/
LOCAL_CFLAGS +=-DAE_MOVIE_STREAM_NO_CACHE
LOCAL_CFLAGS +=-DAE_TIME_DEFINE=1
LOCAL_CFLAGS +=-DAE_TIME_MILLISECOND=1
LOCAL_CFLAGS +=-DAE_MOVIE_SAFE=1

include $(BUILD_STATIC_LIBRARY) 




include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_CFLAGS +=-DAEVIEWER=1

#SDK_ROOT points to folder with SDL and oxygine-framework
LOCAL_SRC_FILES := ../../../../../..//SDL/src/main/android/SDL_android_main.c

LOCAL_SRC_FILES += ../../../src/example.cpp ../../../src/main.cpp ../../../src/test.cpp ../../../src/AEMovieWork.cpp 

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../ae-movie/include/


LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../src
AE_SRC := ../../../../../src/ae
LOCAL_SRC_FILES += $(AE_SRC)/AEMovie.cpp $(AE_SRC)/AERenderer.cpp $(AE_SRC)/AEMovieResource.cpp 


LOCAL_CFLAGS +=-DAE_MOVIE_STREAM_NO_CACHE
LOCAL_CFLAGS +=-DAE_TIME_DEFINE=1
LOCAL_CFLAGS +=-DAE_TIME_MILLISECOND=1
LOCAL_CFLAGS +=-DAE_MOVIE_SAFE=1


LOCAL_STATIC_LIBRARIES := oxygine-framework_static aesdk_static
LOCAL_SHARED_LIBRARIES := SDL2

include $(BUILD_SHARED_LIBRARY)


#import from NDK_MODULE_PATH defined in build.cmd
$(call import-module, oxygine-framework)
