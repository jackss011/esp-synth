#include "wavetable.hpp"
#include <math.h>
#include "esp_attr.h"
#include "generated/luts.hpp"

float wave_silence(float x) {
    return 0;
}

static float slow_sin(float x) {
    float intpart;
    const float p = modf(x, &intpart) * M_PI * 2;
    const float p1 = p <= M_PI ? p-1.570796f : p-4.712388f;
    const float r2 = p1 * p1;
    const float res = 1.f - r2/2.f + r2*r2/26.05072f;
    return p <= M_PI ? res : -res;
}

float wave_sin(float x) {
    // return slow_sin(x);
    return lutgen_sin[LUTEGEN_INDEX(x)];
}

float wave_saw(float x) {
    float intpart;
    float phase = modf(x, &intpart);
    return 2.f * (phase <= 0.5f ? phase : -0.5f + (phase - 0.5f));
}

float wave_tri_saw(float x) {
    float intpart;
    float phase = std::modf(x, &intpart);
    return phase < 0.5f
        ? 4.f * phase - 1.f         // rising
        : -4.f * (phase - 0.5f) + 1.f; // falling
}

float wave_saw_rev(float x) {
    float intpart;
    float phase = modf(x, &intpart);
    return -2.f * (phase <= 0.5f ? phase : -0.5f + (phase - 0.5f));
}

float wave_tri(float x) {
    float intpart;
    float phase = modf(x, &intpart);
    return 4.f * std::fabs(phase - 0.5f) - 1.f;
}

float wave_square(float x) {
    float intpart;
    float phase = modf(x, &intpart);
    return phase <= (1.f/2) ? 1.f : -1.f;
}

float wave_rect_wide(float x) {
    float intpart;
    float phase = modf(x, &intpart);
    return phase <= (1.f/4) ? 1.f : -1.f;
}
 
float wave_rect_narrow(float x) {
    float intpart;
    float phase = modf(x, &intpart);
    return phase <= (1.f/10) ? 1.f : -1.f;
}
