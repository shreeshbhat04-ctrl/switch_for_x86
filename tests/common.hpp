#pragma once

#include <cstdio>

inline int g_fail = 0;

#define CHECK(c) do { \
    if (!(c)) { \
        std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); \
        ++g_fail; \
    } \
} while (0)

#define CHECKF(c, ...) do { \
    if (!(c)) { \
        std::printf("  FAIL %s:%d  ", __FILE__, __LINE__); \
        std::printf(__VA_ARGS__); \
        std::printf("\n"); \
        ++g_fail; \
    } \
} while (0)

inline int report()
{
    if (g_fail) {
        std::printf("RESULT: FAILED (%d checks)\n", g_fail);
        return 1;
    }
    std::printf("RESULT: PASSED\n");
    return 0;
}
