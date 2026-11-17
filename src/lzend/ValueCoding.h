#pragma once

#include "Constants.h"

#include <bit>
#include <cstdint>

namespace lzend {

constexpr uint8_t lengthCode(uint32_t length) noexcept {
    if (length < lengthExtraThreshold) {
        return static_cast<uint8_t>(length);
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(length) - 1);
    return static_cast<uint8_t>(lengthCodeBase + log);
}

constexpr uint32_t lengthExtra(uint32_t length) noexcept {
    if (length < lengthExtraThreshold) {
        return 0;
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(length) - 1);
    return length - (1 << log);
}

constexpr uint8_t offsetCode(uint32_t offset) noexcept {
    if (offset < offsetExtraThreshold) {
        return static_cast<uint8_t>(offset);
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(offset) - 1);
    return static_cast<uint8_t>(offsetCodeBase + log);
}

constexpr uint32_t offsetExtra(uint32_t offset) noexcept {
    if (offset < offsetExtraThreshold) {
        return 0;
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(offset) - 1);
    return offset - (1 << log);
}

} // namespace lzend
