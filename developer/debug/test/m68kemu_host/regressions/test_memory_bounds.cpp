/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Host fixture for the production accessors and Moira memory callbacks.
    Addresses and range lengths stay 32-bit on both ILP32 and LP64 hosts.
    The independent range oracle adds in 64 bits.
*/
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using UBYTE = uint8_t;
using UWORD = uint16_t;
using ULONG = uint32_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
static_assert(sizeof(ULONG) == 4, "Guest addresses must be 32-bit");

#define FALSE 0
#define bug(...) ((void)0)

struct M68KEmuContext
{
    UBYTE *mem = nullptr;
    ULONG mem_size = 0;
    bool running = true, low_guard = false, trace_calls = false;
    ULONG low_trap_count = 0, plane_base = 0, guard_calls = 0;
};

static void m68k_low_write_trap(M68KEmuContext *ctx, ULONG, const char *)
{
    ctx->guard_calls++;
}

static void m68k_planar_touch(M68KEmuContext *, ULONG, ULONG) {}

#include "constants.inc"
#include "accessors.inc"

struct MemoryCallbacks
{
    virtual u16 read16(u32) const = 0;
    virtual void write16(u32, u16) const = 0;
    virtual u16 read16OnReset(u32) const = 0;
    virtual u16 read16Dasm(u32) const = 0;
};

struct AROSMoira : MemoryCallbacks
{
    M68KEmuContext *ctx;
    mutable bool nested = false, nested_done = false;
    mutable unsigned custom_writes = 0;

    explicit AROSMoira(M68KEmuContext *c) : ctx(c) {}
    u16 customRead16(u16) const { return 0xBEEF; }
    void customWrite16(u16, u16) const { custom_writes++; }
    u32 getPC0() const { return 0; }

#include "callbacks.inc"
};

static unsigned checks;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
    std::exit(1); } } while (0)

static bool fits(ULONG size, ULONG address, unsigned width)
{
    return uint64_t(address) + width <= uint64_t(size);
}

static void check_arena(ULONG size)
{
    std::vector<UBYTE> memory(size, 0xA5);
    M68KEmuContext ctx;
    ctx.mem = memory.data();
    ctx.mem_size = size;
    AROSMoira cpu(&ctx);
    std::vector<ULONG> addresses = {
        0, 1, 2, 3, size, size + 1,
        0xFFFFFFFCu, 0xFFFFFFFDu, 0xFFFFFFFEu, 0xFFFFFFFFu
    };
    if (size) addresses.push_back(size - 1);
    if (size >= 2) addresses.push_back(size - 2);
    if (size >= 4) addresses.push_back(size - 4);
    std::sort(addresses.begin(), addresses.end());
    addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());

    for (ULONG address : addresses)
    {
        std::fill(memory.begin(), memory.end(), 0xA5);
        const bool word = fits(size, address, 2);
        const bool longword = fits(size, address, 4);
        CHECK(m68k_read16(&ctx, address) == (word ? 0xA5A5 : 0));
        CHECK(m68k_read32(&ctx, address) == (longword ? 0xA5A5A5A5u : 0));
        CHECK(cpu.read16(address) == (word ? 0xA5A5 : 0));
        CHECK(cpu.read16OnReset(address) == (word ? 0xA5A5 : 0));
        CHECK(cpu.read16Dasm(address) == (word ? 0xA5A5 : 0));

        auto expected = memory;
        if (word)
        {
            expected[address] = 0x12;
            expected[address + 1] = 0x34;
        }
        m68k_write16(&ctx, address, 0x1234);
        CHECK(memory == expected);
        if (word)
        {
            expected[address] = 0x56;
            expected[address + 1] = 0x78;
        }
        cpu.write16(address, 0x5678);
        CHECK(memory == expected);
        if (longword)
        {
            expected[address] = 0x89;
            expected[address + 1] = 0xAB;
            expected[address + 2] = 0xCD;
            expected[address + 3] = 0xEF;
        }
        m68k_write32(&ctx, address, 0x89ABCDEFu);
        CHECK(memory == expected);
        CHECK(ctx.running);
    }
}

static void special_addresses()
{
    std::vector<UBYTE> memory(1024, 0xA5);
    M68KEmuContext ctx;
    ctx.mem = memory.data();
    ctx.mem_size = memory.size();
    AROSMoira cpu(&ctx);
    const auto before = memory;
    CHECK(cpu.read16(CUSTOM_BASE + 0x16) == 0xBEEF);
    cpu.write16(CUSTOM_BASE + 0x2C, 0x1234);
    CHECK(cpu.custom_writes == 1 &&
          std::equal(memory.begin(), memory.end(), before.begin()));
    CHECK(cpu.read16OnReset(CUSTOM_BASE + 0x16) == 0);
    CHECK(cpu.read16Dasm(CUSTOM_BASE + 0x16) == 0);
    CHECK(cpu.read16(SENTINEL_ADDR) == M68K_OP_STOP && !ctx.running);
    ctx.running = true;
    CHECK(cpu.read16OnReset(SENTINEL_ADDR) == 0 && ctx.running);
    CHECK(cpu.read16Dasm(SENTINEL_ADDR) == 0 && ctx.running);
#ifdef HAVE_NEST_SENTINEL
    cpu.nested = true;
    CHECK(cpu.read16(NEST_SENTINEL) == M68K_OP_STOP);
    CHECK(cpu.nested_done && ctx.running);
#endif
#ifdef HAVE_LOW_GUARD
    ctx.low_guard = true;
    m68k_write16(&ctx, 0x100, 0x1234);
    m68k_write32(&ctx, 0x100, 0x12345678);
    CHECK(ctx.guard_calls == 2 &&
          std::equal(memory.begin(), memory.end(), before.begin()));
    cpu.write16(0x100, 0x1234);
    CHECK(ctx.low_trap_count == 1 &&
          std::equal(memory.begin(), memory.end(), before.begin()));
#endif
}

int main()
{
    for (ULONG size : {0u, 1u, 2u, 3u, 4u, 17u})
        check_arena(size);
    special_addresses();
    std::printf("m68kemu memory bounds: %u checks, 0 failed\n", checks);
}
