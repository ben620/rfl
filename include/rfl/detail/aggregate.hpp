#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef RFL_META_MAX_MEMBERS
#define RFL_META_MAX_MEMBERS 48
#endif

static_assert(RFL_META_MAX_MEMBERS >= 0 && RFL_META_MAX_MEMBERS <= 48,
              "RFL_META_MAX_MEMBERS must be between 0 and 48.");

namespace rfl::meta::detail {

inline constexpr std::size_t max_members = RFL_META_MAX_MEMBERS;

template <std::size_t Size>
struct fixed_string {
    char value[Size]{};

    constexpr fixed_string(const char (&text)[Size]) noexcept {
        for (std::size_t index = 0; index < Size; ++index) {
            value[index] = text[index];
        }
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return {value, Size - 1};
    }
};

template <auto Pointer, fixed_string Name>
struct registered_member {
    static constexpr auto pointer = Pointer;
    static constexpr auto name = Name;
};

template <class T>
struct single_base_registration {
    static constexpr bool enabled = false;
};

template <class T>
concept registered_single_base = single_base_registration<T>::enabled;

template <class MemberPointer>
struct member_pointer_traits;

template <class Value, class Owner>
struct member_pointer_traits<Value Owner::*> {
    using value_type = Value;
    using owner_type = Owner;
};

// Every member is initialized from its own braced list, so no brace elision happens and the count
// stays exact even for members that are arrays or aggregates.
struct universal_initializer {
    template <class T>
        requires(!std::is_pointer_v<T> && !std::is_member_pointer_v<T> &&
                 !std::is_null_pointer_v<T>)
    constexpr operator T() const noexcept;
};

// A member of type std::string_view is copy-list-initialized from {universal_initializer{}}. GCC
// also considers std::string_view(std::nullptr_t) and std::string_view(const char*) and reports the
// conversion as ambiguous, so every aggregate holding a text field (TextMessage) counted one member,
// which then broke the structured binding in tie_members. Excluding pointer conversions leaves the
// copy constructor as the only candidate; aggregates that do have pointer members are counted with
// pointer_initializer, which converts to pointers only and is likewise never ambiguous.
struct pointer_initializer {
    template <class T>
        requires(std::is_pointer_v<T> || std::is_member_pointer_v<T>)
    constexpr operator T() const noexcept;
};

template <class T, std::size_t... Index>
consteval bool aggregate_initializable_impl(std::index_sequence<Index...>) {
    return requires {
        T{{(static_cast<void>(Index), universal_initializer{})}...};
    } || requires {
        T{{(static_cast<void>(Index), pointer_initializer{})}...};
    };
}

template <class T, std::size_t Count>
inline constexpr bool aggregate_initializable_v =
    aggregate_initializable_impl<T>(std::make_index_sequence<Count>{});

template <class T, std::size_t Count = max_members>
consteval std::size_t count_members_impl() {
    if constexpr (aggregate_initializable_v<T, Count>) {
        return Count;
    } else if constexpr (Count == 0) {
        return 0;
    } else {
        return count_members_impl<T, Count - 1>();
    }
}

template <class T>
consteval std::size_t count_members() {
    using object_type = std::remove_cv_t<T>;
    static_assert(std::is_aggregate_v<object_type>,
                  "rfl::meta requires an aggregate type.");
    static_assert(!std::is_union_v<object_type>,
                  "rfl::meta does not support union types.");
    static_assert(!aggregate_initializable_v<object_type, max_members + 1>,
                  "rfl::meta aggregate exceeds RFL_META_MAX_MEMBERS.");
    return count_members_impl<object_type>();
}

template <class T>
inline constexpr std::size_t member_count_v = count_members<T>();

template <class T>
constexpr auto tie_members(T&, std::integral_constant<std::size_t, 0>) noexcept {
    return std::tuple<>{};
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 1>) noexcept {
    auto& [m00] = value;
    return std::forward_as_tuple(m00);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 2>) noexcept {
    auto& [m00, m01] = value;
    return std::forward_as_tuple(m00, m01);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 3>) noexcept {
    auto& [m00, m01, m02] = value;
    return std::forward_as_tuple(m00, m01, m02);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 4>) noexcept {
    auto& [m00, m01, m02, m03] = value;
    return std::forward_as_tuple(m00, m01, m02, m03);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 5>) noexcept {
    auto& [m00, m01, m02, m03, m04] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 6>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 7>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 8>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 9>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 10>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 11>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 12>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 13>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 14>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 15>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 16>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 17>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 18>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 19>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 20>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 21>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 22>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 23>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 24>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 25>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 26>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 27>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 28>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 29>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 30>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 31>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 32>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 33>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 34>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 35>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 36>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 37>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 38>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 39>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 40>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 41>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 42>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 43>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 44>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 45>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 46>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 47>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 48>) noexcept {
    auto& [m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47] = value;
    return std::forward_as_tuple(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47);
}

template <class T>
constexpr auto tie_members(T& value) noexcept {
    return tie_members(value,
                       std::integral_constant<std::size_t, member_count_v<T>>{});
}

template <std::size_t Index, class T>
[[nodiscard]] constexpr decltype(auto) get_member(T& value) noexcept {
    return std::get<Index>(tie_members(value));
}

template <std::size_t Index, class T>
using member_type_t = std::remove_reference_t<
    std::tuple_element_t<Index, decltype(tie_members(std::declval<T&>()))>>;

template <class T>
struct external_object {
    const T value;
};

template <class T>
extern const external_object<T> fake_storage;

template <class T>
constexpr const T& fake_object() noexcept {
    return fake_storage<T>.value;
}

template <class Owner, std::size_t Index>
inline constexpr auto member_address =
    std::addressof(get_member<Index>(fake_object<Owner>()));

constexpr bool identifier_character(char character) noexcept {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') ||
           character == '_';
}

consteval std::string_view trailing_identifier(std::string_view expression) {
    auto end = expression.size();
    while (end != 0 && !identifier_character(expression[end - 1])) {
        --end;
    }
    auto begin = end;
    while (begin != 0 && identifier_character(expression[begin - 1])) {
        --begin;
    }
    return expression.substr(begin, end - begin);
}

template <class Owner, auto Address>
consteval std::string_view member_name_signature() noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
    constexpr std::string_view signature = __FUNCSIG__;
    constexpr std::string_view prefix = "member_name_signature<";
    constexpr auto begin = signature.find(prefix);
    constexpr auto end = signature.rfind(">(void)");
    static_assert(begin != std::string_view::npos &&
                  end != std::string_view::npos,
                  "rfl::meta could not parse the MSVC member signature.");
    return signature.substr(begin + prefix.size(),
                            end - begin - prefix.size());
#elif defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view signature = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix = "Address = ";
    constexpr auto begin = signature.find(prefix);
    static_assert(begin != std::string_view::npos,
                  "rfl::meta could not parse the GCC member signature.");
    constexpr auto value_begin = begin + prefix.size();
    constexpr auto semicolon = signature.find(';', value_begin);
    constexpr auto bracket = signature.find(']', value_begin);
    constexpr auto end = semicolon != std::string_view::npos ? semicolon : bracket;
    static_assert(end != std::string_view::npos,
                  "rfl::meta could not find the end of the GCC member signature.");
    return signature.substr(value_begin, end - value_begin);
#else
#   error rfl::meta supports GCC-compatible compilers and MSVC.
#endif
}

