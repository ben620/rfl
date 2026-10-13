#pragma once

#include <rfl/meta.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace rfl {

// Native binary format for same-ABI file and network transports.
//
// Plain objects are copied as their native object representation after natural
// alignment. Variable members are emitted in declaration order without a
// structure-level header. Big-endian lengths use a high-bit size tag: 0 = two
// bytes (15 length bits), 1 = four bytes (31 length bits). Four-byte lengths must
// exceed 0x7fff. Lengths are element counts except strings, measured in bytes.
//
// This format intentionally excludes wstring and wstring_view. Payloads use native
// endian and native ABI: peers must agree on byte order, scalar representation,
// compiler ABI, and the definitions of packed plain structs.

// Legacy opt-in for audited opaque native-layout types. Reflectable aggregates
// still require recursive plain members. BinaryPlain never accepts opaque opt-ins.
// The application is responsible for excluding pointers and agreeing on the ABI.
template <class T>
inline constexpr bool enable_native_binary = false;

struct BinaryPackResult {
    std::size_t writtenBytes{};
    std::size_t requiredBytes{};
    bool valid{};

    [[nodiscard]] constexpr bool complete() const noexcept
    {
        return valid && writtenBytes == requiredBytes;
    }
};

namespace binDetail {

static_assert(std::endian::native == std::endian::little,
              "rfl::bin currently supports little-endian platforms only.");

template <class>
inline constexpr bool dependentFalse = false;

template <class T>
using ObjectType = std::remove_cvref_t<T>;

template <class T>
struct VectorTraits : std::false_type {
    using ValueType = void;
};

template <class T, class Allocator>
struct VectorTraits<std::vector<T, Allocator>> : std::true_type {
    using ValueType = T;
};

template <class T>
inline constexpr bool isVector = VectorTraits<ObjectType<T>>::value;

template <class T>
struct ListTraits : std::false_type {
    using ValueType = void;
};

template <class T, class Allocator>
struct ListTraits<std::list<T, Allocator>> : std::true_type {
    using ValueType = T;
};

template <class T>
inline constexpr bool isList = ListTraits<ObjectType<T>>::value;

template <class T>
struct ArrayTraits : std::false_type {
    using ValueType = void;
};

template <class T, std::size_t Size>
struct ArrayTraits<std::array<T, Size>> : std::true_type {
    using ValueType = T;
    static constexpr std::size_t size = Size;
};

template <class T, std::size_t Size>
struct ArrayTraits<T[Size]> : std::true_type {
    using ValueType = T;
    static constexpr std::size_t size = Size;
};

template <class T>
inline constexpr bool isArray = ArrayTraits<ObjectType<T>>::value;

template <class T>
struct SpanTraits : std::false_type {
    using ElementType = void;
    static constexpr std::size_t extent = std::dynamic_extent;
};

template <class T, std::size_t Extent>
struct SpanTraits<std::span<T, Extent>> : std::true_type {
    using ElementType = T;
    static constexpr std::size_t extent = Extent;
};

template <class T>
inline constexpr bool isSpan = SpanTraits<ObjectType<T>>::value;

template <class T>
struct SequenceElement;

template <class T, class Allocator>
struct SequenceElement<std::vector<T, Allocator>> {
    using Type = T;
};

template <class T, class Allocator>
struct SequenceElement<std::list<T, Allocator>> {
    using Type = T;
};

template <class T, std::size_t Extent>
struct SequenceElement<std::span<T, Extent>> {
    using Type = std::remove_cv_t<T>;
};

template <class T>
using SequenceElementType = typename SequenceElement<ObjectType<T>>::Type;

template <class T>
inline constexpr bool isString = std::same_as<ObjectType<T>, std::string>;

template <class T>
inline constexpr bool isStringView =
    std::same_as<ObjectType<T>, std::string_view>;

template <class T>
inline constexpr bool isDirectPlain =
    std::is_arithmetic_v<ObjectType<T>> || std::is_enum_v<ObjectType<T>> ||
    std::same_as<ObjectType<T>, std::byte>;

template <class T, bool AllowNative = true>
consteval bool isPlainType();

template <class T>
consteval bool isSerializableType();

template <class T>
consteval bool hasNativeValueLayout()
{
    using ValueType = ObjectType<T>;
    if constexpr (meta::detail::registered_single_base<ValueType>)
    {
        using BaseType = typename meta::detail::single_base_registration<ValueType>::base_type;
        return hasNativeValueLayout<BaseType>();
    }
    else
    {
        return std::is_standard_layout_v<ValueType>;
    }
}

template <class T, bool AllowNative>
consteval bool isPlainAggregate()
{
    using ValueType = ObjectType<T>;
    if constexpr (!meta::reflectable_type<ValueType> ||
                  !std::is_trivially_copyable_v<ValueType> ||
                  !hasNativeValueLayout<ValueType>())
    {
        return false;
    }
    else
    {
        bool result = true;
        meta::for_each(meta::nonstatic_data_members_of(meta::reflect<ValueType>()),
                       [&]<class Member>(Member member) {
                           using FieldType = meta::reflected_type_t<
                               decltype(meta::type_of(member))>;
                           result = result && isPlainType<FieldType, AllowNative>();
                       });
        return result;
    }
}

template <class T, bool AllowNative>
consteval bool isPlainType()
{
    using ValueType = ObjectType<T>;
    if constexpr (std::is_pointer_v<ValueType> ||
                  std::is_member_pointer_v<ValueType> || std::is_reference_v<T> ||
                  isString<ValueType> || isStringView<ValueType> ||
                  isVector<ValueType> || isList<ValueType> || isSpan<ValueType>)
    {
        return false;
    }
    else if constexpr (isDirectPlain<ValueType>)
    {
        return true;
    }
    else if constexpr (isArray<ValueType>)
    {
        return isPlainType<typename ArrayTraits<ValueType>::ValueType, AllowNative>();
    }
    else if constexpr (meta::reflectable_type<ValueType>)
    {
        return isPlainAggregate<ValueType, AllowNative>();
    }
    else if constexpr (AllowNative && enable_native_binary<ValueType>)
    {
        static_assert(std::is_trivially_copyable_v<ValueType>,
                      "Native binary opt-in requires a trivially copyable type.");
        return true;
    }
    else
    {
        return false;
    }
}

template <class T>
consteval bool isSerializableAggregate()
{
    using ValueType = ObjectType<T>;
    if constexpr (!meta::reflectable_type<ValueType> ||
                  (std::is_trivially_copyable_v<ValueType> && !hasNativeValueLayout<ValueType>()))
    {
        return false;
    }
    else
    {
        bool result = true;
        meta::for_each(meta::nonstatic_data_members_of(meta::reflect<ValueType>()),
                       [&]<class Member>(Member member) {
                           using FieldType = meta::reflected_type_t<
                               decltype(meta::type_of(member))>;
                           result = result && isSerializableType<FieldType>();
                       });
        return result;
    }
}

template <class T>
consteval bool isSerializableType()
{
    using ValueType = ObjectType<T>;
    if constexpr (isPlainType<ValueType>() || isString<ValueType> ||
                  isStringView<ValueType>)
    {
        return true;
    }
    else if constexpr (isVector<ValueType>)
    {
        return isSerializableType<typename VectorTraits<ValueType>::ValueType>();
    }
    else if constexpr (isList<ValueType>)
    {
        return isSerializableType<typename ListTraits<ValueType>::ValueType>();
    }
    else if constexpr (isArray<ValueType>)
    {
        return isSerializableType<typename ArrayTraits<ValueType>::ValueType>();
    }
    else if constexpr (isSpan<ValueType>)
    {
        return isSerializableType<std::remove_cv_t<
            typename SpanTraits<ValueType>::ElementType>>();
    }
    else if constexpr (std::is_pointer_v<ValueType> || std::is_reference_v<T>)
    {
        return false;
    }
    else
    {
        return isSerializableAggregate<ValueType>();
    }
}

template <class T>
inline constexpr bool isPlainTypeV = isPlainType<T>();

template <class T>
inline constexpr bool isSerializableTypeV = isSerializableType<T>();

class PackWriter {
public:
    explicit PackWriter(std::span<std::byte> output) noexcept : output_(output) {}

