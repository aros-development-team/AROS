/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The production loader of 68k disk libraries on well-formed and on
    malformed library images. DOS, the heap and the placement of the hunks
    are recording fixtures; no CPU runs. This is not an AROS image test.
*/
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <strings.h>
#include <utility>
#include <vector>
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

/* Containment is the first ARENA bytes of one host buffer. The bytes after
   it stand for native memory that is not the program's: more of them than
   the largest offset an init table can name, so that an access the loader
   must not make lands there and is seen, instead of leaving the buffer. */
static const ULONG ARENA = 0x40000;
static const size_t BEYOND = (size_t(1) << 24) + 0x10000;
static const UBYTE BEYOND_BYTE = 0xA5;
static const ULONG HEAP_BASE = 0x1000;
static std::vector<UBYTE> host(ARENA + BEYOND);

struct Failure { std::string what; };
static unsigned checks;
#define CHECK(c, label) do { ++checks; if (!(c)) throw Failure{label}; } while (0)

struct M68KTaskState { ULONG tail_pc = 0, tail_d0 = 0, tail_a0 = 0; };

/* What the fixture heap hands out: address and size of every live block */
struct Block { ULONG size; bool image; };

struct M68KEmuContext
{
    UBYTE *mem = host.data();
    ULONG mem_size = ARENA;
    ULONG entry_point = 0x00011110, seg_start = 0x00022220;
    ULONG load_lo = 0x00033330, load_hi = 0x00044440;
    bool trace_calls = false, low_guard = false;
    M68KTaskState task;
    struct { char name[64]; ULONG base, reopen; } m68klibs[16] = {};
    UWORD num_m68klibs = 0;

    /* the fixture's own state */
    ULONG heap = HEAP_BASE, file_pos = 0;
    unsigned heap_calls = 0, fail_call = 0, wild_call = 0, bad_frees = 0, clears_outside = 0;
    /* the DOS side of the file the loader looks for: Open() fails fail_opens
       times with probe_err, then finds native_opens files that are no hunk
       file, then the library; the read can fail on the way */
    unsigned fail_opens = 0, native_opens = 0, opens = 0, quiet_opens = 0, reads = 0;
    LONG probe_err = 0;
    bool seek_fails = false, no_buffer = false, short_read = false;
    std::map<ULONG, Block> live;
    /* the image the next load places: sizes of its blocks (a block that is
       not a hunk of the image stands for something else in the heap), and
       what to write into them once their addresses are known */
    struct Piece { ULONG size; bool hunk; };
    std::vector<Piece> pieces;
    std::function<void(M68KEmuContext &, const std::vector<ULONG> &)> fill;
    std::vector<ULONG> placed;
};

struct Process { APTR pr_WindowPtr = reinterpret_cast<APTR>(0x1234); };
static Process process;
static M68KEmuContext *active;

