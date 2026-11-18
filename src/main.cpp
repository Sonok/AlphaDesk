// itch_count: counts the messages of each type in a Nasdaq ITCH 5.0 file.
//
// The file is a row of messages back to back. Each one is a 2-byte big-endian
// length followed by that many bytes, and the first of those bytes is a letter
// giving the message type.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <itch file>\n", argv[0]);
        return 1;
    }

    // Give the stream a 1 MB buffer so it reads the disk in big chunks instead
    // of a few bytes at a time. This has to be set before the file is opened.
    std::vector<char> stream_buffer(1 << 20);
    std::ifstream in;
    in.rdbuf()->pubsetbuf(stream_buffer.data(),
                          static_cast<std::streamsize>(stream_buffer.size()));
    in.open(argv[1], std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }

    // One counter for every possible value of the type byte.
    std::array<std::uint64_t, 256> counts{};
    std::uint64_t total = 0;

    // A 2-byte length can be at most 65535, so this holds any message.
    std::array<char, 65535> message;

    const auto start = std::chrono::steady_clock::now();

    while (true) {
        unsigned char length_bytes[2];
        in.read(reinterpret_cast<char*>(length_bytes), 2);
        if (in.gcount() == 0) {
            break;  // clean end of file
        }
        if (in.gcount() != 2) {
            std::fprintf(stderr, "file ends in the middle of a length\n");
            return 1;
        }

        // Big-endian: the first byte is the high half of the number.
        const std::size_t length = (std::size_t{length_bytes[0]} << 8) | length_bytes[1];
        if (length == 0) {
            std::fprintf(stderr, "message %llu has length 0\n",
                         static_cast<unsigned long long>(total));
            return 1;
        }

        in.read(message.data(), static_cast<std::streamsize>(length));
        if (static_cast<std::size_t>(in.gcount()) != length) {
            std::fprintf(stderr, "file ends in the middle of message %llu\n",
                         static_cast<unsigned long long>(total));
            return 1;
        }

        const auto type = static_cast<unsigned char>(message[0]);
        ++counts[type];
        ++total;
    }

    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    // Print the types that appeared, most common first.
    std::vector<int> seen;
    for (int type = 0; type < 256; ++type) {
        if (counts[type] != 0) {
            seen.push_back(type);
        }
    }
    std::sort(seen.begin(), seen.end(),
              [&](int a, int b) { return counts[a] > counts[b]; });

    for (int type : seen) {
        std::printf("%c  %12llu\n", type, static_cast<unsigned long long>(counts[type]));
    }
    std::printf("total %10llu\n", static_cast<unsigned long long>(total));
    std::printf("%.1f seconds, %.1f million messages per second\n", elapsed.count(),
                static_cast<double>(total) / elapsed.count() / 1e6);
    return 0;
}
