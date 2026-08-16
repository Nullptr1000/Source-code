#define _GNU_SOURCE
#include <link.h>
#include <elf.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include "plt_hook.h"
#include "rlog.h"

#if defined(__aarch64__)
#  define RC_R_JUMP_SLOT R_AARCH64_JUMP_SLOT   /* 1026 */
#  define RC_R_GLOB_DAT  R_AARCH64_GLOB_DAT    /* 1025 */
#elif defined(__arm__)
#  define RC_R_JUMP_SLOT R_ARM_JUMP_SLOT       /*  22  */
#  define RC_R_GLOB_DAT  R_ARM_GLOB_DAT        /*  21  */
#else
#  error "plt_hook: only arm/arm64 supported"
#endif

#if defined(__LP64__)
#  define RC_R_SYM(x)  ELF64_R_SYM(x)
#  define RC_R_TYPE(x) ELF64_R_TYPE(x)
#else
#  define RC_R_SYM(x)  ELF32_R_SYM(x)
#  define RC_R_TYPE(x) ELF32_R_TYPE(x)
#endif

struct Ctx { PltHookSpec* specs; size_t n; int patched; };

static int patch_relas(const ElfW(Rela)* rela, size_t count,
                       uintptr_t base, uintptr_t symtab, uintptr_t strtab, Ctx* ctx) {
    int patched = 0;
    for (size_t i = 0; i < count; i++) {
        uint32_t type = RC_R_TYPE(rela[i].r_info);
        if (type != RC_R_JUMP_SLOT && type != RC_R_GLOB_DAT) continue;

        uint32_t sidx = RC_R_SYM(rela[i].r_info);
        const ElfW(Sym)* sym = &((const ElfW(Sym)*)symtab)[sidx];
        const char* name = (const char*)(strtab + sym->st_name);

        for (size_t k = 0; k < ctx->n; k++) {
            PltHookSpec& s = ctx->specs[k];
            if (strcmp(name, s.name) != 0) continue;

            uintptr_t* got = (uintptr_t*)(base + rela[i].r_offset);
            void* real = dlsym(RTLD_DEFAULT, s.name);
            if (!real) continue;
            // Only hijack slots still bound to the real export
            // (skips already-hooked or interposed entries).
            if (*got != (uintptr_t)real) continue;

            __atomic_store_n(got, (uintptr_t)s.hook, __ATOMIC_SEQ_CST);
            if (s.real) *s.real = real;
            s.patched++;
            patched++;
        }
    }
    return patched;
}

static int cb(struct dl_phdr_info* info, size_t, void* data) {
    Ctx* ctx = (Ctx*)data;
    if (info->dlpi_name && strstr(info->dlpi_name, "rainbow")) return 0; // skip self

    const ElfW(Phdr)* dyn = nullptr;
    for (ElfW(Half) i = 0; i < info->dlpi_phnum; i++) {
        if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) { dyn = &info->dlpi_phdr[i]; break; }
    }
    if (!dyn) return 0;

    uintptr_t base = info->dlpi_addr;
    const ElfW(Dyn)* d = (const ElfW(Dyn)*)(base + dyn->p_vaddr);
    uintptr_t symtab = 0, strtab = 0, jmprel = 0, rela = 0;
    size_t jmprelsz = 0, relasz = 0, relaent = sizeof(ElfW(Rela));

    for (; d->d_tag != DT_NULL; d++) {
        switch (d->d_tag) {
            case DT_SYMTAB:   symtab   = base + d->d_un.d_ptr; break;
            case DT_STRTAB:   strtab   = base + d->d_un.d_ptr; break;
            case DT_JMPREL:   jmprel   = base + d->d_un.d_ptr; break;
            case DT_PLTRELSZ: jmprelsz = d->d_un.d_val;        break;
            case DT_RELA:     rela     = base + d->d_un.d_ptr; break;
            case DT_RELASZ:   relasz   = d->d_un.d_val;        break;
            case DT_RELAENT:  if (d->d_un.d_val) relaent = d->d_un.d_val; break;
            default: break;
        }
    }
    if (!symtab || !strtab) return 0;

    int n = 0;
    if (jmprel && jmprelsz)
        n += patch_relas((const ElfW(Rela)*)jmprel, jmprelsz / relaent,
                         base, symtab, strtab, ctx);
    if (rela && relasz)
        n += patch_relas((const ElfW(Rela)*)rela, relasz / relaent,
                         base, symtab, strtab, ctx);
    ctx->patched += n;
    return 0;
}

int plt_hook_install(PltHookSpec* specs, size_t n) {
    Ctx ctx{specs, n, 0};
    dl_iterate_phdr(cb, &ctx);
    if (!ctx.patched) LOGW("plt_hook: no slots patched (engine may use dlsym/vtable)");
    return ctx.patched;
}