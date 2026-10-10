#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace rfl {

template <class E>
concept enum_type = std::is_enum_v<E>;

// Specialize before first use to scan a different inclusive range.
// Equal-valued aliases share one compiler-selected name; values are sorted numerically.
template <enum_type E>
struct enum_range {
    static constexpr int min = -128;
    static constexpr int max = 127;
};

namespace enum_detail {

constexpr bool identifier_start(char value) noexcept {
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') || value == '_' ||
           static_cast<unsigned char>(value) >= 128;
}

constexpr bool identifier_character(char value) noexcept {
    return identifier_start(value) || (value >= '0' && value <= '9');
}

template <enum_type E, E Value>
consteval std::string_view value_name() noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
    constexpr std::string_view signature = __FUNCSIG__;
    constexpr auto end = signature.rfind(">(void)");
    static_assert(end != std::string_view::npos,
                  "rfl could not parse the MSVC enum signature.");
    constexpr auto comma = signature.rfind(',', end);
    static_assert(comma != std::string_view::npos,
                  "rfl could not locate the enum value in the MSVC signature.");
    auto expression = signature.substr(comma + 1, end - comma - 1);
#elif defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view signature = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix = "Value = ";
    constexpr auto position = signature.find(prefix);
    static_assert(position != std::string_view::npos,
                  "rfl could not parse the enum signature.");
    constexpr auto begin = position + prefix.size();
    constexpr auto semicolon = signature.find(';', begin);
    constexpr auto end = semicolon != std::string_view::npos
        ? semicolon : signature.rfind(']');
    static_assert(end != std::string_view::npos && begin < end,
                  "rfl could not locate the enum value in the signature.");
    auto expression = signature.substr(begin, end - begin);
#else
#   error rfl enum reflection supports Clang, GCC and MSVC.
#endif
    while (!expression.empty() && expression.front() == ' ') {
        expression.remove_prefix(1);
    }
    while (!expression.empty() && expression.back() == ' ') {
        expression.remove_suffix(1);
    }
    const auto scope = expression.rfind("::");
    const auto name = scope == std::string_view::npos
        ? expression : expression.substr(scope + 2);
    if (name.empty() || !identifier_start(name.front())) {
        return {};
    }
    for (const char character : name) {
        if (!identifier_character(character)) {
            return {};
        }
    }
    return name;
}

template <enum_type E>
struct scan_range {
    using underlying_type = std::underlying_type_t<E>;
    static_assert(enum_range<E>::min <= enum_range<E>::max,
                  "rfl enum_range min must not exceed max.");

    static constexpr std::int64_t min = [] {
        std::int64_t result = enum_range<E>::min;
        if constexpr (std::is_unsigned_v<underlying_type>) {
            if (result < 0) {
                result = 0;
            }
        } else if constexpr (std::numeric_limits<underlying_type>::digits < 31) {
            if (result < std::numeric_limits<underlying_type>::min()) {
                result = std::numeric_limits<underlying_type>::min();
            }
        }
        return result;
    }();

    static constexpr std::int64_t max = [] {
        std::int64_t result = enum_range<E>::max;
        if constexpr (std::numeric_limits<underlying_type>::digits < 31) {
            if (result > static_cast<std::int64_t>(std::numeric_limits<underlying_type>::max())) {
                result = std::numeric_limits<underlying_type>::max();
            }
        }
        return result;
    }();

    static constexpr std::size_t size = max < min ? 0
        : static_cast<std::size_t>(max - min + 1);
    static_assert(size <= 65536,
                  "rfl enum_range is too large; use a narrower scan range.");
};

template <enum_type E, std::int64_t Candidate>
consteval std::string_view candidate_name() noexcept {
    if constexpr (requires { value_name<E, static_cast<E>(Candidate)>(); }) {
        return value_name<E, static_cast<E>(Candidate)>();
    } else {
        return {};
    }
}

template <enum_type E, std::size_t... Index>
consteval auto scanned_names(std::index_sequence<Index...>) noexcept {
    return std::array<std::string_view, sizeof...(Index)>{
        candidate_name<E, scan_range<E>::min + static_cast<std::int64_t>(Index)>()...};
}

template <enum_type E>
struct enum_data {
    static constexpr auto scanned = scanned_names<E>(
        std::make_index_sequence<scan_range<E>::size>{});
    static constexpr std::size_t count = [] {
        std::size_t result = 0;
        for (const auto name : scanned) {
            result += !name.empty();
        }
        return result;
    }();

    static constexpr auto values = [] {
        std::array<E, count> result{};
        std::size_t next = 0;
        for (std::size_t index = 0; index < scanned.size(); ++index) {
            if (!scanned[index].empty()) {
                result[next++] = static_cast<E>(scan_range<E>::min +
                                               static_cast<std::int64_t>(index));
            }
        }
        return result;
    }();

    static constexpr auto names = [] {
        std::array<std::string_view, count> result{};
        std::size_t next = 0;
        for (const auto name : scanned) {
            if (!name.empty()) {
                result[next++] = name;
            }
        }
        return result;
    }();
};

} // namespace enum_detail

template <enum_type E>
[[nodiscard]] constexpr std::string_view enum_name(E value) noexcept {
    const auto& values = enum_detail::enum_data<E>::values;
    const auto& names = enum_detail::enum_data<E>::names;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (values[index] == value) {
            return names[index];
        }
    }
    return {};
}

// A constant value can be named without scanning, even outside enum_range.
template <auto Value>
    requires enum_type<decltype(Value)>
[[nodiscard]] consteval std::string_view enum_name() noexcept {
    return enum_detail::value_name<decltype(Value), Value>();
}

template <enum_type E>
[[nodiscard]] constexpr std::optional<E> enum_cast(std::string_view name) noexcept {
    const auto& names = enum_detail::enum_data<E>::names;
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (names[index] == name) {
            return enum_detail::enum_data<E>::values[index];
        }
    }
    return std::nullopt;
}

template <enum_type E>
[[nodiscard]] constexpr const auto& enum_values() noexcept {
    return enum_detail::enum_data<E>::values;
}

template <enum_type E>
[[nodiscard]] constexpr const auto& enum_names() noexcept {
    return enum_detail::enum_data<E>::names;
}

template <enum_type E>
[[nodiscard]] constexpr std::size_t enum_count() noexcept {
    return enum_detail::enum_data<E>::count;
}

} // namespace rfl
