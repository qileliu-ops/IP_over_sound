/**
 * encapsulate.c - 将 IP 包封装成帧（调用 protocol_encapsulate）
 */

#include "encapsulate.h"
#include "../include/protocol.h"

int encapsulate(const uint8_t *ip_payload, int ip_len, uint8_t *frame_out)
{
    return protocol_encapsulate(ip_payload, ip_len, frame_out);
}
