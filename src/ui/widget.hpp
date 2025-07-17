#pragma once
#include <cinttypes>
#include <cstddef>
#include <Preferences.h>
#include <Adafruit_SSD1306.h>
#include <array>
#include "input/events.hpp"


struct Widget {
    const char *key;
    int16_t x, y;

    virtual const char *get_key() { return key; }
    virtual void render(Adafruit_SSD1306 *gfx) {}
    virtual void process_event(const InputEvent &event) {}
    virtual void nudge(int16_t dir) {}
    virtual void load(const Preferences &prefs) {}
    virtual void store(const Preferences &prefs) {}

    virtual ~Widget() = default;

protected:
    Widget(const char *key, int16_t x, int16_t y)
        : key(key), x(x), y(y) {}
};


using SwitchCallback = void(*)(bool);

struct Switch : Widget {
    bool value = false;
    SwitchCallback cb = nullptr;

    Switch(const char *key, int16_t x, int16_t y)
        : Widget(key, x, y) {}

    virtual void nudge(int16_t dir) override {
        if(dir == 0) return;
        const bool new_value = dir > 0;

        if (new_value != value) {
            value = new_value;
            if(cb) cb(value);
        }
    }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        gfx->setCursor(x, y);
        gfx->print(value ? "ON" : "OFF");
    }

    bool get_value() { return value; }
};

using SelectorCallback = void(*)(int32_t value);

struct SelectorConfig
{
    const char    **display_values;
    int32_t       *values;
    int32_t       norm_factor;
    size_t        count;
    size_t        default_index;
};

struct Selector : Widget {
    int32_t index = 0;
    SelectorConfig config;
    SelectorCallback cb = nullptr;

    Selector(const char *key, int16_t x, int16_t y, const SelectorConfig &config)
        : Widget(key, x, y), config(config) {
            index = config.default_index;
        }

    virtual void nudge(int16_t dir) override {
        if(dir == 0) return;
        const int32_t new_index = constrain(index + dir, 0, config.count -1);

        if (new_index != index) {
            index = new_index;
            if(cb) cb(config.values[index]);
        }
    }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        gfx->setCursor(x, y);
        gfx->print(config.display_values[index]);
    }

    int32_t get_value() { return config.values[index]; }
    float   get_value_asf32() { return (float)(config.values[index]) / config.norm_factor; }
};

struct WidgetGroup {
    static const size_t MAX_CHILDREN = 64;

    Widget *children [MAX_CHILDREN];
    InputId input_masks[MAX_CHILDREN];
    bool   input_shifted[MAX_CHILDREN];
    size_t count = 0;

    void add(Widget *child, InputId mask = InputId::None, bool shift = false) {
        assert(count < MAX_CHILDREN);
        children[count] = child;
        input_masks[count] = mask;
        input_shifted[count] = shift;
        count += 1;
    }

    void process_event(const InputEvent &event) {
        for(size_t i = 0; i < count; i++) {
            if(event.id == input_masks[i] && event.shifed == input_shifted[i]) {
                Serial.printf("Hello %d\n", event.value);
                children[i]->nudge(event.value);
            }
        }
    }

    void render(Adafruit_SSD1306 *gfx) {
        for(size_t i = 0; i < count; i++) 
            children[i]->render(gfx);
    }

    Widget *get(size_t index) {
        if (index < count) return children[index];
        return nullptr;
    }
};
