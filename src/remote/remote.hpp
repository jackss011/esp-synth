#pragma once
#include "input/events.hpp"


using InputEventCallback = void(*)(const InputEvent &);

namespace remote {
    void init();
    void set_input_cb(InputEventCallback cb);
    void send_screen(const uint8_t *data);
};

