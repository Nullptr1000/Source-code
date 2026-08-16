#pragma once
#include <android/log.h>

#define RC_TAG "RainbowChams"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  RC_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  RC_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, RC_TAG, __VA_ARGS__)

namespace rc {
struct Config {
    int enabled  = 1;     // 0 = all hooks pass-through
    int mode     = 2;     // 0=off  1=chams only  2=chams + rainbow tint
    int retry_ms = 2000;  // GL hook retry interval
};
void load_config();           // reads /sdcard/RC/rc.conf
void set_mode(int m);
const Config& cfg();
}