#include "UiController.hpp"
#include "input/Btn.hpp"
#include "audio/wavetable.hpp"


// ---------- GAIN SELECTOR ----------
static const char* gain_labels[] = {"0.0", "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0"};
static int32_t gain_values[] =     {0,        10,    20,    30,    40,    50,    60,    70,    80,    90,  100}; // scaled by 100

static const SelectorConfig gain_config = {
    .display_values = gain_labels,
    .values = gain_values,
    .norm_factor = 100,
    .count = 11,
    .default_index = 5 // "0.5"
};

// ---------- SHAPE SELECTOR ----------
static const char* shape_labels[] = {"tri", "t_s", "saw", "squ", "re1", "re2"};
static int32_t shape_values[] = {
    WaveIndex::Tri,
    WaveIndex::TriSaw,
    WaveIndex::Saw,
    WaveIndex::Square,
    WaveIndex::RectWide,
    WaveIndex::RectNarrow
};

static const SelectorConfig shape_config = {
    .display_values = shape_labels,
    .values = shape_values,
    .norm_factor = 1,
    .count = 6,
    .default_index = 0
};

// ---------- DETUNE SELECTOR ----------
static const char* detune_labels[] = {"-20", "-16", "-12", "-10", "-7", "-5", "-3", "0"};
static int32_t detune_values[] =     {-20, -16, -12, -10, -7, -5, -3, 0};

static const SelectorConfig detune_config = {
    .display_values = detune_labels,
    .values = detune_values,
    .norm_factor = 100,
    .count = 8,
    .default_index = 7
};

// ---------- RANGE SELECTOR ----------
static const char* range_labels[] = {"32'", "16'", "8'", "4'", "2'"};
static int32_t range_values[] = {32, 16, 8, 4, 2};

static const SelectorConfig range_config = {
    .display_values = range_labels,
    .values = range_values,
    .norm_factor = 8,
    .count = 5,
    .default_index = 2 // "8'"
};

// ---------- TIME SELECTOR ----------
static const char* time_labels[] = {
    "0.1s", "0.2s", "0.3s", "0.4s", "0.5s", "0.6s", "0.7s", "0.8s", "0.9s", "1s",
    "2s", "3s", "4s", "5s", "8s", "10s", "15s", "30s", "60s"
};

static int32_t time_values[] = {
    100, 200, 300, 400, 500, 600, 700, 800, 900, 1000,
    2000, 3000, 4000, 5000, 8000, 10000, 15000, 30000, 60000
};

static const SelectorConfig time_config = {
    .display_values = time_labels,
    .values         = time_values,
    .norm_factor    = 1000,
    .count          = sizeof(time_values) / sizeof(time_values[0]),
    .default_index  = 9 // "1s"
};

// ---------- Layout Constants ----------
static const int16_t COL1 = 6;
static const int16_t COL2 = 128 / 2 - 8;
static const int16_t COL3 = 128 - 20;
static const int16_t ROW2 = 16;
static const int16_t ROW1 = 40;

struct OscTab : Widget {
    OscillatorConfig *config;

    Selector range   = Selector("range",   COL1, ROW1,  range_config);
    Selector detune  = Selector("detune",  COL2, ROW1,  detune_config);
    Selector shape   = Selector("shape",   COL3, ROW1,  shape_config);
    Selector gain    = Selector("gain",    COL2, ROW2,  gain_config);
    Switch   en      = Switch  ("en",      COL3, ROW2);

    OscTab(const char* key, OscillatorConfig *config = nullptr) : Widget(key, 0, 0), config(config) {

    }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        range.render(gfx);
        detune.render(gfx); 
        shape.render(gfx);
        gain.render(gfx); 
        en.render(gfx);     
    }

    virtual void process_event(const InputEvent &event) override {
        if (!event.shifed) {
            if (event.id == InputId::Encoder0) {
                range.nudge(event.value);
                config->set_freq_mult(1.f/range.get_value_asf32(), detune.get_value());
            } 
            else if (event.id == InputId::Encoder1) {
                detune.nudge(event.value);
                config->set_freq_mult(1.f/range.get_value_asf32(), detune.get_value());
            } 
            else if (event.id == InputId::Encoder2) {
                shape.nudge(event.value);
                config->wave_index = shape.get_value();
            }
        } else {
            if (event.id == InputId::Encoder1) {
                gain.nudge(event.value);
            } else if (event.id == InputId::Encoder2) {
                en.nudge(event.value);
            }
        }

        // Serial.printf("%s: %.6f\n", key, config->freq_mult);
    }
};


