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


static FORCE_INLINE float fast_tanh(float x) 
{
	float x2 = x * x;
	return x * (27.f + x2) / (27.f + 9.f * x2);
}

#define HZ_TO_WC(hz) ((hz) * 6.28318530718f / SYNTH_SR)

#define EXP_APPROACH(target, current, rate) \
    ((current) + ((target) - (current)) * (rate))

#define SYNTH_LOWPASS_CUTOFF_TIMING_SECS 0.1f

/**
 * adapted from: https://github.com/ddiakopoulos/MoogLadders
 * added dynamic frequency
 */
void LowPassState::process_block(float *samples, size_t n, const LowPassConfig &config) {
    float k = config.emphasis_perc * 4;
    float base_wc = HZ_TO_WC(config.cutoff_hz);

    for (size_t i = 0; i < n; ++i)
    {
        current_wc = EXP_APPROACH(base_wc, current_wc, 1.f / (SYNTH_LOWPASS_CUTOFF_TIMING_SECS * SYNTH_SR));
 
        float out = p3 * 0.360891f + p32 * 0.417290f + p33 * 0.177896f + p34 * 0.0439725f;

        p34 = p33;
        p33 = p32;
        p32 = p3;

        p0 += (fast_tanh(samples[i] - k * out) - fast_tanh(p0)) * current_wc;
        p1 += (fast_tanh(p0) - fast_tanh(p1)) * current_wc;
        p2 += (fast_tanh(p1) - fast_tanh(p2)) * current_wc;
        p3 += (fast_tanh(p2) - fast_tanh(p3)) * current_wc;

        samples[i] = out;
    }
}



void Synth::process_midi_event(const MidiEvent &event)
{
    switch (event.get_event_type()) {
        case MidiEventType::NoteOn: {
            const MidiNote note = event.get_note();

            // if(!voice_state.enabled || note != voice_state.note) {
            //     voice_state.note = note;
            //     voice_state.envelope_state.trigger_on();
            //     voice_state.enabled = true;
            // }
            Serial.printf("\npush: %d\n", note.note_index);
            arp_state.push(note);
            Serial.print("LIST: ");
            for(int i = 0; i < arp_state.get_count(); i++) {
                Serial.print(arp_state.get_at(i).note_index);
                Serial.print(" ");
            }
            Serial.println();

            break;
        }
        case MidiEventType::NoteOff: {
            const MidiNote note = event.get_note();

            // if(voice_state.enabled && note == voice_state.note) {
            //     voice_state.envelope_state.trigger_off();
            //     voice_state.enabled = false;
            // }
            Serial.printf("\npop: %d\n", note.note_index);
            arp_state.pop(note);
            Serial.print("LIST: ");
            for(int i = 0; i < arp_state.get_count(); i++) {
                Serial.print(arp_state.get_at(i).note_index);
                Serial.print(" ");
            }
            Serial.println();
            break;
        }
    }
}



void Synth::process_block(float *data, size_t len) {
    sync_config();
    // Serial.printf("%f, %f, %f\n", config.lowpass.cutoff_hz, config.lowpass.emphasis_perc, config.lowpass.countour_dhz);
    // delay(1000);

    const auto last_note = arp_state.last_note();

    // not playing any notes
    if(last_note == MidiNote::None) {
        if(voice_state.enabled) {
            voice_state.enabled = false;
            voice_state.envelope_state.trigger_off();
            Serial.println("off");
        }

        arp_state.active_time = -1;
    }
    // is playing at least 1 note
    else {
        MidiNote note_to_play = last_note;

        if(config.argeggiator.enabled || true) {
            int64_t now = millis();

            if(arp_state.active_time < 0) {
                arp_state.active_time = now;
                arp_state.current_note = last_note;
                arp_state.current_note_index = 0;
            }

            if(now >= arp_state.active_time + config.argeggiator.period_ms()/4) {
                arp_state.active_time = now;
                arp_state.current_note_index++;
                arp_state.current_note = arp_state.get_at(arp_state.current_note_index % arp_state.get_count());
            }

            note_to_play = arp_state.current_note;
        } 

        if(voice_state.note != note_to_play || !voice_state.enabled) {
            voice_state.enabled = true;
            voice_state.note = note_to_play;
            voice_state.envelope_state.trigger_on();
            Serial.print(note_to_play.note_index);
            Serial.println(" on");
        }
    }

    // precompute variables for the entire block
    voice_state.envelope_state.set_rates(config.envelope);
    const float freq = voice_state.note.get_frequency();
    const float dt = 1.f / SYNTH_SR * freq;
    
    // compute block
    for(size_t i = 0; i < len; i++) {
        // oscillators
        const float y1 = voice_state.osc1_state.step(dt, config.osc1);
        const float y2 = voice_state.osc2_state.step(dt, config.osc2);
        const float y3 = voice_state.osc3_state.step(dt, config.osc3);

        // envelope
        voice_state.envelope_state.step();
        data[i] = (y1 + y2 + y3) * voice_state.envelope_state.value;

        // boost
        data[i] = saturate_hard(data[i] * config.boost.boost_mult) * config.boost.gain_mult; 
    }

    // apply low pass
    // lowpass_state.process_block(data, len, config.lowpass);

    // saturate for good measure
    for(size_t i = 0; i < len; i++) {
        data[i] = saturate_hard(data[i]);
    }
}

void Synth::update_config(const SynthConfig &new_config) {
    xQueueOverwrite(config_queue, &new_config);
}

void Synth::sync_config() {
    xQueueReceive(config_queue, &config, 0);
}