    bool align(std::size_t alignment) noexcept
    {
        std::size_t padding = (alignment - (requiredBytes_ & (alignment - 1))) &
                              (alignment - 1);
        std::array<std::byte, alignof(std::max_align_t)> zeros{};
        while (padding != 0)
        {
            const auto count = (std::min)(padding, zeros.size());
            if (!write(std::span<const std::byte>(zeros.data(), count)))
            {
                return false;
            }
            padding -= count;
        }
        return true;
    }

    bool write(std::span<const std::byte> bytes) noexcept
    {
        const auto available = output_.size() - writtenBytes_;
        const auto count = (std::min)(available, bytes.size());
        if (count != 0)
        {
            std::memcpy(output_.data() + writtenBytes_, bytes.data(), count);
            writtenBytes_ += count;
        }
        if (requiredBytes_ > (std::numeric_limits<std::size_t>::max)() - bytes.size())
        {
            valid_ = false;
            return false;
        }
        requiredBytes_ += bytes.size();
        return true;
    }

    bool writeByte(std::byte value) noexcept
    {
        return write(std::span<const std::byte>(&value, 1));
    }

    [[nodiscard]] BinaryPackResult result() const noexcept
    {
        return {writtenBytes_, requiredBytes_, valid_};
    }

private:
    std::span<std::byte> output_;
    std::size_t writtenBytes_{};
    std::size_t requiredBytes_{};
    bool valid_{true};
};

class UnpackReader {
public:
    explicit UnpackReader(std::span<const std::byte> input) noexcept : input_(input) {}

