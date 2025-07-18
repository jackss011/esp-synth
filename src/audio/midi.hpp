#pragma once
#include "stdint.h"
#include <cmath>


namespace MidiEventType {
    enum Value {
        Empty = 0,
        NoteOn,
        NoteOff,
        Other,
    };
}

struct MidiNote {
    uint8_t note_index;

    MidiNote() : note_index(69) {}
    MidiNote(uint8_t idx) : note_index(idx) {}

    float get_frequency() const {
        // A4 = MIDI note 69 = 440 Hz
        return 440.0f * std::pow(2.0f, (note_index - 69) / 12.0f);
    }

    bool operator==(const MidiNote& other) const {
        return note_index == other.note_index;
    }

    bool operator!=(const MidiNote& other) const {
        return note_index != other.note_index;
    }

    bool operator<(const MidiNote& other) const {
        return note_index < other.note_index;
    }

    bool operator>(const MidiNote& other) const {
        return note_index > other.note_index;
    }

    static const MidiNote None;
};

struct __attribute__((packed)) MidiEvent {
    uint8_t header;
    uint8_t status;
    uint8_t data1;
    uint8_t data2;

    MidiEvent() {
        header = 0;
        status = 0;
        data1  = 0;
        data2  = 0;
    }

    MidiEvent(uint8_t buffer4[]) {
        header = buffer4[0];
        status = buffer4[1];
        data1  = buffer4[2];
        data2  = buffer4[3];
    }

    uint8_t get_channel() const {
        return status & 0x0F;;
    }

    MidiEventType::Value get_event_type() const {
        uint8_t cin = header & 0x0F;
        if (cin == 0x9 && data2 != 0) 
            return MidiEventType::NoteOn;
        else if (cin == 0x8 || (cin == 0x9 && data2 == 0)) 
            return MidiEventType::NoteOff;
        else 
            return MidiEventType::Other;
    }
 
    MidiNote get_note() const {
        return MidiNote(data1);
    }

    uint8_t get_velocity() const{
        return data2;
    }
};


constexpr size_t MAX_TRACKED_NOTES = 5;


struct NoteTracker {
    inline size_t get_count() const { return count; };

    bool has(MidiNote n) const {
        for(size_t i = 0; i < count; i++) 
            if(n == notes[i]) return true;

        return false;
    }

    void push(MidiNote n) {
        // ensure unique list (set)
        if (n == MidiNote::None || has(n)) return;

        if (count < MAX_TRACKED_NOTES) ++count;

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

    MidiNote next_note(MidiNote current) const {
        MidiNote res = MidiNote::None; // idx 255, greatest value
        if(count == 1) return last_note();

        for(size_t i = 0; i < count; i++) {
            if(notes[i] > current && notes[i] < res)
                res = notes[i];
        }

        return res;
    }

    MidiNote last_note() const { return count == 0 ? MidiNote::None : notes[0]; }

    MidiNote get_at(size_t index) const {
        return (index < count) ? notes[index] : MidiNote::None;
    }

private:
    MidiNote notes[MAX_TRACKED_NOTES];
    size_t   count = 0;
};