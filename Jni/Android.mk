LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := rainbow
LOCAL_SRC_FILES := entry.cpp key_system.cpp plt_hook.cpp gl_hook.cpp memscan.cpp rlog.cpp \
    imgui_overlay.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_android.cpp imgui/backends/imgui_impl_opengl3.cpp
LOCAL_CFLAGS    := -O2 -fvisibility=hidden -ffunction-sections -fdata-sections \
    -I$(LOCAL_PATH)/imgui -I$(LOCAL_PATH)/imgui/backends
LOCAL_CPPFLAGS  := -std=c++17 -fexceptions -frtti
LOCAL_LDLIBS    := -llog -lGLESv2 -ldl -lm
LOCAL_ARM_MODE  := arm
LOCAL_LDFLAGS   := -Wl,--gc-sections
include $(BUILD_SHARED_LIBRARY)