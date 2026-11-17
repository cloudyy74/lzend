#pragma once

#include "BitWriter.h"
#include "ByteIO.h"
#include "Constants.h"
#include "Error.h"
#include "ValueCoding.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <fse.h>
#include <span>

class LzEndEncoder {
public:
    explicit LzEndEncoder(uint8_t level = 0);

    Result<uint32_t> encode(std::span<const uint8_t> buffer, uint8_t* out);

private:
    template <bool is_offset>
    Result<uint32_t> writeValues(uint8_t* out, std::span<const uint32_t> values) {
        constexpr uint32_t extra_threshold =
            is_offset ? lzend::offsetExtraThreshold : lzend::lengthExtraThreshold;

        const size_t code_count = values.size();
        uint32_t extra_bit_count = 0;

        for (uint32_t i = 0; i < values.size(); ++i) {
            const uint32_t v = values[i];
            codes[i] = is_offset ? lzend::offsetCode(v) : lzend::lengthCode(v);

            if (v >= extra_threshold) {
                extra_bit_count += std::bit_width(v) - 1;
            }
        }

        const uint32_t extra_size = (extra_bit_count + 7) / 8;

        uint8_t* const out_start = out;
        out += 6;

        size_t code_size = 0;
        if (code_count > 0) {
            code_size = FSE_compress(out, FSE_compressBound(code_count), codes, code_count);
            if (FSE_isError(code_size)) {
                return std::unexpected{std::string{"FSE compression failed: "} +
                                       FSE_getErrorName(code_size)};
            }

            if (code_size == 0) {
                std::memcpy(out, codes, code_count);
                code_size = code_count;
            } else if (code_size == 1) {
                out[0] = codes[0];
            }
        }

        writeUint<3>(out_start, code_size);
        writeUint<3>(out_start + 3, extra_size);
        out += code_size;

        BitWriter writer(out);
        for (uint32_t i = 0; i < values.size(); ++i) {
            const uint32_t v = values[i];
            if (v < extra_threshold) {
                continue;
            }

            const uint32_t bits = std::bit_width(v) - 1;
            const uint32_t extra = is_offset ? lzend::offsetExtra(v) : lzend::lengthExtra(v);

            writer.writeBits(extra, bits);
        }

        out += writer.finish();
        return out - out_start;
    }

    void greedyParse(std::span<const uint8_t> buffer);

    void optimalParse(std::span<const uint8_t> buffer);

    uint8_t level;
    uint32_t num_seq = 0;
    uint32_t num_lit = 0;
    uint8_t literals[lzend::maxChunkSize];
    uint32_t match_offsets[lzend::maxChunkSize];
    uint32_t match_lengths[lzend::maxChunkSize];
    uint32_t literal_lengths[lzend::maxChunkSize];
    uint8_t codes[lzend::maxChunkSize];
    uint32_t phrase_ends[lzend::maxChunkSize];
    uint32_t pi[lzend::maxChunkSize];
};
