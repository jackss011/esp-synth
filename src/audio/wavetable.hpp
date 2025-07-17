#pragma once
#include "esp_attr.h"

using WaveFn = float(*)(float);

namespace WaveIndex {
    enum Value {
        Silence = 0,
        Sin,
        Tri,
        TriSaw,
        Saw,
        SawRev,
        Square,
        RectWide,
        RectNarrow,
    };
};

constexpr int WAVE_COUNT = WaveIndex::RectNarrow + 1;

float wave_silence(float x);
float wave_sin(float x);
float wave_tri(float x);
float wave_saw(float x);
float wave_tri_saw(float x);
float wave_saw_rev(float x);
float wave_square(float x);
float wave_rect_wide(float x);
float wave_rect_narrow(float x);

constexpr WaveFn waves[WAVE_COUNT] = {
    [WaveIndex::Silence]     = wave_silence,
    [WaveIndex::Sin]         = wave_sin,
    [WaveIndex::Tri]         = wave_tri,
    [WaveIndex::TriSaw]      = wave_tri_saw,
    [WaveIndex::Saw]         = wave_saw,
    [WaveIndex::SawRev]      = wave_saw_rev,
    [WaveIndex::Square]      = wave_square,
    [WaveIndex::RectWide]    = wave_rect_wide,
    [WaveIndex::RectNarrow]  = wave_rect_narrow,
};

// Function declaration
IRAM_ATTR void init_sine_lut();


