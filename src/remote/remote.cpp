#include "remote.hpp"
#include <NimBLEDevice.h>

#include "perf.h"
#include "config.h"
#include "uuids.h"
#include "compress.hpp"

#define SCREEN_BLOCK_NUM  4
#define SCREEN_BLOCK_SIZE (128*2)

static NimBLEServer* s_server;
static InputEventCallback s_input_callback = nullptr;

NimBLECharacteristic* s_block_chars[4] = {
    nullptr, nullptr, nullptr, nullptr
};

static uint8_t s_screen_buffer[SCREEN_BUFFER_SIZE] = {0};

static const char *BLE_REMOTE_TAG = "BLE_REMOTE";


class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGD(BLE_REMOTE_TAG, "Client address: %s\n", connInfo.getAddress().toString().c_str());
        // SET: min connection interval, max connection interval, latency, supervision timeout.
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        ESP_LOGD(BLE_REMOTE_TAG, "Client disconnected - start advertising\n");
        NimBLEDevice::startAdvertising();
    }

} server_callbacks;


namespace RemoteEvents {
    enum Value {
        None = 0,
        Input = 0x01,
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

            ESP_LOGD(BLE_REMOTE_TAG, "id: %d, val: %d, shift: %d\n", event.id, event.value, event.shifed);
            if(s_input_callback) s_input_callback(event);
        }
    }
} command_blechar_cb;


static void align_screen_blocks(bool notify = false) {
    static uint8_t rle_data[SCREEN_BUFFER_SIZE * 2];
    
    for(size_t i = 0; i < SCREEN_BLOCK_NUM; i++) {
        auto c = s_block_chars[i];
        
        // START_PERF(read_bt)
        size_t write_offset = i * SCREEN_BLOCK_SIZE;
        uint8_t *data = s_screen_buffer + write_offset;
        size_t data_len = SCREEN_BLOCK_SIZE;
        
        size_t rle_len = 0;
        rle_compress(data, data_len, rle_data, &rle_len);

        c->setValue(rle_data, rle_len);
        if(notify) c->notify();
    }
}


void remote::init() {
    NimBLEDevice::init(BLE_NAME);

    s_server = NimBLEDevice::createServer();
    s_server->setCallbacks(&server_callbacks);

    NimBLEService* service = s_server->createService(SERVER_UUID);

    auto command_blechar = service->createCharacteristic(COMMAND_BLECHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    command_blechar->setValue("");
    command_blechar->setCallbacks(&command_blechar_cb);

    for(size_t i = 0; i < SCREEN_BLOCK_NUM; i++) {
        s_block_chars[i] = service->createCharacteristic(UUID_BLOCK[i], NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    }
    align_screen_blocks(false);

    service->start();

    auto advertising = NimBLEDevice::getAdvertising();
    advertising->setName(BLE_NAME);
    advertising->addServiceUUID(SERVER_UUID);
    advertising->enableScanResponse(true);

    advertising->start();
}


void remote::set_input_cb(InputEventCallback cb) {
    s_input_callback = cb;
}


/**
 * rle on paged data -> x0.65
 * rle on colmaj data -> x0.93 -> nope
 */

void remote::send_screen(const uint8_t *data) {
    bool is_same = memcmp(data, s_screen_buffer, SCREEN_BUFFER_SIZE) == 0;
    if(!is_same) {
        memcpy(s_screen_buffer, data, SCREEN_BUFFER_SIZE);
        align_screen_blocks(true);
    }        
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
