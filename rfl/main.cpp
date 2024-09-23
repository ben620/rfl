//inspired by https://github.com/yosh-matsuda/field-reflection
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



template <typename T>
constexpr auto MakePtrTuple(T&& t)
{
    auto& [f0] = t;
    return std::tuple(&f0);
}

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