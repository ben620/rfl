#pragma once

#include <rfl/meta.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace rfl {

struct json_write_result {
    std::size_t written{};
    std::size_t required{};
    bool valid{};

    [[nodiscard]] constexpr bool complete() const noexcept {
        return valid && written == required;
    }
};

class json_error : public std::invalid_argument {
public:
    explicit json_error(std::size_t position)
        : std::invalid_argument("invalid JSON"), position_(position) {}

    [[nodiscard]] std::size_t position() const noexcept {
        return position_;
    }

private:
    std::size_t position_;
};

namespace format_detail {

template <class>
inline constexpr bool dependent_false = false;

template <class T>
using object_type_t = std::remove_cvref_t<T>;

template <class T>
struct is_optional : std::false_type {};

template <class T>
struct is_optional<std::optional<T>> : std::true_type {};

template <class T>
inline constexpr bool is_optional_v = is_optional<object_type_t<T>>::value;

template <class T>
struct is_vector : std::false_type {};

template <class T, class Allocator>
struct is_vector<std::vector<T, Allocator>> : std::true_type {};

template <class T>
inline constexpr bool is_vector_v = is_vector<object_type_t<T>>::value;

template <class T>
struct is_std_array : std::false_type {};

template <class T, std::size_t Size>
struct is_std_array<std::array<T, Size>> : std::true_type {};

template <class T>
inline constexpr bool is_std_array_v = is_std_array<object_type_t<T>>::value;

template <class T>
inline constexpr bool is_character_pointer_v =
    std::is_pointer_v<object_type_t<T>> &&
    (std::same_as<std::remove_pointer_t<object_type_t<T>>, char> ||
     std::same_as<std::remove_pointer_t<object_type_t<T>>, const char>);

template <class T>
inline constexpr bool is_character_array_v =
    std::is_array_v<std::remove_reference_t<T>> &&
    (std::same_as<std::remove_extent_t<std::remove_reference_t<T>>, char> ||
     std::same_as<std::remove_extent_t<std::remove_reference_t<T>>, const char>);

template <class T>
inline constexpr bool is_string_like_v =
    std::same_as<object_type_t<T>, std::string> ||
    std::same_as<object_type_t<T>, std::string_view> ||
    is_character_pointer_v<T> || is_character_array_v<T>;

template <class Sink>
using sink_result_t = std::invoke_result_t<Sink&, std::string_view>;

template <class Sink>
concept json_sink = std::invocable<Sink&, std::string_view> &&
    (std::same_as<sink_result_t<Sink>, void> ||
     std::same_as<sink_result_t<Sink>, bool> ||
     (std::integral<sink_result_t<Sink>> &&
      !std::same_as<sink_result_t<Sink>, bool>));

// 0: plain ASCII, copied verbatim.
// 1: must be escaped (control characters, quote, backslash).
// 2: start of a multi-byte UTF-8 sequence, needs validation.
inline constexpr auto byte_class = [] {
    std::array<unsigned char, 256> table{};
    for (std::size_t index = 0; index != table.size(); ++index) {
        table[index] = index < 0x20 || index == '"' || index == '\\'
            ? 1u
            : (index >= 0x80 ? 2u : 0u);
    }
    return table;
}();

[[nodiscard]] inline std::size_t utf8_sequence_length(
    std::string_view text, std::size_t index) noexcept {
    const auto* data = reinterpret_cast<const unsigned char*>(text.data());
    const auto lead = data[index];
    std::size_t length = 0;
    std::uint32_t code_point = 0;
    if ((lead & 0xe0) == 0xc0) {
        length = 2;
        code_point = lead & 0x1fu;
    } else if ((lead & 0xf0) == 0xe0) {
        length = 3;
        code_point = lead & 0x0fu;
    } else if ((lead & 0xf8) == 0xf0) {
        length = 4;
        code_point = lead & 0x07u;
    } else {
        return 0;
    }

    if (text.size() - index < length) {
        return 0;
    }
    for (std::size_t offset = 1; offset != length; ++offset) {
        const auto continuation = data[index + offset];
        if ((continuation & 0xc0) != 0x80) {
            return 0;
        }
        code_point = (code_point << 6) | (continuation & 0x3fu);
    }

    static constexpr std::uint32_t minimum[5] = {0, 0, 0x80, 0x800, 0x10000};
    if (code_point < minimum[length] || code_point > 0x10ffff ||
        (code_point >= 0xd800 && code_point <= 0xdfff)) {
        return 0;
    }
    return length;
}

[[nodiscard]] inline bool valid_utf8(std::string_view text) noexcept {
    std::size_t index = 0;
    while (index != text.size()) {
        if (static_cast<unsigned char>(text[index]) < 0x80) {
            ++index;
            continue;
        }
        const auto length = utf8_sequence_length(text, index);
        if (length == 0) {
            return false;
        }
        index += length;
    }
    return true;
}

class sink_ref {
public:
    template <class Sink>
        requires json_sink<Sink> &&
                 (!std::same_as<std::remove_cvref_t<Sink>, sink_ref>)
    explicit sink_ref(Sink& sink) noexcept
        : context_(std::addressof(sink)), invoke_(&invoke_impl<Sink>) {}

