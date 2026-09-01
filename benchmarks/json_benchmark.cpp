#include <rfl/json.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

std::size_t g_allocations = 0;
std::size_t g_allocated_bytes = 0;

struct allocation_scope {
    std::size_t allocations;
    std::size_t bytes;

    allocation_scope() : allocations(g_allocations), bytes(g_allocated_bytes) {}

    [[nodiscard]] std::size_t count() const {
        return g_allocations - allocations;
    }

    [[nodiscard]] std::size_t total_bytes() const {
        return g_allocated_bytes - bytes;
    }
};

template <class T>
void keep(const T& value) {
    static volatile std::uintptr_t sink = 0;
    sink = sink ^ reinterpret_cast<std::uintptr_t>(&value);
}

void keep_size(std::size_t value) {
    static volatile std::size_t sink = 0;
    sink = sink ^ value;
}

struct result {
    double nanoseconds_per_op;
    std::size_t allocations_per_op;
    std::size_t bytes_per_op;
};

template <class Body>
result measure(std::size_t iterations, Body&& body) {
    for (std::size_t index = 0; index < iterations / 10 + 1; ++index) {
        body();
    }

    result best{-1.0, 0, 0};
    for (int round = 0; round < 3; ++round) {
        const allocation_scope allocations;
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t index = 0; index < iterations; ++index) {
            body();
        }
        const auto stop = std::chrono::steady_clock::now();

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        const auto per_op =
            static_cast<double>(elapsed) / static_cast<double>(iterations);
        if (best.nanoseconds_per_op < 0.0 || per_op < best.nanoseconds_per_op) {
            best = {per_op,
                    allocations.count() / iterations,
                    allocations.total_bytes() / iterations};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return best;
}

std::string_view g_filter;

template <class Body>
void run(std::string_view name,
         std::size_t bytes,
         std::size_t iterations,
         Body&& body) {
    if (!g_filter.empty() && name.find(g_filter) == std::string_view::npos) {
        return;
    }
    const auto value = measure(iterations, body);
    const auto throughput = value.nanoseconds_per_op > 0.0
        ? static_cast<double>(bytes) / value.nanoseconds_per_op
        : 0.0;
    std::printf("%-42s %10.1f ns %8.2f GB/s %6zu alloc %8zu B\n",
                std::string(name).c_str(),
                value.nanoseconds_per_op,
                throughput,
                value.allocations_per_op,
                value.bytes_per_op);
}

} // namespace

namespace bench {

struct User {
    std::uint64_t id;
    std::string name;
    bool active;
};

struct Wide {
    int m00;
    int m01;
    int m02;
    int m03;
    int m04;
    int m05;
    int m06;
    int m07;
    int m08;
    int m09;
    int m10;
    int m11;
    int m12;
    int m13;
    int m14;
    int m15;
};

struct Point {
    double x;
    double y;
};

struct Track {
    std::string label;
    std::vector<Point> points;
};

struct Message {
    std::uint32_t sequence;
    std::string payload;
};

} // namespace bench

