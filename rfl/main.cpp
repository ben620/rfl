//inspired by https://github.com/yosh-matsuda/field-reflection
//and by http://www.purecpp.cn/detail?id=2437
//and by https://towardsdev.com/counting-the-number-of-fields-in-an-aggregate-in-c-20-c81aecfd725c
#include <iostream>
#include <string_view>
#include <source_location>

struct Test
{
    char a[2];
};


template <typename T>
struct Wrapper
{
    const T value;
    const static Wrapper fake;
};

template <typename T, std::size_t N>
struct MakePtrTupleHelper
{
    static consteval auto Make() {}
};


template <typename T>
struct MakePtrTupleHelper<T, 0>
{
    static consteval auto Make() { return std::tie(); }
};

#define MAKE_PTR_ANY(N, ...) \
template <typename T> \
struct MakePtrTupleHelper<T, N>\
{\
    static consteval auto Make(T &&t)\
    {\
        auto& [__VARARGS__] = t;\
        auto tup = std::tie(__VARARGS__);\
        auto lambda = [](auto&... refs) { return std::make_tuple(&refs...); };\
        return std::apply(lambda, tup);\
    }\
}

MAKE_PTR_ANY(1, _0);

#undef MAKE_PTR_ANY


template <typename T>
consteval auto MakePtrTuple(T&& t)
{
    auto& [f0] = t;
    return std::tuple(&f0);
}

template <typename T>
consteval auto MakeRefTuple(T&& t)
{
    auto& [f0] = t;
    return std::tuple(f0);
}

#define MAKE_PTR_TUPLE(...)

template <typename T, std::size_t N>
consteval auto GetMemPtr() noexcept
{
#if defined(__clang__)
    return wrapper(std::get<N>(to_ptr_tuple(wrapper<T>::fake.value)));
#else
    return std::get<N>(MakePtrTuple(Wrapper<T>::fake.value));
#endif
}


template <typename T, auto Ptr>
consteval std::string_view GetFunctionName()
{
#if defined(__clang__) && defined(_WIN32)
    return std::string_view{ __PRETTY_FUNCTION__ };
#else
    return std::string_view{ std::source_location::current().function_name() };
#endif
}

template <typename T, auto Ptr>
consteval std::string_view GetFieldName()
{
    struct FD
    {
        void* dummy;
    };
    constexpr const std::string_view dummy = "dummy";

    constexpr auto detectorName = GetFunctionName<FD, GetMemPtr<FD, 0>()>();
    constexpr auto dummyBegin = detectorName.rfind(dummy);
    constexpr auto suffix = detectorName.substr(dummyBegin + dummy.size());
    constexpr auto beginSentinel = detectorName[dummyBegin - 1];

    const auto realName = GetFunctionName<T, Ptr>();
    const auto last = realName.rfind(suffix);
    const auto begin = realName.rfind(beginSentinel, last - 1) + 1;
    return realName.substr(begin, last - begin);
}



int main()
{
    
    std::cout << GetFieldName<Test, GetMemPtr<Test, 0>()>();
    return 0;
}