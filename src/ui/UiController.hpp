#pragma once
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#include "ui/widget.hpp"
#include "input/events.hpp"
#include "audio/synth.hpp"


namespace Tab {
    enum Value {
        Input,
        Osc1,
        Osc2,
        Osc3,
        Envelope,
        Filter,
    };
};

constexpr const char* tab_names[] = {
    "arp", "o1", "o2", "o3", "env", "flt"
};

const uint8_t TAB_COUNT = Tab::Filter + 1;

class UiController {
    Adafruit_SSD1306 *gfx;
    Tab::Value tab_index = Tab::Osc1;
    bool layer_shift_on;

public:
    SynthConfig config;
    UiController(Adafruit_SSD1306 *gfx): gfx(gfx) {}

    void init();
    void process_event(const InputEvent &event);
    bool render_to_buffer();
};
