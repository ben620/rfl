import std;
import rfl;

struct Base
{
    int type;
    int length;
};

struct Test
{
    Base base;
    char a[2];
    int p;
    int* d;
};



int main()
{
    Test t;
    t.p = 1;
    rfl::MemberType<Test, 0> member;
    rfl::MemberType<Test, 1> member1;
    static_assert(std::is_same_v<decltype(member1), decltype(Test::a)>);

    std::cout << rfl::MemberValue<Test, 2, 4>(t);
    
#if 0
    struct FD
    {
        void* dummy;
    };
    std::cout << MemberCount<FD>() << std::endl;
    std::cout << MemberCount<Test>() << std::endl;
    std::cout << MemberName<Test, 0>() << std::endl;
    for (auto& name : MemberNames<Test>())
    {
        std::cout << name << "\n";
    }
#endif
    return 0;
}