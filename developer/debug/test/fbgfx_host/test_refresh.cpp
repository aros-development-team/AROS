/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Independent pixel oracle for the portable framebuffer refresh helper.
*/

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "fbgfx_refresh.h"

static unsigned checks;
static unsigned failures;
static uint32_t random_state = 0xA73142E9;

static void check(bool ok, const char *description)
{
    ++checks;
    if (!ok)
    {
        ++failures;
        std::printf("FAIL %s\n", description);
    }
}

static uint32_t random32()
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void compare(unsigned bytes_per_pixel, int32_t xoffset, int32_t yoffset,
                    int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    const unsigned source_width = 1 + random32() % 11;
    const unsigned source_height = 1 + random32() % 8;
    const unsigned destination_width = 1 + random32() % 13;
    const unsigned destination_height = 1 + random32() % 10;
    const unsigned source_pitch = source_width * bytes_per_pixel + random32() % 9;
    const unsigned destination_pitch = destination_width * 4 + random32() % 11;
    const size_t guard = 16;
    std::vector<uint8_t> source_storage(guard + source_pitch * source_height + guard);
    std::vector<uint8_t> destination_storage(guard + destination_pitch * destination_height + guard, 0xC7);
    std::array<uint8_t, 768> palette;

    for (auto &value : source_storage)
        value = random32();
    for (auto &value : palette)
        value = random32();

    const auto source_before = source_storage;
    auto expected = destination_storage;
    const size_t source_bytes = (random32() & 1) ? source_pitch * source_height :
        (source_height - 1) * source_pitch + source_width * bytes_per_pixel;
    const size_t destination_bytes = (random32() & 1) ? destination_pitch * destination_height :
        (destination_height - 1) * destination_pitch + destination_width * 4;
    FBGfxSurface source =
    {
        source_storage.data() + guard, source_bytes,
        source_width, source_height, source_pitch, static_cast<uint8_t>(bytes_per_pixel)
    };
    FBGfxSurface destination =
    {
        destination_storage.data() + guard, destination_bytes,
        destination_width, destination_height, destination_pitch, 4
    };

    bool touched = false;

    /* Visit each source pixel independently, without rectangle clipping. */
    for (unsigned y = 0; y < source_height; ++y)
    {
        for (unsigned x = 0; x < source_width; ++x)
        {
            const int64_t destination_x = int64_t(x) + xoffset;
            const int64_t destination_y = int64_t(y) + yoffset;

            if (int64_t(x) < x1 || int64_t(x) >= x2 || int64_t(y) < y1 || int64_t(y) >= y2 ||
                destination_x < 0 || destination_y < 0 ||
                destination_x >= destination_width || destination_y >= destination_height)
                continue;

            touched = true;
            const size_t source_offset = guard + y * source_pitch + x * bytes_per_pixel;
            const size_t destination_offset = guard + destination_y * destination_pitch + destination_x * 4;

            if (bytes_per_pixel == 4)
            {
                for (unsigned component = 0; component < 4; ++component)
                    expected[destination_offset + component] = source_storage[source_offset + component];
            }
            else
            {
                for (unsigned component = 0; component < 3; ++component)
                    expected[destination_offset + component] = palette[source_storage[source_offset] * 3 + component];
                expected[destination_offset + 3] = 0;
            }
        }
    }

    const int result = fbgfx_refresh_rect(&source, &destination, palette.data(),
                                         xoffset, yoffset, x1, y1, x2, y2);

    check((result != 0) == touched, "return reports actual clipped pixels");
    check(std::memcmp(destination.pixels - guard, expected.data(), expected.size()) == 0,
          "all pixels, strides and canaries match independent oracle");
    check(std::memcmp(source.pixels - guard, source_before.data(), source_before.size()) == 0,
          "source pixels and all source guards unchanged");
}

