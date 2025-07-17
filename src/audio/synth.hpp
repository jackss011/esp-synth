#pragma once
#include <cstddef>
#include <cstdint>
#include "midi.hpp"
#include "config.h"
#include "perf.h"

// ------- OSCILLATOR --------
struct OscillatorConfig {
    bool enabled = false;
    uint8_t wave_index = 0;
    float freq_mult = 1;
    float gain_mult = 1;

    inline void set_freq_mult(float base_mult, int32_t detune_cents) {
        freq_mult = base_mult * powf(2.0f, detune_cents / 1200.0f);
    }
};

struct OscState {
    float phase = 0;
    FORCE_INLINE float step(float dt, const OscillatorConfig &config);
};


// ------- ENVELOPE --------
struct EnvelopeConfig {
    float attack_secs = 1;
    float decay_secs  = 1;
    float sustain_gain = 0.5;
    float release_secs = 1;
};

enum class EnvelopeSection {
    Off,
    Attack,
    Decay,
    Sustain,
    Release,
};

struct EnvelopeState {
    EnvelopeSection section = EnvelopeSection::Off;
    float value = 0;

    float attack_rate = 0;
    float decay_rate = 0;
    float sustain_gain = 0;
    float release_rate = 0;

    inline void trigger_on() { section = EnvelopeSection::Attack; }
    inline void trigger_off() { section = EnvelopeSection::Release; }

    inline void set_rates(EnvelopeConfig config) {
        attack_rate = 1.f / (config.attack_secs * SYNTH_SR);
        decay_rate = 1.f / (config.decay_secs * SYNTH_SR);
        sustain_gain = config.sustain_gain;
        release_rate = 1.f / (config.release_secs * SYNTH_SR);
    }

    FORCE_INLINE void step();
};


// ------- FILTER --------
struct LowPassConfig {
    float cutoff_freq = 100;
    float emphasis_perc = 0;
    float countour_perc = 0;
    EnvelopeConfig cutoff_envelope;
};



// ------- VOICE --------
struct VoiceState {
    bool enabled = false;
    MidiNote note;
    OscState osc1_state;
    OscState osc2_state;
    OscState osc3_state;
    EnvelopeState envelope_state;
};


// ------- SYNTH --------
struct SynthConfig {
    OscillatorConfig osc1_config;
    OscillatorConfig osc2_config;
    OscillatorConfig osc3_config;
    EnvelopeConfig envelope;
};


class Synth {
public:
    void update_config(const SynthConfig &new_config);
    void process_midi_event(const MidiEvent &event);
    void process_block(float *data, size_t len);

private:
    VoiceState voice_state;
};
