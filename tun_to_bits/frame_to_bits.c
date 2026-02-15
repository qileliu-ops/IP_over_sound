/**
 * frame_to_bits.c - 将一帧字节转为比特流（每字节 8 比特，高位在前）
 */

#include "frame_to_bits.h"
#include <stdint.h>

void frame_to_bits(const uint8_t *frame, int frame_len, uint8_t *bits_out, int *nbits_out)
{
    int i, b;

    *nbits_out = frame_len * 8;
    for (i = 0; i < frame_len; i++) {
        for (b = 7; b >= 0; b--) {
            int bit_idx = i * 8 + (7 - b);
            int byte_idx = bit_idx / 8;
            int bit_in_byte = 7 - (bit_idx % 8);
            if (frame[i] & (1 << b))
                bits_out[byte_idx] |= (1 << bit_in_byte);
            else
                bits_out[byte_idx] &= ~(1 << bit_in_byte);
        }
    }
}
