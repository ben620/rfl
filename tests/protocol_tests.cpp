#include <rfl/bin.hpp>
#include <rfl/enum.hpp>
#include RFL_PROTOCOL_HEADER

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

template <>
inline constexpr bool rfl::enable_native_binary<protocol::PlayerActionCmd> = true;

template <>
struct rfl::enum_range<protocol::ID> {
    static constexpr int min = 1;
    static constexpr int max = 0x613;
};

namespace {

template <class T>
void populate(T& value, unsigned& seed)
{
    if constexpr (std::is_array_v<T>)
    {
        for (auto& element : value)
        {
            populate(element, seed);
        }
    }
    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        value = static_cast<T>(++seed % 63 + 1);
    }
    else
    {
        rfl::meta::for_each_member(value, [&](auto, auto& field) {
            populate(field, seed);
        });
    }
}

template <class T>
void assertNativeBytes(const T& source)
{
    static_assert(rfl::BinaryPackable<T>);
    static_assert(rfl::BinaryTransactionalUnpackable<T>);
    static_assert(rfl::binDetail::isPlainTypeV<T>);
    static_assert(alignof(T) == 1);
    static_assert(std::is_trivially_copyable_v<T>);

    const auto size = rfl::packSize(source);
    assert(size && *size == sizeof(T));
    std::vector<std::byte> buffer(*size);
    const auto result = rfl::pack(source, buffer);
    assert(result.complete());
    assert(result.writtenBytes == sizeof(T));
    assert(std::memcmp(buffer.data(), std::addressof(source), sizeof(T)) == 0);

    T decoded{};
    assert(rfl::unpack(buffer, decoded));
    assert(std::memcmp(std::addressof(source), std::addressof(decoded), sizeof(T)) == 0);
    const auto view = protocol::Decode<T>(std::string_view(
        reinterpret_cast<const char*>(buffer.data()), buffer.size()));
    assert(view && std::memcmp(view, std::addressof(source), sizeof(T)) == 0);

    T unchanged = decoded;
    const std::span<const std::byte> input(buffer);
    assert(!rfl::unpack(input.first(input.size() - 1), decoded));
    assert(std::memcmp(std::addressof(unchanged), std::addressof(decoded), sizeof(T)) == 0);
    buffer.push_back(std::byte{0});
    assert(!rfl::unpack(buffer, decoded));
    assert(std::memcmp(std::addressof(unchanged), std::addressof(decoded), sizeof(T)) == 0);

    std::array<std::byte, sizeof(T) - 1> shortBuffer{};
    const auto shortResult = rfl::pack(source, shortBuffer);
    assert(shortResult.valid && !shortResult.complete());
    assert(shortResult.requiredBytes == sizeof(T));
}

template <class T>
void testProtocolType()
{
    T source{};
    assertNativeBytes(source);
    unsigned seed = 0;
    populate(source, seed);
    assertNativeBytes(source);

    std::size_t end = 0;
    rfl::meta::for_each_member(source, [&](auto member, auto&) {
        const auto offset = rfl::meta::offset_of(member, source);
        assert(offset.bytes == end && offset.bits == 0);
        assert(rfl::meta::offset_of(member) == offset);
        end += rfl::meta::size_of(member);
    });
    assert(end == sizeof(T));
}

void testMetadata()
{
    constexpr auto system = rfl::meta::members_of(rfl::meta::reflect<protocol::SystemInfo>());
    static_assert(system.size() == 2);
    constexpr auto reserved = rfl::meta::get<1>(system);
    static_assert(rfl::meta::identifier_of(reserved) == "reserved");
    static_assert(rfl::meta::size_of(reserved) == 63);
    static_assert(rfl::meta::type_of(reserved) == rfl::meta::reflect<char[63]>());
    assert(rfl::meta::offset_of(reserved).bytes == offsetof(protocol::SystemInfo, reserved));

    constexpr auto parameters = rfl::meta::members_of(rfl::meta::reflect<protocol::EabParam>());
    static_assert(parameters.size() == 15);
    constexpr auto custom = rfl::meta::get<14>(parameters);
    static_assert(rfl::meta::identifier_of(custom) == "custom");
    static_assert(rfl::meta::size_of(custom) == 128);
    assert(rfl::meta::offset_of(custom).bytes == offsetof(protocol::EabParam, custom));

    constexpr auto command = rfl::meta::members_of(rfl::meta::reflect<protocol::AgentCommand>());
    static_assert(command.size() == 9);
    constexpr auto request = rfl::meta::get<1>(command);
    static_assert(rfl::meta::alignment_of(request) == alignof(std::uint64_t));
    assert(rfl::meta::offset_of(request).bytes == 4);
    constexpr auto unused = rfl::meta::get<8>(command);
    static_assert(rfl::meta::identifier_of(unused) == "unused");
    static_assert(rfl::meta::size_of(unused) == 396);
    assert(rfl::meta::offset_of(unused).bytes == offsetof(protocol::AgentCommand, unused));
}