template <class Owner, std::size_t Index>
consteval std::string_view member_name() noexcept {
    constexpr auto expression =
        member_name_signature<Owner, member_address<Owner, Index>>();
    constexpr auto result = trailing_identifier(expression);
    static_assert(!result.empty(),
                  "rfl::meta extracted an empty member name.");
    return result;
}

} // namespace rfl::meta::detail

#define RFL_META_DETAIL_REGISTERED_MEMBER(Owner, Member) \
    ::rfl::meta::detail::registered_member<                \
        &Owner::Member,                                    \
        ::rfl::meta::detail::fixed_string{#Member}>

#define RFL_META_DETAIL_EXPAND(...) __VA_ARGS__

#define RFL_META_DETAIL_MEMBERS_1(D, A) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A)
#define RFL_META_DETAIL_MEMBERS_2(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_1(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_3(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_2(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_4(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_3(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_5(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_4(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_6(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_5(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_7(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_6(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_8(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_7(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_9(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_8(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_10(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_9(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_11(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_10(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_12(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_11(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_13(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_12(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_14(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_13(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_15(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_14(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_16(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_15(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_17(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_16(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_18(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_17(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_19(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_18(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_20(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_19(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_21(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_20(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_22(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_21(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_23(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_22(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_24(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_23(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_25(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_24(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_26(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_25(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_27(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_26(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_28(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_27(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_29(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_28(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_30(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_29(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_31(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_30(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_32(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_31(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_33(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_32(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_34(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_33(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_35(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_34(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_36(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_35(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_37(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_36(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_38(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_37(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_39(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_38(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_40(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_39(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_41(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_40(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_42(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_41(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_43(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_42(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_44(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_43(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_45(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_44(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_46(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_45(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_47(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_46(D, __VA_ARGS__))
#define RFL_META_DETAIL_MEMBERS_48(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_MEMBERS_47(D, __VA_ARGS__))

#define RFL_META_DETAIL_SELECT_MEMBERS( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
    _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, \
    _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, \
    Name, ...) Name

