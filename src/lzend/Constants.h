#pragma once

#include <bit>
#include <cstdint>

namespace lzend {

inline constexpr uint32_t maxChunkSize = 1 << 17; // 128k

inline constexpr uint32_t offsetExtraThreshold = 2;
inline constexpr uint32_t lengthExtraThreshold = 16;

inline constexpr uint32_t offsetCodeBase =
    offsetExtraThreshold - (std::bit_width(offsetExtraThreshold) - 1);
inline constexpr uint32_t lengthCodeBase =
    lengthExtraThreshold - (std::bit_width(lengthExtraThreshold) - 1);

} // namespace lzend
