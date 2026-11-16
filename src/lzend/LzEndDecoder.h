#pragma once

#include "BitReader.h"
#include "ByteIO.h"
#include "lzend/Constants.h"
#include <cstdint>
#include <cstring>
#include <fse.h>

class LzEndDecoder {
public:
    bool decode(uint8_t* buf, uint8_t* end, uint8_t* out, uint8_t* out_end);

private:
    template <bool is_offset>
    int64_t decodeValues(const uint8_t* buf, const uint8_t* end, uint32_t num_seq,
                         uint32_t* values) {
        const uint32_t code_size = readInt<3>(buf);
        const uint32_t extra_size = readInt<3>(buf + 3);
        buf += 6;

        uint8_t codes[lzend::maxChunkSize];

        if (code_size == num_seq) {
            memcpy(codes, buf, num_seq);
        } else if (code_size == 1) {
            memset(codes, buf[0], num_seq);
        } else {
            const size_t decoded_size = FSE_decompress(codes, num_seq, buf, code_size);

            if (FSE_isError(decoded_size) || decoded_size != num_seq) {
                return -1;
            }
        }

        const uint8_t* extra = buf + code_size;
        BitReader reader(extra, extra + extra_size);
        uint64_t bits_used = 0;

        for (uint32_t i = 0; i < num_seq; ++i) {
            const uint32_t code = codes[i];

            if (code < (is_offset ? 2 : 16)) {
                values[i] = code;
                continue;
            }

            const uint32_t bits = code - (is_offset ? 1 : 12);

            const uint32_t base = 1 << bits;

            reader.refill();
            values[i] = base | reader.readBits(bits);
            bits_used += bits;
        }

        return 6 + code_size + extra_size;
    }
};