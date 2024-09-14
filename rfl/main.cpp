import rfl;
import std;

struct A
{
    int a = 0;
    int b = 1;
    char c[10] = { '\0' };
    int* d = nullptr;
    std::vector<int> e;
};

int main()
{
    std::cout << rfl::MemberCounter<A>() << std::endl;;
    return 0;
}