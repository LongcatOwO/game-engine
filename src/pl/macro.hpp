#pragma once

#if defined(_MSC_VER)
#   define PL_API_ATTR_EMPTY_BASES __declspec(empty_bases)
#else
#   define PL_API_ATTR_EMPTY_BASES
#endif

#define PL_UNREACHABLE() assert(false); ::std::unreachable();

#define PL_REMOVE_PAREN_IF_EXIST(x) PL_REMOVE_PAREN_IF_EXIST_EVAL_0(PL_REMOVE_PAREN_IF_EXIST_STRIP x)
#define PL_REMOVE_PAREN_IF_EXIST_STRIP(...) PL_REMOVE_PAREN_IF_EXIST_STRIP __VA_ARGS__
#define PL_REMOVE_PAREN_IF_EXIST_EVAL_0(...) PL_REMOVE_PAREN_IF_EXIST_EVAL_1(__VA_ARGS__)
#define PL_REMOVE_PAREN_IF_EXIST_EVAL_1(...) PL_REMOVE_PAREN_IF_EXIST_CONSUME_##__VA_ARGS__
#define PL_REMOVE_PAREN_IF_EXIST_CONSUME_PL_REMOVE_PAREN_IF_EXIST_STRIP

#define PL_TRY_ASSIGN(var, ...)                   PL_TRY_ASSIGN_HELPER_0(__COUNTER__, var, __VA_ARGS__)
#define PL_TRY_ASSIGN_HELPER_0(counter, var, ...) PL_TRY_ASSIGN_HELPER_1(counter, var, __VA_ARGS__)
#define PL_TRY_ASSIGN_HELPER_1(counter, var, ...) \
    auto pl_try_expr_var_##counter = (__VA_ARGS__); \
    if (!pl_try_expr_var_##counter) \
        return ::pl::ErrorResult(std::move(pl_try_expr_var_##counter).error()); \
    PL_REMOVE_PAREN_IF_EXIST(var) = *std::move(pl_try_expr_var_##counter);

#define PL_TRY_DISCARD(...)                   PL_TRY_DISCARD_HELPER_0(__COUNTER__, __VA_ARGS__)
#define PL_TRY_DISCARD_HELPER_0(counter, ...) PL_TRY_DISCARD_HELPER_1(counter, __VA_ARGS__)
#define PL_TRY_DISCARD_HELPER_1(counter, ...) \
    auto pl_try_expr_var_##counter = (__VA_ARGS__); \
    if (!pl_try_expr_var_##counter) \
        return ::pl::ErrorResult(std::move(pl_try_expr_var_##counter).error());

#ifdef NDEBUG
#   define PL_ASSERT(...)
#else
#   define PL_ASSERT(...) if !consteval { assert(__VA_ARGS__); }
#endif


#define PL_DECLARE_ERROR_TYPE(parent, type, display_name) \
class type : public SuberrorType<parent> \
{ \
public: \
    using SuberrorType<parent>::SuberrorType; \
    constexpr char const *name() const noexcept final \
    { \
        return display_name; \
    } \
}

#define PL_DEFER(...) PL_DEFER_HELPER_0(__COUNTER__, __VA_ARGS__)
#define PL_DEFER_HELPER_0(counter, ...) PL_DEFER_HELPER_1(counter, __VA_ARGS__)
#define PL_DEFER_HELPER_1(counter, ...) ::pl::Defer pl_Defer_##counter{[&]{__VA_ARGS__;}}
