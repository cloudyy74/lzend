#include "ByteIO.h"
#include "lzend/Constants.h"
#include "lzend/Error.h"
#include "lzend/LzEndDecoder.h"
#include "lzend/LzEndEncoder.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fse.h>
#include <fstream>
#include <huf.h>
#include <iostream>
#include <span>
#include <vector>

Result<uint64_t> lzCompress(std::span<uint8_t> buf, uint8_t* out) {
    const uint8_t* const out_start = out;
    LzEndEncoder encoder(0);

    for (uint64_t start = 0; start < buf.size();) {
        const uint32_t remaining =
            std::min(static_cast<std::uint64_t>(lzend::maxChunkSize), buf.size() - start);
        uint8_t* marker = out;
        out += 3;

        const auto chunk_written = encoder.encode(buf.subspan(start, remaining), out);
        if (!chunk_written) {
            return std::unexpected(std::string{"Encoder error: "} + chunk_written.error());
        }
        writeUint<3>(marker, static_cast<int32_t>(*chunk_written));
        out += *chunk_written;
        start += remaining;
    }

    return out - out_start;
}

Result<void> lzDecompress(std::span<const uint8_t> buf, std::span<uint8_t> out) {
    LzEndDecoder decoder;

    while (buf.size() > 0) {
        uint32_t compressed_size = readUint<3>(buf.data());
        buf = buf.subspan(3);

        const uint32_t chunk_size = std::min(static_cast<size_t>(lzend::maxChunkSize), out.size());
        auto result = decoder.decode(buf.subspan(0, compressed_size), out.data());
        if (!result) {
            return std::unexpected(result.error());
        }

        buf = buf.subspan(compressed_size);
        out = out.subspan(chunk_size);
    }

    return {};
}

Result<std::vector<uint8_t>> readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::unexpected{std::string{"Can't open file"}};
    }

    const uint64_t size = file.tellg();
    std::vector<uint8_t> buf(size);
    file.seekg(0);

    if (!file.read(reinterpret_cast<char*>(buf.data()), size)) {
        return std::unexpected{std::string{"Can't get file size"}};
    }

    return buf;
}

Result<void> compressFile(const std::filesystem::path& input_path,
                          const std::filesystem::path& output_path) {
    auto input = readFile(input_path);
    if (!input) {
        return std::unexpected{std::string{"Can't read file: "} + input.error()};
    }
    const uint64_t input_size = input->size();

    const uint64_t count = lzend::maxChunkSize;
    const uint64_t extra_bits = std::bit_width(count) - 1;

    const uint64_t extra_size = (extra_bits * count + 7) / 8;
    const uint64_t values_size = 6 + FSE_compressBound(count) + extra_size;
    const uint64_t literals_size = 6 + HUF_compressBound(count);

    const uint64_t chunk_size = 3 + literals_size + 3 + 3 * values_size;
    ;
    const uint64_t num_chunks =
        input_size / lzend::maxChunkSize + (input_size % lzend::maxChunkSize != 0);

    auto compressed = std::make_unique_for_overwrite<uint8_t[]>(num_chunks * chunk_size);
    const auto bytes_written = lzCompress(*input, compressed.get());
    if (!bytes_written) {
        return std::unexpected(std::string{"Can't compress file: "} + bytes_written.error());
    }

    std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(compressed.get()), *bytes_written);
    output.close();

    std::cout << "Encoded " << input_size << " bytes into " << *bytes_written << std::endl;

    std::vector<uint8_t> decoded(input_size);
    auto result = lzDecompress(std::span(compressed.get(), *bytes_written), decoded);
    if (!result) {
        return std::unexpected(result.error());
    }

    if (input != decoded) {
        return std::unexpected(std::string{"Decompression validation failed"});
    }

    std::cout << "Decompression validated!" << std::endl;
    return {};
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " [input_path] [output_path]\n";
        return 1;
    }

    auto result = compressFile(argv[1], argv[2]);
    if (!result) {
        std::cerr << result.error() << '\n';
        return 1;
    }

    return 0;
}