    [[nodiscard]] std::size_t position() const noexcept
    {
        return position_;
    }

    bool align(std::size_t alignment) noexcept
    {
        const auto padding = (alignment - (position_ & (alignment - 1))) &
                             (alignment - 1);
        return skip(padding);
    }

    bool read(void* destination, std::size_t size) noexcept
    {
        if (remaining() < size)
        {
            return false;
        }
        std::memcpy(destination, input_.data() + position_, size);
        position_ += size;
        return true;
    }

    [[nodiscard]] std::span<const std::byte> view(std::size_t size) noexcept
    {
        if (remaining() < size)
        {
            return {};
        }
        const auto result = input_.subspan(position_, size);
        position_ += size;
        return result;
    }

    bool skip(std::size_t size) noexcept
    {
        if (remaining() < size)
        {
            return false;
        }
        position_ += size;
        return true;
    }

    [[nodiscard]] bool finished() const noexcept
    {
        return position_ == input_.size();
    }

    [[nodiscard]] std::size_t remaining() const noexcept
    {
        return input_.size() - position_;
    }

private:
    std::span<const std::byte> input_;
    std::size_t position_{};
};

inline bool writeLength(PackWriter& writer, std::size_t value) noexcept
{
    if (value > 0x7fffffffu)
    {
        return false;
    }
    const auto length = static_cast<std::uint32_t>(value);
    const std::size_t bytes = length <= 0x7fff ? 2 : 4;
    const std::uint8_t tag = bytes == 2 ? 0 : 0x80;
    const auto highShift = static_cast<unsigned>((bytes - 1) * 8);
    if (!writer.writeByte(static_cast<std::byte>(tag | (length >> highShift))))
    {
        return false;
    }
    for (std::size_t index = bytes - 1; index != 0; --index)
    {
        const auto shift = static_cast<unsigned>((index - 1) * 8);
        if (!writer.writeByte(static_cast<std::byte>(length >> shift)))
        {
            return false;
        }
    }
    return true;
}

inline bool readLength(UnpackReader& reader, std::size_t& value) noexcept
{
    const auto first = reader.view(1);
    if (first.empty())
    {
        return false;
    }
    const auto firstValue = std::to_integer<std::uint8_t>(first.front());
    const std::size_t bytes = (firstValue & 0x80) == 0 ? 2 : 4;
    const auto tail = reader.view(bytes - 1);
    if (tail.size() != bytes - 1)
    {
        return false;
    }
    std::uint32_t result = firstValue & 0x7f;
    for (const auto next : tail)
    {
        result = (result << 8) | std::to_integer<std::uint8_t>(next);
    }
    if (bytes == 4 && result <= 0x7fff)
    {
        return false;
    }
    if constexpr (sizeof(std::size_t) < sizeof(std::uint32_t))
    {
        if (result > (std::numeric_limits<std::size_t>::max)())
        {
            return false;
        }
    }
    value = static_cast<std::size_t>(result);
    return true;
}

template <class T>
bool packValue(PackWriter& writer, const T& value, bool naturallyAligned);

template <class T>
bool unpackValue(UnpackReader& reader, T& value, bool naturallyAligned);

template <class T>
bool packPlain(PackWriter& writer, const T& value, bool naturallyAligned)
{
    if (naturallyAligned && !writer.align(alignof(T)))
    {
        return false;
    }
    return writer.write(std::as_bytes(std::span<const T, 1>(std::addressof(value), 1)));
}

template <class T>
bool unpackPlain(UnpackReader& reader, T& value, bool naturallyAligned)
{
    if (naturallyAligned && !reader.align(alignof(T)))
    {
        return false;
    }
    return reader.read(std::addressof(value), sizeof(T));
}

template <class T>
bool packSequence(PackWriter& writer, const T& sequence)
{
    using ElementType = SequenceElementType<T>;
    if (sequence.size() > (std::numeric_limits<std::size_t>::max)() / sizeof(ElementType) ||
        !writeLength(writer, sequence.size()))
    {
        return false;
    }

    if constexpr (isPlainTypeV<ElementType>)
    {
        if (!writer.align(alignof(ElementType)))
        {
            return false;
        }
        if constexpr (isVector<T> && std::same_as<ElementType, bool>)
        {
            for (const bool item : sequence)
            {
                if (!packPlain(writer, item, false))
                {
                    return false;
                }
            }
            return true;
        }
        else if constexpr (isVector<T> || isSpan<T>)
        {
            return writer.write(std::as_bytes(std::span(sequence.data(), sequence.size())));
        }
        else
        {
            for (const auto& item : sequence)
            {
                if (!packPlain(writer, item, false))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        for (const auto& item : sequence)
        {
            if (!packValue(writer, item, false))
            {
                return false;
            }
        }
    }
    return true;
}

template <class T>
bool packValue(PackWriter& writer, const T& value, bool naturallyAligned)
{
    using ValueType = ObjectType<T>;
    if constexpr (isPlainTypeV<ValueType>)
    {
        return packPlain(writer, value, naturallyAligned);
    }
    else if constexpr (isString<ValueType> || isStringView<ValueType>)
    {
        if (!writeLength(writer, value.size()))
        {
            return false;
        }
        return writer.write(std::as_bytes(std::span(value.data(), value.size())));
    }
    else if constexpr (isVector<ValueType> || isList<ValueType> || isSpan<ValueType>)
    {
        return packSequence(writer, value);
    }
    else if constexpr (isArray<ValueType>)
    {
        for (const auto& item : value)
        {
            if (!packValue(writer, item, isPlainTypeV<ObjectType<decltype(item)>>))
            {
                return false;
            }
        }
        return true;
    }
    else if constexpr (meta::reflectable_type<ValueType>)
    {
        bool success = true;
        meta::for_each_member_while(value, [&](auto, const auto& field) {
            using FieldType = ObjectType<decltype(field)>;
            success = packValue(writer, field, isPlainTypeV<FieldType>);
            return success;
        });
        return success;
    }
    else
    {
        static_assert(dependentFalse<ValueType>, "Unsupported binary type.");
    }
}

template <class T>
bool unpackSequence(UnpackReader& reader, T& sequence)
{
    using ValueType = ObjectType<T>;
    using ElementType = typename VectorTraits<ValueType>::ValueType;
    std::size_t count{};
    if (!readLength(reader, count))
    {
        return false;
    }
    if (count > ((std::numeric_limits<std::size_t>::max)() / sizeof(ElementType)))
    {
        return false;
    }

    if constexpr (isPlainTypeV<ElementType>)
    {
        if (!reader.align(alignof(ElementType)) ||
            count > reader.remaining() / sizeof(ElementType))
        {
            return false;
        }
        sequence.resize(count);
        if constexpr (std::same_as<ElementType, bool>)
        {
            for (std::size_t index = 0; index != count; ++index)
            {
                bool item{};
                if (!unpackPlain(reader, item, false))
                {
                    return false;
                }
                sequence[index] = item;
            }
            return true;
        }
        else
        {
            return reader.read(sequence.data(), count * sizeof(ElementType));
        }
    }
    else
    {
        if (count > reader.remaining())
        {
            return false;
        }
        sequence.clear();
        for (std::size_t index = 0; index != count; ++index)
        {
            sequence.emplace_back();
            if (!unpackValue(reader, sequence.back(), false))
            {
                return false;
            }
        }
        return true;
    }
}

template <class T>
bool unpackList(UnpackReader& reader, T& sequence)
{
    using ValueType = ObjectType<T>;
    using ElementType = typename ListTraits<ValueType>::ValueType;
    std::size_t count{};
    if (!readLength(reader, count))
    {
        return false;
    }

    if constexpr (isPlainTypeV<ElementType>)
    {
        if (!reader.align(alignof(ElementType)) ||
            count > reader.remaining() / sizeof(ElementType))
        {
            return false;
        }
        for (std::size_t index = 0; index != count; ++index)
        {
            sequence.emplace_back();
            if (!unpackPlain(reader, sequence.back(), false))
            {
                return false;
            }
        }
    }
    else
    {
        if (count > reader.remaining())
        {
            return false;
        }
        for (std::size_t index = 0; index != count; ++index)
        {
            sequence.emplace_back();
            if (!unpackValue(reader, sequence.back(), false))
            {
                return false;
            }
        }
    }
    return true;
}

template <class T>
bool unpackSpan(UnpackReader& reader, T& value)
{
    using SpanType = ObjectType<T>;
    using ElementType = typename SpanTraits<SpanType>::ElementType;
    using ObjectElementType = std::remove_cv_t<ElementType>;
    static_assert(std::is_const_v<ElementType>,
                  "unpack supports only std::span<const T>; input storage is read-only.");
    static_assert(isPlainTypeV<ObjectElementType>,
                  "Zero-copy unpack for std::span requires a plain element type.");

    std::size_t count{};
    if (!readLength(reader, count))
    {
        return false;
    }
    constexpr auto extent = SpanTraits<SpanType>::extent;
    if constexpr (extent != std::dynamic_extent)
    {
        if (count != extent)
        {
            return false;
        }
    }
    if (count > ((std::numeric_limits<std::size_t>::max)() / sizeof(ObjectElementType)) ||
        !reader.align(alignof(ObjectElementType)))
    {
        return false;
    }

    const auto bytes = reader.view(count * sizeof(ObjectElementType));
    if (bytes.size() != count * sizeof(ObjectElementType))
    {
        return false;
    }
    const auto address = reinterpret_cast<std::uintptr_t>(bytes.data());
    if ((address & (alignof(ObjectElementType) - 1)) != 0)
    {
        return false;
    }
    value = SpanType(reinterpret_cast<const ObjectElementType*>(bytes.data()), count);
    return true;
}

template <class T>
bool unpackValue(UnpackReader& reader, T& value, bool naturallyAligned)
{
    using ValueType = ObjectType<T>;
    if constexpr (isPlainTypeV<ValueType>)
    {
        return unpackPlain(reader, value, naturallyAligned);
    }
    else if constexpr (isString<ValueType>)
    {
        std::size_t size{};
        if (!readLength(reader, size) || size > reader.remaining())
        {
            return false;
        }
        value.resize(size);
        return reader.read(value.data(), size);
    }
    else if constexpr (isStringView<ValueType>)
    {
        std::size_t size{};
        if (!readLength(reader, size))
        {
            return false;
        }
        const auto bytes = reader.view(size);
        if (bytes.size() != size)
        {
            return false;
        }
        value = std::string_view(reinterpret_cast<const char*>(bytes.data()), size);
        return true;
    }
    else if constexpr (isVector<ValueType>)
    {
        return unpackSequence(reader, value);
    }
    else if constexpr (isList<ValueType>)
    {
        value.clear();
        return unpackList(reader, value);
    }
    else if constexpr (isSpan<ValueType>)
    {
        return unpackSpan(reader, value);
    }
    else if constexpr (isArray<ValueType>)
    {
        for (auto& item : value)
        {
            if (!unpackValue(reader, item, isPlainTypeV<ObjectType<decltype(item)>>))
            {
                return false;
            }
        }
        return true;
    }
    else if constexpr (meta::reflectable_type<ValueType>)
    {
        bool success = true;
        meta::for_each_member_while(value, [&](auto, auto& field) {
            using FieldType = ObjectType<decltype(field)>;
            success = unpackValue(reader, field, isPlainTypeV<FieldType>);
            return success;
        });
        return success;
    }
    else
    {
        static_assert(dependentFalse<ValueType>, "Unsupported binary type.");
    }
}

template <class T>
consteval std::size_t minimumPackedSize()
{
    using ValueType = ObjectType<T>;
    constexpr auto maximum = (std::numeric_limits<std::size_t>::max)();
    if constexpr (isPlainTypeV<ValueType>)
    {
        return sizeof(ValueType);
    }
    else if constexpr (isString<ValueType> || isStringView<ValueType> ||
                       isVector<ValueType> || isList<ValueType> || isSpan<ValueType>)
    {
        return 2;
    }
    else if constexpr (isArray<ValueType>)
    {
        constexpr auto size = minimumPackedSize<typename ArrayTraits<ValueType>::ValueType>();
        constexpr auto count = ArrayTraits<ValueType>::size;
        if constexpr (size != 0 && count > maximum / size)
        {
            return maximum;
        }
        return count * size;
    }
    else
    {
        std::size_t result{};
        meta::for_each(meta::nonstatic_data_members_of(meta::reflect<ValueType>()),
                       [&]<class Member>(Member member) {
                           using FieldType = meta::reflected_type_t<
                               decltype(meta::type_of(member))>;
                           constexpr auto size = minimumPackedSize<FieldType>();
                           result = size > maximum - result ? maximum : result + size;
                       });
        return result;
    }
}

template <class T>
bool validateValue(UnpackReader& reader, bool naturallyAligned) noexcept;

template <class T>
bool validateSequence(UnpackReader& reader) noexcept
{
    using ValueType = ObjectType<T>;
    using ElementType = SequenceElementType<ValueType>;
    std::size_t count{};
    if (!readLength(reader, count) ||
        count > (std::numeric_limits<std::size_t>::max)() / sizeof(ElementType))
    {
        return false;
    }
    if constexpr (isSpan<ValueType>)
    {
        constexpr auto extent = SpanTraits<ValueType>::extent;
        if constexpr (extent != std::dynamic_extent)
        {
            if (count != extent)
            {
                return false;
            }
        }
    }
    if constexpr (isPlainTypeV<ElementType>)
    {
        if (!reader.align(alignof(ElementType)) ||
            count > reader.remaining() / sizeof(ElementType))
        {
            return false;
        }
        if constexpr (isSpan<ValueType>)
        {
            const auto bytes = reader.view(count * sizeof(ElementType));
            const auto address = reinterpret_cast<std::uintptr_t>(bytes.data());
            return (address & (alignof(ElementType) - 1)) == 0;
        }
        return reader.skip(count * sizeof(ElementType));
    }
    else
    {
        constexpr auto minimum = minimumPackedSize<ElementType>();
        if constexpr (minimum == 0)
        {
            return true;
        }
        else
        {
            if (count > reader.remaining() / minimum)
            {
                return false;
            }
            for (std::size_t index = 0; index != count; ++index)
            {
                if (!validateValue<ElementType>(reader, false))
                {
                    return false;
                }
            }
            return true;
        }
    }
}

template <class T>
bool validateValue(UnpackReader& reader, bool naturallyAligned) noexcept
{
    using ValueType = ObjectType<T>;
    if constexpr (isPlainTypeV<ValueType>)
    {
        return (!naturallyAligned || reader.align(alignof(ValueType))) &&
               reader.skip(sizeof(ValueType));
    }
    else if constexpr (isString<ValueType> || isStringView<ValueType>)
    {
        std::size_t size{};
        return readLength(reader, size) && reader.skip(size);
    }
    else if constexpr (isVector<ValueType> || isList<ValueType> || isSpan<ValueType>)
    {
        return validateSequence<ValueType>(reader);
    }
    else if constexpr (isArray<ValueType>)
    {
        using ElementType = typename ArrayTraits<ValueType>::ValueType;
        if constexpr (minimumPackedSize<ElementType>() != 0)
        {
            for (std::size_t index = 0; index != ArrayTraits<ValueType>::size; ++index)
            {
                if (!validateValue<ElementType>(reader, isPlainTypeV<ElementType>))
                {
                    return false;
                }
            }
        }
        return true;
    }
    else
    {
        bool success = true;
        meta::for_each(meta::nonstatic_data_members_of(meta::reflect<ValueType>()),
                       [&]<class Member>(Member member) {
                           using FieldType = meta::reflected_type_t<
                               decltype(meta::type_of(member))>;
                           if (success)
                           {
                               success = validateValue<FieldType>(reader, isPlainTypeV<FieldType>);
                           }
                       });
        return success;
    }
}

} // namespace binDetail

template <class T>
concept BinaryPlain = binDetail::isPlainType<T, false>();

template <class T>
concept BinaryPackable = binDetail::isSerializableTypeV<T>;

template <class T>
concept BinaryTransactionalUnpackable =
    BinaryPackable<T> && std::default_initializable<std::remove_cvref_t<T>> &&
    std::assignable_from<std::remove_cvref_t<T>&, std::remove_cvref_t<T>>;

template <BinaryPackable T>
[[nodiscard]] std::optional<std::size_t> packSize(const T& value) noexcept
{
    if constexpr (binDetail::isPlainTypeV<T>)
    {
        return sizeof(T);
    }
    binDetail::PackWriter writer(std::span<std::byte>{});
    if (!binDetail::packValue(writer, value, binDetail::isPlainTypeV<T>) )
    {
        return std::nullopt;
    }
    const auto result = writer.result();
    if (!result.valid)
    {
        return std::nullopt;
    }
    return result.requiredBytes;
}

template <BinaryPackable T>
[[nodiscard]] BinaryPackResult pack(const T& value,
                                     std::span<std::byte> output) noexcept
{
    binDetail::PackWriter writer(output);
    const bool valid = binDetail::packValue(writer, value, binDetail::isPlainTypeV<T>);
    auto result = writer.result();
    result.valid = result.valid && valid;
    return result;
}

template <BinaryTransactionalUnpackable T>
[[nodiscard]] bool unpack(std::span<const std::byte> input, T& value)
{
    if constexpr (binDetail::isPlainTypeV<T>)
    {
        if (input.size() != sizeof(T))
        {
            return false;
        }
        std::memcpy(std::addressof(value), input.data(), sizeof(T));
        return true;
    }
    std::remove_cvref_t<T> parsed{};
    binDetail::UnpackReader reader(input);
    if (!binDetail::unpackValue(reader, parsed, binDetail::isPlainTypeV<T>) ||
        !reader.finished())
    {
        return false;
    }
    value = std::move(parsed);
    return true;
}

// Checks structure and exact consumption without constructing T or copying payloads.
// Native scalar representations and padding bytes are not semantically validated.
template <BinaryPackable T>
[[nodiscard]] bool validate(std::span<const std::byte> input) noexcept
{
    binDetail::UnpackReader reader(input);
    return binDetail::validateValue<T>(reader, binDetail::isPlainTypeV<T>) &&
           reader.finished();
}

template <BinaryPackable T>
[[nodiscard]] bool unpackInPlace(std::span<const std::byte> input, T& value)
{
    binDetail::UnpackReader reader(input);
    return binDetail::unpackValue(reader, value, binDetail::isPlainTypeV<T>) &&
           reader.finished();
}

} // namespace rfl
