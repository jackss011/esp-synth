#pragma once

#include "Arduino.h"

const int BTN_DEBOUNCE_MILLIS = 50;

typedef enum
{
    None = 0,
    Press = 1,
    Release = 2,
} BtnEvent;

struct Btn
{
private:
    uint8_t gpio_pin;

    long last_down_millis = -1000;
    bool _pressed = false;
    bool last_reading = false;
    BtnEvent _event = BtnEvent::None;

public:
    Btn(uint8_t gpio_pin) : gpio_pin(gpio_pin) {}
    void begin();
    void update();

    bool inline pressed() { return _pressed; }
    BtnEvent inline event() { return _event; }
};
