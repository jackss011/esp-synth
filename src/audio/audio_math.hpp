#pragma once
#include "perf.h"

FORCE_INLINE float saturate_soft(float x) {
    return x / (1.0f + fabsf(x));
}

FORCE_INLINE float saturate_hard(float x) {
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}
