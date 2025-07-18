#pragma once
#include "perf.h"
#include "stdlib.h"
#include <Arduino.h>

FORCE_INLINE float saturate_soft(float x) {
    return x / (1.0f + fabsf(x));
}

FORCE_INLINE float saturate_hard(float x) {
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}


FORCE_INLINE float volume_to_gain(float volume) {
    // static const float MIN_DB = -60.0f;  // silence
    // volume = constrain(volume, 0.0f, 1.0f);
    // float db = MIN_DB + (0.0f - MIN_DB) * volume;  // linear fade in dB
    // return std::pow(10.0f, db / 20.0f);
    // return volume*volume*volume*volume;  // x^4

    return volume*volume*volume;  // x^3
}
