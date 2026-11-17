#include "LzEndEncoder.h"

#include "ByteIO.h"
#include "lzend/Constants.h"
#include <cstdint>
#include <cstring>
#include <expected>
#include <huf.h>

LzEndEncoder::LzEndEncoder(uint8_t level) : level(level) {}

Result<uint32_t> LzEndEncoder::encode(std::span<const uint8_t> buf, uint8_t* out) {
    if (buf.size() > lzend::maxChunkSize) {
        return std::unexpected(std::string{"Input block exceeds maxChunkSize"});
    }

    uint8_t* out_start = out;

    if (level == 0) {
        greedyParse(buf);
    } else {
        optimalParse(buf);
    }

    // Write literal section
    {
        writeUint<3>(out, num_lit);
        out += 3;

        uint8_t* marker = out;
        out += 3;

        size_t bytes_written = 0;

        if (num_lit > 0) {
            bytes_written = HUF_compress(out, HUF_compressBound(num_lit), literals, num_lit);

            if (HUF_isError(bytes_written)) {
                return std::unexpected(std::string{"HUF compression failed: "} +
                                       HUF_getErrorName(bytes_written));
            }

            if (bytes_written == 0) {
                memcpy(out, literals, num_lit);
                bytes_written = num_lit;
            }
        }

        writeUint<3>(marker, bytes_written);
        out += bytes_written;
    }

    writeUint<3>(out, num_seq);
    out += 3;

    // a. Literal lengths
    auto written = writeValues<false>(out, std::span(literal_lengths, num_seq));
    if (!written) {
        return std::unexpected(std::string{"Can't encode literal lengths: "} + written.error());
    }
    out += *written;

    // b. Match offsets
    written = writeValues<true>(out, std::span(match_offsets, num_seq));
    if (!written) {
        return std::unexpected(std::string{"Can't encode match offsets: "} + written.error());
    }
    out += *written;

    // c. Match lengths
    written = writeValues<false>(out, std::span(match_lengths, num_seq));
    if (!written) {
        return std::unexpected(std::string{"Can't encode match lengths: "} + written.error());
    }
    out += *written;

    return out - out_start;
}

void LzEndEncoder::greedyParse(std::span<const uint8_t> buffer) {
    num_seq = 0;
    num_lit = 0;

    uint32_t p = 0;
    const uint32_t p_end = buffer.size();

    uint32_t num_phrases = 0;

    uint32_t num_literals = 0;

    while (p < p_end) {
        const uint32_t pattern_size = p_end - p - 1;
        pi[0] = 0;

        // Prefix function for buffer[p .. p_end - 1)
        for (uint32_t i = 1; i < pattern_size; ++i) {
            uint32_t j = pi[i - 1];

            while (j > 0 && buffer[p + i] != buffer[p + j]) {
                j = pi[j - 1];
            }

            if (buffer[p + i] == buffer[p + j]) {
                ++j;
            }

            pi[i] = j;
        }

        uint32_t best_length = 0;
        uint32_t best_offset = 0;

        uint32_t matched = 0;
        uint32_t scan = 0;

        if (pattern_size > 0) {
            for (uint32_t source = 0; source < num_phrases; ++source) {
                for (; scan < phrase_ends[source]; ++scan) {
                    while (matched > 0 &&
                           (matched == pattern_size || buffer[p + matched] != buffer[scan])) {
                        matched = pi[matched - 1];
                    }

                    if (matched < pattern_size && buffer[p + matched] == buffer[scan]) {
                        ++matched;
                    }
                }

                if (matched > best_length) {
                    best_length = matched;
                    best_offset = p - (phrase_ends[source] - best_length);

                    if (best_length == pattern_size) {
                        break;
                    }
                }
            }
        }

        if (best_length > 0) {
            literal_lengths[num_seq] = num_literals;
            match_offsets[num_seq] = best_offset;
            match_lengths[num_seq] = best_length;

            num_literals = 0;
            num_seq++;
        }

        literals[num_lit++] = buffer[p + best_length];
        ++num_literals;

        p += best_length + 1;
        phrase_ends[num_phrases++] = p;
    }

    if (num_literals > 0) {
        literal_lengths[num_seq] = num_literals;
        match_offsets[num_seq] = 0;
        match_lengths[num_seq] = 0;
        num_seq++;
    }
}

void LzEndEncoder::optimalParse(std::span<const uint8_t> buffer) {
    // TODO
}
