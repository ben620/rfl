#include <rfl/enum.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace enum_test_types {

enum class Color { Red = -2, Green = 0, Blue = 7 };
enum Plain { PlainFirst = 1, PlainLast = 9 };
enum PlainSigned { PlainNegative = -7, PlainPositive = 3 };
enum FixedPlain : int { FixedNegative = -5, FixedPositive = 5 };
enum class Empty {};
enum class Aliases { Original = 3, Alias = 3, Other = 5 };
enum class Byte : std::uint8_t { Zero = 0, High = 128, Last = 255 };
enum class SignedByte : std::int8_t { First = -128, Last = 127 };
enum class Sparse : std::uint16_t { Low = 1025, High = 1540 };
enum class Negative { First = -300, Last = -298 };
enum class Outside { Missing = 300 };
enum class Flag : bool { Off = false, On = true };
enum class Wide : std::uint64_t { Last = (std::numeric_limits<std::uint64_t>::max)() };

struct Owner {
    enum class State { Ready = 1, Done = 2 };
};

template <class T, class U>
struct TemplateOwner {
    enum class State { Ready = 1 };
};

} // namespace enum_test_types

template <>
struct rfl::enum_range<enum_test_types::Byte> {
    static constexpr int min = -1000;
    static constexpr int max = 1000;
};

template <>
struct rfl::enum_range<enum_test_types::SignedByte> {
    static constexpr int min = -1000;
    static constexpr int max = 1000;
};

template <>
struct rfl::enum_range<enum_test_types::Sparse> {
    static constexpr int min = 1025;
    static constexpr int max = 1540;
};

template <>
struct rfl::enum_range<enum_test_types::Negative> {
    static constexpr int min = -300;
    static constexpr int max = -298;
};

namespace {

using namespace enum_test_types;

static_assert(rfl::enum_name(Color::Red) == "Red");
static_assert(rfl::enum_name<Color::Blue>() == "Blue");
static_assert(rfl::enum_cast<Color>("Green") == Color::Green);
static_assert(rfl::enum_name(static_cast<Color>(1)).empty());
static_assert(rfl::enum_name<static_cast<Color>(1)>().empty());
static_assert(!rfl::enum_cast<Color>("unknown"));
static_assert(!rfl::enum_cast<Color>("red"));
static_assert(!rfl::enum_cast<Color>("Color::Red"));
static_assert(!rfl::enum_cast<Color>(""));
static_assert(!rfl::enum_cast<Color>("7"));
static_assert(rfl::enum_count<Color>() == 3);
static_assert((rfl::enum_values<Color>() == std::array{Color::Red, Color::Green, Color::Blue}));
static_assert((rfl::enum_names<Color>() ==
               std::array<std::string_view, 3>{"Red", "Green", "Blue"}));
static_assert(rfl::enum_name(PlainLast) == "PlainLast");
static_assert(rfl::enum_cast<Plain>("PlainFirst") == PlainFirst);
static_assert(rfl::enum_name(PlainNegative) == "PlainNegative");
static_assert(rfl::enum_cast<PlainSigned>("PlainPositive") == PlainPositive);
static_assert(rfl::enum_count<PlainSigned>() == 2);
static_assert(rfl::enum_name(FixedNegative) == "FixedNegative");
static_assert(rfl::enum_cast<FixedPlain>("FixedPositive") == FixedPositive);
static_assert(rfl::enum_count<FixedPlain>() == 2);
static_assert(rfl::enum_count<Empty>() == 0);
static_assert(rfl::enum_name(static_cast<Empty>(0)).empty());
static_assert(!rfl::enum_cast<Empty>("Empty"));
static_assert(rfl::enum_count<Aliases>() == 2);
static_assert(rfl::enum_name(Aliases::Original) == rfl::enum_name(Aliases::Alias));
static_assert(rfl::enum_cast<Aliases>(rfl::enum_name(Aliases::Alias)) == Aliases::Alias);
static_assert(rfl::enum_count<Byte>() == 3);
static_assert(rfl::enum_name(Byte::High) == "High");
static_assert(rfl::enum_cast<Byte>("Last") == Byte::Last);
static_assert(rfl::enum_count<SignedByte>() == 2);
static_assert(rfl::enum_name(SignedByte::First) == "First");
static_assert(rfl::enum_name(SignedByte::Last) == "Last");
static_assert(rfl::enum_name(Sparse::Low) == "Low");
static_assert(rfl::enum_cast<Sparse>("High") == Sparse::High);
static_assert(rfl::enum_name(Negative::First) == "First");
static_assert(rfl::enum_cast<Negative>("Last") == Negative::Last);
static_assert(rfl::enum_count<Outside>() == 0);
static_assert(rfl::enum_name(Outside::Missing).empty());
static_assert(rfl::enum_name<Outside::Missing>() == "Missing");
static_assert(rfl::enum_name<Wide::Last>() == "Last");
static_assert(rfl::enum_name(Owner::State::Ready) == "Ready");
static_assert(rfl::enum_name(TemplateOwner<int, double>::State::Ready) == "Ready");
static_assert(rfl::enum_name<static_cast<TemplateOwner<int, double>::State>(0)>().empty());
static_assert(rfl::enum_count<Flag>() == 2);
static_assert(rfl::enum_name(Flag::On) == "On");
static_assert(rfl::enum_name(static_cast<Color>(1000)).empty());
static_assert(noexcept(rfl::enum_name(Color::Red)));
static_assert(noexcept(rfl::enum_cast<Color>("Red")));

void testRuntimeConversions() {
    const std::string input = "Blue";
    const auto parsed = rfl::enum_cast<Color>(input);
    assert(parsed && *parsed == Color::Blue);
    assert(rfl::enum_name(*parsed) == input);
    assert(!rfl::enum_cast<Color>(std::string_view("Blue\0ignored", 12)));
    for (const auto value : rfl::enum_values<Color>()) {
        assert(rfl::enum_cast<Color>(rfl::enum_name(value)) == value);
    }
    for (int value = 0; value <= 255; ++value) {
        const auto name = rfl::enum_name(static_cast<Byte>(value));
        assert(name.empty() == (value != 0 && value != 128 && value != 255));
    }
}

} // namespace

int main() {
    testRuntimeConversions();
}
