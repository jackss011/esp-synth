#pragma once 
#include <cinttypes>
#include <cstddef>

void bit_rle_compress(const uint8_t *src, const size_t src_len, uint8_t *dest, size_t *dest_len);


inline void rle_compress(const uint8_t *src, const size_t src_len, uint8_t *dest, size_t *dest_len) {
    if (src_len == 0 || src == NULL || dest == NULL || dest_len == NULL) {
        if (dest_len) *dest_len = 0;
        return;
    }

    size_t write_index = 0;
    size_t read_index = 0;

    while (read_index < src_len) {
        uint8_t value = src[read_index];
        size_t run_length = 1;

        // Count how many times this byte repeats (max 255 to fit in 1 byte)
        while ((read_index + run_length < src_len) &&
               (src[read_index + run_length] == value) &&
               (run_length < 255)) {
            run_length++;
        }

        // Write value and count
        dest[write_index++] = value;
        dest[write_index++] = (uint8_t)run_length;

        read_index += run_length;
    }

    *dest_len = write_index;
}