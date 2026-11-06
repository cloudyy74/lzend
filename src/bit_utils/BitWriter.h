#pragma once
#include <cstdint>

class BitWriter {
public:
    BitWriter(uint8_t* buffer) : start_(buffer), current_(buffer) {}

    // Write a single bit.
    void writeBit(uint32_t b) {
        bits_ = (bits_ << 1) | b;
        ++position_;
        if (position_ >= 8) {
            flush();
        }
    }

    void writeBits(uint32_t b, uint32_t n) {
        bits_ = (bits_ << n) | b;
        position_ += n;
        if (position_ >= 8) {
            flush();
        }
    }

    // Flush all bits, and return number of bytes written to buffer.
    int64_t finish() {
        flush();
        if (position_ > 0) {
            *current_ = (bits_ & ((1 << position_) - 1)) << (8 - position_);
            ++current_;
            position_ = 0;
        }
        return current_ - start_;
    }

private:
    // Write accumulated bits to buffer.
    void flush() {
        while (position_ >= 8) {
            position_ -= 8;
            *current_ = (bits_ >> position_) & 0xFF;
            ++current_;
        }
    }

    uint8_t* start_;
    uint8_t* current_;
    uint32_t bits_ = 0;
    int position_ = 0;
};