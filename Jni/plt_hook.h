#pragma once
#include <stddef.h>

struct PltHookSpec {
    const char* name;    // symbol to hijack, e.g. "glDrawElements"
    void*       hook;    // replacement function
    void**      real;    // out: saved real address
    int         patched; // out: how many slots were rewritten
};

// Patch every matching GOT slot in every loaded module. Returns total patches.
int plt_hook_install(PltHookSpec* specs, size_t n);