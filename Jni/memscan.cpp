#include "memscan.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <mutex>

namespace mem {
static std::mutex g_mtx;
static std::vector<uintptr_t> g_res;
static std::vector<Region> g_regions;

static void load_regions() {
    g_regions.clear();
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned long long s = 0, e = 0;
        char perms[8] = {0};
        if (sscanf(line, "%llx-%llx %7s", &s, &e, perms) == 3 &&
            perms[0] == 'r' && perms[1] == 'w' && (e - s) >= 0x1000) {
            g_regions.push_back({(uintptr_t)s, (uintptr_t)e});
        }
    }
    fclose(f);
}

void scan_f32(float value, float eps) {
    std::lock_guard<std::mutex> lk(g_mtx);
    load_regions();
    g_res.clear();
    for (const Region& r : g_regions) {
        const uint8_t* p = (const uint8_t*)r.start;
        size_t n = r.end - r.start;
        for (size_t i = 0; i + 4 <= n; i += 4) {
            float v; memcpy(&v, p + i, 4);
            if (fabsf(v - value) <= eps) g_res.push_back(r.start + i);
        }
    }
}

void refine(int op, float value) {
    std::lock_guard<std::mutex> lk(g_mtx);
    std::vector<uintptr_t> keep;
    keep.reserve(g_res.size());
    for (uintptr_t a : g_res) {
        float v; memcpy(&v, (void*)a, 4);
        bool ok = false;
        switch (op) {
            case 0: ok = fabsf(v - value) <= 1e-3f; break;
            case 1: ok = fabsf(v - value) >  1e-3f; break;
            case 2: ok = v >  value; break;
            case 3: ok = v <  value; break;
            case 4: ok = v >= value; break;
            case 5: ok = v <= value; break;
        }
        if (ok) keep.push_back(a);
    }
    g_res.swap(keep);
}

std::vector<uintptr_t> results() { std::lock_guard<std::mutex> lk(g_mtx); return g_res; }
void clear() { std::lock_guard<std::mutex> lk(g_mtx); g_res.clear(); }

static void unprotect(uintptr_t addr) {
    mprotect((void*)(addr & ~(uintptr_t)0xFFF), 0x1000, PROT_READ | PROT_WRITE);
}

void write_f32(uintptr_t addr, float value) { unprotect(addr); *(volatile float*)addr = value; }
void write_i32(uintptr_t addr, int32_t value) { unprotect(addr); *(volatile int32_t*)addr = value; }
}