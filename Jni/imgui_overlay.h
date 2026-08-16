#pragma once
#include <android/native_window.h>
extern "C" void rc_imgui_init(ANativeWindow* window);
extern "C" void rc_imgui_shutdown();
extern "C" void rc_imgui_frame(ANativeWindow* window, int enabled, int mode);