    sink_ref(const sink_ref&) noexcept = default;
    sink_ref& operator=(const sink_ref&) noexcept = default;

    [[nodiscard]] std::size_t operator()(std::string_view text) const {
        return invoke_(context_, text);
    }

private:
    template <class Sink>
    static std::size_t invoke_impl(void* context, std::string_view text) {
        auto& sink = *static_cast<Sink*>(context);
        if constexpr (std::same_as<sink_result_t<Sink>, void>) {
            std::invoke(sink, text);
            return text.size();
        } else if constexpr (std::same_as<sink_result_t<Sink>, bool>) {
            return std::invoke(sink, text) ? text.size() : 0;
        } else {
            const auto count = std::invoke(sink, text);
            if (count <= 0 || static_cast<std::size_t>(count) > text.size()) {
                return 0;
            }
            return static_cast<std::size_t>(count);
        }
    }

    void* context_;
    std::size_t (*invoke_)(void*, std::string_view);
};

struct string_writer {
    std::string& output;

    bool append(char character) {
        output.push_back(character);
        return true;
    }

    template <std::size_t Size>
    bool append(const char (&text)[Size]) {
        output.append(text, Size - 1);
        return true;
    }

    bool append(std::string_view text) {
        output.append(text);
        return true;
    }
};

template <std::size_t LocalCapacity = 256>
class small_string_writer {
public:
    bool append(char character) {
        if (!spilled_ && used_ != local_.size()) {
            local_[used_++] = character;
            return true;
        }
        spill(1);
        output_.push_back(character);
        return true;
    }

    template <std::size_t Size>
    bool append(const char (&text)[Size]) {
        return append(std::string_view(text, Size - 1));
    }

    bool append(std::string_view text) {
        if (!spilled_ && text.size() <= local_.size() - used_) {
            if (!text.empty()) {
                std::memcpy(local_.data() + used_, text.data(), text.size());
                used_ += text.size();
            }
            return true;
        }
        spill(text.size());
        output_.append(text);
        return true;
    }

    [[nodiscard]] std::string finish() && {
        if (spilled_) {
            return std::move(output_);
        }
        return std::string(local_.data(), used_);
    }

private:
    void spill(std::size_t additional) {
        if (spilled_) {
            return;
        }
        const auto minimum = used_ + additional;
        const auto growth = (std::max)(
            local_.size() + local_.size() / 2,
            minimum + minimum / 2);
        output_.reserve(growth);
        output_.append(local_.data(), used_);
        spilled_ = true;
    }

    std::array<char, LocalCapacity> local_;
    std::string output_;
    std::size_t used_{};
    bool spilled_{};
};

// Precondition: `output` must not overlap the storage of the serialized value
// or any of its nested strings.
struct span_writer {
    std::span<char> output;
    std::size_t written{};
    std::size_t required{};

    bool append(char character) noexcept {
        if (written != output.size()) {
            output[written++] = character;
        }
        ++required;
        return true;
    }

    template <std::size_t Size>
    bool append(const char (&text)[Size]) noexcept {
        return append(std::string_view(text, Size - 1));
    }

