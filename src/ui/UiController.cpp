#include "UiController.hpp"
#include "input/Btn.hpp"
#include "audio/wavetable.hpp"


// ---------- GAIN SELECTOR ----------
static const char* gain_labels[] = {"0.0", "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0"};
static int32_t gain_values[] =     {0,        10,    20,    30,    40,    50,    60,    70,    80,    90,  100};
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

// ---------- BOOST -----------
static const char* boost_labels[] = {"+0", "+1", "+2"};
static int32_t boost_values[] = {10, 15, 20};
static const SelectorConfig boost_config = {
    .display_values = boost_labels,
    .values = boost_values,
    .norm_factor = 10,
    .count = 3,
    .default_index = 0
};


// --------- FILTER ----------
static const char* contour_labels[] = {
    "none", "+100", "+500", "+1k", "+2k", "+3k", "+4k"
};
static int32_t contour_values[] = {
    0, 100, 500, 1000, 2000, 3000, 4000
};
static const SelectorConfig contour_config = {
    .display_values = contour_labels,
    .values         = contour_values,
    .norm_factor    = 1,
    .count          = sizeof(contour_values) / sizeof(contour_values[0]),
    .default_index  = 0  // none
};


static const char* cutoff_labels[] = {
    "1kHz", "2kHz", "3kHz", "4kHz", "5kHz", "6kHz", "7kHz",
    "8kHz", "9kHz", "10kHz", "12kHz", "15kHz", "18kHz"
};
static int32_t cutoff_values[] = {
    1000, 2000, 3000, 4000, 5000, 6000, 7000,
    8000, 9000, 10000, 12000, 15000, 18000
};
static const SelectorConfig cutoff_config = {
    .display_values = cutoff_labels,
    .values         = cutoff_values,
    .norm_factor    = 1,
    .count          = sizeof(cutoff_values) / sizeof(cutoff_values[0]),
    .default_index  = 9 // Starts at 1kHz
};


// ---------- Layout Constants ----------
static const int16_t COL1 = 6;
static const int16_t COL2 = 128 / 2 - 8;
static const int16_t COL3 = 128 - 24;
static const int16_t ROW2 = 16;
static const int16_t ROW1 = 40;

struct Table2x3Layout {
    Widget *table[2][3] = { nullptr };

    inline void first_row(Widget *c1, Widget *c2, Widget *c3) {
        table[0][0] = c1; table[0][1] = c2; table[0][2] = c3;

        if(c1) { c1->x = COL1; c1->y = ROW1; }
        if(c2) { c2->x = COL2; c2->y = ROW1; }
        if(c3) { c3->x = COL3; c3->y = ROW1; }
    }

    inline void second_row(Widget *c1, Widget *c2, Widget * c3) {
        table[1][0] = c1; table[1][1] = c2; table[1][2] = c3;

        if(c1) { c1->x = COL1; c1->y = ROW2; }
        if(c2) { c2->x = COL2; c2->y = ROW2; }
        if(c3) { c3->x = COL3; c3->y = ROW2; }
    }

    void render(Adafruit_SSD1306 *gfx) {
        for(size_t i = 0; i < 2; i++)
            for(size_t j = 0; j < 3; j++)
                if(table[i][j]) table[i][j]->render(gfx);
    }

    void process_event(const InputEvent &event) {
        const int row = event.shifed ? 1 : 0;
        const int col = (int)event.id - (int)InputId::Encoder0;
        if(col < 0 || col >= 3) return;

        Widget *w = table[row][col];
        if(w) w->process_event(event);
    }
};

// =============================
// || TABS
// =============================
struct OscTab : Widget {
    OscillatorConfig *config;

    Selector range   = Selector("range",   range_config);
    Selector detune  = Selector("detune",  detune_config);
    Selector shape   = Selector("shape",   shape_config);
    Selector gain    = Selector("gain",    gain_config);
    Switch   en      = Switch  ("en");

    Table2x3Layout layout;

