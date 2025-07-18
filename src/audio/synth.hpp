#pragma once
#include <cstddef>
#include <cstdint>
#include "midi.hpp"
#include "config.h"
#include "perf.h"
#include "arpeggiator.hpp"

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

// ------- BOOST --------
struct BoostConfig {
    float boost_mult = 1.f;
    float gain_mult  = 1.f;
};


// ------- FILTER --------
struct LowPassConfig {
    float cutoff_hz = 2000;
    float emphasis_perc = 0.1;
    float countour_dhz = 0;
    EnvelopeConfig cutoff_envelope;
};

struct LowPassState {
    float p0   = 0.f;
	float p1   = 0.f;
	float p2   = 0.f;
	float p3   = 0.f;
	float p32  = 0.f;
	float p33  = 0.f;
	float p34  = 0.f;

    float current_wc = 0.f;

    void process_block(float *samples, size_t n, const LowPassConfig &config);
};


// #include <cmath>

class TPTLowPass {
public:
    void process_block(float* samples, size_t n, const LowPassConfig& config) {
        // Convert cutoff Hz to angular frequency (rad/s)
        float wc = 2.0f * (float)M_PI * config.cutoff_hz;
        float T = 1.0f / SYNTH_SR;
        float g = tanf(0.5f * wc * T);  // TPT bilinear transform

        // Convert emphasis percentage to normalized resonance factor [0.0 - 1.0]
        float R = config.emphasis_perc;
        if (R > 1.0f) R = 1.0f;
        if (R < 0.0f) R = 0.0f;

        for (size_t i = 0; i < n; ++i) {
            float v = (samples[i] - R * state_ - state_) / (1.0f + g);
            float lp = v + state_;
            state_ = lp + v;

            samples[i] = lp;
        }
    }

    void reset() {
        state_ = 0.0f;
    }

private:
    float state_ = 0.0f;
};

// ------- VOICE --------
struct VoiceState {
    bool enabled = false;
    MidiNote note = MidiNote::None;
    OscState osc1_state;
    OscState osc2_state;
    OscState osc3_state;
    EnvelopeState envelope_state;
};


// ------- SYNTH --------
struct SynthConfig {
    ArpeggiatorConfig argeggiator;
    OscillatorConfig osc1;
    OscillatorConfig osc2;
    OscillatorConfig osc3;
    EnvelopeConfig envelope;
    BoostConfig boost;
    LowPassConfig lowpass;
};


class Synth {
public:
    void update_config(const SynthConfig &new_config);
    void process_midi_event(const MidiEvent &event);
    void process_block(float *data, size_t len);

    void begin() {
        config_queue = xQueueCreate(1, sizeof(SynthConfig));
    }

private:
    ArpeggiatorState arp_state;
    VoiceState voice_state;
    TPTLowPass lowpass_state;

    QueueHandle_t config_queue;
    SynthConfig config;

    void sync_config();
};
