#pragma once

#include "BitReader.h"
#include "ByteIO.h"
#include "Error.h"
#include "lzend/Constants.h"
#include <cstdint>
#include <cstring>
#include <fse.h>
#include <span>

class LzEndDecoder {
public:
    Result<void> decode(std::span<const uint8_t> buf, uint8_t* out);

private:
    template <bool is_offset>
    Result<uint32_t> decodeValues(std::span<const uint8_t> buf, std::span<uint32_t> values) {
        constexpr uint32_t extra_threshold =
            is_offset ? lzend::offsetExtraThreshold : lzend::lengthExtraThreshold;
        constexpr uint32_t code_base = is_offset ? lzend::offsetCodeBase : lzend::lengthCodeBase;
        if (buf.size() < 6) {
            return std::unexpected(std::string{"Unexpected end of input"});
        }

        const uint32_t code_size = readUint<3>(buf.data());
        const uint32_t extra_size = readUint<3>(buf.data() + 3);
        buf = buf.subspan(6);

        uint8_t codes[lzend::maxChunkSize];

        if (code_size == values.size()) {
            memcpy(codes, buf.data(), values.size());
        } else if (code_size == 1) {
            memset(codes, buf[0], values.size());
        } else {
            const size_t decoded_size = FSE_decompress(codes, values.size(), buf.data(), code_size);

            if (FSE_isError(decoded_size)) {
                return std::unexpected(std::string{"FSE decompression failed: "} +
                                       FSE_getErrorName(decoded_size));
            }
        }

        const uint8_t* extra = buf.data() + code_size;
        BitReader reader(extra, extra + extra_size);
        uint32_t bits_used = 0;

        for (uint32_t i = 0; i < values.size(); ++i) {
            const uint32_t code = codes[i];

            if (code < extra_threshold) {
                values[i] = code;
                continue;
            }

            const uint32_t bits = code - code_base;

            const uint32_t base = 1 << bits;

            reader.refill();
            values[i] = base | reader.readBits(bits);
            bits_used += bits;
        }

        return 6 + code_size + extra_size;
    }
};