static void fixture_bug(const char *, ...) {}
#define bug(...) fixture_bug(__VA_ARGS__)
static void m68k_log(M68KEmuContext *, const char *, ...) {}
static void m68k_engine_invalidate(M68KEmuContext *) {}    /* no engine here */
static void m68k_low_write_trap(M68KEmuContext *, ULONG, const char *) { throw Failure{"write into the low memory guard"}; }
static M68KTaskState *m68k_ts(M68KEmuContext *ctx) { return &ctx->task; }
/* the caller's DOS error, as IoErr() shows it */
static LONG dos_err;
static LONG IoErr() { return dos_err; }
static LONG SetIoErr(LONG err) { LONG old = dos_err; dos_err = err; return old; }
static APTR FindTask(APTR) { return &process; }
static BPTR Open(const char *, LONG)
{
    ++active->opens;
    if (process.pr_WindowPtr == reinterpret_cast<APTR>(-1)) ++active->quiet_opens;
    if (active->fail_opens) { --active->fail_opens; dos_err = active->probe_err; return 0; }
    active->file_pos = 0;
    active->reads = 0;
    if (active->native_opens) { --active->native_opens; return 2; }
    return 1;
}
static void Close(BPTR) {}
static LONG Seek(BPTR, LONG, LONG mode)
{
    LONG old = static_cast<LONG>(active->file_pos);
    if (active->seek_fails) { dos_err = 219; return -1; }    /* ERROR_SEEK_ERROR */
    active->file_pos = mode == OFFSET_END ? 4 : 0;
    return old;
}
static LONG Read(BPTR fh, APTR out, LONG size)
{
    const UBYTE hunk[] = {0, 0, 3, 0xF3}, native[] = {0x7F, 'E', 'L', 'F'};
    CHECK(size == 4, "fixture native file read has the expected size");
    if (active->short_read && active->reads++ == 1) { dos_err = 223; return 2; }    /* the whole file comes short */
    std::memcpy(out, fh == 2 ? native : hunk, 4); active->file_pos += 4; return 4;
}
static APTR AllocMem(ULONG size, ULONG)
{
    if (active->no_buffer) { dos_err = 103; return nullptr; }    /* ERROR_NO_FREE_STORE */
    return std::malloc(size);
}
static void FreeMem(APTR ptr, ULONG) { std::free(ptr); }
static STRPTR FilePart(STRPTR name)
{
    const char *p = std::strrchr(name, ':');
    return const_cast<char *>(p ? p + 1 : name);
}

/* A heap that only records: blocks are never reused, so what a block held
   stays readable after it was given back. A request that does not fit, or
   the call a case asks to fail, returns 0 as the real heap does. The call a
   case names as wild answers with an address outside containment, as a heap
   whose lists a program has damaged could; it is no block of this heap. */
static ULONG M68KEmu_HeapAlloc(M68KEmuContext *ctx, ULONG size, ULONG)
{
    ++ctx->heap_calls;
    if (ctx->fail_call && ctx->heap_calls == ctx->fail_call)
        return 0;
    if (ctx->wild_call && ctx->heap_calls == ctx->wild_call)
        return ARENA + 0x100;
    if (size > ARENA - ctx->heap)
        return 0;
    ULONG result = ctx->heap;
    ctx->heap += (size + 7) & ~7u;
    ctx->live[result] = Block{size, false};
    return result;
}
static void M68KEmu_HeapFree(M68KEmuContext *ctx, ULONG addr, ULONG size)
{
    auto it = ctx->live.find(addr);
    if (it == ctx->live.end() || it->second.size != size) { ++ctx->bad_frees; return; }
    ctx->live.erase(it);
}

static void put32(M68KEmuContext &ctx, ULONG a, ULONG v)
{
    ctx.mem[a] = static_cast<UBYTE>(v >> 24); ctx.mem[a + 1] = static_cast<UBYTE>(v >> 16);
    ctx.mem[a + 2] = static_cast<UBYTE>(v >> 8); ctx.mem[a + 3] = static_cast<UBYTE>(v);
}
static void put16(M68KEmuContext &ctx, ULONG a, UWORD v)
{
    ctx.mem[a] = static_cast<UBYTE>(v >> 8); ctx.mem[a + 1] = static_cast<UBYTE>(v);
}
static ULONG get32(M68KEmuContext &ctx, ULONG a)
{
    return ULONG(ctx.mem[a]) << 24 | ULONG(ctx.mem[a + 1]) << 16 | ULONG(ctx.mem[a + 2]) << 8 | ctx.mem[a + 3];
}
static UWORD get16(M68KEmuContext &ctx, ULONG a) { return static_cast<UWORD>(ctx.mem[a] << 8 | ctx.mem[a + 1]); }

/* Places the image of the case the way the real loader does: every hunk a
   heap block of its size plus the two words before its data, the size of
   the block and the link to the next hunk; then the fields that describe
   "the image last loaded", which are also those of the running program. */
