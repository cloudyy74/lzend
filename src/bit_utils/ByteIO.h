#pragma once

#include <cstdint>

template <uint32_t bytes>
uint32_t readUint(const uint8_t* buf) {
    uint32_t v = 0;
    for (uint32_t i = 0; i < bytes; ++i) {
        v |= buf[i] << (i * 8);
    }
    return v;
}

template <uint32_t bytes>
void writeUint(uint8_t* buf, uint32_t v) {
    for (uint32_t i = 0; i < bytes; ++i) {
        *buf++ = v & 0xff;
        v >>= 8;
    }
}