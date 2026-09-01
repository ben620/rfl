#include <rfl/json.hpp>
#include <rfl/meta.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

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

void test_json_string_round_trip() {
    const User source{42, "Ada\nLovelace", true};
    const auto json = rfl::ToJson(source);
    assert(json == R"({"id":42,"name":"Ada\nLovelace","active":true})");

    const auto decoded = rfl::FromJson<User>(
        R"({"active":false,"unknown":[1,null,{}],"name":"Grace \u03bb","id":7})");
    assert(decoded.id == 7);
    assert(decoded.name == "Grace \xce\xbb");
    assert(!decoded.active);

    User output{};
    assert(rfl::FromJson(json, output));
    assert(output.id == source.id);
    assert(output.name == source.name);
    assert(output.active == source.active);
    assert(!rfl::FromJson(R"({"id":})", output));
    assert(!rfl::FromJson(
        R"({"unknown":"\q123,"id":7,"name":"x","active":true})", output));
    assert(rfl::ToJson("text") == R"("text")");

    assert(rfl::FromJson(R"({"\u006ename":"x","name":"y","id":1,"active":true})",
                         output));
    assert(output.name == "y");
    assert(output.id == 1);

    assert(rfl::FromJson(R"({"\u006eame":"escaped-key","id":2,"active":false})",
                         output));
    assert(output.name == "escaped-key");

    assert(!rfl::FromJson(R"({"unknown":"\u00","id":1})", output));

    std::vector<std::string> strings;
    assert(rfl::FromJson(R"(["a","b\n","c"])", strings));
    assert(strings.size() == 3);
    assert(strings[1] == "b\n");
    assert(rfl::FromJson(R"([])", strings));
    assert(strings.empty());
}

void test_json_transactional_failure() {
    User original{7, "Linus", true};
    User target = original;

    assert(!rfl::FromJson(R"({"id":1,"name":"partial","active":oops})", target));
    assert(target.id == original.id);
    assert(target.name == original.name);
    assert(target.active == original.active);

    User in_place = original;
    assert(!rfl::FromJsonInPlace(
        R"({"id":1,"name":"partial","active":oops})", in_place));
    assert(in_place.id == 1);

    std::vector<int> numbers{1, 2, 3};
    assert(!rfl::FromJson(R"([4,5,bad])", numbers));
    assert((numbers == std::vector<int>{1, 2, 3}));
    assert(rfl::FromJsonInPlace(R"([7,8])", numbers));
    assert((numbers == std::vector<int>{7, 8}));
}

void test_json_utf8_validation() {
    const User valid{1, "\xe4\xbd\xa0\xe5\xa5\xbd", true};
    const auto json = rfl::ToJson(valid);
    assert(json.find("\xe4\xbd\xa0\xe5\xa5\xbd") != std::string::npos);

    User decoded{};
    assert(rfl::FromJson(json, decoded));
    assert(decoded.name == valid.name);

    bool rejected = false;
    try {
        const auto ignored = rfl::ToJson(std::string("\xff\xfe"));
        (void)ignored;
    } catch (const std::domain_error&) {
        rejected = true;
    }
    assert(rejected);

    const User lone_continuation{1, std::string("\x80"), true};
    std::string sink;
    assert(!rfl::ToJson(lone_continuation, sink));
    assert(!rfl::JsonSize(lone_continuation));

    assert(!rfl::FromJson("\"\xc3\"", decoded.name));
    assert(!rfl::FromJson("\"\xed\xa0\x80\"", decoded.name));
    assert(!rfl::FromJson("\"\xc0\xaf\"", decoded.name));
    assert(!rfl::FromJson("{\"id\":1,\"name\":\"\xff\"}", decoded));
}

void test_json_span_overlap() {
    std::string text = "abcdefghijklmnop";
    const auto expected = rfl::ToJson(std::string_view(text));

    std::array<char, 64> separate{};
    const auto result =
        rfl::ToJson(std::string_view(text), std::span<char>(separate));
    assert(result.complete());
    assert(std::string_view(separate.data(), result.written) == expected);
}

void test_json_nested_and_fixed_buffer() {
    const Nested source{{2, 4.5}, {1, 2, 3}};
    const auto expected = rfl::ToJson(source);

    std::array<char, 128> storage{};
    const auto result = rfl::ToJson(source, std::span<char>(storage));
    assert(result.complete());
    assert(std::string_view(storage.data(), result.written) == expected);

    std::array<char, 8> small{};
    const auto truncated = rfl::ToJson(source, std::span<char>(small));
    assert(!truncated.complete());
    assert(truncated.valid);
    assert(truncated.written == small.size());
    assert(truncated.required == expected.size());

    const auto decoded = rfl::FromJson<Nested>(expected);
    assert(decoded.point.x == 2);
    assert(decoded.point.y == 4.5);
    assert((decoded.values == std::array<int, 3>{1, 2, 3}));

    const auto size = rfl::JsonSize(source);
    assert(size && *size == expected.size());

    std::string reused;
    reused.reserve(256);
    const auto capacity = reused.capacity();
    assert(rfl::ToJson(source, reused));
    assert(reused == expected);
    assert(reused.capacity() == capacity);

    assert(rfl::ToJson(0.1) == "0.1");
    assert(rfl::FromJson<double>(rfl::ToJson(0.1)) == 0.1);
}

void test_json_chunk_sink() {
    const User source{100, "Grace", false};
    std::string transmitted;
    const bool sent = rfl::ToJson<8>(source, [&](std::string_view chunk) {
        assert(!chunk.empty());
        assert(chunk.size() <= 8);
        transmitted.append(chunk);
        return true;
    });
    assert(sent);
    assert(transmitted == rfl::ToJson(source));

    transmitted.clear();
    const bool partially_sent = rfl::ToJson<8>(source, [&](std::string_view chunk) {
        const auto consumed = (std::min)(chunk.size(), std::size_t{3});
        transmitted.append(chunk.substr(0, consumed));
        return consumed;
    });
    assert(partially_sent);
    assert(transmitted == rfl::ToJson(source));

    std::size_t calls = 0;
    const bool stopped = rfl::ToJson<4>(source, [&](std::string_view) {
        return ++calls < 2;
    });
    assert(!stopped);
    assert(calls == 2);

    transmitted.clear();
    auto void_sink = [&](std::string_view chunk) { transmitted.append(chunk); };
    assert(rfl::ToJson<16>(source, void_sink));
    assert(transmitted == rfl::ToJson(source));

    transmitted.clear();
    assert(rfl::ToJson<1>(source, void_sink));
    assert(transmitted == rfl::ToJson(source));
}

} // namespace test_types

int main() {
    test_types::test_info_sequence_iteration();
    test_types::test_member_metadata_and_mutation();
    test_types::test_const_access();
    test_types::test_single_inheritance();
    test_types::test_nested_and_empty_aggregates();
    test_types::test_json_string_round_trip();
    test_types::test_json_nested_and_fixed_buffer();
    test_types::test_json_chunk_sink();
    test_types::test_json_transactional_failure();
    test_types::test_json_utf8_validation();
    test_types::test_json_span_overlap();
}