static ULONG M68KEmu_LoadHunksFromMemory(M68KEmuContext *ctx, UBYTE *, LONG)
{
    ULONG first = 0, previous = 0, lo = 0xFFFFFFFFu, hi = 0;

    ctx->placed.clear();
    for (const auto &piece : ctx->pieces)
    {
        ULONG block = M68KEmu_HeapAlloc(ctx, piece.size + 8, 0);
        if (!block) throw Failure{"the fixture could not place the image of this case"};
        ULONG data = block + 8;
        ctx->placed.push_back(data);
        if (!piece.hunk) continue;
        ctx->live[block].image = true;
        put32(*ctx, block, piece.size + 8);
        if (previous) put32(*ctx, previous - 4, (block + 4) >> 2);
        if (!first) first = data;
        previous = data;
        lo = std::min(lo, data);
        hi = std::max(hi, data + piece.size);
    }
    if (ctx->fill) ctx->fill(*ctx, ctx->placed);
    ctx->entry_point = ctx->seg_start = first;
    ctx->load_lo = lo; ctx->load_hi = hi;
    return first;
}

/* The loader clears the base of the library with memset(). A clear that
   does not lie in containment is counted and not carried out. */
static void *fixture_memset(void *dst, int value, size_t n)
{
    auto *p = static_cast<UBYTE *>(dst);
    if (p < active->mem || n > ARENA || size_t(p - active->mem) > ARENA - n)
    {
        ++active->clears_outside;
        return dst;
    }
    return std::memset(dst, value, n);
}

#include "accessors.inc"
#define memset fixture_memset
#include "library_loading.inc"
#undef memset

/* ---- library images ------------------------------------------------------ */

/* Layout of the standard library inside one hunk at address a */
static const ULONG RT = 0x10, INIT = 0x40, FUNCS = 0x60, TABLE = 0x80, CODE = 0x100;

/* A complete AUTOINIT library with three functions and no init function */
static void standard(M68KEmuContext &ctx, ULONG a, ULONG dataSize = 64, ULONG dataInit = 0)
{
    put16(ctx, a + RT, M68K_RTC_MATCHWORD);
    put32(ctx, a + RT + M68K_RT_MATCHTAG, a + RT);
    ctx.mem[a + RT + M68K_RT_FLAGS] = M68K_RTF_AUTOINIT;
    ctx.mem[a + RT + M68K_RT_VERSION] = 3;
    ctx.mem[a + RT + M68K_RT_TYPE] = NT_LIBRARY;
    put32(ctx, a + RT + M68K_RT_NAME, a + 0xC0);
    put32(ctx, a + RT + M68K_RT_INIT, a + INIT);
    put32(ctx, a + INIT + M68K_AUTOINIT_DATASIZE, dataSize);
    put32(ctx, a + INIT + M68K_AUTOINIT_FUNCTABLE, a + FUNCS);
    put32(ctx, a + INIT + M68K_AUTOINIT_DATAINIT, dataInit);
    put32(ctx, a + INIT + M68K_AUTOINIT_INITFUNC, 0);
    for (ULONG i = 0; i < 3; ++i) put32(ctx, a + FUNCS + 4 * i, a + CODE + 4 * i);
    put32(ctx, a + FUNCS + 12, M68K_FUNCTABLE_END);
}

static void bytes(M68KEmuContext &ctx, ULONG a, std::initializer_list<UBYTE> v)
{
    for (UBYTE b : v) ctx.mem[a++] = b;
}

/* ---- one case ------------------------------------------------------------- */

struct Outcome
{
    ULONG base = 0;
    std::vector<ULONG> placed;
};

static bool beyond_untouched()
{
    return std::all_of(host.begin() + ARENA, host.end(), [](UBYTE b) { return b == BEYOND_BYTE; });
}

/* Runs the loader once on a fresh context. An allocation made before the
   attempt stands for what the running program owns in the same heap. */