    bool append(std::string_view text) noexcept {
        const auto available = output.size() - written;
        const auto count = (std::min)(available, text.size());
        if (count != 0) {
            std::memcpy(output.data() + written, text.data(), count);
            written += count;
        }
        required += text.size();
        return true;
    }
};

class buffered_sink_writer {
public:
    buffered_sink_writer(std::span<char> buffer, sink_ref sink) noexcept
        : buffer_(buffer), sink_(sink) {}

    buffered_sink_writer(const buffered_sink_writer&) = delete;
    buffered_sink_writer& operator=(const buffered_sink_writer&) = delete;

    bool append(char character) {
        if (used_ == buffer_.size() && !flush()) {
            return false;
        }
        buffer_[used_++] = character;
        return true;
    }

    template <std::size_t Size>
    bool append(const char (&text)[Size]) {
        constexpr auto length = Size - 1;
        if (buffer_.size() - used_ >= length) {
            std::memcpy(buffer_.data() + used_, text, length);
            used_ += length;
            return true;
        }
        return append(std::string_view(text, length));
    }

    bool append(std::string_view text) {
        if (used_ != 0) {
            const auto count = (std::min)(buffer_.size() - used_, text.size());
            std::memcpy(buffer_.data() + used_, text.data(), count);
            used_ += count;
            text.remove_prefix(count);
            if (used_ == buffer_.size() && !flush()) {
                return false;
            }
        }

        while (text.size() >= buffer_.size()) {
            if (!send_all(text.substr(0, buffer_.size()))) {
                return false;
            }
            text.remove_prefix(buffer_.size());
        }

        if (!text.empty()) {
            std::memcpy(buffer_.data(), text.data(), text.size());
            used_ = text.size();
        }
        return true;
    }

    bool flush() {
        if (used_ == 0) {
            return true;
        }
        if (!send_all(std::string_view(buffer_.data(), used_))) {
            return false;
        }
        used_ = 0;
        return true;
    }

private:
    bool send_all(std::string_view text) {
        while (!text.empty()) {
            const auto count = sink_(text);
            if (count == 0) {
                return false;
            }
            text.remove_prefix(count);
        }
        return true;
    }

