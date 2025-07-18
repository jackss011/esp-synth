#pragma once
#include "midi.hpp"

const size_t MAX_ARPEGGIATOR_NOTES = 5;

struct ArpeggiatorConfig {
    bool enabled = false;
    float tempo_bpm = 120;

    inline uint32_t period_ms() const {
        return static_cast<uint32_t>(60000.0f / tempo_bpm);
    }
};


struct ArpeggiatorState {

    uint32_t active_time = -1;
    MidiNote current_note = MidiNote::None;
    size_t   current_note_index = 0;

    inline size_t get_count() { return count; };

    bool has(MidiNote n) {
        for(size_t i = 0; i < count; i++) 
            if(n == notes[i]) return true;

        return false;
    }

    void push(MidiNote n) {
        // ensure unique list (set)
        if (n == MidiNote::None || has(n)) return;

        if (count < MAX_ARPEGGIATOR_NOTES) ++count;

        // shift right
        for (size_t i = count - 1; i > 0; --i) {
            notes[i] = notes[i - 1];
        }

        // add in 0
        notes[0] = n;
    }

    void pop(MidiNote n) {
        if (n == MidiNote::None) return;

        // search for value to pop
        int32_t found_index = -1;
        for(size_t i = 0; i < count; i++) {
            if(notes[i] == n) found_index = i;
        }

        if(found_index < 0) return;

        // shift left to overwrite the popped value
        for (size_t i = found_index; i < count - 1; ++i) {
            notes[i] = notes[i + 1];
        }
        count--;
    }

    MidiNote next_note(MidiNote current) {
        MidiNote res = MidiNote::None; // idx 255, greatest value
        if(count == 1) return last_note();

        for(size_t i = 0; i < count; i++) {
            if(notes[i] > current && notes[i] < res)
                res = notes[i];
        }

        return res;
    }

    MidiNote last_note() { return count == 0 ? MidiNote::None : notes[0]; }

    MidiNote get_at(size_t index) {
        return (index < count) ? notes[index] : MidiNote::None;
    }

private:
    MidiNote notes[MAX_ARPEGGIATOR_NOTES];
    size_t   count = 0;
};