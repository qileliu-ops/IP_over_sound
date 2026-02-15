/**
 * frame_to_bits.h - 将一帧字节转为比特流（每字节 8 比特，高位在前）
 */

#ifndef FRAME_TO_BITS_H
#define FRAME_TO_BITS_H

#include <stdint.h>

/**
 * 帧转比特流
 * @param frame    帧数据
 * @param frame_len 帧字节数
 * @param bits_out 输出比特流（按字节存，每字节 8 比特）
 * @param nbits_out 输出总比特数（= frame_len * 8）
 */
void frame_to_bits(const uint8_t *frame, int frame_len, uint8_t *bits_out, int *nbits_out);

#endif /* FRAME_TO_BITS_H */