    std::span<char> buffer_;
    sink_ref sink_;
    std::size_t used_{};
};

template <class Writer>
bool write_escaped_string(Writer& writer, std::string_view value) {
    static constexpr char hex[] = "0123456789abcdef";
    if (!writer.append('"')) {
        return false;
    }

    std::size_t plain_begin = 0;
    std::size_t index = 0;
    while (index != value.size()) {
        const auto character = static_cast<unsigned char>(value[index]);
        const auto kind = byte_class[character];
        if (kind == 0) {
            ++index;
            continue;
        }
        if (kind == 2) {
            const auto length = utf8_sequence_length(value, index);
            if (length == 0) {
                return false;
            }
            index += length;
            continue;
        }
        if (index != plain_begin &&
            !writer.append(value.substr(plain_begin, index - plain_begin))) {
            return false;
        }

        switch (character) {
        case '"':
            if (!writer.append("\\\"")) return false;
            break;
        case '\\':
            if (!writer.append("\\\\")) return false;
            break;
        case '\b':
            if (!writer.append("\\b")) return false;
            break;
        case '\f':
            if (!writer.append("\\f")) return false;
            break;
        case '\n':
            if (!writer.append("\\n")) return false;
            break;
        case '\r':
            if (!writer.append("\\r")) return false;
            break;
        case '\t':
            if (!writer.append("\\t")) return false;
            break;
        default: {
            const char encoded[] = {
                '\\', 'u', '0', '0', hex[character >> 4], hex[character & 0x0f]};
            if (!writer.append(std::string_view(encoded, sizeof(encoded)))) {
                return false;
            }
            break;
        }
        }
        plain_begin = ++index;
    }

    return writer.append(value.substr(plain_begin)) && writer.append('"');
}

template <class Writer, class T>
bool write_json(Writer& writer, const T& value);

template <class Writer, class Range>
bool write_array(Writer& writer, const Range& range) {
    if (!writer.append('[')) {
        return false;
    }
    bool first = true;
    for (const auto& item : range) {
        if ((!first && !writer.append(',')) || !write_json(writer, item)) {
            return false;
        }
        first = false;
    }
    return writer.append(']');
}

template <class Writer, class T>
bool write_json(Writer& writer, const T& value) {
    using value_type = object_type_t<T>;
    if constexpr (std::same_as<value_type, std::nullptr_t>) {
        return writer.append("null");
    } else if constexpr (std::same_as<value_type, bool>) {
        return writer.append(value ? "true" : "false");
    } else if constexpr (std::is_integral_v<value_type>) {
        std::array<char, std::numeric_limits<value_type>::digits10 + 4> buffer;
        const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        return result.ec == std::errc{} &&
               writer.append(std::string_view(buffer.data(), result.ptr));
    } else if constexpr (std::is_floating_point_v<value_type>) {
        if (!std::isfinite(value)) {
            return false;
        }
        std::array<char, 128> buffer;
        const auto result =
            std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        return result.ec == std::errc{} &&
               writer.append(std::string_view(buffer.data(), result.ptr));
    } else if constexpr (std::is_enum_v<value_type>) {
        return write_json(writer, static_cast<std::underlying_type_t<value_type>>(value));
    } else if constexpr (is_string_like_v<value_type>) {
        if constexpr (is_character_pointer_v<value_type>) {
            if (value == nullptr) {
                return writer.append("null");
            }
            return write_escaped_string(writer, std::string_view(value));
        } else if constexpr (is_character_array_v<value_type>) {
            constexpr auto extent = std::extent_v<value_type>;
            const auto size = extent != 0 && value[extent - 1] == '\0'
                ? extent - 1
                : extent;
            return write_escaped_string(writer, std::string_view(value, size));
        } else {
            return write_escaped_string(writer, std::string_view(value));
        }
    } else if constexpr (is_optional_v<value_type>) {
        return value ? write_json(writer, *value) : writer.append("null");
    } else if constexpr (is_std_array_v<value_type> || is_vector_v<value_type>) {
        return write_array(writer, value);
    } else if constexpr (meta::reflectable_type<value_type>) {
        if (!writer.append('{')) {
            return false;
        }
        const bool success = meta::for_each_member_while(
            value, [&](auto member, const auto& field) {
                if constexpr (meta::index_of(member) != 0) {
                    if (!writer.append(',')) {
                        return false;
                    }
                }
                return writer.append('"') &&
                       writer.append(meta::identifier_of(member)) &&
                       writer.append("\":") && write_json(writer, field);
            });
        return success && writer.append('}');
    } else {
        static_assert(dependent_false<value_type>, "Type is not JSON serializable.");
    }
}

class json_reader {
public:
    explicit json_reader(std::string_view input) noexcept : input_(input) {}

    [[nodiscard]] std::size_t position() const noexcept {
        return position_;
    }

    void skip_whitespace() noexcept {
        while (position_ < input_.size()) {
            const char character = input_[position_];
            if (character != ' ' && character != '\t' &&
                character != '\r' && character != '\n') {
                break;
            }
            ++position_;
        }
    }

    [[nodiscard]] std::optional<char> peek() noexcept {
        skip_whitespace();
        if (position_ == input_.size()) {
            return std::nullopt;
        }
        return input_[position_];
    }

