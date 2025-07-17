#pragma once

enum class InputId : uint8_t {
    None = 0,

    BtnLx = 0x01,
    BtnRx = 0x02,
    BtnShift = 0x03,

    Encoder0 = 0x10,
    Encoder1 = 0x11,
    Encoder2 = 0x12,
};

struct __attribute__((packed)) InputEvent {
    InputId id = InputId::None;
    int16_t value = 0;
    uint8_t shifed = 0;
};
