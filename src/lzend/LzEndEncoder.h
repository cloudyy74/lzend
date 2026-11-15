#pragma once

#include "BitWriter.h"
#include "ByteIO.h"
#include "ValueCoding.h"
#include "lzend/Constants.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <fse.h>
#include <stdexcept>

class LzEndEncoder {
public:
    explicit LzEndEncoder(uint8_t level = 0);

    // Compresses `buffer` from `buffer+p` to `buffer+p_end`.
    // Writes compressed sequence to `out`.
    // Returns number of bytes written to `out`.
    int64_t encode(uint8_t* buffer, int64_t p, int64_t p_end, uint8_t* out);

private:
    template <bool is_offset>
    int64_t writeValues(uint8_t* out, const int32_t* values) {
        const size_t code_count = static_cast<size_t>(num_seq);
        size_t extra_bit_count = 0;

        for (int32_t i = 0; i < num_seq; ++i) {
            const uint64_t v = static_cast<uint64_t>(values[i]);
            codes[i] = is_offset ? lzend::offsetCode(v) : lzend::lengthCode(v);

            if (v >= (is_offset ? 2 : 16)) {
                extra_bit_count += static_cast<uint32_t>(std::bit_width(v) - 1);
            }
        }

        const size_t extra_size = (extra_bit_count + 7) / 8;

        uint8_t* const out_start = out;
        out += 6;

        size_t code_size = 0;
        if (code_count > 0) {
            code_size = FSE_compress(out, FSE_compressBound(code_count), codes, code_count);
            if (FSE_isError(code_size)) {
                throw std::runtime_error(FSE_getErrorName(code_size));
            }

            if (code_size == 0) {
                std::memcpy(out, codes, code_count);
                code_size = code_count;
            } else if (code_size == 1) {
                out[0] = codes[0];
            }
        }

        writeInt<3>(out_start, static_cast<int32_t>(code_size));
        writeInt<3>(out_start + 3, static_cast<int32_t>(extra_size));
        out += code_size;

        BitWriter writer(out);
        for (int32_t i = 0; i < num_seq; ++i) {
            const uint64_t v = static_cast<uint64_t>(values[i]);
            if (v < (is_offset ? 2 : 16)) {
                continue;
            }

            const uint32_t bits = static_cast<uint32_t>(std::bit_width(v) - 1);
            const uint64_t extra = is_offset ? lzend::offsetExtra(v) : lzend::lengthExtra(v);

            writer.writeBits(static_cast<uint32_t>(extra), bits);
        }

        out += writer.finish();
        return out - out_start;
    }

    void greedyParse(const uint8_t* buffer, int64_t p, int64_t p_end);

    void optimalParse(const uint8_t* buffer, int64_t p, int64_t p_end);

    uint8_t level;
    int32_t num_seq = 0;
    int32_t num_lit = 0;
    uint8_t literals[lzend::maxChunkSize];
    uint8_t codes[lzend::maxChunkSize];
    int32_t match_offsets[lzend::maxChunkSize];
    int32_t match_lengths[lzend::maxChunkSize];
    int32_t literal_lengths[lzend::maxChunkSize];
};
