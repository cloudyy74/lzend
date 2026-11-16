#include "LzEndDecoder.h"
#include "ByteIO.h"
#include "Constants.h"
#include <huf.h>

bool LzEndDecoder::decode(uint8_t* buf, uint8_t* end, uint8_t* out, uint8_t* out_end) {
    uint8_t literals[lzend::maxChunkSize];
    uint32_t literal_lengths[lzend::maxChunkSize];
    uint32_t match_offsets[lzend::maxChunkSize];
    uint32_t match_lengths[lzend::maxChunkSize];

    {
        const uint32_t num_literals = readInt<3>(buf);
        const uint32_t compressed_size = readInt<3>(buf + 3);
        buf += 6;

        if (num_literals == 0) {
            if (compressed_size != 0) {
                return false;
            }
        } else {
            const size_t decoded_size =
                HUF_decompress(literals, num_literals, buf, compressed_size);

            if (HUF_isError(decoded_size)) {
                return false;
            }
        }

        buf += compressed_size;
    }

    const uint32_t num_seq = readInt<3>(buf);
    buf += 3;

    buf += decodeValues<false>(buf, end, num_seq, literal_lengths);
    buf += decodeValues<true>(buf, end, num_seq, match_offsets);
    buf += decodeValues<false>(buf, end, num_seq, match_lengths);

    uint8_t* l_head = literals;
    for (uint32_t i = 0; i < num_seq; ++i) {
        for (uint32_t j = 0; j < literal_lengths[i]; ++j) {
            *out++ = *l_head++;
        }

        const uint8_t* match_base = out - match_offsets[i];
        for (uint32_t j = 0; j < match_lengths[i]; ++j) {
            *out++ = match_base[j];
        }
    }

    return true;
}