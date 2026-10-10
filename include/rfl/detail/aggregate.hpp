#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef RFL_META_MAX_MEMBERS
#define RFL_META_MAX_MEMBERS 16
#endif

static_assert(RFL_META_MAX_MEMBERS >= 0 && RFL_META_MAX_MEMBERS <= 16,
              "RFL_META_MAX_MEMBERS must be between 0 and 16.");

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

struct universal_initializer {
    template <class T>
    constexpr operator T() const noexcept;
};

template <class T, std::size_t... Index>
consteval bool aggregate_initializable_impl(std::index_sequence<Index...>) {
    return requires {
        T{{(static_cast<void>(Index), universal_initializer{})}...};
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
    auto& [a] = value;
    return std::forward_as_tuple(a);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 2>) noexcept {
    auto& [a, b] = value;
    return std::forward_as_tuple(a, b);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 3>) noexcept {
    auto& [a, b, c] = value;
    return std::forward_as_tuple(a, b, c);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 4>) noexcept {
    auto& [a, b, c, d] = value;
    return std::forward_as_tuple(a, b, c, d);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 5>) noexcept {
    auto& [a, b, c, d, e] = value;
    return std::forward_as_tuple(a, b, c, d, e);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 6>) noexcept {
    auto& [a, b, c, d, e, f] = value;
    return std::forward_as_tuple(a, b, c, d, e, f);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 7>) noexcept {
    auto& [a, b, c, d, e, f, g] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 8>) noexcept {
    auto& [a, b, c, d, e, f, g, h] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 9>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 10>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 11>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j, k] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j, k);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 12>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j, k, l] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j, k, l);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 13>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j, k, l, m] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j, k, l, m);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 14>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, n] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 15>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}

template <class T>
constexpr auto tie_members(T& value, std::integral_constant<std::size_t, 16>) noexcept {
    auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = value;
    return std::forward_as_tuple(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
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

#define RFL_META_DETAIL_MEMBERS_1(D, A) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A)
#define RFL_META_DETAIL_MEMBERS_2(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_1(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_3(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_2(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_4(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_3(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_5(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_4(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_6(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_5(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_7(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_6(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_8(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_7(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_9(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_8(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_10(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_9(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_11(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_10(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_12(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_11(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_13(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_12(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_14(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_13(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_15(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_14(D, __VA_ARGS__)
#define RFL_META_DETAIL_MEMBERS_16(D, A, ...) \
    RFL_META_DETAIL_REGISTERED_MEMBER(D, A), RFL_META_DETAIL_MEMBERS_15(D, __VA_ARGS__)

#define RFL_META_DETAIL_SELECT_MEMBERS(                                      \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
    Name, ...) Name

#define RFL_META_DETAIL_MEMBERS(D, ...)                                      \
    RFL_META_DETAIL_SELECT_MEMBERS(                                          \
        __VA_ARGS__,                                                         \
        RFL_META_DETAIL_MEMBERS_16, RFL_META_DETAIL_MEMBERS_15,              \
        RFL_META_DETAIL_MEMBERS_14, RFL_META_DETAIL_MEMBERS_13,              \
        RFL_META_DETAIL_MEMBERS_12, RFL_META_DETAIL_MEMBERS_11,              \
        RFL_META_DETAIL_MEMBERS_10, RFL_META_DETAIL_MEMBERS_9,               \
        RFL_META_DETAIL_MEMBERS_8, RFL_META_DETAIL_MEMBERS_7,                \
        RFL_META_DETAIL_MEMBERS_6, RFL_META_DETAIL_MEMBERS_5,                \
        RFL_META_DETAIL_MEMBERS_4, RFL_META_DETAIL_MEMBERS_3,                \
        RFL_META_DETAIL_MEMBERS_2, RFL_META_DETAIL_MEMBERS_1)                \
    (D, __VA_ARGS__)

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
