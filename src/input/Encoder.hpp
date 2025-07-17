#pragma once

#include "Arduino.h"

const int ENCODER_DEBOUNCE_MILLIS = 9;

namespace EncoderEvent{
    typedef enum
    {
        Left = -1,
        None = 0,
        Right = 1,
    } Value;
};

struct Encoder
{
private:
    uint8_t clk_pin;
    uint8_t dt_pin;

    long last_down_millis = -1000;
    bool _clk_state = false;
    int _last_dt = 0;
    bool last_clk = false;
    EncoderEvent::Value _event = EncoderEvent::None;

public:
    bool inverted;

    Encoder(uint8_t clk_pin, uint8_t dt_pin, bool inverted = false) 
        : clk_pin(clk_pin), dt_pin(dt_pin), inverted(inverted)
    {}

    void begin();
    void update();

    EncoderEvent::Value inline event() { return _event; }
};