void testAnonymousUnionsAndBitFields()
{
    protocol::PlayerActionCmd command{};
    assertNativeBytes(command);
    command.gmCmdID = protocol::GMCmdID::SetUin;
    command.iGmParam0 = 0x1122334455667788ull;
    command.lookUp = -2.5;
    command.forwardMove = 1.25;
    command.rightMove = 9.5;
    command.moveKeyType = protocol::MoveKey::Forward;
    command.moveKeyDown = 1;
    command.moveKeyUp = 0;
    assertNativeBytes(command);

    command.moveKeyDown = 0;
    command.moveKeyUp = 1;
    assertNativeBytes(command);
}

void testGoldenBytes()
{
    const protocol::LoadingChange source{2, 1, 0x12345678};
    const std::array<std::byte, 6> expected{
        std::byte{2}, std::byte{1}, std::byte{0x78},
        std::byte{0x56}, std::byte{0x34}, std::byte{0x12}};
    std::array<std::byte, 6> bytes{};
    assert(rfl::pack(source, bytes).complete());
    assert(bytes == expected);
    protocol::LoadingChange decoded{};
    assert(rfl::unpack(expected, decoded));
    assert(decoded.type == 2 && decoded.visible == 1 && decoded.source == 0x12345678);
}

void testProtocolEnums()
{
    static_assert(rfl::enum_name(protocol::ID::SessionIdentity) == "SessionIdentity");
    static_assert(rfl::enum_cast<protocol::ID>("CltDStartResult") == protocol::ID::CltDStartResult);
    static_assert(rfl::enum_name(protocol::LogLevel::Err) == "Err");
    static_assert(rfl::enum_cast<protocol::GMCmdID>("SetUin") == protocol::GMCmdID::SetUin);
    static_assert(rfl::enum_cast<protocol::MoveKey>("Forward") == protocol::MoveKey::Forward);
    static_assert(rfl::enum_name(static_cast<protocol::ID>(7)).empty());
    for (const auto value : rfl::enum_values<protocol::ID>())
    {
        assert(rfl::enum_cast<protocol::ID>(rfl::enum_name(value)) == value);
    }
}

} // namespace

int main()
{
    testProtocolEnums();
    testProtocolType<protocol::Identity>();
    testProtocolType<protocol::Vec3>();
    testProtocolType<protocol::PlayerReady>();
    testProtocolType<protocol::PawnLocation>();
    testProtocolType<protocol::LoadingChange>();
    testProtocolType<protocol::MapGameReady>();
    testProtocolType<protocol::MapGameExited>();
    testProtocolType<protocol::SystemInfo>();
    testProtocolType<protocol::MapRspConnectedGame>();
    testProtocolType<protocol::MapRspPos>();
    testProtocolType<protocol::MapRspLineTrace>();
    testProtocolType<protocol::LineTrace>();
    testProtocolType<protocol::AgentInfo>();
    testProtocolType<protocol::AgentLoad>();
    testProtocolType<protocol::ADBDeviceInfo>();
    testProtocolType<protocol::AgentDeviceUpdate>();
    testProtocolType<protocol::MapDeviceQuality>();
    testProtocolType<protocol::MapRspDeviceQuality>();
    testProtocolType<protocol::EabParam>();
    testProtocolType<protocol::AgentReserve>();
    testProtocolType<protocol::AgentReserveResult>();
    testProtocolType<protocol::AgentStartResult>();
    testProtocolType<protocol::AgentStartAlgo>();
    testProtocolType<protocol::AgentCommand>();
    testProtocolType<protocol::AgentExit>();
    testProtocolType<protocol::AgentProcessStatus>();
    testProtocolType<protocol::AgentError>();
    testProtocolType<protocol::MapProgress>();
    testProtocolType<protocol::MapGameCtrl>();
    testProtocolType<protocol::MapRspGameCtrl>();
    testProtocolType<protocol::MapReqJobStatus>();
    testProtocolType<protocol::MapRspJobStatus>();
    testProtocolType<protocol::ProcAction>();
    testProtocolType<protocol::UInt32>();
    testMetadata();
    testAnonymousUnionsAndBitFields();
    testGoldenBytes();
}
