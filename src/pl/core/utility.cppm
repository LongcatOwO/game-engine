export module pl.core:utility;

export namespace pl
{
template<class ...Fn>
class Overloaded : public Fn...
{
public:
    using Fn::operator()...;
};

template<class ...Fn>
Overloaded(Fn...) -> Overloaded<Fn...>;
} // namespace pl
