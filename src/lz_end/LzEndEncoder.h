#pragma once

#include <cstdint>

class LzEndEncoder {
public:
    LzEndEncoder();

    // Compresses `buffer` from `buffer+p` to `buffer+p_end`.
    // Writes compressed sequence to `out`.
    // Returns number of bytes written to `out`.
    int64_t encode(uint8_t* buffer, int64_t p, int64_t p_end, uint8_t* out);

private:

    template <bool is_length>
    int64_t writeValues(uint8_t* out, const int32_t* values) {
    }
};