    bool consume(char expected) noexcept {
        skip_whitespace();
        if (position_ == input_.size() || input_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    bool consume_literal(std::string_view literal) noexcept {
        skip_whitespace();
        if (input_.substr(position_, literal.size()) != literal) {
            return false;
        }
        position_ += literal.size();
        return true;
    }

    bool finished() noexcept {
        skip_whitespace();
        return position_ == input_.size();
    }

    bool scan_string(std::string_view& raw, bool& escaped) noexcept {
        skip_whitespace();
        if (position_ == input_.size() || input_[position_] != '"') {
            return false;
        }
        ++position_;
        const auto begin = position_;
        escaped = false;
        while (position_ < input_.size()) {
            const auto character = static_cast<unsigned char>(input_[position_]);
            if (character == '"') {
                raw = input_.substr(begin, position_ - begin);
                ++position_;
                return true;
            }
            const auto kind = byte_class[character];
            if (kind == 0) {
                ++position_;
                continue;
            }
            if (kind == 2) {
                const auto length = utf8_sequence_length(input_, position_);
                if (length == 0) {
                    return false;
                }
                position_ += length;
                continue;
            }
            if (character == '\\') {
                escaped = true;
                position_ += 2;
                if (position_ > input_.size()) {
                    return false;
                }
                continue;
            }
            return false;
        }
        return false;
    }

    bool skip_string() noexcept {
        std::string_view raw;
        bool escaped = false;
        if (!scan_string(raw, escaped)) {
            return false;
        }
        if (!escaped) {
            return true;
        }
        const auto end = position_;
        position_ = end - raw.size() - 1;
        std::string decoded;
        const bool valid = decode_string(decoded);
        position_ = end;
        return valid;
    }

    bool parse_string(std::string& output) {
        const auto begin = position_;
        std::string_view raw;
        bool escaped = false;
        if (!scan_string(raw, escaped)) {
            return false;
        }
        if (!escaped) {
            output.assign(raw);
            return true;
        }
        const auto end = position_;
        position_ = begin;
        skip_whitespace();
        ++position_;
        const bool valid = decode_string(output);
        if (!valid) {
            return false;
        }
        position_ = end;
        return true;
    }

    bool parse_key(std::string_view& raw, std::string& storage) {
        bool escaped = false;
        if (!scan_string(raw, escaped)) {
            return false;
        }
        if (!escaped) {
            return true;
        }
        const auto end = position_;
        position_ = end - raw.size() - 1;
        const bool valid = decode_string(storage);
        position_ = end;
        if (!valid) {
            return false;
        }
        raw = storage;
        return true;
    }

private:
    bool decode_string(std::string& output) {
        output.clear();
        std::size_t plain_begin = position_;
        while (position_ < input_.size()) {
            const auto character = static_cast<unsigned char>(input_[position_]);
            if (character == '"') {
                output.append(input_.substr(plain_begin, position_ - plain_begin));
                ++position_;
                return true;
            }
            if (character < 0x20) {
                return false;
            }
            if (character != '\\') {
                ++position_;
                continue;
            }

            output.append(input_.substr(plain_begin, position_ - plain_begin));
            ++position_;
            if (position_ == input_.size()) {
                return false;
            }
            const char escape = input_[position_++];
            switch (escape) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u': {
                std::uint32_t code_point{};
                if (!parse_hex_quad(code_point)) {
                    return false;
                }
                if (code_point >= 0xd800 && code_point <= 0xdbff) {
                    if (position_ + 2 > input_.size() || input_[position_] != '\\' ||
                        input_[position_ + 1] != 'u') {
                        return false;
                    }
                    position_ += 2;
                    std::uint32_t low{};
                    if (!parse_hex_quad(low) || low < 0xdc00 || low > 0xdfff) {
                        return false;
                    }
                    code_point = 0x10000 + ((code_point - 0xd800) << 10) +
                                 (low - 0xdc00);
                } else if (code_point >= 0xdc00 && code_point <= 0xdfff) {
                    return false;
                }
                append_utf8(output, code_point);
                break;
            }
            default: return false;
            }
            plain_begin = position_;
        }
        return false;
    }

public:

    bool read_number_token(std::string_view& token) noexcept {
        skip_whitespace();
        const auto begin = position_;
        if (position_ < input_.size() && input_[position_] == '-') {
            ++position_;
        }
        if (position_ == input_.size()) {
            position_ = begin;
            return false;
        }
        if (input_[position_] == '0') {
            ++position_;
            if (position_ < input_.size() && input_[position_] >= '0' &&
                input_[position_] <= '9') {
                position_ = begin;
                return false;
            }
        } else if (input_[position_] >= '1' && input_[position_] <= '9') {
            do {
                ++position_;
            } while (position_ < input_.size() && input_[position_] >= '0' &&
                     input_[position_] <= '9');
        } else {
            position_ = begin;
            return false;
        }
        if (position_ < input_.size() && input_[position_] == '.') {
            ++position_;
            if (!consume_digits()) {
                position_ = begin;
                return false;
            }
        }
        if (position_ < input_.size() &&
            (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() &&
                (input_[position_] == '+' || input_[position_] == '-')) {
                ++position_;
            }
            if (!consume_digits()) {
                position_ = begin;
                return false;
            }
        }
        token = input_.substr(begin, position_ - begin);
        return true;
    }

private:
    bool consume_digits() noexcept {
        const auto begin = position_;
        while (position_ < input_.size() && input_[position_] >= '0' &&
               input_[position_] <= '9') {
            ++position_;
        }
        return position_ != begin;
    }

