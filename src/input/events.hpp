#pragma once
#include <stdint.h>

enum class InputId : uint8_t {
    None = 0,

    BtnLx = 0x01,
    BtnRx = 0x02,
    BtnShift = 0x03,

    Encoder0 = 0x10,
    Encoder1 = 0x11,
    Encoder2 = 0x12,
};

inline InputId input_id_from_uint8(uint8_t value) {
    switch (value) {
        case 0x00: return InputId::None;
        case 0x01: return InputId::BtnLx;
        case 0x02: return InputId::BtnRx;
        case 0x03: return InputId::BtnShift;
        case 0x10: return InputId::Encoder0;
        case 0x11: return InputId::Encoder1;
        case 0x12: return InputId::Encoder2;
        default:   return InputId::None; // fallback for invalid values
    }
}

struct __attribute__((packed)) InputEvent {
    InputId id = InputId::None;
    int16_t value = 0;
    uint8_t shifed = 0;
};
