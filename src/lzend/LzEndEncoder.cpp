#include "LzEndEncoder.h"

#include "ByteIO.h"
#include <cstdint>
#include <cstring>
#include <huf.h>
#include <stdexcept>
#include <vector>

LzEndEncoder::LzEndEncoder(uint8_t level) : level(level) {}

int64_t LzEndEncoder::encode(uint8_t* buffer, int64_t p, int64_t p_end, uint8_t* out) {
    if (p_end - p > lzend::maxChunkSize) {
        throw std::length_error("Input block exceeds maxChunkSize");
    }

    uint8_t* out_start = out;

    if (level == 0) {
        greedyParse(buffer, p, p_end);
    } else {
        optimalParse(buffer, p, p_end);
    }

    // Write literal section
    {
        writeInt<3>(out, num_lit);
        out += 3;

        uint8_t* marker = out;
        out += 3;

        size_t bytes_written = 0;

        if (num_lit > 0) {
            bytes_written = HUF_compress(out, HUF_compressBound(num_lit), literals, num_lit);

            if (HUF_isError(bytes_written)) {
                throw std::runtime_error(HUF_getErrorName(bytes_written));
            }

            if (bytes_written == 0) {
                memcpy(out, literals, num_lit);
                bytes_written = num_lit;
            }
        }

        writeInt<3>(marker, bytes_written);
        out += bytes_written;
    }

    writeInt<3>(out, num_seq);
    out += 3;

    // a. Literal lengths
    out += writeValues<false>(out, literal_lengths);

    // b. Match offsets
    out += writeValues<true>(out, match_offsets);

    // c. Match lengths
    out += writeValues<false>(out, match_lengths);

    return out - out_start;
}

void LzEndEncoder::greedyParse(const uint8_t* buffer, int64_t p, int64_t p_end) {
    num_seq = 0;
    num_lit = 0;
    const int64_t block_begin = p;

    std::vector<int64_t> phrase_ends;
    std::vector<int64_t> pi;

    int64_t num_literals = 0;

    while (p < p_end) {
        const int64_t pattern_size = p_end - p - 1;
        pi.assign(pattern_size, 0);

        // Prefix function for buffer[p .. p_end - 1)
        for (int64_t i = 1; i < pattern_size; ++i) {
            int64_t j = pi[i - 1];

            while (j > 0 && buffer[p + i] != buffer[p + j]) {
                j = pi[j - 1];
            }

            if (buffer[p + i] == buffer[p + j]) {
                ++j;
            }

            pi[i] = j;
        }

        int64_t best_length = 0;
        int64_t best_offset = 0;

        int64_t matched = 0;
        int64_t scan = block_begin;

        if (pattern_size > 0) {
            for (int64_t source = 0; source < static_cast<int64_t>(phrase_ends.size()); ++source) {
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
        phrase_ends.push_back(p);
    }

    if (num_literals > 0) {
        literal_lengths[num_seq] = num_literals;
        match_offsets[num_seq] = 0;
        match_lengths[num_seq] = 0;
        num_seq++;
    }
}

void LzEndEncoder::optimalParse(const uint8_t* buffer, int64_t p, int64_t p_end) {
    // TODO
}