namespace {

using bench::Message;
using bench::Point;
using bench::Track;
using bench::User;
using bench::Wide;

std::string repeat(std::string_view unit, std::size_t total) {
    std::string text;
    text.reserve(total);
    while (text.size() < total) {
        text.append(unit.substr(0, (std::min)(unit.size(), total - text.size())));
    }
    return text;
}

void benchmark_serialize() {
    std::puts("--- serialize ---");

    const User user{4096, "Ada Lovelace", true};
    const auto user_json = rfl::ToJson(user);
    run("User -> new string", user_json.size(), 2000000,
        [&] { keep(rfl::ToJson(user)); });

    std::string reused;
    reused.reserve(1024);
    run("User -> reused string", user_json.size(), 2000000,
        [&] { keep_size(rfl::ToJson(user, reused)); });

    run("User -> JsonSize", user_json.size(), 2000000,
        [&] { keep_size(*rfl::JsonSize(user)); });

    char storage[512];
    run("User -> span", user_json.size(), 2000000, [&] {
        keep_size(rfl::ToJson(user, std::span<char>(storage)).written);
    });

    const Wide wide{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const auto wide_size = rfl::ToJson(wide).size();
    run("Wide(16 fields) -> reused string", wide_size, 1000000,
        [&] { keep_size(rfl::ToJson(wide, reused)); });

    std::vector<int> integers(10000);
    for (std::size_t index = 0; index < integers.size(); ++index) {
        integers[index] = static_cast<int>(index) - 5000;
    }
    const auto integers_size = rfl::ToJson(integers).size();
    run("vector<int>[10k] -> reused string", integers_size, 200,
        [&] { keep_size(rfl::ToJson(integers, reused)); });

    std::vector<double> doubles(10000);
    for (std::size_t index = 0; index < doubles.size(); ++index) {
        doubles[index] = static_cast<double>(index) * 0.25;
    }
    const auto doubles_size = rfl::ToJson(doubles).size();
    run("vector<double>[10k] -> reused string", doubles_size, 100,
        [&] { keep_size(rfl::ToJson(doubles, reused)); });

    Track track{"plain-label", {}};
    track.points.resize(5000);
    for (std::size_t index = 0; index < track.points.size(); ++index) {
        track.points[index] = {static_cast<double>(index),
                               static_cast<double>(index) * 0.5};
    }
    const auto track_size = rfl::ToJson(track).size();
    run("Track(nested 5k) -> reused string", track_size, 100,
        [&] { keep_size(rfl::ToJson(track, reused)); });

    const Message plain{7, repeat("abcdefghijklmnopqrstuvwxyz0123456789", 65536)};
    const auto plain_size = rfl::ToJson(plain).size();
    run("Message(64K plain) -> reused string", plain_size, 200,
        [&] { keep_size(rfl::ToJson(plain, reused)); });

    const Message escaped{7, repeat("abcd\"efg\nhij\\klmnopqrst", 65536)};
    const auto escaped_size = rfl::ToJson(escaped).size();
    run("Message(64K escaped) -> reused string", escaped_size, 200,
        [&] { keep_size(rfl::ToJson(escaped, reused)); });

    std::string transmitted;
    transmitted.reserve(plain_size + 64);
    run("Message(64K plain) -> sink", plain_size, 200, [&] {
        transmitted.clear();
        keep_size(rfl::ToJson(plain, [&](std::string_view chunk) {
            transmitted.append(chunk);
            return true;
        }));
    });
}

void benchmark_deserialize() {
    std::puts("--- deserialize ---");

    const User user{4096, "Ada Lovelace", true};
    const auto user_json = rfl::ToJson(user);
    run("User <- json", user_json.size(), 500000, [&] {
        User output{};
        keep_size(rfl::FromJson(user_json, output));
        keep(output);
    });

    const std::string user_unknown =
        R"({"schema":"v3","tags":["alpha","beta",{"nested":[1,2,3]}],)"
        R"("id":4096,"name":"Ada Lovelace","active":true,"trailing":null})";
    run("User <- json with unknown fields", user_unknown.size(), 200000, [&] {
        User output{};
        keep_size(rfl::FromJson(user_unknown, output));
        keep(output);
    });

    const Wide wide{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const auto wide_json = rfl::ToJson(wide);
    run("Wide(16 fields) <- json", wide_json.size(), 200000, [&] {
        Wide output{};
        keep_size(rfl::FromJson(wide_json, output));
        keep(output);
    });

    std::vector<int> integers(10000);
    for (std::size_t index = 0; index < integers.size(); ++index) {
        integers[index] = static_cast<int>(index) - 5000;
    }
    const auto integers_json = rfl::ToJson(integers);
    std::vector<int> integer_output;
    run("vector<int>[10k] <- json", integers_json.size(), 200, [&] {
        keep_size(rfl::FromJsonInPlace(integers_json, integer_output));
        keep(integer_output);
    });

    std::vector<std::string> strings(5000, "value-with-some-length");
    const auto strings_json = rfl::ToJson(strings);
    std::vector<std::string> string_output;
    run("vector<string>[5k] <- json", strings_json.size(), 100, [&] {
        keep_size(rfl::FromJsonInPlace(strings_json, string_output));
        keep(string_output);
    });

    Track track{"plain-label", {}};
    track.points.resize(5000);
    for (std::size_t index = 0; index < track.points.size(); ++index) {
        track.points[index] = {static_cast<double>(index),
                               static_cast<double>(index) * 0.5};
    }
    const auto track_json = rfl::ToJson(track);
    Track track_output;
    run("Track(nested 5k) <- json", track_json.size(), 100, [&] {
        keep_size(rfl::FromJsonInPlace(track_json, track_output));
        keep(track_output);
    });

    const Message plain{7, repeat("abcdefghijklmnopqrstuvwxyz0123456789", 65536)};
    const auto plain_json = rfl::ToJson(plain);
    Message message_output;
    run("Message(64K plain) <- json", plain_json.size(), 200, [&] {
        keep_size(rfl::FromJsonInPlace(plain_json, message_output));
        keep(message_output);
    });
}

} // namespace

void* operator new(std::size_t size) {
    ++g_allocations;
    g_allocated_bytes += size;
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

int main(int argc, char** argv) {
    if (argc > 1) {
        g_filter = argv[1];
    }
    benchmark_serialize();
    benchmark_deserialize();
}
