#pragma once

#include <cstdint>

template <int32_t bytes>
int readInt(const uint8_t* buf) {
    int32_t v = 0;
    for (int32_t i = 0; i < bytes; ++i) {
        v |= buf[i] << (i * 8);
    }
    return v;
}

template <int32_t bytes>
void writeInt(uint8_t* buf, int32_t v) {
    for (int32_t i = 0; i < bytes; ++i) {
        *buf++ = v & 0xff;
        v >>= 8;
    }
}