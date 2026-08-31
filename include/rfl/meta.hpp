#pragma once

#include <rfl/detail/aggregate.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rfl::meta {

struct info {
protected:
    constexpr info() noexcept = default;
    constexpr info(const info&) noexcept = default;
    constexpr info& operator=(const info&) noexcept = default;
};

struct access_context {
    [[nodiscard]] static consteval access_context current() noexcept {
        return {};
    }
};

template <class T>
using object_type_t = std::remove_cvref_t<T>;

inline constexpr std::size_t max_supported_members = detail::max_members;

template <class T>
concept reflectable_type =
    std::is_aggregate_v<object_type_t<T>> &&
    !std::is_union_v<object_type_t<T>>;

template <class T>
struct type_info final : info {
    using type = T;

    constexpr type_info() noexcept : info{} {}
};

template <class Owner,
          std::size_t StructuralIndex,
          std::size_t FlatIndex = StructuralIndex>
struct member_info final : info {
    using owner_type = Owner;
    using type = detail::member_type_t<StructuralIndex, Owner>;
    static constexpr std::size_t structural_index = StructuralIndex;
    static constexpr std::size_t index = FlatIndex;

    constexpr member_info() noexcept : info{} {}
};

template <class Owner,
          std::size_t DirectIndex,
          std::size_t FlatIndex>
    requires detail::registered_single_base<Owner>
struct registered_member_info final : info {
    using owner_type = Owner;
    using registration = detail::single_base_registration<Owner>;
    using specification = std::tuple_element_t<
        DirectIndex, typename registration::direct_members>;
    using pointer_traits = detail::member_pointer_traits<
        std::remove_cv_t<decltype(specification::pointer)>>;
    using type = typename pointer_traits::value_type;
    static constexpr std::size_t direct_index = DirectIndex;
    static constexpr std::size_t index = FlatIndex;

    static_assert(std::same_as<typename pointer_traits::owner_type, Owner>,
                  "Registered fields must be declared directly in the derived type.");

    constexpr registered_member_info() noexcept : info{} {}
};

template <class T>
concept info_type = std::derived_from<object_type_t<T>, info>;

template <info_type Left, info_type Right>
[[nodiscard]] consteval bool operator==(Left, Right) noexcept {
    return std::same_as<object_type_t<Left>, object_type_t<Right>>;
}

template <class... Infos>
class info_sequence {
public:
    static constexpr std::size_t size_value = sizeof...(Infos);

    [[nodiscard]] static consteval std::size_t size() noexcept {
        return size_value;
    }

    [[nodiscard]] static consteval bool empty() noexcept {
        return size_value == 0;
    }

    template <std::size_t Index>
        requires (Index < size_value)
    [[nodiscard]] static consteval auto get() noexcept {
        using result_type = std::tuple_element_t<Index, std::tuple<Infos...>>;
        return result_type{};
    }

    template <class Visitor>
    constexpr void for_each(Visitor&& visitor) const
        noexcept((std::is_nothrow_invocable_v<Visitor&, Infos> && ...)) {
        auto&& callback = visitor;
        (std::invoke(callback, Infos{}), ...);
    }
};