#define RFL_META_DETAIL_MEMBERS(D, ...) \
    RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_EXPAND(RFL_META_DETAIL_SELECT_MEMBERS( \
        __VA_ARGS__, \
        RFL_META_DETAIL_MEMBERS_48, RFL_META_DETAIL_MEMBERS_47, RFL_META_DETAIL_MEMBERS_46, RFL_META_DETAIL_MEMBERS_45, \
        RFL_META_DETAIL_MEMBERS_44, RFL_META_DETAIL_MEMBERS_43, RFL_META_DETAIL_MEMBERS_42, RFL_META_DETAIL_MEMBERS_41, \
        RFL_META_DETAIL_MEMBERS_40, RFL_META_DETAIL_MEMBERS_39, RFL_META_DETAIL_MEMBERS_38, RFL_META_DETAIL_MEMBERS_37, \
        RFL_META_DETAIL_MEMBERS_36, RFL_META_DETAIL_MEMBERS_35, RFL_META_DETAIL_MEMBERS_34, RFL_META_DETAIL_MEMBERS_33, \
        RFL_META_DETAIL_MEMBERS_32, RFL_META_DETAIL_MEMBERS_31, RFL_META_DETAIL_MEMBERS_30, RFL_META_DETAIL_MEMBERS_29, \
        RFL_META_DETAIL_MEMBERS_28, RFL_META_DETAIL_MEMBERS_27, RFL_META_DETAIL_MEMBERS_26, RFL_META_DETAIL_MEMBERS_25, \
        RFL_META_DETAIL_MEMBERS_24, RFL_META_DETAIL_MEMBERS_23, RFL_META_DETAIL_MEMBERS_22, RFL_META_DETAIL_MEMBERS_21, \
        RFL_META_DETAIL_MEMBERS_20, RFL_META_DETAIL_MEMBERS_19, RFL_META_DETAIL_MEMBERS_18, RFL_META_DETAIL_MEMBERS_17, \
        RFL_META_DETAIL_MEMBERS_16, RFL_META_DETAIL_MEMBERS_15, RFL_META_DETAIL_MEMBERS_14, RFL_META_DETAIL_MEMBERS_13, \
        RFL_META_DETAIL_MEMBERS_12, RFL_META_DETAIL_MEMBERS_11, RFL_META_DETAIL_MEMBERS_10, RFL_META_DETAIL_MEMBERS_9, \
        RFL_META_DETAIL_MEMBERS_8, RFL_META_DETAIL_MEMBERS_7, RFL_META_DETAIL_MEMBERS_6, RFL_META_DETAIL_MEMBERS_5, \
        RFL_META_DETAIL_MEMBERS_4, RFL_META_DETAIL_MEMBERS_3, RFL_META_DETAIL_MEMBERS_2, RFL_META_DETAIL_MEMBERS_1)) \
    (D, __VA_ARGS__))

#define RFL_META_REGISTER_SINGLE_BASE(Derived, Base, ...)                    \
    template <>                                                              \
    struct rfl::meta::detail::single_base_registration<Derived> {            \
        static constexpr bool enabled = true;                                \
        using derived_type = Derived;                                        \
        using base_type = Base;                                              \
        using direct_members = std::tuple<                                   \
            RFL_META_DETAIL_MEMBERS(Derived, __VA_ARGS__)>;                  \
        static_assert(std::is_base_of_v<base_type, derived_type>,            \
                      #Base " must be a base of " #Derived);                 \
        static_assert(std::is_convertible_v<derived_type*, base_type*>,      \
                      #Base " must be an accessible, unambiguous base of "  \
                      #Derived);                                             \
    }