static Outcome load(M68KEmuContext &ctx, std::vector<M68KEmuContext::Piece> pieces,
                    std::function<void(M68KEmuContext &, const std::vector<ULONG> &)> fill,
                    const char *name = "widget.library")
{
    std::fill(host.begin(), host.begin() + ARENA, UBYTE(0));
    std::fill(host.begin() + ARENA, host.end(), BEYOND_BYTE);
    active = &ctx;
    ULONG own = M68KEmu_HeapAlloc(&ctx, 64, 0);
    CHECK(own && ctx.live.size() == 1, "fixture: the program's own block is live");
    ctx.heap_calls = 0;
    ctx.pieces = std::move(pieces);
    ctx.fill = std::move(fill);
    Outcome out;
    out.base = M68KEmu_LoadM68KLibrary(&ctx, name, 0);
    out.placed = ctx.placed;
    CHECK(process.pr_WindowPtr == reinterpret_cast<APTR>(0x1234), "native window policy restored");
    CHECK(ctx.live.count(own) == 1, "the program's own block is still allocated");
    return out;
}

/* What every refusal has to leave behind */
static void refused_cleanly(M68KEmuContext &ctx, const Outcome &out, size_t others = 0)
{
    CHECK(out.base == 0, "the library is refused");
    CHECK(ctx.clears_outside == 0, "no native clear outside containment was attempted");
    CHECK(beyond_untouched(), "nothing outside containment was written");
    CHECK(ctx.bad_frees == 0, "only blocks of this attempt, with their own sizes, were given back");
    CHECK(ctx.live.size() == 1 + others, "every block this attempt took is given back, and no other");
    CHECK(ctx.num_m68klibs == 0, "a refused library is not recorded");
    CHECK(ctx.task.tail_pc == 0, "no code is queued for a refused library");
}

static void program_state_kept(M68KEmuContext &ctx)
{
    CHECK(ctx.entry_point == 0x00011110 && ctx.seg_start == 0x00022220,
          "entry point and segment start are still the running program's");
    CHECK(ctx.load_lo == 0x00033330 && ctx.load_hi == 0x00044440,
          "the load span is still the running program's");
}

static unsigned failed, cases;
static void run(const char *name, const std::function<void()> &body)
{
    ++cases;
    try
    {
        body();
        std::printf("ok   %s\n", name);
    }
    catch (const Failure &failure)
    {
        ++failed;
        std::printf("FAIL %s: %s\n", name, failure.what.c_str());
    }
}

