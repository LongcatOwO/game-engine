#pragma once

#define PL_STATIC_ASSERTION_TEST(name) \
[[maybe_unused]] void name()
