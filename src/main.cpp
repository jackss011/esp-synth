#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/i2s.h"

#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#include "config.h"
#include "perf.h"
#include "audio/synth.hpp"
#include "audio/midi.hpp"
#include "audio/wavetable.hpp"
#include "comms/uart_rx.hpp"
#include "input/events.hpp"
#include "input/Btn.hpp"
#include "input/Encoder.hpp"
#include "ui/UiController.hpp"
#include "remote/remote.hpp"

QueueHandle_t midi_event_queue;
QueueHandle_t input_event_queue;

// ─────────────────────────────────────────────────────────────
// ||   TASK: UART RX (for midi notes only)
// ─────────────────────────────────────────────────────────────
namespace PacketType {
    enum Value {
        Midi = 0xA0,
        Log = 0xF0,
    };
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    
    uint8_t* data = (uint8_t*) malloc(UART_RX_BUFFER_SIZE);
    PacketDecoder decoder;
    Packet packet;

    while (true) {
        const size_t rx_bytes = uart_read_bytes(UART_NUM_1, data, UART_RX_BUFFER_SIZE, 0);

        for(size_t i = 0; i < rx_bytes; i++) {                
            bool has_packet = decoder.decode(data[i], &packet);
            
            if(has_packet) {
                switch(packet.type) {
                    case PacketType::Midi: {
                        MidiEvent midi_event(packet.payload);
                        xQueueGenericSend(midi_event_queue, &midi_event, 0, queueSEND_TO_BACK);
                        break;
                    }
                    case PacketType::Log: {
                        Serial.print("UART LOG: ");
                        char buffer[PacketDecoder::MAX_PAYLOAD_LEN*2] = {0};
                        strncpy(buffer, (char*)packet.payload, packet.payload_len);
                        buffer[packet.payload_len] = '\0';
                        Serial.println(buffer);
                        break;
                    }
                }
            }
        }

        delay(1);
    }
    free(data);
}


// ─────────────────────────────────────────────────────────────
// ||   TASK: AUDIO
// ─────────────────────────────────────────────────────────────
Synth synth;

struct __attribute__((packed)) AudioFrame {
    int16_t ch1;
    int16_t ch2;

    static const int16_t MAX = INT16_MAX / 2; 
};

static void i2s_task(void *arg) {
    AudioFrame frames[SYNTH_CHUNK_SIZE] = {0};
    float synth_buffer[SYNTH_CHUNK_SIZE] = {0};

    MidiEvent midi_event;

    while(true) {
        // process midi events
        while(xQueueReceive(midi_event_queue, &midi_event, 0) == pdTRUE) {
            synth.process_midi_event(midi_event);
            // ESP_LOGE("I2S_TASK", "midi event: %02X %02X %02X %02X", midi_event.header, midi_event.status, midi_event.data1, midi_event.data2);
        }

        // process synth audio
        // START_PERF(synth_loop);
        memset(synth_buffer, 0.f, SYNTH_CHUNK_SIZE * sizeof(float));
        synth.process_block(synth_buffer, SYNTH_CHUNK_SIZE);
        // STOP_PERF(synth_loop, 300);

        // set frame data
        for(size_t i = 0; i < SYNTH_CHUNK_SIZE; i++) {
            frames[i].ch1 = synth_buffer[i] * AudioFrame::MAX;
            frames[i].ch2 = synth_buffer[i] * AudioFrame::MAX;
        }

        // write to i2s
        size_t bytes_written = 0;
        const size_t write_size = SYNTH_CHUNK_SIZE * sizeof(AudioFrame);
        if(i2s_write(I2S_NUM_1, (void*)frames, write_size, &bytes_written, portMAX_DELAY) != ESP_OK) {
            ESP_LOGE("I2SR_TAG", "i2s write failed");
        }

        // chill
        delay(0);
    }
}


// ─────────────────────────────────────────────────────────────
// ||   TASK: DISPLAY
// ─────────────────────────────────────────────────────────────
Adafruit_SSD1306 display(128, 64, &Wire);


void display_task(void *arg) {
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setTextWrap(false);
    display.setCursor(0, 0);
    display.display();

    UiController controller(&display);
    controller.init();
    synth.update_config(controller.config);

    InputEvent event;

    while(true) {
        // copy to for comparison
        const auto old_config = controller.config;

        // process events
        while(xQueueReceive(input_event_queue, &event, 0) == pdTRUE) {
            controller.process_event(event);
        }

        // check for change by memory comparison
        bool config_changed = 0 != memcmp(&old_config, &controller.config, sizeof(SynthConfig));
        if(config_changed) {
            synth.update_config(controller.config); 
        }

        display.clearDisplay();
        if(controller.render_to_buffer()) {
            display.display();
        }

        delay(20); // 50hz
    }
}


