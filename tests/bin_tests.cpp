#include <rfl/bin.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <list>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace allocationProbe {

inline bool enabled{};
inline std::size_t count{};

} // namespace allocationProbe

void* operator new(std::size_t size)
{
    if (allocationProbe::enabled)
    {
        ++allocationProbe::count;
    }
    if (void* memory = std::malloc(size == 0 ? 1 : size))
    {
        return memory;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

namespace binaryTestTypes {

#pragma pack(push, 1)
struct ConstReference {
    const std::uint64_t& value;
};
struct ArrayReference {
    const std::uint64_t (&values)[2];
};
struct NestedReference {
    ConstReference values[2];
};
struct OptedInReference {
    const std::uint64_t& value;
};
struct ConstantValue {
    const std::uint64_t value;
};
#pragma pack(pop)

struct OptedInViews {
    std::string_view text;
    std::span<const std::uint32_t> values;
};

struct NestedOptedInViews {
    std::array<OptedInViews, 2> values;
};

union NativeUnion {
    std::uint32_t integer;
    float real;
};

struct NoDefaultConstructor {
    const std::uint32_t required;
    std::string text;
};

struct EmptyDynamic {
    ~EmptyDynamic() {}
};

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

template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::OptedInReference> = true;
template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::NestedReference> = true;
template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::OptedInViews> = true;
template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::NestedOptedInViews> = true;
template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::Record> = true;
template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::PointerMember> = true;
template <>
inline constexpr bool rfl::enable_native_binary<binaryTestTypes::NativeUnion> = true;
template <>
inline constexpr bool rfl::enable_native_binary<std::string_view> = true;
template <>
inline constexpr bool rfl::enable_native_binary<std::span<const std::uint32_t>> = true;

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

static_assert(std::is_trivially_copyable_v<binaryTestTypes::ConstReference>);
static_assert(alignof(binaryTestTypes::ConstReference) == 1);
static_assert(rfl::BinaryPlain<binaryTestTypes::ConstantValue>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::ConstReference>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::ArrayReference>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::NestedReference>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::OptedInReference>);
static_assert(!rfl::BinaryPlain<std::array<binaryTestTypes::ConstReference, 2>>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::ConstReference[2]>);
static_assert(!rfl::binDetail::isPlainTypeV<binaryTestTypes::OptedInReference>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::ConstReference>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::ArrayReference>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::NestedReference>);
static_assert(!rfl::BinaryPackable<binaryTestTypes::OptedInReference>);
static_assert(!rfl::BinaryPackable<std::vector<binaryTestTypes::ConstReference>>);
static_assert(rfl::BinaryPlain<Plain>);
static_assert(rfl::BinaryPlain<std::uint32_t>);
static_assert(rfl::BinaryPlain<binaryTestTypes::FixedArrays>);
static_assert(rfl::BinaryPlain<SessionIdentityPacket>);
static_assert(rfl::BinaryPlain<PlayerReadyPacket>);
static_assert(!rfl::BinaryPlain<Record>);
static_assert(!rfl::BinaryPlain<Views>);
static_assert(!rfl::BinaryPlain<ProcPathPacket>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::DynamicArrays>);
static_assert(!rfl::BinaryPlain<std::string>);
static_assert(!rfl::BinaryPlain<std::string_view>);
static_assert(!rfl::BinaryPlain<std::vector<int>>);
static_assert(!rfl::BinaryPlain<std::list<int>>);
static_assert(!rfl::BinaryPlain<std::span<const std::uint32_t>>);
static_assert(!rfl::BinaryPlain<int*>);
static_assert(!rfl::BinaryPlain<int&>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::PointerMember>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::OptedInViews>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::NestedOptedInViews>);
static_assert(!rfl::BinaryPlain<binaryTestTypes::NativeUnion>);
static_assert(rfl::BinaryPackable<binaryTestTypes::NativeUnion>);
static_assert(!rfl::binDetail::isPlainTypeV<binaryTestTypes::OptedInViews>);
static_assert(!rfl::binDetail::isPlainTypeV<binaryTestTypes::NestedOptedInViews>);
static_assert(!rfl::binDetail::isPlainTypeV<std::string_view>);
static_assert(!rfl::binDetail::isPlainTypeV<std::span<const std::uint32_t>>);
static_assert(rfl::BinaryPackable<binaryTestTypes::NoDefaultConstructor>);
static_assert(!std::default_initializable<binaryTestTypes::NoDefaultConstructor>);
static_assert(noexcept(rfl::validate<Record>(std::span<const std::byte>{})));
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
    assert(rfl::validate<T>(output));
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
    const std::array<std::size_t, 8> lengths{0, 1, 63, 64, 16383, 16384, 32767, 32768};
    for (const auto length : lengths)
    {
        const auto bytes = encode(std::string(length, 'a'));
        const auto prefix = length <= 0x7fff ? 2u : 4u;
        assert(bytes.size() == length + prefix);
        assert((std::to_integer<unsigned>(bytes[0]) & 0x80) ==
               (prefix == 4 ? 0x80u : 0u));
        rfl::binDetail::UnpackReader reader(bytes);
        std::size_t decoded{};
        assert(rfl::binDetail::readLength(reader, decoded) && decoded == length);
        assert(reader.position() == prefix);
        assert(rfl::validate<std::string>(bytes));
        std::string text;
        assert(rfl::unpack(bytes, text) && text == std::string(length, 'a'));
    }
    const auto shortBytes = encode(std::string(32767, 'a'));
    assert(shortBytes[0] == std::byte{0x7f} && shortBytes[1] == std::byte{0xff});
    const auto longBytes = encode(std::string(32768, 'a'));
    assert(longBytes[0] == std::byte{0x80} && longBytes[1] == std::byte{0x00});
    assert(longBytes[2] == std::byte{0x80} && longBytes[3] == std::byte{0x00});

    std::array<std::byte, 4> maximumBytes{};
    rfl::binDetail::PackWriter writer(maximumBytes);
    assert(rfl::binDetail::writeLength(writer, 0x7fffffffu));
    assert(writer.result().complete());
    assert(maximumBytes[0] == std::byte{0xff} && maximumBytes[1] == std::byte{0xff});
    assert(maximumBytes[2] == std::byte{0xff} && maximumBytes[3] == std::byte{0xff});
    rfl::binDetail::UnpackReader reader(maximumBytes);
    std::size_t maximum{};
    assert(rfl::binDetail::readLength(reader, maximum) && maximum == 0x7fffffffu);
    assert(reader.finished());
    assert(!rfl::validate<std::string>(maximumBytes));

    for (const auto length : {std::size_t{0x80000000u},
                              (std::numeric_limits<std::size_t>::max)()})
    {
        rfl::binDetail::PackWriter overflowWriter(maximumBytes);
        assert(!rfl::binDetail::writeLength(overflowWriter, length));
        assert(overflowWriter.result().writtenBytes == 0);
    }

    for (const auto& bytes : {shortBytes, longBytes})
    {
        const auto prefix = bytes[0] == std::byte{0x80} ? 4u : 2u;
        for (std::size_t size = 0; size != prefix; ++size)
        {
            const auto truncated = std::span<const std::byte>(bytes).first(size);
            rfl::binDetail::UnpackReader truncatedReader(truncated);
            std::size_t unchanged = 17;
            assert(!rfl::binDetail::readLength(truncatedReader, unchanged));
            assert(unchanged == 17 && !rfl::validate<std::string>(truncated));
        }
    }
    for (const auto length : {0u, 1u, 32767u})
    {
        std::array<std::byte, 4> noncanonical{
            std::byte{0x80}, std::byte{0},
            static_cast<std::byte>(length >> 8), static_cast<std::byte>(length)};
        rfl::binDetail::UnpackReader malformedReader(noncanonical);
        std::size_t unchanged = 17;
        assert(!rfl::binDetail::readLength(malformedReader, unchanged));
        assert(unchanged == 17 && !rfl::validate<std::string>(noncanonical));
        std::string text = "unchanged";
        assert(!rfl::unpack(noncanonical, text) && text == "unchanged");
    }
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
    assert(flagsBytes.size() == 2 + flags.size() * sizeof(bool));
    std::vector<bool> decodedFlags;
    assert(rfl::unpack(flagsBytes, decodedFlags) && decodedFlags == flags);
}

void testValidationWithoutAllocation()
{
    const Record record{7, {8, -9}, std::string(256, 'x'), {{1, 2}, {3, 4}}, {5, 6}};
    const auto recordBytes = encode(record);
    const NestedSequence nested{{std::string(256, 'a'), "beta"}, {{{1, 2}, {3, 4}}}};
    const auto nestedBytes = encode(nested);
    const binaryTestTypes::NoDefaultConstructor nondefault{42, std::string(128, 'b')};
    const auto nondefaultBytes = encode(nondefault);
    const std::array<std::string, 2> strings{"first", "second"};
    const auto spanBytes = encode(std::span<const std::string, 2>(strings));
    const std::string cArray[2]{"one", "two"};
    const auto cArrayBytes = encode(cArray);
    const binaryTestTypes::NativeUnion native{42};
    const auto nativeBytes = encode(native);
    auto trailingBytes = recordBytes;
    trailingBytes.push_back(std::byte{});
    const std::array<std::byte, 4> hugeCount{
        std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}};
    const auto before = recordBytes;

    allocationProbe::count = 0;
    allocationProbe::enabled = true;
    const bool recordValid = rfl::validate<Record>(recordBytes);
    const bool nestedValid = rfl::validate<NestedSequence>(nestedBytes);
    const bool nondefaultValid = rfl::validate<binaryTestTypes::NoDefaultConstructor>(nondefaultBytes);
    const bool spanValid = rfl::validate<std::span<const std::string, 2>>(spanBytes);
    const bool cArrayValid = rfl::validate<std::string[2]>(cArrayBytes);
    const bool nativeValid = rfl::validate<binaryTestTypes::NativeUnion>(nativeBytes);
    const bool trailingValid = rfl::validate<Record>(trailingBytes);
    const bool hugePlainValid = rfl::validate<std::vector<std::uint64_t>>(hugeCount);
    const bool hugeDynamicValid = rfl::validate<std::list<std::string>>(hugeCount);
    const bool emptyDynamicValid = rfl::validate<std::vector<binaryTestTypes::EmptyDynamic>>(hugeCount);
    bool truncationsRejected = true;
    for (std::size_t size = 0; size != recordBytes.size(); ++size)
    {
        truncationsRejected = truncationsRejected &&
            !rfl::validate<Record>(std::span<const std::byte>(recordBytes).first(size));
    }
    allocationProbe::enabled = false;

    assert(allocationProbe::count == 0);
    assert(recordValid && nestedValid && nondefaultValid && spanValid && cArrayValid && nativeValid);
    assert(!trailingValid && !hugePlainValid && !hugeDynamicValid && emptyDynamicValid);
    assert(truncationsRejected && recordBytes == before);
    assert(!rfl::validate<std::string>({}));
    const std::array<std::byte, 2> emptyString{};
    assert(rfl::validate<std::string>(emptyString));
}

void testValidationSpanAddressAndExtent()
{
    const std::array<std::uint32_t, 2> values{42, 99};
    const auto bytes = encode(std::span<const std::uint32_t>(values));
    assert(rfl::validate<std::span<const std::uint32_t>>(bytes));
    assert((rfl::validate<std::span<const std::uint32_t, 2>>(bytes)));
    assert(!(rfl::validate<std::span<const std::uint32_t, 3>>(bytes)));

    alignas(std::uint32_t) std::array<std::byte, 32> misaligned{};
    std::memcpy(misaligned.data() + 1, bytes.data(), bytes.size());
    const auto input = std::span<const std::byte>(misaligned).subspan(1, bytes.size());
    assert(!rfl::validate<std::span<const std::uint32_t>>(input));
    assert(rfl::validate<std::vector<std::uint32_t>>(input));
    std::span<const std::uint32_t> decoded;
    assert(!rfl::unpack(input, decoded));

    const std::span<const std::uint32_t> empty;
    const auto emptyBytes = encode(empty);
    assert(emptyBytes.size() == alignof(std::uint32_t));
    assert(rfl::validate<std::span<const std::uint32_t>>(emptyBytes));
    assert((rfl::validate<std::span<const std::uint32_t, 0>>(emptyBytes)));
    assert(!rfl::validate<std::span<const std::uint32_t>>(
        std::span<const std::byte>(emptyBytes).first(2)));
    std::memcpy(misaligned.data() + 1, emptyBytes.data(), emptyBytes.size());
    assert(!rfl::validate<std::span<const std::uint32_t>>(
        std::span<const std::byte>(misaligned).subspan(1, emptyBytes.size())));

    const binaryTestTypes::OptedInViews source{"dynamic data", values};
    const auto dynamicBytes = encode(source);
    assert(dynamicBytes.size() != sizeof(source));
    binaryTestTypes::OptedInViews result{};
    assert(rfl::unpack(dynamicBytes, result));
    assert(result.text == source.text && result.values[1] == 99);

    std::array<std::byte, sizeof(Plain) + 1> plainBytes{};
    assert(rfl::validate<Plain>(std::span<const std::byte>(plainBytes).first(sizeof(Plain))));
    assert(!rfl::validate<Plain>(plainBytes));
    assert(!rfl::validate<Plain>(std::span<const std::byte>(plainBytes).first(sizeof(Plain) - 1)));
}

} // namespace

int main()
{
    testValidationWithoutAllocation();
    testValidationSpanAddressAndExtent();
    testFixedAndDynamicArrays();
    testLengthPrefixes();
    testPlainAndVariableRoundTrip();
    testProtocolFixedMessages();
    testProtocolPathMessage();
    testViewUnpackAndAlignment();
    testNestedVariableContainers();
    testInvalidPayloadIsTransactional();
}
