module;
#include <utility>

export module pl.core:defer;

export namespace pl
{
template<class Fn>
class Defer
{
public:
    constexpr Defer(Fn &&fn): _fn(std::move(fn)) {}
    Defer(Defer const &) = delete;
    Defer &operator=(Defer const &) = delete;
    constexpr ~Defer() { _fn(); }

private:
    Fn _fn;
};

template<class Fn>
Defer(Fn) -> Defer<Fn>;
} // export namespace pl