namespace detail {

template <class T>
consteval std::string_view type_name_impl() noexcept {
#if defined(_MSC_VER)
    constexpr std::string_view signature = __FUNCSIG__;
    constexpr std::string_view prefix = "type_name_impl<";
    constexpr auto prefix_position = signature.find(prefix);
    constexpr auto end = signature.rfind(">(void)");
    static_assert(prefix_position != std::string_view::npos &&
                  end != std::string_view::npos,
                  "rfl::meta could not parse the MSVC function signature.");
    constexpr auto begin = prefix_position + prefix.size();
#elif defined(__GNUC__)
    constexpr std::string_view signature = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix = "T = ";
    constexpr auto prefix_position = signature.find(prefix);
    static_assert(prefix_position != std::string_view::npos,
                  "rfl::meta could not parse the GCC function signature.");
    constexpr auto begin = prefix_position + prefix.size();
    constexpr auto semicolon = signature.find(';', begin);
    constexpr auto bracket = signature.find(']', begin);
    constexpr auto end = semicolon != std::string_view::npos ? semicolon : bracket;
    static_assert(end != std::string_view::npos,
                  "rfl::meta could not find the end of the GCC type name.");
#else
#   error rfl::meta supports GCC-compatible compilers and MSVC.
#endif
    static_assert(begin < end,
                  "rfl::meta extracted an invalid type name range.");
    return signature.substr(begin, end - begin);
}

consteval std::string_view unqualified_name(std::string_view name) noexcept {
    for (const auto prefix : {std::string_view{"class "},
                              std::string_view{"struct "},
                              std::string_view{"enum "},
                              std::string_view{"union "}}) {
        if (name.starts_with(prefix)) {
            name.remove_prefix(prefix.size());
            break;
        }
    }

    std::size_t last_scope = std::string_view::npos;
    std::size_t template_depth = 0;
    for (std::size_t index = 0; index + 1 < name.size(); ++index) {
        if (name[index] == '<') {
            ++template_depth;
        } else if (name[index] == '>') {
            --template_depth;
        } else if (template_depth == 0 && name[index] == ':' &&
                   name[index + 1] == ':') {
            last_scope = index;
            ++index;
        }
    }
    return last_scope == std::string_view::npos
        ? name
        : name.substr(last_scope + 2);
}

template <class T, std::size_t... Index>
consteval auto members_impl(std::index_sequence<Index...>) noexcept {
    return info_sequence<member_info<T, Index>...>{};
}

template <class T, std::size_t FlatOffset, std::size_t... Index>
consteval auto registered_members_impl(
    std::index_sequence<Index...>) noexcept {
    return info_sequence<
        registered_member_info<T, Index, FlatOffset + Index>...>{};
}

template <class... BaseInfos, class... DirectInfos>
consteval auto concatenate(
    info_sequence<BaseInfos...>,
    info_sequence<DirectInfos...>) noexcept {
    return info_sequence<BaseInfos..., DirectInfos...>{};
}

template <class T>
consteval auto reflected_members_impl() noexcept {
    if constexpr (registered_single_base<T>) {
        using registration = single_base_registration<T>;
        using base_type = typename registration::base_type;
        using direct_members = typename registration::direct_members;
        constexpr auto base_members = reflected_members_impl<base_type>();
        constexpr auto base_count = decltype(base_members)::size();
        constexpr auto direct_count = std::tuple_size_v<direct_members>;
        return concatenate(
            base_members,
            registered_members_impl<T, base_count>(
                std::make_index_sequence<direct_count>{}));
    } else {
        return members_impl<T>(
            std::make_index_sequence<member_count_v<T>>{});
    }
}

template <class Owner,
          std::size_t StructuralIndex,
          std::size_t FlatIndex,
          class Object>
    requires std::derived_from<object_type_t<Object>, Owner>
[[nodiscard]] constexpr decltype(auto) member_value(
    member_info<Owner, StructuralIndex, FlatIndex>,
    Object& object) noexcept {
    using qualified_owner = std::conditional_t<
        std::is_const_v<std::remove_reference_t<Object>>,
        const Owner,
        Owner>;
    auto& owner = static_cast<qualified_owner&>(object);
    return get_member<StructuralIndex>(owner);
}

template <class Owner,
          std::size_t DirectIndex,
          std::size_t FlatIndex,
          class Object>
    requires std::same_as<object_type_t<Object>, Owner>
[[nodiscard]] constexpr decltype(auto) member_value(
    registered_member_info<Owner, DirectIndex, FlatIndex>,
    Object& object) noexcept {
    using member = registered_member_info<Owner, DirectIndex, FlatIndex>;
    using specification = typename member::specification;
    return object.*specification::pointer;
}

template <class Info>
struct plain_member_traits : std::false_type {
    using owner_type = void;
    static constexpr std::size_t structural_index = 0;
};

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
struct plain_member_traits<member_info<Owner, StructuralIndex, FlatIndex>>
    : std::true_type {
    using owner_type = Owner;
    static constexpr std::size_t structural_index = StructuralIndex;
};

template <class T, class... Infos>
inline constexpr bool tied_traversal_v =
    sizeof...(Infos) != 0 &&
    (plain_member_traits<Infos>::value && ...) &&
    (std::same_as<typename plain_member_traits<Infos>::owner_type,
                  object_type_t<T>> && ...);

template <class T, class Visitor, class... Infos>
constexpr void for_each_object_member_impl(
    T& object,
    Visitor&& visitor,
    info_sequence<Infos...>)
    noexcept((std::is_nothrow_invocable_v<
                  Visitor&,
                  Infos,
                  decltype(member_value(Infos{}, object))> && ...)) {
    auto&& callback = visitor;
    if constexpr (tied_traversal_v<T, Infos...>) {
        auto tied = tie_members(object);
        (std::invoke(callback,
                     Infos{},
                     std::get<plain_member_traits<Infos>::structural_index>(tied)),
         ...);
    } else {
        (std::invoke(callback, Infos{}, member_value(Infos{}, object)), ...);
    }
}

template <class T, class Visitor, class... Infos>
constexpr bool for_each_object_member_while_impl(
    T& object,
    Visitor&& visitor,
    info_sequence<Infos...>)
    noexcept((std::is_nothrow_invocable_v<
                  Visitor&,
                  Infos,
                  decltype(member_value(Infos{}, object))> && ...)) {
    auto&& callback = visitor;
    if constexpr (tied_traversal_v<T, Infos...>) {
        auto tied = tie_members(object);
        return (static_cast<bool>(std::invoke(
                    callback,
                    Infos{},
                    std::get<plain_member_traits<Infos>::structural_index>(tied))) &&
                ...);
    } else {
        return (static_cast<bool>(
                    std::invoke(callback, Infos{}, member_value(Infos{}, object))) &&
                ...);
    }
}

} // namespace detail

