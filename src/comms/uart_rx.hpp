#pragma once
#include <stdint.h>
#include <esp_log.h>

static const char *DECODER_TAG = "PACKET_DECODER";

struct Packet {
    uint8_t type = 0x00;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
};

static inline __attribute__((always_inline)) 
void crc16_add(uint16_t *crc, uint8_t x) {
    *crc ^= x;
    for (int j = 0; j < 8; j++) {
        if (*crc & 1)
            *crc = (*crc >> 1) ^ 0x8408;
        else
            *crc >>= 1;
    }
}

struct PacketDecoder {
    inline __attribute__((always_inline)) bool decode(uint8_t x, Packet *out) {
        bool decoded_packet = false;

        switch(x) {
            case 0x7E: {
                size_t packet_len = write_index;
                if(packet_len >= 4 && packet_len <= MAX_PACKET_LEN) {
                    if(verify_self(packet_len)) {
                        out->type = buffer[1];
                        out->payload = buffer + 2;
                        out->payload_len = write_index - 2 - 2; // -header size -crc size
                        assert(out->payload_len <= MAX_PAYLOAD_LEN);
                        decoded_packet = true;
                    } else {
                        ESP_LOGE(DECODER_TAG, "Packet failed CRC check (total: %d)", ++_missed_packets);
                    }
                }
                else if (packet_len > MAX_PAYLOAD_LEN) {
                    ESP_LOGE(DECODER_TAG, "Packet too long %d", packet_len);
                }
                
                write_index = 0;
                escaping = false;
                break;
            } 
            case 0x7D: {
                escaping = true;
                break;
            }
            default: {
                if(write_index >= MAX_PACKET_LEN) return false; // do not write if out of buffer

                if(escaping) {
                    if(x == 0x54)         buffer[write_index++] = 0x7D;
                    else if (x == 0x53)   buffer[write_index++] = 0x7E;
                    else ESP_LOGE(DECODER_TAG, "Wrong byte after escape: %02X", x);
                    escaping = false;
                } else {
                    buffer[write_index++] = x;
                }
                break;
            } 
        }

        return decoded_packet;
    }

    void reset() {
        //memset
        write_index = 0;
        escaping = false;
    }
    
    static constexpr uint8_t MAX_PAYLOAD_LEN = 128;
    static constexpr size_t  MAX_PACKET_LEN  = MAX_PAYLOAD_LEN + 2 + 2; // full payload + header + crc
private:
    uint8_t buffer[MAX_PACKET_LEN] = {0};
    size_t write_index = 0;
    bool escaping = false;
    uint32_t _missed_packets = 0;

    bool verify_self(size_t packet_len) {
        uint16_t crc_true = (uint16_t)buffer[packet_len-2] | ((uint16_t)buffer[packet_len-1] << 8);
        uint16_t crc = 0xFFFF;

        for(size_t i = 0; i < packet_len - 2; i++) {
            crc16_add(&crc, buffer[i]);
        }

        crc = ~crc;
        return crc == crc_true;
    }
};