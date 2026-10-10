#include <rfl/bin.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

struct PointerMember {
    int* pointer;
};

struct PointerArray {
    int* pointers[2];
};

struct DynamicPointerMember {
    std::vector<PointerMember> entries;
};

struct FixedArrays {
    char name[96];
    int matrix[2][3];
    Plain entries[2];
};

struct DynamicArrays {
    FixedArrays header;
    std::vector<FixedArrays> entries;
    std::string labels[2];
};

enum class AIMsgID : std::uint16_t {
    PlayerReady = 5,
    SessionIdentity = 0x401,
    ProcPath = 25,
};

enum class SessionIdentityType : std::uint8_t {
    Unknown,
    GameClient,
    MapExplore,
    Manage,
    CltDaemon,
};

#pragma pack(push, 1)
template <class MsgType, AIMsgID Message>
struct MsgHeader {
    AIMsgID message{Message};
    std::uint32_t length{
        static_cast<std::uint32_t>(sizeof(MsgType) - sizeof(message) - sizeof(length))};

    bool operator==(const MsgHeader&) const = default;
};

struct SessionIdentityPacket;
struct PlayerReadyPacket;
struct ProcPathPacket;

using SessionIdentityHeader =
    MsgHeader<SessionIdentityPacket, AIMsgID::SessionIdentity>;
using PlayerReadyHeader = MsgHeader<PlayerReadyPacket, AIMsgID::PlayerReady>;
using ProcPathHeader = MsgHeader<ProcPathPacket, AIMsgID::ProcPath>;

struct SessionIdentityBody {
    std::uint32_t magic{0x41535649};
    std::uint16_t version{1};
    SessionIdentityType sessionType{SessionIdentityType::Unknown};
    std::uint8_t reserved{};

    bool operator==(const SessionIdentityBody&) const = default;
};

struct SessionIdentityPacket : SessionIdentityHeader {
    SessionIdentityBody body{};

    bool operator==(const SessionIdentityPacket&) const = default;
};

struct PlayerReadyBody {
    std::uint64_t uin{};
    std::uint32_t processID{};

    bool operator==(const PlayerReadyBody&) const = default;
};

struct PlayerReadyPacket : PlayerReadyHeader {
    PlayerReadyBody body{};

    bool operator==(const PlayerReadyPacket&) const = default;
};

struct FVec3 {
    float x{};
    float y{};
    float z{};

    bool operator==(const FVec3&) const = default;
};
#pragma pack(pop)

struct ProcPathPacket : ProcPathHeader {
    std::vector<FVec3> points;

    bool operator==(const ProcPathPacket&) const = default;
};

} // namespace binaryTestTypes

RFL_META_REGISTER_SINGLE_BASE(binaryTestTypes::SessionIdentityPacket,
                              binaryTestTypes::SessionIdentityHeader,
                              body);
RFL_META_REGISTER_SINGLE_BASE(binaryTestTypes::PlayerReadyPacket,
                              binaryTestTypes::PlayerReadyHeader,
                              body);
RFL_META_REGISTER_SINGLE_BASE(binaryTestTypes::ProcPathPacket,
                              binaryTestTypes::ProcPathHeader,
                              points);

