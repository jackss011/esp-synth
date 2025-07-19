#include "compress.hpp"

void bit_rle_compress(const uint8_t *src, size_t src_len, uint8_t *dest, size_t *dest_len) {
    size_t write_index = 0;
    size_t bit_index = 0;
    uint8_t current_bit = (src[0] >> 7) & 1;
    uint8_t run_length = 0;

    if (dest == NULL || dest_len == NULL || src == NULL || src_len == 0) {
        if (dest_len) *dest_len = 0;
        return;
    }

    dest[write_index++] = current_bit;  // store starting bit

    for (size_t byte = 0; byte < src_len; ++byte) {
        for (int bit = 7; bit >= 0; --bit) {
            uint8_t b = (src[byte] >> bit) & 1;
            if (b == current_bit) {
                run_length++;
                if (run_length == 255) {
                    dest[write_index++] = run_length;
                    run_length = 0;
                }
            } else {
                dest[write_index++] = run_length;
                current_bit = b;
                run_length = 1;
            }
        }
    }

    if (run_length > 0) {
        dest[write_index++] = run_length;
    }

    *dest_len = write_index;
}
