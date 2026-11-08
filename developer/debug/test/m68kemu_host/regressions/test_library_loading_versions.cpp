/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Production loader and real Moira instruction execution. DOS, heap and
    hunk placement are recording fixtures; this is not an AROS image test.
*/
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <vector>
#include "Moira/Moira.h"
#include "library_constants.inc"

using UBYTE = uint8_t;
using UWORD = uint16_t;
using WORD = int16_t;
using ULONG = uint32_t;
using LONG = int32_t;
using BPTR = uintptr_t;
using APTR = void *;
using STRPTR = char *;
using BOOL = int;
#define TRUE 1
#define FALSE 0
#define BNULL 0
#define MODE_OLDFILE 1005
#define OFFSET_END 1
#define OFFSET_BEGINNING -1
#define MEMF_ANY 0
#define LIBF_SUMUSED (1 << 2)
#define LIBF_CHANGED 2
#define NT_LIBRARY 9
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define AROS_BE2LONG(x) (x)
#else
#define AROS_BE2LONG(x) __builtin_bswap32(x)
#endif

static unsigned checks;
#define CHECK(c, label) do { ++checks; if (!(c)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, label); std::exit(1); } } while (0)
struct M68KTaskState { ULONG tail_pc = 0, tail_d0 = 0, tail_a0 = 0; };
struct M68KEmuContext
{
    std::vector<UBYTE> storage = std::vector<UBYTE>(65536);
    UBYTE *mem = storage.data();
    ULONG mem_size = 65536, load_lo = 0, load_hi = 0, heap = 0x4000;
    ULONG opens = 0, heap_calls = 0, file_pos = 0;
    bool trace_calls = false, low_guard = false;
    M68KTaskState task;
    struct { char name[64]; ULONG base, reopen; } m68klibs[16] = {};
    UWORD num_m68klibs = 0;
};
struct Process { APTR pr_WindowPtr = reinterpret_cast<APTR>(0x1234); };
static Process process;
static M68KEmuContext *active;
static void fixture_bug(const char *, ...) {}
#define bug(...) fixture_bug(__VA_ARGS__)
static void m68k_log(M68KEmuContext *, const char *, ...) {}
static void m68k_low_write_trap(M68KEmuContext *, ULONG, const char *) { std::abort(); }
static M68KTaskState *m68k_ts(M68KEmuContext *ctx) { return &ctx->task; }
static APTR FindTask(APTR) { return &process; }
static BPTR Open(const char *, LONG) { ++active->opens; active->file_pos = 0; return 1; }
static void Close(BPTR) {}
static LONG Seek(BPTR, LONG, LONG mode)
{
    LONG old = static_cast<LONG>(active->file_pos);
    active->file_pos = mode == OFFSET_END ? 4 : 0;
    return old;
}
static LONG Read(BPTR, APTR out, LONG size)
{
    const UBYTE hunk[] = {0, 0, 3, 0xF3};
    CHECK(size == 4, "fixture native file read has expected size");
    std::memcpy(out, hunk, 4); active->file_pos += 4; return 4;
}
static APTR AllocMem(ULONG size, ULONG) { return std::malloc(size); }
static void FreeMem(APTR ptr, ULONG) { std::free(ptr); }
static STRPTR FilePart(STRPTR name)
{
    const char *p = std::strrchr(name, ':');
    return const_cast<char *>(p ? p + 1 : name);
}
static ULONG M68KEmu_LoadHunksFromMemory(M68KEmuContext *ctx, UBYTE *, LONG)
{ ctx->load_lo = 0x1000; ctx->load_hi = 0x2000; return 0x1000; }
static ULONG M68KEmu_HeapAlloc(M68KEmuContext *ctx, ULONG size, ULONG)
{
    ++ctx->heap_calls;
    CHECK(size <= ctx->mem_size - ctx->heap, "fixture heap request fits");
    ULONG result = ctx->heap; ctx->heap += (size + 7) & ~7u; return result;
}
#include "accessors.inc"
#include "library_loading.inc"

class CPU : public moira::Moira
{
    M68KEmuContext &ctx;
public:
    explicit CPU(M68KEmuContext &c) : ctx(c) { setModel(moira::Model::M68000); }
    moira::u8 read8(moira::u32 a) const override
    { CHECK(a < ctx.mem_size, "68k byte read fits"); return ctx.mem[a]; }
    moira::u16 read16(moira::u32 a) const override
    { CHECK(a + 2 <= ctx.mem_size, "68k word read fits"); return m68k_read16(&ctx, a); }
    void write8(moira::u32 a, moira::u8 v) const override
    { CHECK(a < ctx.mem_size, "68k byte write fits"); ctx.mem[a] = v; }
    void write16(moira::u32 a, moira::u16 v) const override
    { CHECK(a + 2 <= ctx.mem_size, "68k word write fits"); m68k_write16(&ctx, a, v); }
};

static ULONG execute_tail(M68KEmuContext &ctx)
{
    m68k_write32(&ctx, 0, 0xF000);
    m68k_write32(&ctx, 4, ctx.task.tail_pc);
    m68k_write32(&ctx, 0xEFFC, 0x300);
    CPU cpu(ctx); cpu.reset();
    cpu.setA(7, 0xEFFC); cpu.setA(6, 0xA600);
    cpu.setD(0, ctx.task.tail_d0); cpu.setA(0, ctx.task.tail_a0);
    unsigned steps = 0;
    while (cpu.getPC() != 0x300 && steps++ < 64) cpu.execute();
    CHECK(cpu.getPC() == 0x300, "generated init/Open sequence returned");
    CHECK(cpu.getA(6) == 0xA600 && cpu.getA(7) == 0xF000, "generated code restores A6 and stack");
    return cpu.getD(0);
}