static void test_refresh()
{
    for (unsigned bytes_per_pixel : {1U, 4U})
    {
        for (int32_t xoffset : {INT32_MIN, -9, -1, 0, 1, 8, INT32_MAX})
            for (int32_t yoffset : {INT32_MIN, -7, 0, 3, INT32_MAX})
                compare(bytes_per_pixel, xoffset, yoffset, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX);

        for (unsigned i = 0; i < 1800; ++i)
            compare(bytes_per_pixel, int(random32() % 25) - 12, int(random32() % 21) - 10,
                    int(random32() % 26) - 8, int(random32() % 20) - 6,
                    int(random32() % 28) - 8, int(random32() % 22) - 6);
    }

    std::array<uint8_t, 128> source_bytes{};
    std::array<uint8_t, 128> destination_bytes{};
    std::array<uint8_t, 768> palette{};
    source_bytes[0] = 1;
    source_bytes[1] = 2;
    palette[3] = 255;
    palette[7] = 255;
    FBGfxSurface source = {source_bytes.data(), 128, 2, 1, 2, 1};
    FBGfxSurface destination = {destination_bytes.data(), 128, 2, 1, 8, 4};

    check(fbgfx_refresh_rect(&source, &destination, palette.data(), 0, 0, 0, 0, 2, 1) == 1 &&
          destination_bytes[0] == 255 && destination_bytes[5] == 255, "indexed RGB byte order");
    palette[3] = 0;
    palette[5] = 255;
    check(fbgfx_refresh_rect(&source, &destination, palette.data(), 0, 0, 0, 0, 2, 1) == 1 &&
          destination_bytes[0] == 0 && destination_bytes[2] == 255,
          "palette changes recolor unchanged indices");

    const auto valid_source = source;
    const auto valid_destination = destination;
    auto reject = [&](const FBGfxSurface *src, const FBGfxSurface *dst, const uint8_t *colors)
    {
        const auto destination_before = destination_bytes;
        const auto source_before = source_bytes;
        FBGfxSurface mutable_destination = dst ? *dst : valid_destination;

        check(fbgfx_refresh_rect(src, dst ? &mutable_destination : nullptr, colors, 0, 0, 0, 0, 2, 1) == 0,
              "invalid surface is refused");
        check(std::memcmp(destination_bytes.data(), destination_before.data(), destination_before.size()) == 0 &&
              std::memcmp(source_bytes.data(), source_before.data(), source_before.size()) == 0,
              "refusal preserves source, destination and guards");
    };

    reject(nullptr, &destination, palette.data());
    reject(&source, nullptr, palette.data());
    reject(&source, &destination, nullptr);

    for (unsigned field = 0; field < 9; ++field)
    {
        source = valid_source;
        destination = valid_destination;
        switch (field)
        {
        case 0: source.pixels = nullptr; break;
        case 1: source.width = 0; break;
        case 2: source.height = 0; break;
        case 3: source.pitch = 1; break;
        case 4: source.bytes = 1; break;
        case 5: source.bytes_per_pixel = 3; break;
        case 6: destination.bytes_per_pixel = 1; break;
        case 7: destination.pitch = 7; break;
        case 8: destination.bytes = 7; break;
        }
        reject(&source, &destination, palette.data());
    }

    source = valid_source;
    destination = valid_destination;
    source.pixels = reinterpret_cast<uint8_t *>(UINTPTR_MAX - 8);
    source.bytes = 128;
    reject(&source, &destination, palette.data());

    source = valid_source;
    destination = valid_destination;
    source.width = UINT32_MAX;
    source.height = UINT32_MAX;
    source.pitch = UINT32_MAX;
    source.bytes_per_pixel = 4;
    source.bytes = SIZE_MAX;
    reject(&source, &destination, palette.data());

    source = valid_source;
    destination = valid_destination;
    destination.pixels = source_bytes.data();
    reject(&source, &destination, palette.data());
}

static void test_buffer_sizes()
{
    struct SizeCase
    {
        uint32_t width;
        uint32_t height;
        uint32_t pitch;
        uint8_t bytes_per_pixel;
        uint32_t expected;
    };
    const SizeCase valid[] =
    {
        {1, 1, 1, 1, 1},
        {3, 2, 5, 1, 10},
        {1, 1, 4, 4, 4},
        {3, 2, 16, 4, 32},
        {1, 1, UINT32_MAX, 1, UINT32_MAX},
        {UINT32_MAX / 4, 1, UINT32_MAX - 3, 4, UINT32_MAX - 3}
    };
    const SizeCase invalid[] =
    {
        {0, 1, 4, 4, 0},
        {1, 0, 4, 4, 0},
        {1, 1, 0, 1, 0},
        {1, 1, 4, 0, 0},
        {1, 1, 4, 2, 0},
        {1, 1, 4, 3, 0},
        {4, 1, 3, 1, 0},
        {3, 1, 11, 4, 0},
        {UINT32_MAX, 1, UINT32_MAX, 4, 0},
        {1, 2, UINT32_MAX, 1, 0},
        {1, 0x40000000, 8, 4, 0}
    };

    for (const auto &test : valid)
    {
        uint32_t bytes = 0;
        check(fbgfx_buffer_size(test.width, test.height, test.pitch, test.bytes_per_pixel, &bytes),
              "valid backing allocation accepted");
        check(bytes == test.expected, "allocation includes full pitch of every row");
    }

    for (const auto &test : invalid)
    {
        uint32_t bytes = 0x76543210;
        check(!fbgfx_buffer_size(test.width, test.height, test.pitch, test.bytes_per_pixel, &bytes),
              "invalid backing allocation refused");
        check(bytes == 0x76543210, "refused allocation leaves output unchanged");
    }

    check(!fbgfx_buffer_size(1, 1, 4, 4, nullptr), "null allocation output refused");
}

int main()
{
    test_refresh();
    const unsigned refresh_checks = checks;
    check(refresh_checks == 11042, "all original refresh-oracle checks retained");
    test_buffer_sizes();
    std::printf("Framebuffer refresh: %u checks, %u failed (%u original oracle checks)\n",
                checks, failures, refresh_checks);
    return failures ? 1 : 0;
}