int main()
{
    using Pieces = std::vector<M68KEmuContext::Piece>;
    const Pieces one = {{0x400, true}};

    /* ---- libraries that are in order: behaviour that has to stay ---- */

    run("a well-formed library loads", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0]); });
        ULONG a = out.placed[0], base = out.base;
        CHECK(base != 0 && ctx.num_m68klibs == 1 && ctx.m68klibs[0].base == base, "loaded and recorded once");
        for (ULONG i = 0; i < 3; ++i)
        {
            CHECK(get16(ctx, base - 6 * (i + 1)) == M68K_OP_JMP_ABS_L, "jump table slot holds a jump");
            CHECK(get32(ctx, base - 6 * (i + 1) + 2) == a + CODE + 4 * i, "jump table slot names its function");
        }
        CHECK(ctx.mem[base + 8] == NT_LIBRARY && get16(ctx, base + 16) == 18 && get16(ctx, base + 18) == 64,
              "node type, negative size and positive size of the base");
        CHECK(get16(ctx, base + M68K_LIB_VERSION) == 3 && get32(ctx, base + M68K_LIB_LNNAME) == a + 0xC0,
              "version and name of the base");
        CHECK(ctx.task.tail_pc == ctx.m68klibs[0].reopen && ctx.task.tail_pc != 0, "the Open sequence is queued");
        CHECK(ctx.live.size() == 4 && ctx.bad_frees == 0, "the hunk, the base and the code sequences stay allocated");
        CHECK(ctx.clears_outside == 0 && beyond_untouched(), "nothing outside containment was touched");
    });

    run("a table of relative function offsets", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            put16(c, a[0] + FUNCS, 0xFFFF);
            put16(c, a[0] + FUNCS + 2, 0x00A0); put16(c, a[0] + FUNCS + 4, 0x00A4);
            put16(c, a[0] + FUNCS + 6, 0xFFFF);
        });
        ULONG a = out.placed[0];
        CHECK(out.base != 0 && get16(ctx, out.base + 16) == 12, "two functions");
        CHECK(get32(ctx, out.base - 6 + 2) == a + FUNCS + 0xA0 && get32(ctx, out.base - 12 + 2) == a + FUNCS + 0xA4,
              "offsets count from the table");
    });

    run("the Resident in the second hunk", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, Pieces{{0x80, true}, {0x400, true}},
                           [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[1]); });
        CHECK(out.base != 0 && get32(ctx, out.base - 6 + 2) == out.placed[1] + CODE, "found through the chain of hunks");
        CHECK(ctx.live.size() == 5, "both hunks stay allocated");
    });

    run("init table: copy, repeat, byte offset, 24-bit offset, alignment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, a[0] + TABLE);
            bytes(c, a[0] + TABLE, {
                0x81, 36, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,   /* two LONGs at offset 36 */
                0x10, 0xEE, 0xAB, 0xCD,                                     /* one WORD here; 0xEE is padding */
                0x63, 0x5A,                                                 /* one BYTE four times */
                0xE0, 0x00, 0x00, 60, 0x7E, 0xEE,                           /* one BYTE at 24-bit offset 60 */
                0x00 });
        });
        ULONG base = out.base;
        CHECK(base != 0, "loaded");
        CHECK(get32(ctx, base + 36) == 0x11223344 && get32(ctx, base + 40) == 0x55667788, "two LONGs at the byte offset");
        CHECK(get16(ctx, base + 44) == 0xABCD, "a WORD at the running position, from an aligned source");
        CHECK(get32(ctx, base + 46) == 0x5A5A5A5A && ctx.mem[base + 50] == 0, "a BYTE repeated four times");
        CHECK(ctx.mem[base + 60] == 0x7E && ctx.mem[base + 61] == 0, "a BYTE at the 24-bit offset");
    });

    run("an init table may fill the base to its last byte", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, a[0] + TABLE);
            bytes(c, a[0] + TABLE, { 0xA0, 63, 0x99, 0x00 });               /* one BYTE at offset 63 */
        });
        CHECK(out.base != 0 && ctx.mem[out.base + 63] == 0x99, "the last byte of the base is written");
    });

    run("a function table longer than the limit is cut at 1000, as before", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, Pieces{{0x2000, true}}, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            put32(c, a[0] + INIT + M68K_AUTOINIT_FUNCTABLE, a[0] + 0x200);
            for (ULONG i = 0; i < 1200; ++i) put32(c, a[0] + 0x200 + 4 * i, a[0] + CODE);
            put32(c, a[0] + 0x200 + 4 * 1200, M68K_FUNCTABLE_END);
        });
        CHECK(out.base != 0 && get16(ctx, out.base + 16) == 6000, "1000 functions");
    });

    /* ---- what the file says about sizes and places ---- */

    run("a data size that wraps the allocation", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0], 0xFFFFFFF0u); });
        refused_cleanly(ctx, out);
        CHECK(ctx.heap_calls == 1, "nothing is allocated for a base that cannot exist");
    });

    run("a data size larger than containment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0], ARENA + 0x1000); });
        refused_cleanly(ctx, out);
        CHECK(ctx.heap_calls == 1, "nothing is allocated for a base that cannot exist");
    });

    run("an init table outside containment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0], 64, ARENA + 0x100); });
        refused_cleanly(ctx, out);
    });

    run("an init table that runs out of containment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, ARENA - 2);
            bytes(c, ARENA - 2, { 0x20, 0x11 });                            /* one BYTE here, then the end */
        });
        refused_cleanly(ctx, out);
    });

    run("an init item addressed outside the base, inside containment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, a[0] + TABLE);
            bytes(c, a[0] + TABLE, { 0xE0, 0x00, 0xFF, 0x00, 0x77, 0x00, 0x00 });
        });
        refused_cleanly(ctx, out);
    });

    run("an init item addressed outside containment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, a[0] + TABLE);
            bytes(c, a[0] + TABLE, { 0xE0, 0xFF, 0xFF, 0xFF, 0x77, 0x00, 0x00 });
        });
        refused_cleanly(ctx, out);
    });

    run("an init item that ends one byte past the base", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, a[0] + TABLE);
            bytes(c, a[0] + TABLE, { 0x80, 61, 0x11, 0x22, 0x33, 0x44, 0x00 }); /* one LONG at offset 61 */
        });
        refused_cleanly(ctx, out);
    });

    run("an init command with size code 3", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0], 64, a[0] + TABLE);
            bytes(c, a[0] + TABLE, { 0x30, 0x00, 0x00, 0x00 });
        });
        refused_cleanly(ctx, out);
    });

    run("a table of the init pointers that runs out of containment", [&] {
        M68KEmuContext ctx;
        /* data size and function table lie in containment and are in
           order; the other two longwords of the table would lie after it */
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            put32(c, a[0] + RT + M68K_RT_INIT, ARENA - 8);
            put32(c, ARENA - 8, 64);
            put32(c, ARENA - 4, a[0] + FUNCS);
        });
        refused_cleanly(ctx, out);
    });

    run("a function table that runs out of containment", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            put32(c, a[0] + INIT + M68K_AUTOINIT_FUNCTABLE, ARENA - 6);
            put32(c, ARENA - 6, a[0] + CODE);
        });
        refused_cleanly(ctx, out);
    });

    run("a Resident that is not whole inside its hunk", [&] {
        M68KEmuContext ctx;
        /* the hunk ends 16 bytes into the Resident; what follows in the
           heap belongs to something else and completes a usable library */
        Outcome out = load(ctx, Pieces{{0x40, true}, {0x400, false}},
                           [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            ULONG rt = a[0] + 0x30, other = a[1];
            put16(c, rt, M68K_RTC_MATCHWORD);
            put32(c, rt + M68K_RT_MATCHTAG, rt);
            c.mem[rt + M68K_RT_FLAGS] = M68K_RTF_AUTOINIT;
            c.mem[rt + M68K_RT_VERSION] = 3;
            c.mem[rt + M68K_RT_TYPE] = NT_LIBRARY;
            put32(c, rt + M68K_RT_INIT, other + INIT);
            put32(c, other + INIT + M68K_AUTOINIT_DATASIZE, 64);
            put32(c, other + INIT + M68K_AUTOINIT_FUNCTABLE, other + FUNCS);
            put32(c, other + FUNCS, other + CODE);
            put32(c, other + FUNCS + 4, M68K_FUNCTABLE_END);
        });
        refused_cleanly(ctx, out, 1);
        CHECK(ctx.live.count(out.placed[1] - 8) == 1, "the neighbouring block is not given back");
    });

    run("another library's Resident between two hunks of the image", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, Pieces{{0x80, true}, {0x400, false}, {0x80, true}},
                           [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[1]); });
        refused_cleanly(ctx, out, 1);
        CHECK(ctx.live.count(out.placed[1] - 8) == 1, "the other library's block is not given back");
    });

    /* ---- what a refused attempt leaves in the heap ---- */

    run("no Resident: the hunks are given back", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, Pieces{{0x80, true}, {0x80, true}}, nullptr);
        refused_cleanly(ctx, out);
    });

    run("a Resident without AUTOINIT: the hunks are given back", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            c.mem[a[0] + RT + M68K_RT_FLAGS] = 0;
        });
        refused_cleanly(ctx, out);
    });

    run("no function table: the hunks are given back", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            put32(c, a[0] + INIT + M68K_AUTOINIT_FUNCTABLE, 0);
        });
        refused_cleanly(ctx, out);
    });

    run("an empty function table: the hunks are given back", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) {
            standard(c, a[0]);
            put32(c, a[0] + FUNCS, M68K_FUNCTABLE_END);
        });
        refused_cleanly(ctx, out);
    });

    run("no memory for the base: the hunks are given back", [&] {
        M68KEmuContext ctx;
        ctx.fail_call = 2;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0]); });
        refused_cleanly(ctx, out);
    });

    run("no memory for the code sequences: the base and the hunks are given back", [&] {
        M68KEmuContext ctx;
        ctx.fail_call = 3;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0]); });
        refused_cleanly(ctx, out);
    });

    run("a heap that answers with a base outside containment", [&] {
        M68KEmuContext ctx;
        ctx.wild_call = 2;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0]); });
        refused_cleanly(ctx, out);
    });

    /* ---- the running program ---- */

    run("the program's fields after a library was loaded", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0]); });
        CHECK(out.base != 0, "loaded");
        program_state_kept(ctx);
    });

    run("the program's fields after a library was refused", [&] {
        M68KEmuContext ctx;
        Outcome out = load(ctx, one, nullptr);
        CHECK(out.base == 0, "refused");
        program_state_kept(ctx);
    });

    /* ---- the caller's DOS error: the file search and read are the loader's own ---- */

    /* 7777 stands for an error the program had before it asked for the
       library: no probe of the fixture produces it */
    run("a missing library keeps the caller's zero (the observed version.library case)", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 4; ctx.probe_err = 205;
        dos_err = 0;
        Outcome out = load(ctx, one, nullptr, "version.library");
        refused_cleanly(ctx, out);
        CHECK(ctx.opens == 4 && ctx.quiet_opens == 4, "four places probed, none with a requester");
        CHECK(dos_err == 0, "IoErr() is still 0, not the 205 of the probes");
    });

    run("a missing library keeps the caller's own error", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 4; ctx.probe_err = 205;
        dos_err = 7777;
        Outcome out = load(ctx, one, nullptr, "version.library");
        refused_cleanly(ctx, out);
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777");
    });

    run("a path name that is missing keeps the caller's error", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 5; ctx.probe_err = 205;
        dos_err = 7777;
        Outcome out = load(ctx, one, nullptr, "PROGDIR:libs/version.library");
        refused_cleanly(ctx, out);
        CHECK(ctx.opens == 5 && ctx.quiet_opens == 5, "the path and four places probed, none with a requester");
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777");
    });

    run("found after failed probes, the caller's error is kept", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 2; ctx.probe_err = 205;
        dos_err = 7777;
        Outcome out = load(ctx, one, [](M68KEmuContext &c, const std::vector<ULONG> &a) { standard(c, a[0]); });
        CHECK(out.base != 0 && ctx.num_m68klibs == 1, "loaded from the third place");
        CHECK(ctx.opens == 3 && ctx.quiet_opens == 3, "three probes, none with a requester");
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777");
    });

    run("a native library in LIBS: is passed over, the caller's error is kept", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 3; ctx.probe_err = 205; ctx.native_opens = 1;
        dos_err = 7777;
        Outcome out = load(ctx, one, nullptr, "version.library");
        refused_cleanly(ctx, out);
        CHECK(ctx.opens == 4, "four places probed");
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777");
    });

    run("a file that cannot be sized keeps the caller's error", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 1; ctx.probe_err = 205; ctx.seek_fails = true;
        dos_err = 7777;
        Outcome out = load(ctx, one, nullptr);
        refused_cleanly(ctx, out);
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777, not 219");
    });

    run("no buffer for the file keeps the caller's error", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 1; ctx.probe_err = 205; ctx.no_buffer = true;
        dos_err = 7777;
        Outcome out = load(ctx, one, nullptr);
        refused_cleanly(ctx, out);
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777, not 103");
    });

    run("a short read keeps the caller's error", [&] {
        M68KEmuContext ctx;
        ctx.fail_opens = 1; ctx.probe_err = 205; ctx.short_read = true;
        dos_err = 7777;
        Outcome out = load(ctx, one, nullptr);
        refused_cleanly(ctx, out);
        CHECK(dos_err == 7777, "IoErr() is still the caller's 7777, not 223");
    });

    dos_err = 0;

    std::printf("m68kemu 68k library loading bounds: %u cases, %u failed, %u checks\n", cases, failed, checks);
    return failed ? 1 : 0;
}
