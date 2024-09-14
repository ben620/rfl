export module rfl;

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
}