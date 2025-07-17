#include "synth.hpp"
#include "wavetable.hpp"
#include "audio_math.hpp"


FORCE_INLINE float OscState::step(float dt, const OscillatorConfig &config) {
    static float intpart;
    if(!config.enabled) return 0.f;
    
    dt *= config.freq_mult;
    this->phase = std::modf(this->phase + dt, &intpart);
    return waves[config.wave_index](this->phase) * config.gain_mult;
}

FORCE_INLINE void EnvelopeState::step() {
    switch (section) {
        case EnvelopeSection::Off:
            value = 0.0f;
            break;

        case EnvelopeSection::Attack:
            value += attack_rate;
            if (value >= 1.0f) {
                value = 1.0f;
                section = EnvelopeSection::Decay;
            }
            break;

        case EnvelopeSection::Decay:
            value -= decay_rate;
            if (value <= sustain_gain) {
                value = sustain_gain;
                section = EnvelopeSection::Sustain;
            }
            break;

        case EnvelopeSection::Sustain:
            value = sustain_gain;
            break;

        case EnvelopeSection::Release:
            value -= release_rate;
            if (value <= 0.0f) {
                value = 0.0f;
                section = EnvelopeSection::Off;
            }
            break;
    }
}


void Synth::process_midi_event(const MidiEvent &event)
{
    switch (event.get_event_type()) {
        case MidiEventType::NoteOn: {
            if(!voice_state.enabled || event.get_note() != voice_state.note) {
                voice_state.note = event.get_note();
                voice_state.envelope_state.trigger_on();
                voice_state.enabled = true;
            }
            break;
        }
        case MidiEventType::NoteOff: {
            if(voice_state.enabled && event.get_note() == voice_state.note) {
                voice_state.envelope_state.trigger_off();
                voice_state.enabled = false;
            }
            break;
        }
    }
}



void Synth::process_block(float *data, size_t len) {
    sync_config();

    // precompute variables for the entire block
    voice_state.envelope_state.set_rates(config.envelope);
    const float freq = voice_state.note.get_frequency();
    const float dt = 1.f / SYNTH_SR * freq;
    
    // compute block
    for(size_t i = 0; i < len; i++) {
        // oscillators
        const float y1 = voice_state.osc1_state.step(dt, config.osc1_config);
        const float y2 = voice_state.osc2_state.step(dt, config.osc2_config);
        const float y3 = voice_state.osc3_state.step(dt, config.osc3_config);

        // envelope
        voice_state.envelope_state.step();
        data[i] = (y1 + y2 + y3) * voice_state.envelope_state.value;

        // boost
        data[i] = saturate_hard(data[i] * config.boost.boost_mult) * config.boost.gain_mult; 
    }
}

void Synth::update_config(const SynthConfig &new_config) {
    xQueueOverwrite(config_queue, &new_config);
}

void Synth::sync_config() {
    xQueueReceive(config_queue, &config, 0);
}