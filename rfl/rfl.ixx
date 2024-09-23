export module rfl;

import std;

export namespace rfl
{
    struct UniversalType 
    {
        template<typename T>
        operator T() {}
    };

    template<typename T>
    consteval auto MemberCounter(auto ...Members) 
    {
        if constexpr (requires { T{ Members... }; } == false)
            return sizeof...(Members) - 1;
        else
            return MemberCounter<T>(Members..., UniversalType{});
    }

    template <class C, std::size_t N>
    struct ObjTupleView
    {
        static consteval auto TupleView() {}
    };


    template <class C>
    struct ObjTupleView<C, 0>
    {
        static consteval auto TupleView() { return std::tie(); }
    };

    template <class C>
    struct ObjTupleView<C, 1>
    {
        static consteval auto TupleView() 
        {
            auto& [e0] = std::declval<std::remove_cvref_t<C>>();
            auto tup = std::tie(e0);
            auto get_ptrs = [](auto&... _refs) {
                return std::make_tuple(&_refs...);
                };
            return std::apply(get_ptrs, tup);
        }
    };

    template <auto ptr>
    inline constexpr std::string_view MemberName()
    {
        constexpr std::string_view func = __FUNCSIG__;
        return func;
    }

    template <class T>
    struct Wrapper {
        using Type = T;
        T v;
    };

    template <class T>
    Wrapper(T) -> Wrapper<T>;

    // This workaround is necessary for clang.
    template <class T>
    inline constexpr auto wrap(const T& arg) noexcept {
        return Wrapper{ arg };
    }

    
}