static void run_case(UBYTE resident, int data_version, int init_version,
                     ULONG requested, bool init_fails = false)
{
    M68KEmuContext ctx; active = &ctx;
    const ULONG rt = 0x1000, init = 0x1100, funcs = 0x1200, data = 0x1300;
    m68k_write16(&ctx, rt, M68K_RTC_MATCHWORD);
    m68k_write32(&ctx, rt + M68K_RT_MATCHTAG, rt);
    ctx.mem[rt + M68K_RT_FLAGS] = M68K_RTF_AUTOINIT;
    ctx.mem[rt + M68K_RT_VERSION] = resident;
    ctx.mem[rt + M68K_RT_TYPE] = NT_LIBRARY;
    m68k_write32(&ctx, rt + M68K_RT_INIT, init);
    m68k_write32(&ctx, init + M68K_AUTOINIT_DATASIZE, 64);
    m68k_write32(&ctx, init + M68K_AUTOINIT_FUNCTABLE, funcs);
    m68k_write32(&ctx, init + M68K_AUTOINIT_DATAINIT, data_version < 0 ? 0 : data);
    m68k_write32(&ctx, init + M68K_AUTOINIT_INITFUNC,
                 init_version < 0 && !init_fails ? 0 : 0x1900);
    m68k_write32(&ctx, funcs, 0x1800); m68k_write32(&ctx, funcs + 4, 0xFFFFFFFFu);
    if (data_version >= 0)
    {
        ctx.mem[data] = 0x90; ctx.mem[data + 1] = M68K_LIB_VERSION;
        m68k_write16(&ctx, data + 2, static_cast<UWORD>(data_version));
    }
    const UWORD open[] = {0x52B9, 0, 0x2100, 0x200E, 0x4E75};
    for (unsigned i = 0; i < sizeof(open) / sizeof(open[0]); ++i)
        m68k_write16(&ctx, 0x1800 + 2 * i, open[i]);
    std::vector<UWORD> initialize = {0x52B9, 0, 0x2104};
    if (init_fails) initialize.push_back(0x7000);
    else
        initialize.insert(initialize.end(), {0x2040, 0x317C,
                          static_cast<UWORD>(init_version), M68K_LIB_VERSION});
    initialize.push_back(0x4E75);
    for (unsigned i = 0; i < initialize.size(); ++i)
        m68k_write16(&ctx, 0x1900 + 2 * i, initialize[i]);

    ULONG base = M68KEmu_LoadM68KLibrary(&ctx, "widget.library", requested);
    CHECK(process.pr_WindowPtr == reinterpret_cast<APTR>(0x1234), "native window policy restored");
    if (requested > 0xFFFFu)
    {
        CHECK(!base && !ctx.opens && !ctx.heap_calls && !ctx.num_m68klibs,
              "full-width impossible version refuses before file/heap mutation");
        return;
    }
    CHECK(base && ctx.num_m68klibs == 1, "loader prepared one reusable library");
    UWORD final_version = static_cast<UWORD>(init_version >= 0 ? init_version :
                                           data_version >= 0 ? data_version : resident);
    bool accepted = !init_fails && final_version >= requested;
    CHECK(execute_tail(ctx) == (accepted ? base : 0), "cold result checks actual post-init version");
    CHECK(m68k_read32(&ctx, 0x2100) == (accepted ? 1u : 0u), "too-old cold library does not call Open");
    CHECK(m68k_read32(&ctx, 0x2104) == (init_version >= 0 || init_fails ? 1u : 0u),
          "cold initialization runs exactly when needed");
    if (!init_fails)
    {
        CHECK(m68k_read16(&ctx, base + M68K_LIB_VERSION) == final_version,
              "data/init version changes are retained");
        ctx.task.tail_pc = ctx.m68klibs[0].reopen;
        ctx.task.tail_d0 = 0;
        CHECK(execute_tail(ctx) == base, "initialized library reopens for a lower request");
        CHECK(m68k_read32(&ctx, 0x2100) == (accepted ? 2u : 1u), "cached reuse calls Open once");
        CHECK(m68k_read32(&ctx, 0x2104) == (init_version >= 0 ? 1u : 0u), "cached reuse never repeats init");
    }
}

int main()
{
    run_case(2, -1, -1, 3);
    run_case(2, -1, -1, 2);
    run_case(1, 5, -1, 4);
    run_case(1, -1, 5, 4);
    run_case(5, -1, 2, 4);
    run_case(2, -1, -1, 0);
    run_case(2, -1, -1, 2, true);
    run_case(1, 0x8000, -1, 2);
    run_case(2, -1, -1, 0x8000);
    run_case(1, 0x8000, -1, 0xFFFF);
    run_case(1, 0xFFFF, -1, 0x8000);
    run_case(1, -1, 0x8000, 0xFFFF);
    run_case(1, -1, 0xFFFF, 0x8000);
    run_case(2, -1, -1, 0x12340002u);
    std::printf("m68kemu loaded-library versions: %u checks, 0 failed\n", checks);
}
