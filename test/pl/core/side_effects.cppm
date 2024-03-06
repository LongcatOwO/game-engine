module;
#include <memory>

export module pl.core.test:side_effects;

namespace pl_test
{
class SideEffectResult
{
public:
    constexpr unsigned numRegularConstructorCalls() const noexcept
    {
        return _numRegularConstructorCalls;
    }

    constexpr unsigned numCopyConstructorCalls() const noexcept
    {
        return _numCopyConstructorCalls;
    }

    constexpr unsigned numMoveConstructorCalls() const noexcept
    {
        return _numMoveConstructorCalls;
    }

    constexpr unsigned numCopyAssignmentCalls() const noexcept
    {
        return _numCopyAssignmentCalls;
    }

    constexpr unsigned numMoveAssignmentCalls() const noexcept
    {
        return _numMoveAssignmentCalls;
    }

    constexpr unsigned numDestructorCalls() const noexcept
    {
        return _numDestructorCalls;
    }

    constexpr unsigned numTotalConstructorCalls() const noexcept
    {
        return _numRegularConstructorCalls
             + _numCopyConstructorCalls
             + _numMoveConstructorCalls;
    }

    constexpr unsigned numTotalAssignmentCalls() const noexcept
    {
        return _numCopyAssignmentCalls + _numMoveAssignmentCalls;
    }

private:
    friend class SideEffects;

    unsigned _numRegularConstructorCalls = {};
    unsigned _numCopyConstructorCalls = {};
    unsigned _numMoveConstructorCalls = {};
    unsigned _numCopyAssignmentCalls = {};
    unsigned _numMoveAssignmentCalls = {};
    unsigned _numDestructorCalls = {};
};

class SideEffects
{
public:
    constexpr SideEffects(SideEffectResult &result) noexcept: _result(&result)
    { ++_result->_numRegularConstructorCalls; }

    constexpr SideEffects(SideEffects const &other) noexcept: _result(other._result)
    { ++_result->_numCopyConstructorCalls; }

    constexpr SideEffects(SideEffects &&other) noexcept: _result(other._result)
    { ++_result->_numMoveConstructorCalls; }

    constexpr SideEffects &operator=(SideEffects const &) noexcept
    {
        ++_result->_numCopyAssignmentCalls;
        return *this;
    }

    constexpr SideEffects &operator=(SideEffects &&) noexcept
    {
        ++_result->_numMoveAssignmentCalls;
        return *this;
    }

    constexpr ~SideEffects() noexcept
    {
        ++_result->_numDestructorCalls;
    }
private:
    SideEffectResult *_result;
};

class SideEffectsConstructor
{
public:
    constexpr SideEffectsConstructor(SideEffectResult &result) noexcept : _result(&result) {}
    constexpr SideEffects *operator()(SideEffects *s) const noexcept
    {
        return std::ranges::construct_at(s, *_result);
    }
private:
    SideEffectResult *_result;
};
} // namespace pl_test
