#/* [Android Makefile] */
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := Native
LOCAL_SRC_FILES := AstcTextures.cpp Matrix.cpp Text.cpp Timer.cpp SolidSphere.cpp
LOCAL_LDLIBS    := -llog -lGLESv3

include $(BUILD_SHARED_LIBRARY)
#/* [Android Makefile] */