    bool parse_hex_quad(std::uint32_t& value) noexcept {
        if (position_ + 4 > input_.size()) {
            return false;
        }
        value = 0;
        for (int index = 0; index < 4; ++index) {
            const char character = input_[position_++];
            value <<= 4;
            if (character >= '0' && character <= '9') {
                value += static_cast<std::uint32_t>(character - '0');
            } else if (character >= 'a' && character <= 'f') {
                value += static_cast<std::uint32_t>(character - 'a' + 10);
            } else if (character >= 'A' && character <= 'F') {
                value += static_cast<std::uint32_t>(character - 'A' + 10);
            } else {
                return false;
            }
        }
        return true;
    }

    static void append_utf8(std::string& output, std::uint32_t value) {
        if (value <= 0x7f) {
            output.push_back(static_cast<char>(value));
        } else if (value <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (value >> 6)));
            output.push_back(static_cast<char>(0x80 | (value & 0x3f)));
        } else if (value <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (value >> 12)));
            output.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (value & 0x3f)));
        } else {
            output.push_back(static_cast<char>(0xf0 | (value >> 18)));
            output.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (value & 0x3f)));
        }
    }

    std::string_view input_;
    std::size_t position_{};
};

template <class T>
bool read_json(json_reader& reader, T& value);

inline bool skip_json(json_reader& reader, std::size_t depth = 0) {
    if (depth > 128) {
        return false;
    }
    const auto first = reader.peek();
    if (!first) {
        return false;
    }
    if (*first == 'n') {
        return reader.consume_literal("null");
    }
    if (*first == 't') {
        return reader.consume_literal("true");
    }
    if (*first == 'f') {
        return reader.consume_literal("false");
    }
    if (*first == '"') {
        return reader.skip_string();
    }
    if (*first == '-' || (*first >= '0' && *first <= '9')) {
        std::string_view number;
        return reader.read_number_token(number);
    }
    if (*first == '[') {
        if (!reader.consume('[')) {
            return false;
        }
        if (reader.consume(']')) {
            return true;
        }
        do {
            if (!skip_json(reader, depth + 1)) {
                return false;
            }
        } while (reader.consume(','));
        return reader.consume(']');
    }
    if (*first == '{') {
        if (!reader.consume('{')) {
            return false;
        }
        if (reader.consume('}')) {
            return true;
        }
        do {
            if (!reader.skip_string() || !reader.consume(':') ||
                !skip_json(reader, depth + 1)) {
                return false;
            }
        } while (reader.consume(','));
        return reader.consume('}');
    }
    return false;
}

