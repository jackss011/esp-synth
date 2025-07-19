#pragma once
#include "input/events.hpp"


using InputEventCallback = void(*)(const InputEvent &);

void remote_init();
void remote_set_input_cb(InputEventCallback cb);
