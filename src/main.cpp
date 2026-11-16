#include "ByteIO.h"
#include "lzend/Constants.h"
#include "lzend/LzEndDecoder.h"
#include "lzend/LzEndEncoder.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <fse.h>
#include <fstream>
#include <huf.h>
#include <iostream>
#include <memory>

int64_t lzCompress(uint8_t* buf, int64_t len, uint8_t* out) {
    uint8_t* const out_start = out;
    LzEndEncoder encoder(0);

    for (int64_t start = 0; start < len;) {
        const int64_t remaining = std::min(lzend::maxChunkSize, len - start);
        uint8_t* marker = out;
        out += 3;

        const int64_t chunk_written = encoder.encode(buf, start, start + remaining, out);
        writeInt<3>(marker, static_cast<int32_t>(chunk_written));
        out += chunk_written;
        start += remaining;
    }

    return out - out_start;
}

void lzDecompress(uint8_t* buf, int64_t len, uint8_t* out, int64_t out_len) {
    LzEndDecoder decoder;
    const uint8_t* buf_end = buf + len;

    while (buf < buf_end) {
        int32_t compressed_size = readInt<3>(buf);
        buf += 3;

        decoder.decode(buf, buf + compressed_size, out,
                       out + std::min(lzend::maxChunkSize, out_len));

        buf += compressed_size;
        out += lzend::maxChunkSize;
        out_len -= lzend::maxChunkSize;
    }
}

std::unique_ptr<uint8_t[]> readFile(const char* path, int64_t& len) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    len = static_cast<int64_t>(input.tellg());
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[len]);
    input.seekg(0);
    input.read(reinterpret_cast<char*>(buffer.get()), len);
    return buffer;
}

int checkBytes(uint8_t* buf, uint8_t* actual, int64_t len) {
    int bad = 0;
    for (int64_t i = 0; i < len; ++i) {
        uint8_t orig = actual[i];
        uint8_t v = buf[i];
        if (orig != v) {
            if (++bad > 20) {
                fprintf(stderr, "more than 20 busted\n");
                break;
            }
            fprintf(stderr, "bad idx:%lld,orig:%d,decoded:%d\n", i, orig, v);
        }
    }
    return bad;
}

void compressFile(const char* input_path, const char* output_path) {
    int64_t len = 0;
    std::unique_ptr<uint8_t[]> input = readFile(input_path, len);

    const size_t count = static_cast<size_t>(lzend::maxChunkSize);
    const uint32_t extra_bits = static_cast<uint32_t>(std::bit_width(count) - 1);
    const size_t extra_size = (extra_bits * count + 7) / 8;
    const size_t values_size = 6 + FSE_compressBound(count) + extra_size;
    const int64_t chunk_capacity =
        static_cast<int64_t>(3 + 6 + HUF_compressBound(count) + 3 + 3 * values_size);
    const int64_t num_chunks = len / lzend::maxChunkSize + (len % lzend::maxChunkSize != 0);

    std::unique_ptr<uint8_t[]> compressed(new uint8_t[num_chunks * chunk_capacity]);
    const int64_t bytes_written = lzCompress(input.get(), len, compressed.get());

    std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(compressed.get()), bytes_written);
    output.close();

    std::unique_ptr<uint8_t[]> decoded(new uint8_t[len]);
    lzDecompress(compressed.get(), bytes_written, decoded.get(), len);

    if (checkBytes(decoded.get(), input.get(), len) == 0) {
        printf("Decompression validated!\n");
    } else {
        printf("Decompression failed!\n");
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " [input_path] [output_path]\n";
        return 1;
    }

    try {
        compressFile(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
