#pragma once
#include <stdint.h>
#include <vector>

namespace mem {
struct Region { uintptr_t start, end; };

void scan_f32(float value, float eps);        // full scan over all RW maps
void refine(int op, float value);             // 0=eq 1=ne 2=gt 3=lt 4=ge 5=le
std::vector<uintptr_t> results();
void clear();
void write_f32(uintptr_t addr, float value);
void write_i32(uintptr_t addr, int32_t value);
}