/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Exercise the production framebuffer validators without display changes.
*/

#include <stdio.h>
#include "fbgfx_validation.h"

static unsigned checks, failures;

#define CHECK(expr) do { \
    ++checks; \
    if (!(expr)) { \
        ++failures; \
        printf("line %u: %s\n", (unsigned)__LINE__, #expr); \
    } \
} while (0)

static void geometry(void)
{
    static const uintptr_t dims[] = {0, 1, 2, 16, 1024, 65535, 65536, UINT32_MAX};
    static const uintptr_t pitches[] = {0, 1, 4, 63, 64, 4096, UINT32_MAX};
    unsigned w, h, p, b;

    for (w = 0; w < sizeof(dims) / sizeof(dims[0]); ++w)
        for (h = 0; h < sizeof(dims) / sizeof(dims[0]); ++h)
            for (p = 0; p < sizeof(pitches) / sizeof(pitches[0]); ++p)
                for (b = 0; b <= 5; ++b)
                {
                    uint32_t bytes = 0x12345678;
                    uint64_t row = (uint64_t)dims[w] * b;
                    uint64_t total = (uint64_t)dims[h] * pitches[p];
                    int expected = dims[w] && dims[h] && b >= 1 && b <= 4 &&
                        row <= pitches[p] && total <= UINT32_MAX;
                    int actual = fbgfx_buffer_size(dims[w], dims[h], pitches[p], b, &bytes);

                    CHECK(actual == expected);
                    CHECK(bytes == (expected ? total : UINT32_C(0x12345678)));
                }

    CHECK(!fbgfx_buffer_size(16, 16, 64, 4, NULL));
#if UINTPTR_MAX > UINT32_MAX
    {
        uint32_t bytes = 123;
        uintptr_t large = (uintptr_t)UINT32_MAX + 1;
        CHECK(!fbgfx_buffer_size(large, 1, 64, 4, &bytes));
        CHECK(!fbgfx_buffer_size(1, large, 4, 4, &bytes));
        CHECK(!fbgfx_buffer_size(1, 1, large, 4, &bytes));
        CHECK(bytes == 123);
    }
#endif
}

static void spans(void)
{
    CHECK(fbgfx_buffer_span(0x1000, 4096));
    CHECK(!fbgfx_buffer_span(0, 4096));
    CHECK(!fbgfx_buffer_span(0x1000, 0));
    CHECK(fbgfx_buffer_span(UINTPTR_MAX - 4096, 4096));
    CHECK(!fbgfx_buffer_span(UINTPTR_MAX - 4095, 4096));
    CHECK(!fbgfx_buffer_span(UINTPTR_MAX, 1));
}

static void colors(void)
{
    static const int depths[] = {-1, 0, 1, 4, 8, 16, 24, 31, 32, 33};
    static const uint32_t values[] = {0, 1, 15, 16, 255, 256, 65535, UINT32_MAX - 1, UINT32_MAX};
    unsigned d, f, n, indexed, present;
    char color;

    for (d = 0; d < sizeof(depths) / sizeof(depths[0]); ++d)
        for (f = 0; f < sizeof(values) / sizeof(values[0]); ++f)
            for (n = 0; n < sizeof(values) / sizeof(values[0]); ++n)
                for (indexed = 0; indexed < 2; ++indexed)
                    for (present = 0; present < 2; ++present)
                    {
                        uint64_t limit = 0, end = (uint64_t)values[f] + values[n];
                        int depth = depths[d], expected;

                        /* Independent wide arithmetic, with no 32-bit addition or shift. */
                        if (depth >= 1 && depth <= 32)
                        {
                            int bit;
                            limit = 1;
                            for (bit = 0; bit < depth; ++bit)
                                limit *= 2;
                            if (indexed && limit > 256)
                                limit = 256;
                        }
                        expected = depth >= 1 && depth <= 32 && end <= limit &&
                            (values[n] == 0 || present);
                        CHECK(fbgfx_color_range(depth, indexed, values[f], values[n],
                                               present ? &color : NULL) == expected);
                    }

    CHECK(fbgfx_color_range(8, 1, 255, 1, &color));
    CHECK(!fbgfx_color_range(8, 1, 255, 2, &color));
    CHECK(!fbgfx_color_range(4, 1, 16, 1, &color));
    CHECK(fbgfx_color_range(32, 0, UINT32_MAX, 1, &color));
    CHECK(!fbgfx_color_range(32, 0, UINT32_MAX, 2, &color));
    CHECK(fbgfx_color_range(8, 1, 256, 0, NULL));
    CHECK(!fbgfx_color_range(8, 1, 0, 1, NULL));
}

int main(void)
{
    geometry();
    spans();
    colors();
    printf("fbgfx_validation: %u checks, %u failures\n", checks, failures);
    return failures ? 10 : 0;
}