    OscTab(const char* key, OscillatorConfig *config = nullptr) : Widget(key, 0, 0), config(config) {
        layout.first_row(&range, &detune, &shape);
        layout.second_row(nullptr, &gain, &en);
    }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        layout.render(gfx); 
    }

    virtual void process_event(const InputEvent &event) override {
        layout.process_event(event);
        update_configs();
    }

    void update_configs() {
        config->set_freq_mult(1.f/range.get_value_asf32(), detune.get_value());
        config->wave_index = shape.get_value();
        config->gain_mult = gain.get_value_asf32();
        config->enabled = en.get_value();
    }
};


struct EnvTab : Widget {
    EnvelopeConfig *env_cfg;
    BoostConfig *boost_cfg;

    Selector attack  = Selector("att", time_config);
    Selector decay   = Selector("dec", time_config);
    Selector sustain = Selector("sus", gain_config);

    Selector boost_en = Selector("boost", boost_config);
    Selector gain     = Selector("gain",  gain_config);

    Table2x3Layout layout;

    EnvTab(const char* key, EnvelopeConfig *env_cfg, BoostConfig *boost_cfg) 
        : Widget(key, 0, 0), env_cfg(env_cfg), boost_cfg(boost_cfg) {
            layout.first_row(&attack, &decay, &sustain);
            layout.second_row(nullptr, &boost_en, &gain);
            gain.index = 10;
        }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        layout.render(gfx);   
    }

    virtual void process_event(const InputEvent &event) override {
        layout.process_event(event);
        update_configs();
    }

    void update_configs() {
        env_cfg->attack_secs = attack.get_value_asf32();
        env_cfg->decay_secs = decay.get_value_asf32();
        env_cfg->sustain_gain = sustain.get_value_asf32();
        env_cfg->release_secs = decay.get_value_asf32() * 2;

        boost_cfg->boost_mult = boost_en.get_value_asf32();
        boost_cfg->gain_mult  = gain.get_value_asf32();
    }
};

struct FltTab : Widget {
    LowPassConfig *config;

    Selector attack  = Selector("att", time_config);
    Selector decay   = Selector("dec", time_config);
    Selector sustain = Selector("sus", gain_config);

    Selector cutoff    = Selector("cut",   cutoff_config);
    Selector resonance = Selector("res",   gain_config);
    Selector contour  = Selector("cont",  contour_config);

    Table2x3Layout layout;

    FltTab(const char* key, LowPassConfig *config) 
        : Widget(key, 0, 0), config(config) {
            layout.first_row(&cutoff, &resonance, &contour);
            layout.second_row(&attack, &decay, &sustain);
            resonance.index = 3;
        }

    virtual void render(Adafruit_SSD1306 *gfx) override {
        layout.render(gfx);   
    }

    virtual void process_event(const InputEvent &event) override {
        layout.process_event(event);
        update_configs();
    }

    void update_configs() {
        config->cutoff_envelope.attack_secs =  attack.get_value_asf32();
        config->cutoff_envelope.decay_secs =   decay.get_value_asf32();
        config->cutoff_envelope.sustain_gain = sustain.get_value_asf32();
        config->cutoff_envelope.release_secs = decay.get_value_asf32() / 2;

        config->cutoff_hz   = cutoff.get_value_asf32();
        config->emphasis_perc = resonance.get_value_asf32();
        config->countour_dhz = contour.get_value_asf32();
    }
};


static auto osc1_tab = OscTab("o1",  nullptr);
static auto osc2_tab = OscTab("o2",  nullptr);
static auto osc3_tab = OscTab("o3",  nullptr);
static auto env_tab  = EnvTab("env", nullptr, nullptr);
static auto flt_tab  = FltTab("flt", nullptr);

static auto tabs = WidgetGroup();


void UiController::init() {
    osc1_tab.config = &config.osc1;
    osc2_tab.config = &config.osc2;
    osc3_tab.config = &config.osc3;
    env_tab.env_cfg =  &config.envelope;
    env_tab.boost_cfg = &config.boost;
    flt_tab.config = &config.lowpass;

    tabs.add(nullptr);
    tabs.add(&osc1_tab);
    tabs.add(&osc2_tab);
    tabs.add(&osc3_tab);
    tabs.add(&env_tab);
    tabs.add(&flt_tab);

    osc1_tab.en.nudge(1);
    osc1_tab.update_configs();
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
