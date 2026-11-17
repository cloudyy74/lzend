#include "LzEndDecoder.h"
#include "ByteIO.h"
#include "Constants.h"
#include <expected>
#include <huf.h>
#include <span>

Result<void> LzEndDecoder::decode(std::span<const uint8_t> buf, uint8_t* out) {
    uint8_t literals[lzend::maxChunkSize];
    uint32_t literal_lengths[lzend::maxChunkSize];
    uint32_t match_offsets[lzend::maxChunkSize];
    uint32_t match_lengths[lzend::maxChunkSize];

    {
        if (buf.size() < 6) {
            return std::unexpected(std::string{"Unexpected end of input"});
        }
        const uint32_t num_literals = readUint<3>(buf.data());
        const uint32_t compressed_size = readUint<3>(buf.data() + 3);
        buf = buf.subspan(6);

        if (num_literals == 0) {
            if (compressed_size != 0) {
                return std::unexpected(std::string{"Invalid literal section size"});
            }
        } else {
            const size_t decoded_size =
                HUF_decompress(literals, num_literals, buf.data(), compressed_size);

            if (HUF_isError(decoded_size)) {
                return std::unexpected(std::string{"HUF decompression failed: "} +
                                       HUF_getErrorName(decoded_size));
            }
        }

        buf = buf.subspan(compressed_size);
    }

    const uint32_t num_seq = readUint<3>(buf.data());
    buf = buf.subspan(3);

    auto size = decodeValues<false>(buf, std::span{literal_lengths, num_seq});
    if (!size) {
        return std::unexpected(std::string{"Failed to decode literal lengths"} + size.error());
    }
    buf = buf.subspan(*size);

    size = decodeValues<true>(buf, std::span{match_offsets, num_seq});
    if (!size) {
        return std::unexpected(std::string{"Failed to decode match offsets"} + size.error());
    }
    buf = buf.subspan(*size);

    size = decodeValues<false>(buf, std::span{match_lengths, num_seq});
    if (!size) {
        return std::unexpected(std::string{"Failed to decode match lengths"} + size.error());
    }
    buf = buf.subspan(*size);

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

    return {};
}