// ─────────────────────────────────────────────────────────────
// ||   TASK: INPUT
// ─────────────────────────────────────────────────────────────
Btn btn_lx(5);
Btn btn_rx(33);
Btn btn_shift(32);
Encoder encoder_0(17, 4, false);
Encoder encoder_1(19, 18, false);
Encoder encoder_2(13, 23, false);


void input_task(void *arg) {
    btn_lx.begin();
    btn_rx.begin();
    btn_shift.begin();
    encoder_0.begin();
    encoder_1.begin();
    encoder_2.begin();

    delay(100); // wait for inputs to stabilize (due to internal pullup)

    while(true) {
        btn_lx.update();
        btn_rx.update();
        btn_shift.update();
        encoder_0.update();
        encoder_1.update();
        encoder_2.update();

        InputEvent event;
        event.shifed = btn_shift.pressed() ? 1 : 0;

        if(btn_lx.event()) {
            event.id = InputId::BtnLx;
            event.value = btn_lx.event();
            xQueueSendToBack(input_event_queue, &event, 0);
        }

        if(btn_rx.event()) {
            event.id = InputId::BtnRx;
            event.value = btn_rx.event();
            xQueueSendToBack(input_event_queue, &event, 0);
        }

        if(btn_shift.event()) {
            event.id = InputId::BtnShift;
            event.value = btn_shift.event();
            xQueueSendToBack(input_event_queue, &event, 0);
        }

        if(encoder_0.event()) {
            event.id = InputId::Encoder0;
            event.value = encoder_0.event();
            xQueueSendToBack(input_event_queue, &event, 0);
        }

        if(encoder_1.event()) {
            event.id = InputId::Encoder1;
            event.value = encoder_1.event();
            xQueueSendToBack(input_event_queue, &event, 0);
        }

        if(encoder_2.event()) {
            event.id = InputId::Encoder2;
            event.value = encoder_2.event();
            xQueueSendToBack(input_event_queue, &event, 0);
        }

        delay(5); // 200hz
    }
}


// ─────────────────────────────────────────────────────────────
// ||   TASK: REMOTE
// ─────────────────────────────────────────────────────────────
static void on_remote_input(const InputEvent &event) {
    xQueueSendToBack(input_event_queue, &event, pdMS_TO_TICKS(200));
}


// ─────────────────────────────────────────────────────────────
// ||   MAIN
// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    midi_event_queue =  xQueueCreate(MIDI_EVENTS_QUEUE_SIZE,  sizeof(MidiEvent));
    input_event_queue = xQueueCreate(INPUT_EVENTS_QUEUE_SIZE, sizeof(InputEvent));

    // ---- UART SETUP ----
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, UART_RX_BUFFER_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, PIN_UART_TX, PIN_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_rx_full_threshold(UART_NUM_1, 64); // reduce latency
    uart_set_rx_timeout(UART_NUM_1, 1); // reduce latency

    // ---- I2S SETUP ----
    const i2s_pin_config_t i2s_pin_config = {
        .bck_io_num = PIN_I2S_BCK,
        .ws_io_num = PIN_I2S_LCK,
        .data_out_num = PIN_I2S_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SYNTH_SR,
        .bits_per_sample = (i2s_bits_per_sample_t)16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,  // default interrupt priority
        .dma_buf_count = 2,
        .dma_buf_len = SYNTH_CHUNK_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = true,  // avoiding noise in case of data unavailability
        .fixed_mclk = 0,
        .mclk_multiple = (i2s_mclk_multiple_t) 0, // I2S_MCLK_MULTIPLE_DEFAULT
        .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT
    };
    i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &i2s_pin_config);

    // ---- DISPLAY SETUP ----
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCK);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x64
    display.clearDisplay();
    display.display();

    // ---- SYNTH SETUP ----
    synth.begin();

    // ---- REMOTE SETUP ----
    remote_init();
    remote_set_input_cb(on_remote_input);

    // ---- TASKS ----
    // core 1
    xTaskCreatePinnedToCore(input_task,   "input_task",     4096, NULL,     configMAX_PRIORITIES - 1, NULL, 1);
    xTaskCreatePinnedToCore(rx_task,      "uart_rx_task",   4096, NULL,     configMAX_PRIORITIES - 3, NULL, 1);
    xTaskCreatePinnedToCore(display_task, "display_task",   4096, NULL,     1,                        NULL, 1);
    // core 0
    xTaskCreatePinnedToCore(i2s_task,     "i2s_task",       4096, NULL,     configMAX_PRIORITIES - 1, NULL, 0);
}

void loop() { delay(1000); }
