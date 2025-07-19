#include "remote.hpp"
#include <NimBLEDevice.h>

#include "perf.h"
#include "config.h"
#include "uuids.h"
#include "compress.hpp"

#define SCREEN_CHUNK_NUN 4
#define SCREEN_CHUNK_SIZE (SCREEN_BUFFER_SIZE/SCREEN_CHUNK_NUN)
#define PAGE_SIZE (128*2)

static NimBLEServer* s_server;
static NimBLECharacteristic* s_screen_blechar;
static NimBLECharacteristic* s_screen_diff_blechar;
static InputEventCallback s_input_callback = nullptr;

static SemaphoreHandle_t s_screen_mutex;
static uint8_t s_screen_buffer[SCREEN_BUFFER_SIZE] = {0};
static uint8_t s_screen_buffer_sent[SCREEN_BUFFER_SIZE] = {0};


class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        // Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());

        /**
         *  We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments.
         */
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        // Serial.printf("Client disconnected - start advertising\n");
        memset(s_screen_buffer_sent, 0, SCREEN_BUFFER_SIZE);
        NimBLEDevice::startAdvertising();
    }

} server_callbacks;


namespace RemoteEvents {
    enum Value {
        None = 0,
        Input = 0x01,
        SetScreenPage = 0x02,
    };
};

class CommandCbs : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* blechar, NimBLEConnInfo& conn_info) override {        
        auto value = blechar->getValue();
        const uint8_t *data = value.data();
        size_t len = value.length();
        
        // input command
        if(len == 4 && data[0] == RemoteEvents::Input) {
            InputEvent event;
            event.id = input_id_from_uint8(data[1]);
            event.value = (int8_t)data[2];
            event.shifed = data[3] > 0;
            // Serial.printf("id: %d, val: %d, shift: %d\n", event.id, event.value, event.shifed);
            if(s_input_callback) s_input_callback(event);
        }
        else if(len == 2 && data[0] == RemoteEvents::SetScreenPage) {
            if(screen_page_idx < 0 || screen_page_idx > 4) return;
            screen_page_idx = data[1];
        }
    }
public:
    uint8_t screen_page_idx = 0;
    uint8_t get_page_idx() { return screen_page_idx; }

} command_blechar_cb;


class ScreenCbs : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        uint8_t page_idx = command_blechar_cb.get_page_idx();

        static uint8_t rle_data[SCREEN_BUFFER_SIZE * 2];
        size_t rle_len = 0;

        // START_PERF(read_bt)
        if (xSemaphoreTake(s_screen_mutex, 0)) {
            size_t write_offset = page_idx * PAGE_SIZE;
            uint8_t *data = s_screen_buffer + write_offset;
            size_t data_len = PAGE_SIZE;

            rle_compress(data, data_len, rle_data, &rle_len);

            pCharacteristic->setValue(rle_data, rle_len);

            command_blechar_cb.screen_page_idx++;
            xSemaphoreGive(s_screen_mutex);
        }
        // STOP_PERF(read_bt, 1);
    }
} screen_blechar_cb;


