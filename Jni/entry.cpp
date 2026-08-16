#include "rlog.h"
#include "gl_hook.h"
#include "memscan.h"
#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <jni.h>

static std::atomic<bool> g_started{false};

static void* hook_thread(void*) {
    rc::load_config();
    int retries = 15;                       // retry until the engine's GL module is loaded
    while (retries-- > 0) {
        if (install_gl_hooks() > 0) break;
        usleep((useconds_t)rc::cfg().retry_ms * 1000);
    }
    return nullptr;
}

static void ensure_start() {
    bool expect = false;
    if (g_started.compare_exchange_strong(expect, true)) {
        pthread_t t;
        pthread_create(&t, nullptr, hook_thread, nullptr);
        pthread_detach(t);
    }
}

// Runs the moment dlopen/loadLibrary brings this library in.
__attribute__((constructor)) static void rc_ctor() { ensure_start(); }

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    ensure_start();
    return JNI_VERSION_1_6;
}

/* ---------------- JNI: memory scanner + mode switch ---------------- */

extern "C" JNIEXPORT void JNICALL
Java_com_rc_Native_scanF32(JNIEnv*, jobject, jfloat value, jfloat eps) {
    mem::scan_f32(value, eps);
}

extern "C" JNIEXPORT void JNICALL
Java_com_rc_Native_refine(JNIEnv*, jobject, jint op, jfloat value) {
    mem::refine(op, value);
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_rc_Native_results(JNIEnv* env, jobject) {
    std::vector<uintptr_t> res = mem::results();
    jlongArray arr = env->NewLongArray((jsize)res.size());
    if (!res.empty())
        env->SetLongArrayRegion(arr, 0, (jsize)res.size(), (const jlong*)res.data());
    return arr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_rc_Native_writeF32(JNIEnv*, jobject, jlong addr, jfloat value) {
    mem::write_f32((uintptr_t)addr, value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_rc_Native_writeI32(JNIEnv*, jobject, jlong addr, jint value) {
    mem::write_i32((uintptr_t)addr, value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_rc_Native_toggle(JNIEnv*, jobject, jint mode) {
    rc::set_mode(mode);   // 0=off 1=chams 2=chams+rainbow
}