template <class T>
[[nodiscard]] consteval auto reflect() noexcept {
    return type_info<T>{};
}

template <class T>
inline constexpr auto reflect_type = reflect<T>();

template <class T>
[[nodiscard]] consteval bool is_type(type_info<T>) noexcept {
    return true;
}

template <info_type MetaInfo>
[[nodiscard]] consteval bool is_type(MetaInfo) noexcept {
    return false;
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval bool is_nonstatic_data_member(
    member_info<Owner, StructuralIndex, FlatIndex>) noexcept {
    return true;
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval bool is_nonstatic_data_member(
    registered_member_info<Owner, DirectIndex, FlatIndex>) noexcept {
    return true;
}

template <info_type MetaInfo>
[[nodiscard]] consteval bool is_nonstatic_data_member(MetaInfo) noexcept {
    return false;
}

template <class T>
[[nodiscard]] consteval std::string_view display_name_of(type_info<T>) noexcept {
    return detail::type_name_impl<T>();
}

template <class T>
[[nodiscard]] consteval std::string_view identifier_of(type_info<T> value) noexcept {
    return detail::unqualified_name(display_name_of(value));
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::string_view identifier_of(
    member_info<Owner, StructuralIndex, FlatIndex>) noexcept {
    return detail::member_name<Owner, StructuralIndex>();
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::string_view identifier_of(
    registered_member_info<Owner, DirectIndex, FlatIndex>) noexcept {
    using member = registered_member_info<Owner, DirectIndex, FlatIndex>;
    using specification = typename member::specification;
    return specification::name.view();
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::string_view display_name_of(
    member_info<Owner, StructuralIndex, FlatIndex> member) noexcept {
    return identifier_of(member);
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::string_view display_name_of(
    registered_member_info<Owner, DirectIndex, FlatIndex> member) noexcept {
    return identifier_of(member);
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval auto type_of(
    member_info<Owner, StructuralIndex, FlatIndex>) noexcept {
    return type_info<
        typename member_info<Owner, StructuralIndex, FlatIndex>::type>{};
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval auto type_of(
    registered_member_info<Owner, DirectIndex, FlatIndex>) noexcept {
    return type_info<
        typename registered_member_info<Owner, DirectIndex, FlatIndex>::type>{};
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval auto parent_of(
    member_info<Owner, StructuralIndex, FlatIndex>) noexcept {
    return type_info<Owner>{};
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval auto parent_of(
    registered_member_info<Owner, DirectIndex, FlatIndex>) noexcept {
    return type_info<Owner>{};
}

template <class T>
[[nodiscard]] consteval std::size_t size_of(type_info<T>) noexcept {
    if constexpr (std::is_reference_v<T>) {
        return sizeof(std::remove_reference_t<T>*);
    } else {
        return sizeof(T);
    }
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::size_t size_of(
    member_info<Owner, StructuralIndex, FlatIndex> member) noexcept {
    return size_of(type_of(member));
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::size_t size_of(
    registered_member_info<Owner, DirectIndex, FlatIndex> member) noexcept {
    return size_of(type_of(member));
}

template <class T>
[[nodiscard]] consteval std::size_t alignment_of(type_info<T>) noexcept {
    return alignof(T);
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::size_t alignment_of(
    member_info<Owner, StructuralIndex, FlatIndex> member) noexcept {
    return alignment_of(type_of(member));
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::size_t alignment_of(
    registered_member_info<Owner, DirectIndex, FlatIndex> member) noexcept {
    return alignment_of(type_of(member));
}

template <class Owner, std::size_t StructuralIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::size_t index_of(
    member_info<Owner, StructuralIndex, FlatIndex>) noexcept {
    return FlatIndex;
}

template <class Owner, std::size_t DirectIndex, std::size_t FlatIndex>
[[nodiscard]] consteval std::size_t index_of(
    registered_member_info<Owner, DirectIndex, FlatIndex>) noexcept {
    return FlatIndex;
}

template <reflectable_type T>
[[nodiscard]] consteval std::size_t member_count_of(type_info<T>) noexcept {
    using owner_type = object_type_t<T>;
    return decltype(detail::reflected_members_impl<owner_type>())::size();
}

template <reflectable_type T>
[[nodiscard]] consteval auto nonstatic_data_members_of(
    type_info<T>,
    access_context = access_context::current()) noexcept {
    return detail::reflected_members_impl<object_type_t<T>>();
}

template <reflectable_type T>
[[nodiscard]] consteval auto members_of(
    type_info<T> reflected_type,
    access_context context = access_context::current()) noexcept {
    return nonstatic_data_members_of(reflected_type, context);
}

template <std::size_t Index, class... Infos>
    requires (Index < sizeof...(Infos))
[[nodiscard]] consteval auto get(const info_sequence<Infos...>&) noexcept {
    return info_sequence<Infos...>::template get<Index>();
}

template <class... Infos, class Visitor>
constexpr void for_each(info_sequence<Infos...> sequence, Visitor&& visitor)
    noexcept(noexcept(sequence.for_each(std::forward<Visitor>(visitor)))) {
    sequence.for_each(std::forward<Visitor>(visitor));
}

template <class Owner,
          std::size_t StructuralIndex,
          std::size_t FlatIndex,
          class Object>
    requires std::derived_from<object_type_t<Object>, Owner>
[[nodiscard]] constexpr decltype(auto) get(
    member_info<Owner, StructuralIndex, FlatIndex> member,
    Object& object) noexcept {
    return detail::member_value(member, object);
}

template <class Owner,
          std::size_t DirectIndex,
          std::size_t FlatIndex,
          class Object>
    requires std::same_as<object_type_t<Object>, Owner>
[[nodiscard]] constexpr decltype(auto) get(
    registered_member_info<Owner, DirectIndex, FlatIndex> member,
    Object& object) noexcept {
    return detail::member_value(member, object);
}

template <reflectable_type T, class Visitor>
constexpr void for_each_member(T& object, Visitor&& visitor)
    noexcept(noexcept(detail::for_each_object_member_impl(
        object,
        std::forward<Visitor>(visitor),
        nonstatic_data_members_of(reflect<object_type_t<T>>()))) ) {
    detail::for_each_object_member_impl(
        object,
        std::forward<Visitor>(visitor),
        nonstatic_data_members_of(reflect<object_type_t<T>>()));
}

template <reflectable_type T, class Visitor>
constexpr bool for_each_member_while(T& object, Visitor&& visitor)
    noexcept(noexcept(detail::for_each_object_member_while_impl(
        object,
        std::forward<Visitor>(visitor),
        nonstatic_data_members_of(reflect<object_type_t<T>>()))) ) {
    return detail::for_each_object_member_while_impl(
        object,
        std::forward<Visitor>(visitor),
        nonstatic_data_members_of(reflect<object_type_t<T>>()));
}

template <info_type MetaInfo>
using reflected_type_t = typename object_type_t<MetaInfo>::type;

} // namespace rfl::meta

namespace std {

template <class... Infos>
struct tuple_size<rfl::meta::info_sequence<Infos...>>
    : integral_constant<size_t, sizeof...(Infos)> {};

template <size_t Index, class... Infos>
struct tuple_element<Index, rfl::meta::info_sequence<Infos...>> {
    using type = tuple_element_t<Index, tuple<Infos...>>;
};

} // namespace std
