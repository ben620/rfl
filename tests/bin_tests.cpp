#include <rfl/bin.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace binaryTestTypes {

struct Plain {
    std::uint32_t id;
    std::int16_t value;
};

struct Record {
    std::uint8_t kind;
    Plain header;
    std::string name;
    std::vector<Plain> entries;
    std::array<std::int32_t, 2> fixed;
};

struct Views {
    std::string_view name;
    std::span<const Plain> entries;
};

struct NestedSequence {
    std::list<std::string> labels;
    std::array<std::vector<std::uint16_t>, 2> groups;
};

} // namespace binaryTestTypes

namespace {

using binaryTestTypes::NestedSequence;
using binaryTestTypes::Plain;
using binaryTestTypes::Record;
using binaryTestTypes::Views;

static_assert(rfl::BinaryPackable<Plain>);
static_assert(rfl::BinaryPackable<Record>);
static_assert(!rfl::BinaryPackable<int*>);
static_assert(rfl::BinaryTransactionalUnpackable<Record>);

template <rfl::BinaryPackable T>
std::vector<std::byte> encode(const T& value)
{
    const auto size = rfl::packSize(value);
    assert(size);

    std::vector<std::byte> output(*size);
    const auto result = rfl::pack(value, std::span<std::byte>(output));
    assert(result.complete());
    return output;
}

void testLengthPrefixes()
{
    const auto shortData = encode(std::string(63, 'a'));
    assert(std::to_integer<std::uint8_t>(shortData[0]) == 63);

    const auto mediumData = encode(std::string(64, 'a'));
    assert(std::to_integer<std::uint8_t>(mediumData[0]) == 0x40);
    assert(std::to_integer<std::uint8_t>(mediumData[1]) == 64);

    const auto largeData = encode(std::string(16384, 'a'));
    assert(std::to_integer<std::uint8_t>(largeData[0]) == 0x80);
    assert(std::to_integer<std::uint8_t>(largeData[1]) == 0);
    assert(std::to_integer<std::uint8_t>(largeData[2]) == 64);
    assert(std::to_integer<std::uint8_t>(largeData[3]) == 0);
}

void testPlainAndVariableRoundTrip()
{
    const Record source{
        7,
        {0x12345678u, -321},
        "binary payload",
        {{1, 2}, {3, 4}, {5, 6}},
        {17, -18}};

    const auto size = rfl::packSize(source);
    assert(size);
    const auto encoded = encode(source);
    assert(encoded.size() == *size);

    std::vector<std::byte> storage(encoded.size());
    const auto result = rfl::pack(source, std::span<std::byte>(storage));
    assert(result.complete());
    assert(storage == encoded);

    Record decoded{};
    assert(rfl::unpack(encoded, decoded));
    assert(decoded.kind == source.kind);
    assert(decoded.header.id == source.header.id);
    assert(decoded.header.value == source.header.value);
    assert(decoded.name == source.name);
    assert(decoded.entries.size() == source.entries.size());
    assert(decoded.entries[2].id == source.entries[2].id);
    assert(decoded.entries[2].value == source.entries[2].value);
    assert(decoded.fixed == source.fixed);
}

void testViewUnpackAndAlignment()
{
    const std::array<Plain, 2> entries{{{11, 12}, {13, 14}}};
    const Views source{"view payload", std::span<const Plain>(entries)};
    const auto encoded = encode(source);

    Views decoded{};
    assert(rfl::unpack(encoded, decoded));
    assert(decoded.name == source.name);
    assert(decoded.entries.size() == entries.size());
    assert(decoded.entries[0].id == 11);
    assert(decoded.entries[1].value == 14);

    const auto address = reinterpret_cast<std::uintptr_t>(decoded.entries.data());
    assert(address % alignof(Plain) == 0);
    assert(decoded.name.data() >= reinterpret_cast<const char*>(encoded.data()));
}

void testNestedVariableContainers()
{
    const NestedSequence source{{"alpha", "beta", "gamma"}, {{
        {1, 2, 3},
        {65535, 42}}}};
    const auto encoded = encode(source);

    NestedSequence decoded{};
    assert(rfl::unpack(encoded, decoded));
    assert(decoded.labels == source.labels);
    assert(decoded.groups == source.groups);
}

void testInvalidPayloadIsTransactional()
{
    Record value{9, {8, 7}, "unchanged", {{6, 5}}, {4, 3}};
    const auto original = value;
    auto encoded = encode(value);
    encoded.pop_back();

    assert(!rfl::unpack(encoded, value));
    assert(value.kind == original.kind);
    assert(value.name == original.name);
    assert(value.entries.size() == original.entries.size());

    assert(!rfl::unpackInPlace(encoded, value));

    const std::array<std::byte, 2> malformedLength{
        std::byte{0x7f}, std::byte{0xff}};
    std::string text = "safe";
    assert(!rfl::unpack(malformedLength, text));
    assert(text == "safe");
}

} // namespace

int main()
{
    testLengthPrefixes();
    testPlainAndVariableRoundTrip();
    testViewUnpackAndAlignment();
    testNestedVariableContainers();
    testInvalidPayloadIsTransactional();
}