void remote_init() {
    s_screen_mutex = xSemaphoreCreateMutex();

    NimBLEDevice::init(BLE_NAME);

    s_server = NimBLEDevice::createServer();
    s_server->setCallbacks(&server_callbacks);

    NimBLEService* service = s_server->createService(SERVER_UUID);

    auto command_blechar = service->createCharacteristic(COMMAND_BLECHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    command_blechar->setValue("");
    command_blechar->setCallbacks(&command_blechar_cb);

    s_screen_blechar = service->createCharacteristic(SCREEN_BLECHAR_UUID, NIMBLE_PROPERTY::READ);
    s_screen_blechar->setValue(s_screen_buffer, PAGE_SIZE);
    s_screen_blechar->setCallbacks(&screen_blechar_cb);
    
    s_screen_diff_blechar = service->createCharacteristic(SCREEN_DIFF_BLECHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    s_screen_diff_blechar->setValue("");

    service->start();

    auto advertising = NimBLEDevice::getAdvertising();
    advertising->setName(BLE_NAME);
    advertising->addServiceUUID(SERVER_UUID);
    advertising->enableScanResponse(true);

    advertising->start();
}


void remote_set_input_cb(InputEventCallback cb) {
    s_input_callback = cb;
}


/**
 * rle on paged data -> x0.65
 * rle on colmaj data -> x0.93 -> nope
 */

void remote_send_screen(const uint8_t *data) {
    static uint8_t screen_patch[SCREEN_BUFFER_SIZE];
    static uint8_t screen_patch_rle[SCREEN_BUFFER_SIZE * 2];
    size_t rle_len = 0;

    if (xSemaphoreTake(s_screen_mutex, portMAX_DELAY)) {
        bool is_same = memcmp(data, s_screen_buffer, SCREEN_BUFFER_SIZE) == 0;
        if(!is_same) {
            memcpy(s_screen_buffer, data, SCREEN_BUFFER_SIZE);
        }
        xSemaphoreGive(s_screen_mutex);
        // s_screen_diff_blechar->setValue(s_screen_buffer, PAGE_SIZE);
        if(!is_same) s_screen_diff_blechar->notify();
    }
    return;
}



// void print_byte(uint8_t byte) {
//     for (int bit = 7; bit >= 0; --bit) {
//         if (byte & (1 << bit)) {
//          Serial.print("█"); // Unicode block
//         } else {
//             Serial.print(" ");
//         }
//     }
// }

// void print_screen_paged(const uint8_t *data) {
//     for (size_t y = 0; y < 64; ++y) {
//         for (size_t x = 0; x < 128; ++x) {
//             auto byte = (data[x + (y / 8) * 128] & (1 << (y & 7)));
//             if (byte) {
//                 Serial.print("█"); // Unicode block
//             } else {
//                 Serial.print(" ");
//             }
//         }
//         Serial.println();
//     }   
// }

// void print_screen_colmaj(const uint8_t *data) {
//     for (size_t y = 0; y < 64; ++y) {
//         for (size_t x = 0; x < 128; ++x) {
//             uint8_t byte = data[x * 8 + (y / 8)];
//             if (byte & (1 << (y % 8))) {
//                 Serial.print("█"); // Unicode block
//             } else {
//                 Serial.print(" ");
//             }
//         }
//         Serial.println();
//     }   
// }

// void col_to_row_major(const uint8_t* col_major, uint8_t* row_major, size_t rows, size_t cols) {
//     for (size_t i = 0; i < rows; ++i) {
//         for (size_t j = 0; j < cols; ++j) {
//             row_major[i * cols + j] = col_major[j * rows + i];
//         }
//     }
// }

// void paged_to_colmaj(const uint8_t* src, uint8_t* dest) {
//     for (size_t x = 0; x < 128; ++x) {
//         for (size_t p = 0; p < 64/8; ++p) {
//             dest[p + x * 64/8] = src[x + p * 128];
//         }
//     }
// }



// // compute patch
// for(size_t i = 0; i < SCREEN_BUFFER_SIZE; ++i) {
//     screen_patch[i] = s_screen_buffer[i] ^ s_screen_buffer_sent[i];
// }

// // compress patch
// rle_compress(screen_patch, SCREEN_BUFFER_SIZE, screen_patch_rle, &rle_len);
// Serial.println(rle_len);

// memcpy(s_screen_buffer_sent, s_screen_buffer, SCREEN_BUFFER_SIZE);

// // send patch
// s_screen_diff_blechar->setValue(screen_patch_rle, rle_len);

// if(s_screen_diff_blechar->notify()) {
    //     Serial.println(rle_len);
    //     memcpy(s_screen_buffer_sent, s_screen_buffer, SCREEN_BUFFER_SIZE);
    // }


// static uint8_t screen_colmaj[SCREEN_BUFFER_SIZE];

// paged_to_colmaj(data, screen_colmaj);
// print_screen_colmaj(screen_colmaj);

// size_t rle_len = 0;
// static uint8_t rle_screen[SCREEN_BUFFER_SIZE * 2];
// rle_compress(screen_colmaj, SCREEN_BUFFER_SIZE, rle_screen, &rle_len);

// Serial.print("screen: ");
// Serial.println((float)rle_len/SCREEN_BUFFER_SIZE);

// // print_sceen(data);
// return;

// col_to_row_major(data, screen_colmaj, 64, 128/8);

// static uint8_t rle_screen[SCREEN_BUFFER_SIZE * 2];
// size_t rle_len = 0;


// rle_compress(data, SCREEN_BUFFER_SIZE, rle_screen, &rle_len);

// static uint8_t screen_diff[SCREEN_BUFFER_SIZE];
// static uint8_t screen_diff_rle[SCREEN_BUFFER_SIZE * 2];
// size_t diff_rle_len = 0;

// for(size_t i = 0; i < SCREEN_BUFFER_SIZE; ++i) {
//     screen_diff[i] = s_screen_buffer[i] ^ data[i];
// }

// rle_compress(screen_diff, SCREEN_BUFFER_SIZE, screen_diff_rle, &diff_rle_len);


// Serial.print("screen: ");
// Serial.println((float)rle_len/SCREEN_BUFFER_SIZE);


// Serial.print("diff: ");
// Serial.println((float)diff_rle_len/SCREEN_BUFFER_SIZE);

// memcpy(s_screen_buffer, data, SCREEN_BUFFER_SIZE);
// s_screen_blechar->setValue(s_screen_buffer, SCREEN_BUFFER_SIZE/4);