namespace {

using binaryTestTypes::AIMsgID;
using binaryTestTypes::FVec3;
using binaryTestTypes::NestedSequence;
using binaryTestTypes::Plain;
using binaryTestTypes::PlayerReadyHeader;
using binaryTestTypes::PlayerReadyPacket;
using binaryTestTypes::ProcPathPacket;
using binaryTestTypes::Record;
using binaryTestTypes::SessionIdentityHeader;
using binaryTestTypes::SessionIdentityPacket;
using binaryTestTypes::SessionIdentityType;
using binaryTestTypes::Views;

static_assert(rfl::BinaryPackable<Plain>);
static_assert(rfl::BinaryPackable<Record>);
static_assert(rfl::BinaryPackable<SessionIdentityPacket>);
static_assert(rfl::BinaryPackable<PlayerReadyPacket>);
static_assert(rfl::BinaryPackable<ProcPathPacket>);
static_assert(!rfl::BinaryPackable<int*>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::PointerMember>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::PointerArray>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::DynamicPointerMember>);
static_assert(!rfl::BinaryPackable<std::vector<int*>>);
static_assert(!rfl::BinaryPackable<int Plain::*>);
static_assert(rfl::BinaryPackable<binaryTestTypes::FixedArrays>);
static_assert(rfl::BinaryPackable<binaryTestTypes::DynamicArrays>);
static_assert(rfl::BinaryPackable<std::vector<bool>>);
static_assert(rfl::binDetail::isPlainTypeV<binaryTestTypes::FixedArrays>);
static_assert(!rfl::binDetail::isPlainTypeV<binaryTestTypes::DynamicArrays>);
static_assert(rfl::BinaryTransactionalUnpackable<Record>);
static_assert(sizeof(SessionIdentityHeader) == 6);
static_assert(sizeof(PlayerReadyHeader) == 6);
static_assert(sizeof(SessionIdentityPacket) == 14);
static_assert(sizeof(PlayerReadyPacket) == 18);

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

template <rfl::BinaryPackable T>
void assertBufferRoundTrip(const T& source)
{
    const T beforeSerialization = source;
    const auto size = rfl::packSize(beforeSerialization);
    assert(size);

    std::vector<std::byte> buffer(*size);
    const auto packResult = rfl::pack(beforeSerialization, buffer);
    assert(packResult.complete());

    T afterDeserialization{};
    assert(rfl::unpack(buffer, afterDeserialization));
    assert(afterDeserialization == beforeSerialization);
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

void testProtocolFixedMessages()
{
    const SessionIdentityPacket identity{
        {AIMsgID::SessionIdentity, 8},
        {0x41535649, 1, SessionIdentityType::MapExplore, 0}};
    assertBufferRoundTrip(identity);

    const PlayerReadyPacket ready{
        {AIMsgID::PlayerReady, 12}, {0x1122334455667788ull, 2468}};
    assertBufferRoundTrip(ready);
}

void testProtocolPathMessage()
{
    const ProcPathPacket source{
        {AIMsgID::ProcPath, 38},
        {{1.5f, -2.0f, 3.25f}, {4.0f, 5.5f, -6.75f}, {7.0f, 8.0f, 9.0f}}};
    assertBufferRoundTrip(source);
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

void testFixedAndDynamicArrays()
{
    binaryTestTypes::FixedArrays fixed{};
    fixed.name[0] = 'A';
    fixed.name[95] = 'Z';
    fixed.matrix[1][2] = 42;
    fixed.entries[1] = {7, -8};
    const auto fixedBytes = encode(fixed);
    assert(fixedBytes.size() == sizeof(fixed));
    assert(std::memcmp(fixedBytes.data(), &fixed, sizeof(fixed)) == 0);
    binaryTestTypes::FixedArrays decodedFixed{};
    assert(rfl::unpack(fixedBytes, decodedFixed));
    assert(decodedFixed.name[95] == 'Z' && decodedFixed.matrix[1][2] == 42);
    assert(decodedFixed.entries[1].value == -8);

    const binaryTestTypes::DynamicArrays dynamic{fixed, {fixed, fixed}, {"first", "second"}};
    const auto dynamicBytes = encode(dynamic);
    assert(std::memcmp(dynamicBytes.data(), &fixed, sizeof(fixed)) == 0);
    binaryTestTypes::DynamicArrays decoded{};
    assert(rfl::unpack(dynamicBytes, decoded));
    assert(decoded.header.matrix[1][2] == 42);
    assert(decoded.entries.size() == 2 && decoded.entries[1].name[95] == 'Z');
    assert(decoded.labels[0] == "first" && decoded.labels[1] == "second");

    const std::vector<bool> flags{true, false, true, true, false};
    const auto flagsBytes = encode(flags);
    assert(flagsBytes.size() == 1 + flags.size() * sizeof(bool));
    std::vector<bool> decodedFlags;
    assert(rfl::unpack(flagsBytes, decodedFlags) && decodedFlags == flags);
}

} // namespace

int main()
{
    testFixedAndDynamicArrays();
    testLengthPrefixes();
    testPlainAndVariableRoundTrip();
    testProtocolFixedMessages();
    testProtocolPathMessage();
    testViewUnpackAndAlignment();
    testNestedVariableContainers();
    testInvalidPayloadIsTransactional();
}
