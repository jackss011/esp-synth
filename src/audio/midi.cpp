#include "midi.hpp"

const MidiNote MidiNote::None(255);


void NoteTracker::push(MidiNote n) {
    if (n == MidiNote::None || has(n)) return;

    if (count < MAX_TRACKED_NOTES) ++count;

    for (size_t i = count - 1; i > 0; --i) {
        notes[i] = notes[i - 1];
    }

    notes[0] = n;
}

void NoteTracker::pop(MidiNote n) {
    if (n == MidiNote::None) return;

    int32_t found_index = -1;
    for(size_t i = 0; i < count; i++) {
        if(notes[i] == n) found_index = i;
    }

    if(found_index < 0) return;

    for (size_t i = found_index; i < count - 1; ++i) {
        notes[i] = notes[i + 1];
    }

    count--;
}

bool NoteTracker::has(MidiNote n) const {
    for(size_t i = 0; i < count; i++) 
        if(n == notes[i]) return true;

    return false;
}

MidiNote NoteTracker::smallest() const {
    MidiNote res = MidiNote::None;

    for(size_t i = 0; i < count; i++) {
        if(notes[i] < res) res = notes[i];
    }

    return res;
}

MidiNote NoteTracker::right_of(MidiNote x) const {
    if(x == MidiNote::None) return smallest();
    if(count == 0) return MidiNote::None;
    if(count == 1) return most_recent();

    MidiNote res = MidiNote::None;

    for(size_t i = 0; i < count; i++) {
        if(notes[i] > x && notes[i] < res)
            res = notes[i];
    }

    if(res == MidiNote::None) res = smallest();
    return res;
}
