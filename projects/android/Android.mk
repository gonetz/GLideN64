LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
SRCDIR := ../../src

LOCAL_MODULE := GLideN64es
LOCAL_SHARED_LIBRARIES := ae-imports SDL2 core
LOCAL_STATIC_LIBRARIES := cpufeatures
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES :=         \
    $(M64P_API_INCLUDES)    \
    $(SDL_INCLUDES)         \
    $(AE_BRIDGE_INCLUDES)   \
    $(LOCAL_PATH)/$(SRCDIR)/inc        \

LOCAL_SRC_FILES :=			\
	$(SRCDIR)/3DMath.cpp		\
	$(SRCDIR)/Combiner.cpp		\
	$(SRCDIR)/CommonPluginAPI.cpp	\
	$(SRCDIR)/Config.cpp		\
	$(SRCDIR)/CRC.cpp		\
	$(SRCDIR)/DepthBuffer.cpp	\
	$(SRCDIR)/F3D.cpp		\
	$(SRCDIR)/F3DDKR.cpp		\
	$(SRCDIR)/F3DEX2CBFD.cpp	\
	$(SRCDIR)/F3DEX2.cpp		\
	$(SRCDIR)/F3DEX.cpp		\
	$(SRCDIR)/F3DPD.cpp		\
	$(SRCDIR)/F3DSWSE.cpp		\
	$(SRCDIR)/F3DWRUS.cpp		\
	$(SRCDIR)/FrameBuffer.cpp	\
	$(SRCDIR)/GBI.cpp		\
	$(SRCDIR)/gDP.cpp		\
	$(SRCDIR)/GLideN64.cpp		\
	$(SRCDIR)/GLSLCombiner.cpp	\
	$(SRCDIR)/glState.cpp		\
	$(SRCDIR)/gSP.cpp		\
	$(SRCDIR)/Keys.cpp		\
	$(SRCDIR)/L3D.cpp		\
	$(SRCDIR)/L3DEX2.cpp		\
	$(SRCDIR)/L3DEX.cpp		\
	$(SRCDIR)/MupenPlusPluginAPI.cpp \
	$(SRCDIR)/N64.cpp		\
	$(SRCDIR)/OpenGL.cpp		\
	$(SRCDIR)/PostProcessor.cpp	\
	$(SRCDIR)/RDP.cpp		\
	$(SRCDIR)/RSP.cpp		\
	$(SRCDIR)/S2DEX2.cpp		\
	$(SRCDIR)/S2DEX.cpp		\
	$(SRCDIR)/TextDrawer.cpp	\
	$(SRCDIR)/Textures.cpp		\
	$(SRCDIR)/Turbo3D.cpp		\
	$(SRCDIR)/VI.cpp		\
	$(SRCDIR)/ZSort.cpp		\
	$(SRCDIR)/common/CommonAPIImpl_common.cpp	\
	$(SRCDIR)/mupenplus/CommonAPIImpl_mupenplus.cpp	\
	$(SRCDIR)/mupenplus/Config_mupenplus.cpp	\
	$(SRCDIR)/mupenplus/MupenPlusAPIImpl.cpp	\
	$(SRCDIR)/mupenplus/OpenGL_mupenplus.cpp	\

LOCAL_CFLAGS :=         \
    $(COMMON_CFLAGS)    \
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
