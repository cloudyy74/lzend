#pragma once

#include <cstdint>
#include <vector>

class LzEndEncoder {
public:
    explicit LzEndEncoder(uint8_t level = 0);

    // Compresses `buffer` from `buffer+p` to `buffer+p_end`.
    // Writes compressed sequence to `out`.
    // Returns number of bytes written to `out`.
    int64_t encode(uint8_t* buffer, int64_t p, int64_t p_end, uint8_t* out);

private:
    template <bool is_offset>
    int64_t writeValues(uint8_t* out, const int32_t* values) {}

    void greedyParse(const uint8_t* buffer, int64_t p, int64_t p_end);

    void optimalParse(const uint8_t* buffer, int64_t p, int64_t p_end);

    uint8_t level;
    std::vector<uint8_t> literals;
    std::vector<int64_t> match_offsets;
    std::vector<int64_t> match_lengths;
    std::vector<int64_t> literal_lengths;
};