struct EnvTab : Widget {
    EnvelopeConfig *config;

    Selector attack  = Selector("att",   COL1, ROW1,  time_config);
    Selector decay   = Selector("dec",   COL2, ROW1,  time_config);
    Selector sustain = Selector("sus",   COL3, ROW1,  gain_config);

    EnvTab(const char* key, EnvelopeConfig *config) : Widget(key, 0, 0), config(config) {

    }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        attack.render(gfx);
        decay.render(gfx); 
        sustain.render(gfx);   
    }

    virtual void process_event(const InputEvent &event) override {
        if (!event.shifed) {
            if (event.id == InputId::Encoder0) {
                attack.nudge(event.value);
                config->attack_secs = attack.get_value_asf32();
            }
            else if (event.id == InputId::Encoder1) {
                decay.nudge(event.value);
                config->decay_secs = decay.get_value_asf32();
            } 
            else if (event.id == InputId::Encoder2) {
                sustain.nudge(event.value);
                config->sustain_gain = sustain.get_value_asf32();
            }
        }
    }
};


static auto osc1_tab = OscTab("o1",  nullptr);
static auto osc2_tab = OscTab("o2",  nullptr);
static auto osc3_tab = OscTab("o3",  nullptr);
static auto env_tab  = EnvTab("env", nullptr);

static auto tabs = WidgetGroup();


void UiController::init() {
    osc1_tab.config = &config.osc1_config;
    osc2_tab.config = &config.osc2_config;
    osc3_tab.config = &config.osc3_config;
    env_tab.config =  &config.envelope;

    tabs.add(nullptr);
    tabs.add(&osc1_tab);
    tabs.add(&osc2_tab);
    tabs.add(&osc3_tab);
    tabs.add(&env_tab);
}


void UiController::process_event(const InputEvent &event) {
    switch (event.id) {
        // handle left btn
        case InputId::BtnLx: {
            if(event.value == BtnEvent::Press)
                tab_index = static_cast<Tab::Value>((tab_index + 1) % TAB_COUNT);
            break; 
        }
        // handle right btn
        case InputId::BtnRx: {
            if(event.value == BtnEvent::Press)
                tab_index = static_cast<Tab::Value>((tab_index + TAB_COUNT - 1) % TAB_COUNT);
            break;
        }
        // handle shift btn (visualization only)
        case InputId::BtnShift: {
            if(event.value == BtnEvent::Press) 
                layer_shift_on = true;
            else if(event.value == BtnEvent::Release) 
                layer_shift_on = false;
            break;
        }
        // handle encoder values
        default: {
            Widget *active_tab = tabs.get(tab_index);
            if(active_tab) active_tab->process_event(event);
        }
    }
}


bool UiController::render_to_buffer() {
    // render header
    int16_t x = 1;
    for(size_t i = 0; i < TAB_COUNT; i++) {
        const int16_t spacing = i > Tab::Osc3 ? 25 : 18;
      
        gfx->setCursor(x, 0);
        gfx->print(tab_names[i]);
        if(tab_index == i) {
            gfx->drawFastHLine(x, 12, spacing*0.8, SSD1306_WHITE);
            gfx->drawFastHLine(x, 13, spacing*0.8, SSD1306_WHITE);
        }
        
        x += spacing;
    }

    // render layer indicator
    const int16_t y = layer_shift_on ? 16+2 : 40+2;
    gfx->fillRect(0, y, 3, 20, SSD1306_WHITE);

    // render correct tab
    Widget *active_tab = tabs.get(tab_index);
    if(active_tab) active_tab->render(gfx);

    return true;
}
