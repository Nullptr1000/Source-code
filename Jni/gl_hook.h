#pragma once
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

// Install chams hooks by patching glDrawElements/glDrawArrays
// GOT slots in every loaded module. Returns slots patched (0 = engine
// doesn't route through PLT/GOT, e.g. Unity's internal dispatch).
int install_gl_hooks();

// Hook entry points (referenced by PltHookSpec, callable for testing).
void hk_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
void hk_glDrawArrays(GLenum mode, GLint first, GLsizei count);

#ifdef __cplusplus
}
#endif