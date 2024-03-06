module;
#include <iostream>
#include <vulkan/vulkan.h>

module main;

import pl.core;

class N {};

struct NTrait
{
    static constexpr bool isNull(N instance) noexcept { return false; }
    static constexpr N null() noexcept { return {}; }
};

NTrait plNullable(N);

template<pl::nullable T>
class Test {};

int main(int argc, char *argv [])
{
    Test<N> t;
    Test<int*> t2;

    std::cout << "Hello, World!\n";
    return 0;
}
