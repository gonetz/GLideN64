###########
# osal
###########
include $(CLEAR_VARS)
LOCAL_PATH := $(JNI_LOCAL_PATH)
SRCDIR := ./mupen64plus-video-gliden64/src/osal

LOCAL_MODULE := osal
#LOCAL_STATIC_LIBRARIES := png
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES :=                     \
    $(LOCAL_PATH)/$(SRCDIR)             \

LOCAL_SRC_FILES :=                          \
    $(SRCDIR)/osal_files_unix.c        \

LOCAL_CFLAGS :=         \
    $(COMMON_CFLAGS)    \
    -g                  \
    -DANDROID           \
    -fsigned-char       \
    #-DDEBUG             \
    #-DSDL_NO_COMPAT     \

LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS) -std=c++11 -g -fexceptions

include $(BUILD_STATIC_LIBRARY)
