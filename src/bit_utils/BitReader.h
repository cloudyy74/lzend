#pragma once
#include <cstdint>

class BitReader {
public:
    BitReader(const uint8_t* buffer, const uint8_t* end) :  current_(buffer), end_(end) {
        refill();
    }

    // Consume one bit.  Result is 0 or 1.
    uint32_t readBit() {
        uint32_t result = bits_ >> 31;
        bits_ <<= 1;
        position_++; 
        return result;
    }

    // Consume n bits. Result is bitmap.
    uint32_t readBits(uint32_t n) {
        int r = bits_ >> (32 - n);
        bits_ <<= n;
        position_ += n;
        return r;
    }

    // Fill buffer with up to 32 bits.
    void refill() {
        for (; position_ >= 0; position_ -= 8) {
            bits_ |= static_cast<uint32_t>(current_ < end_ ? *current_ : 0) << position_;
            ++current_;
        }
    }

    void byteAlign() {
        int32_t extra_bits = position_ & 7;
        if (extra_bits) {
            readBits(8 - extra_bits);
        }
  }
private:
    const uint8_t* current_;
    const uint8_t* end_;
    uint32_t bits_ = 0;
    int32_t position_ = 24;
};