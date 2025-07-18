#pragma once
#include "midi.hpp"

struct ArpeggiatorConfig {
    bool enabled = false;
    float tempo_bpm = 120;
    uint8_t time_division = 1;

    inline uint32_t period_ms() const {
        return static_cast<uint32_t>(60000.0f / tempo_bpm / time_division);
    }
};


struct ArpeggiatorState {
    int64_t active_time = -1;
    size_t   note_index = 0;

    bool is_active() { return active_time > 1; }

    void clear() {
        active_time = -1;
        note_index = 0;
    }

    bool step(const ArpeggiatorConfig &config) {
        int64_t now = millis();

        if(active_time < 0) {
            active_time = now;
            note_index = 0;
            return true;
        }

        if(now >= active_time + config.period_ms()) {
            active_time = now;
            note_index++;
            return true;
        }

        return false;
    }
};
