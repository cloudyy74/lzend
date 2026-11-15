#pragma once

#include <bit>
#include <cstdint>

namespace lzend {

constexpr uint8_t lengthCode(uint64_t length) noexcept {
    if (length <= 15) {
        return static_cast<uint8_t>(length);
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(length) - 1);
    return static_cast<uint8_t>(12 + log);
}

constexpr uint64_t lengthExtra(uint64_t length) noexcept {
    if (length <= 15) {
        return 0;
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(length) - 1);
    return length - (uint64_t{1} << log);
}

constexpr uint8_t offsetCode(uint64_t offset) noexcept {
    if (offset < 2) {
        return static_cast<uint8_t>(offset);
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(offset) - 1);
    return static_cast<uint8_t>(1 + log);
}

constexpr uint64_t offsetExtra(uint64_t offset) noexcept {
    if (offset < 2) {
        return 0;
    }

    const uint32_t log = static_cast<uint32_t>(std::bit_width(offset) - 1);
    return offset - (uint64_t{1} << log);
}

} // namespace lzend