template <class T>
bool read_number(json_reader& reader, T& value) {
    std::string_view token;
    if (!reader.read_number_token(token)) {
        return false;
    }
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

template <class T>
bool read_json(json_reader& reader, T& value) {
    using value_type = object_type_t<T>;
    if constexpr (std::same_as<value_type, bool>) {
        if (reader.consume_literal("true")) {
            value = true;
            return true;
        }
        if (reader.consume_literal("false")) {
            value = false;
            return true;
        }
        return false;
    } else if constexpr (std::is_integral_v<value_type> ||
                         std::is_floating_point_v<value_type>) {
        return read_number(reader, value);
    } else if constexpr (std::is_enum_v<value_type>) {
        std::underlying_type_t<value_type> underlying{};
        if (!read_number(reader, underlying)) {
            return false;
        }
        value = static_cast<value_type>(underlying);
        return true;
    } else if constexpr (std::same_as<value_type, std::string>) {
        return reader.parse_string(value);
    } else if constexpr (is_optional_v<value_type>) {
        if (reader.consume_literal("null")) {
            value.reset();
            return true;
        }
        typename value_type::value_type item{};
        if (!read_json(reader, item)) {
            return false;
        }
        value = std::move(item);
        return true;
    } else if constexpr (is_std_array_v<value_type>) {
        if (!reader.consume('[')) {
            return false;
        }
        for (std::size_t index = 0; index < value.size(); ++index) {
            if ((index != 0 && !reader.consume(',')) ||
                !read_json(reader, value[index])) {
                return false;
            }
        }
        return reader.consume(']');
    } else if constexpr (is_vector_v<value_type>) {
        if (!reader.consume('[')) {
            return false;
        }
        value.clear();
        reader.skip_whitespace();
        if (reader.consume(']')) {
            return true;
        }
        do {
            value.emplace_back();
            if (!read_json(reader, value.back())) {
                return false;
            }
        } while (reader.consume(','));
        return reader.consume(']');
    } else if constexpr (meta::reflectable_type<value_type>) {
        if (!reader.consume('{')) {
            return false;
        }
        reader.skip_whitespace();
        if (reader.consume('}')) {
            return true;
        }

        std::string key_storage;
        do {
            std::string_view name;
            if (!reader.parse_key(name, key_storage) || !reader.consume(':')) {
                return false;
            }
            bool matched = false;
            bool success = true;
            meta::for_each_member_while(value, [&](auto member, auto& field) {
                constexpr auto identifier = meta::identifier_of(member);
                if (name.size() == identifier.size() && name == identifier) {
                    matched = true;
                    success = read_json(reader, field);
                    return false;
                }
                return true;
            });
            if (!success || (!matched && !skip_json(reader))) {
                return false;
            }
        } while (reader.consume(','));
        return reader.consume('}');
    } else {
        static_assert(dependent_false<value_type>, "Type is not JSON deserializable.");
    }
}

} // namespace format_detail

template <class T>
[[nodiscard]] std::string ToJson(const T& value) {
    format_detail::small_string_writer<> writer;
    if (!format_detail::write_json(writer, value)) {
        throw std::domain_error("value cannot be represented as JSON");
    }
    return std::move(writer).finish();
}

template <class T>
[[nodiscard]] bool ToJson(const T& value, std::string& output) {
    output.clear();
    format_detail::string_writer writer{output};
    return format_detail::write_json(writer, value);
}

template <class T>
[[nodiscard]] json_write_result ToJson(const T& value, std::span<char> output) noexcept {
    format_detail::span_writer writer{output};
    const bool valid = format_detail::write_json(writer, value);
    return {writer.written, writer.required, valid};
}

template <class T>
[[nodiscard]] std::optional<std::size_t> JsonSize(const T& value) noexcept {
    format_detail::span_writer writer{std::span<char>{}};
    if (!format_detail::write_json(writer, value)) {
        return std::nullopt;
    }
    return writer.required;
}

template <std::size_t BufferSize = 4096, class T, class Sink>
    requires format_detail::json_sink<std::remove_reference_t<Sink>>
[[nodiscard]] bool ToJson(const T& value, Sink&& sink) {
    static_assert(BufferSize > 0, "The JSON sink buffer must not be empty.");
    auto& callback = sink;
    std::array<char, BufferSize> buffer;
    format_detail::buffered_sink_writer writer{
        std::span<char>(buffer), format_detail::sink_ref(callback)};
    return format_detail::write_json(writer, value) && writer.flush();
}

// Parses into a temporary and only commits on success, so `value` is left
// untouched when the input is rejected.
template <class T>
[[nodiscard]] bool FromJson(std::string_view input, T& value) {
    T parsed{};
    format_detail::json_reader reader(input);
    if (!format_detail::read_json(reader, parsed) || !reader.finished()) {
        return false;
    }
    value = std::move(parsed);
    return true;
}

// Parses directly into `value`, reusing its existing capacity. Faster, but
// `value` may be partially modified when parsing fails.
template <class T>
[[nodiscard]] bool FromJsonInPlace(std::string_view input, T& value) {
    format_detail::json_reader reader(input);
    return format_detail::read_json(reader, value) && reader.finished();
}

template <class T>
[[nodiscard]] T FromJson(std::string_view input) {
    T value{};
    format_detail::json_reader reader(input);
    if (!format_detail::read_json(reader, value) || !reader.finished()) {
        throw json_error(reader.position());
    }
    return value;
}

} // namespace rfl
