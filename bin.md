# rfl 二进制格式设计

`<rfl/bin.hpp>` 将反射结构体编码成二进制数据，用于同 ABI 的文件读写和网络收发。

## 支持范围

| 类型 | Pack | Unpack |
|---|---|---|
| 普通结构体 | 支持 | 支持 |
| 含变长成员的反射结构体 | 支持 | 支持 |
| `std::vector<T>` | 支持 | 支持 |
| `std::list<T>` | 支持 | 支持 |
| `std::string` / `std::string_view` | 支持 | 支持；`string_view` 零拷贝指向输入 |
| `std::array<T, N>` | 支持 | 支持 |
| `std::span<T>` | 支持 | 仅 `std::span<const Plain>` 零拷贝支持 |
| `std::wstring` / `std::wstring_view` | 不支持 | 不支持 |
| 指针、引用 | 不支持 | 不支持 |

普通结构体必须是：

- 反射成员均为普通类型；
- `std::is_trivially_copyable_v<T>` 为真；
- 不含指针、引用、变长容器或 view 成员。

普通结构体使用原生对象表示 `memcpy` 编码。其内部 padding、大小端、浮点表示与 ABI 相关，因此当前格式仅适合同一平台、编译器 ABI 与类型定义之间传输。

## API

```cpp
// 仅接受满足 rfl::BinaryPackable 的类型。
std::optional<std::size_t> rfl::packSize(const T& value) noexcept;
rfl::BinaryPackResult rfl::pack(const T& value,
                                std::span<std::byte> output) noexcept;

// 仅接受同时满足 rfl::BinaryTransactionalUnpackable 的类型。
bool rfl::unpack(std::span<const std::byte> input, T& value);
bool rfl::unpackInPlace(std::span<const std::byte> input, T& value);
```

- 所有公开 API 使用 concept 约束，不可序列化类型会在重载选择阶段被拒绝。
- 库不提供会分配并抛异常的便利 `pack(value)` 或 `unpack<T>(input)` 重载。
- `unpack` 使用临时对象，失败时不修改目标对象。
- `unpackInPlace` 直接写入目标，可复用容器容量；失败时目标可能已部分修改。
- `string_view`、`span<const Plain>` 解包后的生命周期不得超过输入缓冲区。

## 对齐与成员布局

1. 普通对象在写入前按 `alignof(T)` 补零对齐，然后完整 `memcpy`。
2. 变长结构体按声明顺序编码，不写结构体总长度或字段表。
3. 变长成员的长度前缀与 payload 紧凑排列。
4. 变长容器的普通元素 payload 在长度前缀后按元素自然对齐，以保证 `span<const Plain>` 能零拷贝引用。
5. 传给 `Pack(value, span)` 的缓冲区若需要被后续零拷贝为 `span<const T>`，其首地址必须满足 `alignof(T)`；`std::vector<std::byte>` 的默认分配器通常满足普通类型对齐。

## 长度前缀

变长成员统一写入元素数量；`string` 和 `string_view` 写入字节数量。

首字节的高两位表示**长度字段总字节数**，剩余位与后续字节组成大端长度值：

| 前两位 | 长度字段字节数 | 有效位数 | 最大值 |
|---|---:|---:|---:|
| `00` | 1 | 6 | 63 |
| `01` | 2 | 14 | 16,383 |
| `10` | 4 | 30 | 1,073,741,823 |
| `11` | 8 | 62 | 4,611,686,018,427,387,903 |

示例：

```text
长度 63:    0b00111111
长度 64:    0b01000000 0b01000000
长度 16384: 0b10000000 0b00000000 0b01000000 0b00000000
```

## 安全限制

- 解码 `string` 或普通元素 `vector` 前会先确认输入剩余字节足够，避免伪造长度直接触发大额分配。
- `list` 和变长元素容器仍应由调用者在外层协议中限制总消息大小与最大元素数。
- 当前版本没有类型版本号、校验和或跨平台字节序转换；网络协议需要由上层提供消息边界、版本和完整性校验。
