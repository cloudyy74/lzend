#include "LzEndEncoder.h"

#include <cstdint>

LzEndEncoder::LzEndEncoder(uint8_t level) : level(level) {}

int64_t LzEndEncoder::encode(uint8_t* buffer, int64_t p, int64_t p_end, uint8_t* out) {
    // TODO

    uint8_t* out_start = out;

    if (level == 0) {
        greedyParse(buffer, p, p_end);
    } else {
        optimalParse(buffer, p, p_end);
    }

    // Write literal section
    {
    }

    // a. Literal lengths

    // b. Match offsets

    // c. Match lengths

    return out - out_start;
}

void LzEndEncoder::greedyParse(const uint8_t* buffer, int64_t p, int64_t p_end) {
    literals.clear();
    match_offsets.clear();
    match_lengths.clear();
    literal_lengths.clear();

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
            literal_lengths.push_back(num_literals);
            match_offsets.push_back(best_offset);
            match_lengths.push_back(best_length);

            num_literals = 0;
        }

        literals.push_back(buffer[p + best_length]);
        ++num_literals;

        p += best_length + 1;
        phrase_ends.push_back(p);
    }

    if (num_literals > 0) {
        literal_lengths.push_back(num_literals);
        match_offsets.push_back(0);
        match_lengths.push_back(0);
    }
}

void LzEndEncoder::optimalParse(const uint8_t* buffer, int64_t p, int64_t p_end) {
    // TODO
}