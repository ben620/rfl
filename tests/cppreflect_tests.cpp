#include <rfl/meta.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace test_types {

struct User {
    std::uint64_t id;
    std::string name;
    bool active;
};

struct Point {
    int x;
    double y;
};

struct Empty {};

struct Nested {
    Point point;
    std::array<int, 3> values;
};

struct Header {
    int size;
    int type;
};

struct Go : public Header {
    int target;
};

} // namespace test_types

RFL_META_REGISTER_SINGLE_BASE(test_types::Go, test_types::Header, target);

namespace test_types {

struct SixteenMembers {
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

struct NotAggregate {
    explicit NotAggregate(int value) : value(value) {}
    int value;
};

union Value {
    int integer;
    float real;
};

struct NoThrowMemberVisitor {
    template <class MetaInfo, class Member>
    constexpr void operator()(MetaInfo, Member&) const noexcept {}
};

struct ThrowingMemberVisitor {
    template <class MetaInfo, class Member>
    constexpr void operator()(MetaInfo, Member&) const {}
};

struct NoThrowInfoVisitor {
    template <class MetaInfo>
    constexpr void operator()(MetaInfo) const noexcept {}
};

struct ThrowingInfoVisitor {
    template <class MetaInfo>
    constexpr void operator()(MetaInfo) const {}
};

constexpr auto user_type = rfl::meta::reflect<User>();
constexpr auto const_user_type = rfl::meta::reflect<const User>();
constexpr auto user_reference_type = rfl::meta::reflect<User&>();
constexpr auto user_members = rfl::meta::nonstatic_data_members_of(
    user_type, rfl::meta::access_context::current());
constexpr auto id_member = rfl::meta::get<0>(user_members);
constexpr auto name_member = rfl::meta::get<1>(user_members);
constexpr auto active_member = rfl::meta::get<2>(user_members);

static_assert(rfl::meta::reflectable_type<User>);
static_assert(rfl::meta::reflectable_type<const User&>);
static_assert(!rfl::meta::reflectable_type<NotAggregate>);
static_assert(!rfl::meta::reflectable_type<Value>);

static_assert(std::derived_from<decltype(user_type), rfl::meta::info>);
static_assert(rfl::meta::is_type(user_type));
static_assert(!rfl::meta::is_nonstatic_data_member(user_type));
static_assert(user_type != const_user_type);
static_assert(user_type != user_reference_type);
static_assert(const_user_type != user_reference_type);
static_assert(std::same_as<rfl::meta::reflected_type_t<decltype(user_type)>, User>);
static_assert(std::same_as<
              rfl::meta::reflected_type_t<decltype(const_user_type)>,
              const User>);
static_assert(std::same_as<
              rfl::meta::reflected_type_t<decltype(user_reference_type)>,
              User&>);
static_assert(rfl::meta::size_of(user_reference_type) == sizeof(User*));
static_assert(rfl::meta::member_count_of(const_user_type) == 3);
static_assert(rfl::meta::nonstatic_data_members_of(user_reference_type).size() == 3);
static_assert(rfl::meta::identifier_of(user_type) == "User");
static_assert(rfl::meta::display_name_of(user_type).find("User") !=
              std::string_view::npos);
static_assert(rfl::meta::size_of(user_type) == sizeof(User));
static_assert(rfl::meta::alignment_of(user_type) == alignof(User));
static_assert(rfl::meta::member_count_of(user_type) == 3);

constexpr auto sixteen_type = rfl::meta::reflect<SixteenMembers>();
constexpr auto sixteen_members =
    rfl::meta::nonstatic_data_members_of(sixteen_type);
static_assert(rfl::meta::member_count_of(sixteen_type) ==
              rfl::meta::max_supported_members);
static_assert(sixteen_members.size() == 16);
static_assert(rfl::meta::identifier_of(rfl::meta::get<0>(sixteen_members)) ==
              "m00");
static_assert(rfl::meta::identifier_of(rfl::meta::get<15>(sixteen_members)) ==
              "m15");
static_assert(rfl::meta::type_of(rfl::meta::get<15>(sixteen_members)) ==
              rfl::meta::reflect<int>());

static_assert(user_members.size() == 3);
static_assert(!user_members.empty());
static_assert(std::tuple_size_v<decltype(user_members)> == 3);
static_assert(std::same_as<
              std::remove_cv_t<std::tuple_element_t<0, decltype(user_members)>>,
              std::remove_cvref_t<decltype(id_member)>>);
static_assert(rfl::meta::members_of(user_type).size() == user_members.size());

consteval bool value_structured_binding_works() {
    auto [id, name, active] = user_members;
    return rfl::meta::identifier_of(id) == "id" &&
           rfl::meta::identifier_of(name) == "name" &&
           rfl::meta::identifier_of(active) == "active";
}

static_assert(value_structured_binding_works());

static_assert(rfl::meta::is_nonstatic_data_member(id_member));
static_assert(!rfl::meta::is_type(id_member));
static_assert(rfl::meta::identifier_of(id_member) == "id");
static_assert(rfl::meta::display_name_of(name_member) == "name");
static_assert(rfl::meta::identifier_of(active_member) == "active");
static_assert(rfl::meta::index_of(id_member) == 0);
static_assert(rfl::meta::index_of(name_member) == 1);
static_assert(rfl::meta::parent_of(id_member) == user_type);
static_assert(rfl::meta::type_of(id_member) == rfl::meta::reflect<std::uint64_t>());
static_assert(rfl::meta::type_of(name_member) == rfl::meta::reflect<std::string>());
static_assert(rfl::meta::type_of(active_member) == rfl::meta::reflect<bool>());
static_assert(rfl::meta::size_of(id_member) == sizeof(std::uint64_t));
static_assert(rfl::meta::size_of(name_member) == sizeof(std::string));
static_assert(rfl::meta::alignment_of(active_member) == alignof(bool));

constexpr auto go_type = rfl::meta::reflect<Go>();
constexpr auto go_members = rfl::meta::nonstatic_data_members_of(go_type);
constexpr auto go_size_member = rfl::meta::get<0>(go_members);
constexpr auto go_type_member = rfl::meta::get<1>(go_members);
constexpr auto go_target_member = rfl::meta::get<2>(go_members);
static_assert(rfl::meta::reflectable_type<Go>);
static_assert(rfl::meta::member_count_of(go_type) == 3);
static_assert(go_members.size() == 3);
static_assert(rfl::meta::identifier_of(go_size_member) == "size");
static_assert(rfl::meta::identifier_of(go_type_member) == "type");
static_assert(rfl::meta::identifier_of(go_target_member) == "target");
static_assert(rfl::meta::index_of(go_size_member) == 0);
static_assert(rfl::meta::index_of(go_type_member) == 1);
static_assert(rfl::meta::index_of(go_target_member) == 2);
static_assert(rfl::meta::parent_of(go_size_member) ==
              rfl::meta::reflect<Header>());
static_assert(rfl::meta::parent_of(go_target_member) == go_type);
static_assert(rfl::meta::type_of(go_target_member) == rfl::meta::reflect<int>());
static_assert(rfl::meta::size_of(go_target_member) == sizeof(int));

using IdType = rfl::meta::reflected_type_t<decltype(rfl::meta::type_of(id_member))>;
using NameType = rfl::meta::reflected_type_t<decltype(rfl::meta::type_of(name_member))>;
static_assert(std::same_as<IdType, std::uint64_t>);
static_assert(std::same_as<NameType, std::string>);

static_assert(noexcept(rfl::meta::for_each(user_members, NoThrowInfoVisitor{})));
static_assert(!noexcept(rfl::meta::for_each(user_members, ThrowingInfoVisitor{})));
static_assert(noexcept(rfl::meta::for_each_member(
    std::declval<User&>(), NoThrowMemberVisitor{})));
static_assert(!noexcept(rfl::meta::for_each_member(
    std::declval<User&>(), ThrowingMemberVisitor{})));

template <class T>
concept rvalue_visitable = requires(T&& value) {
    rfl::meta::for_each_member(std::move(value), NoThrowMemberVisitor{});
};

template <class T>
concept rvalue_member_accessible = requires(T&& value) {
    rfl::meta::get(id_member, std::move(value));
};

static_assert(!rvalue_visitable<User>);
static_assert(!rvalue_member_accessible<User>);
static_assert(std::same_as<decltype(rfl::meta::get(
                               id_member, std::declval<User&>())),
                           std::uint64_t&>);
static_assert(std::same_as<decltype(rfl::meta::get(
                               id_member, std::declval<const User&>())),
                           const std::uint64_t&>);

constexpr bool constexpr_member_iteration_works() {
    Point point{2, 4.5};
    std::size_t fields = 0;
    rfl::meta::for_each_member(
        point,
        [&]<class MetaInfo>(MetaInfo member, auto& value) {
            ++fields;
            if constexpr (rfl::meta::identifier_of(member) == "x") {
                static_assert(std::same_as<
                              rfl::meta::reflected_type_t<
                                  decltype(rfl::meta::type_of(member))>,
                              int>);
                value = 7;
            }
        });
    return fields == 2 && point.x == 7 && point.y == 4.5;
}

static_assert(constexpr_member_iteration_works());

void test_info_sequence_iteration() {
    std::array<std::string_view, 3> names{};
    std::size_t visited = 0;

    rfl::meta::for_each(user_members, [&](auto member) {
        names[visited++] = rfl::meta::identifier_of(member);
    });

    assert(visited == 3);
    assert((names == std::array<std::string_view, 3>{"id", "name", "active"}));
}

void test_member_metadata_and_mutation() {
    User user{42, "Ada", true};
    std::size_t visited = 0;

    rfl::meta::for_each_member(
        user,
        [&]<class MetaInfo>(MetaInfo member, auto& value) {
            static_assert(std::is_lvalue_reference_v<decltype(value)>);
            static_assert(!std::is_const_v<std::remove_reference_t<decltype(value)>>);
            assert(rfl::meta::index_of(member) == visited);
            assert(!rfl::meta::identifier_of(member).empty());
            assert(rfl::meta::size_of(member) ==
                   sizeof(rfl::meta::reflected_type_t<
                          decltype(rfl::meta::type_of(member))>));

            if constexpr (rfl::meta::identifier_of(member) == "id") {
                value = 100;
            } else if constexpr (rfl::meta::identifier_of(member) == "name") {
                value = "Grace";
            } else if constexpr (rfl::meta::identifier_of(member) == "active") {
                value = false;
            }
            ++visited;
        });

    assert(visited == 3);
    assert(user.id == 100);
    assert(user.name == "Grace");
    assert(!user.active);
    assert(rfl::meta::get(name_member, user) == "Grace");
}

void test_const_access() {
    const User user{7, "Linus", true};
    std::size_t visited = 0;

    rfl::meta::for_each_member(user, [&](auto member, const auto& value) {
        static_assert(std::is_lvalue_reference_v<decltype(value)>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(value)>>);
        assert(rfl::meta::get(member, user) == value);
        ++visited;
    });

    assert(visited == 3);
    assert(rfl::meta::get(name_member, user) == "Linus");
}

void test_single_inheritance() {
    Go value{{10, 20}, 30};
    std::array<std::string_view, 3> names{};
    std::size_t visited = 0;

    rfl::meta::for_each_member(value, [&](auto member, auto& field) {
        names[visited++] = rfl::meta::identifier_of(member);
        field += 1;
    });

    assert(visited == 3);
    assert((names == std::array<std::string_view, 3>{"size", "type", "target"}));
    assert(value.size == 11);
    assert(value.type == 21);
    assert(value.target == 31);
    assert(rfl::meta::get(go_size_member, value) == 11);
    assert(rfl::meta::get(go_target_member, value) == 31);

    const Go constant{{40, 50}, 60};
    static_assert(std::same_as<decltype(rfl::meta::get(
                               go_size_member, std::declval<const Go&>())),
                           const int&>);
    static_assert(std::same_as<decltype(rfl::meta::get(
                               go_target_member, std::declval<const Go&>())),
                           const int&>);
    assert(rfl::meta::get(go_size_member, constant) == 40);
    assert(rfl::meta::get(go_target_member, constant) == 60);
}

void test_nested_and_empty_aggregates() {
    constexpr auto nested = rfl::meta::reflect<Nested>();
    constexpr auto nested_members = rfl::meta::nonstatic_data_members_of(nested);
    static_assert(nested_members.size() == 2);
    static_assert(rfl::meta::identifier_of(rfl::meta::get<0>(nested_members)) ==
                  "point");
    static_assert(rfl::meta::identifier_of(rfl::meta::get<1>(nested_members)) ==
                  "values");

    constexpr auto empty = rfl::meta::reflect<Empty>();
    constexpr auto no_members = rfl::meta::nonstatic_data_members_of(empty);
    static_assert(no_members.empty());

    Empty value;
    std::size_t visited = 0;
    rfl::meta::for_each_member(value, [&](auto, auto&) { ++visited; });
    assert(visited == 0);
}

} // namespace test_types

int main() {
    test_types::test_info_sequence_iteration();
    test_types::test_member_metadata_and_mutation();
    test_types::test_const_access();
    test_types::test_single_inheritance();
    test_types::test_nested_and_empty_aggregates();
}
