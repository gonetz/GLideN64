LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := GLideN64es
LOCAL_SHARED_LIBRARIES := ae-imports SDL2
LOCAL_STATIC_LIBRARIES := cpufeatures
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES :=         \
    $(M64P_API_INCLUDES)    \
    $(SDL_INCLUDES)         \
    $(AE_BRIDGE_INCLUDES)   \

LOCAL_SRC_FILES :=                  \
	2xSAI.cpp		\
	3DMath.cpp		\
	Combiner.cpp		\
	Config_mupen.cpp	\
	CRC.cpp			\
	DepthBuffer.cpp		\
	F3D.cpp			\
	F3DDKR.cpp		\
	F3DEX2.cpp		\
	F3DEX.cpp		\
	F3DPD.cpp		\
	F3DWRUS.cpp		\
	FrameBuffer.cpp		\
	GBI.cpp			\
	gDP.cpp			\
	GLideN64.cpp		\
	GLSLCombiner.cpp	\
	gSP.cpp			\
	L3D.cpp			\
	L3DEX2.cpp		\
	L3DEX.cpp		\
	N64.cpp			\
	OpenGL.cpp		\
	RDP.cpp			\
	RSP.cpp			\
	S2DEX2.cpp		\
	S2DEX.cpp		\
	Textures.cpp		\
	VI.cpp			\

LOCAL_CFLAGS :=         \
    $(COMMON_CFLAGS)    \
    -D__CRC_OPT         \
    -D__HASHMAP_OPT     \
    -D__TRIBUFFER_OPT   \
    -D__VEC4_OPT        \
    -DANDROID           \
    -DUSE_SDL           \
    -DGLES2             \
    -DMUPENPLUSAPI      \
    -fsigned-char       \
    #-DSDL_NO_COMPAT     \
    
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
    
LOCAL_LDFLAGS := -Wl,-version-script,$(LOCAL_PATH)/video_api_export.ver

LOCAL_LDLIBS :=         \
    -lGLESv2            \
    -llog               \

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    # Use for ARM7a:
    #LOCAL_SRC_FILES += gSPNeon.cpp.neon
    #LOCAL_SRC_FILES += 3DMathNeon.cpp.neon 
    LOCAL_CFLAGS += -DARM_ASM
    LOCAL_CFLAGS += -D__NEON_OPT

else ifeq ($(TARGET_ARCH_ABI), armeabi)
    # Use for pre-ARM7a:

else ifeq ($(TARGET_ARCH_ABI), x86)
    # TODO: set the proper flags here

else
    # Any other architectures that Android could be running on?

endif

include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)
