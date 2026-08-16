#include "gl_hook.h"
#include "plt_hook.h"
#include "rlog.h"
#include <GLES2/gl2.h>
#include <math.h>
#include <string.h>
#include <time.h>

static void* real_glDrawElements = nullptr;
static void* real_glDrawArrays   = nullptr;

static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void hsv2rgb(float h, float s, float v, float& r, float& g, float& b) {
    float c = v * s;
    float hp = fmodf(h, 360.f) / 60.f;
    float x = c * (1.f - fabsf(fmodf(hp, 2.f) - 1.f));
    float r1 = 0, g1 = 0, b1 = 0;
    switch ((int)hp % 6) {
        case 0: r1 = c; g1 = x; break;
        case 1: r1 = x; g1 = c; break;
        case 2: g1 = c; b1 = x; break;
        case 3: g1 = x; b1 = c; break;
        case 4: r1 = x; b1 = c; break;
        case 5: r1 = c; b1 = x; break;
    }
    float m = v - c;
    r = r1 + m; g = g1 + m; b = b1 + m;
}

// TODO(RE): this tints whatever shader is active. To target ONLY players,
// log GL_CURRENT_PROGRAM + bound VAO during a match and return early
// here unless the program passes your allowlist (see README).
static void apply_rainbow_tint() {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (!prog) return;

    GLint n = 0;
    glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &n);
    if (n <= 0) return;

    GLint loc = -1, fallback = -1;
    for (GLint i = 0; i < n; i++) {
        char name[128];
        GLsizei len = 0; GLint sz = 0; GLenum ty = 0;
        glGetActiveUniform(prog, (GLuint)i, (GLsizei)sizeof(name), &len, &sz, &ty, name);
        if (ty != GL_FLOAT_VEC4 && ty != GL_FLOAT_VEC3 && ty != GL_FLOAT_VEC2) continue;
        GLint l = glGetUniformLocation(prog, name);
        if (l < 0) continue;
        if (strstr(name, "color") || strstr(name, "Color") ||
            strstr(name, "tint")  || strstr(name, "Tint")) { loc = l; break; }
        if (fallback < 0) fallback = l;
    }
    if (loc < 0) loc = fallback;
    if (loc < 0) return;

    float h = fmodf((float)now_sec() * 90.f, 360.f);   // ~4s full cycle
    float r, g, b;
    hsv2rgb(h, 1.f, 1.f, r, g, b);
    glUniform4f(loc, r, g, b, 1.f);
}

static void chams_enter() {
    glDepthFunc(GL_ALWAYS);    // draw regardless of depth buffer -> see through walls
    glDepthMask(GL_FALSE);     // don't write depth -> models don't occlude each other
    glDisable(GL_CULL_FACE);   // render backfaces too
}

static void chams_exit() {
    glDepthFunc(GL_LESS);      // typical engine default; adjust if needed
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

extern "C" void hk_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    const bool on = rc::cfg().enabled && rc::cfg().mode > 0;
    if (on) {
        chams_enter();
        if (rc::cfg().mode >= 2) apply_rainbow_tint();
    }
    ((void(*)(GLenum, GLsizei, GLenum, const void*))real_glDrawElements)(mode, count, type, indices);
    if (on) chams_exit();
}

extern "C" void hk_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    const bool on = rc::cfg().enabled && rc::cfg().mode > 0;
    if (on) {
        chams_enter();
        if (rc::cfg().mode >= 2) apply_rainbow_tint();
    }
    ((void(*)(GLenum, GLint, GLsizei))real_glDrawArrays)(mode, first, count);
    if (on) chams_exit();
}

int install_gl_hooks() {
    static PltHookSpec specs[] = {
        {"glDrawElements", (void*)hk_glDrawElements, &real_glDrawElements, 0},
        {"glDrawArrays",   (void*)hk_glDrawArrays,   &real_glDrawArrays,   0},
    };
    int n = plt_hook_install(specs, 2);
    LOGI("installed %d GL hook slot(s)", n);
    return n;
}