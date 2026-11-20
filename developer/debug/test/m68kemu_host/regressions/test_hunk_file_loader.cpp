/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The production loader of hunk files, M68KEmu_LoadHunksFromMemory(), on
    well-formed and on malformed files. The heap is a recording fixture; no
    CPU runs. This is not an AROS image test.
*/
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "hunk_constants.inc"

using UBYTE = uint8_t;
using UWORD = uint16_t;
using WORD = int16_t;
using ULONG = uint32_t;
using LONG = int32_t;
using APTR = void *;
using BOOL = int;
#define TRUE 1
#define FALSE 0
#define MEMF_CHIP (1u << 1)
#define MEMF_FAST (1u << 2)
#define MEMF_CLEAR (1u << 16)

/* Containment is the first ARENA bytes of one host buffer, filled with one
   value; the bytes after it stand for native memory that is not the
   program's. Whatever the loader writes outside the blocks the heap handed
   out, inside containment or after it, changes a byte the fixture checks. */
static const ULONG ARENA = 0x40000;
static const size_t BEYOND = 0x100000;
static const UBYTE FILL = 0xCC, BEYOND_BYTE = 0xA5;
static const ULONG HEAP_BASE = 0x1000;
static std::vector<UBYTE> host(ARENA + BEYOND);

struct Failure { std::string what; };
static unsigned checks;
#define CHECK(c, label) do { ++checks; if (!(c)) throw Failure{label}; } while (0)

struct M68KEmuContext
{
    UBYTE *mem = host.data();
    ULONG mem_size = ARENA;
    ULONG entry_point = 0x00011110, seg_start = 0x00022220;
    ULONG load_lo = 0x00033330, load_hi = 0x00044440;
    bool trace_calls = false, low_guard = false;

    /* the fixture's own state */
    ULONG heap = HEAP_BASE;
    unsigned heap_calls = 0, wild_call = 0, bad_frees = 0, clears_outside = 0, copies_outside = 0;
    std::map<ULONG, ULONG> live;                    /* address and size of every block in use */
    std::vector<std::pair<ULONG, ULONG>> handed;    /* every block ever handed out */
};
static M68KEmuContext *active;

static void m68k_log(M68KEmuContext *, const char *, ...) {}
static void m68k_low_write_trap(M68KEmuContext *, ULONG, const char *) { throw Failure{"write into the low memory guard"}; }
static APTR AllocMem(size_t size, ULONG) { return std::calloc(1, size); }
static void FreeMem(APTR ptr, size_t) { std::free(ptr); }

/* A heap that only records. Blocks are never reused. A request that does
   not fit returns 0, as the real heap does. The call a case names as wild
   answers with an address outside containment, as a heap whose lists a
   program has damaged could; that is no block of this heap. */
static ULONG M68KEmu_HeapAlloc(M68KEmuContext *ctx, ULONG size, ULONG)
{
    ++ctx->heap_calls;
    if (ctx->wild_call && ctx->heap_calls == ctx->wild_call)
        return ARENA + 0x100;
    if (size > ARENA - ctx->heap)
        return 0;
    ULONG result = ctx->heap;
    ctx->heap += (size + 7) & ~7u;
    ctx->live[result] = size;
    ctx->handed.emplace_back(result, size);
    return result;
}
static void M68KEmu_HeapFree(M68KEmuContext *ctx, ULONG addr, ULONG size)
{
    auto it = ctx->live.find(addr);
    if (it == ctx->live.end() || it->second != size) { ++ctx->bad_frees; return; }
    ctx->live.erase(it);
}

/* The loader clears and fills hunks with memset() and memcpy(). One that
   does not lie in containment is counted and not carried out. */
static bool in_arena(const void *dst, size_t n)
{
    auto *p = static_cast<const UBYTE *>(dst);
    return p && p >= active->mem && n <= ARENA && size_t(p - active->mem) <= ARENA - n;
}
static void *fixture_memset(void *dst, int value, size_t n)
{
    if (!in_arena(dst, n)) { ++active->clears_outside; return dst; }
    return std::memset(dst, value, n);
}
static void *fixture_memcpy(void *dst, const void *src, size_t n)
{
    if (!in_arena(dst, n)) { ++active->copies_outside; return dst; }
    return std::memcpy(dst, src, n);
}

#include "accessors.inc"
#define memset fixture_memset
#define memcpy fixture_memcpy
#include "hunk_file_loader.inc"
#undef memcpy
#undef memset

/* ---- files ----------------------------------------------------------------- */

struct File
{
    std::vector<UBYTE> bytes;
    File &l(ULONG v)
    {
        bytes.push_back(static_cast<UBYTE>(v >> 24)); bytes.push_back(static_cast<UBYTE>(v >> 16));
        bytes.push_back(static_cast<UBYTE>(v >> 8)); bytes.push_back(static_cast<UBYTE>(v));
        return *this;
    }
    File &ls(std::initializer_list<ULONG> v) { for (ULONG x : v) l(x); return *this; }
    File &ws(std::initializer_list<UWORD> v)
    {
        for (UWORD x : v) { bytes.push_back(static_cast<UBYTE>(x >> 8)); bytes.push_back(static_cast<UBYTE>(x)); }
        return *this;
    }
};

/* HUNK_HEADER without resident names: table size, first, last, size words */
static File header(ULONG table, ULONG first, ULONG last, std::initializer_list<ULONG> sizes)
{
    File f;
    f.ls({HUNK_HEADER, 0, table, first, last}).ls(sizes);
    return f;
}

static ULONG get32(M68KEmuContext &ctx, ULONG a)
{
    return ULONG(ctx.mem[a]) << 24 | ULONG(ctx.mem[a + 1]) << 16 | ULONG(ctx.mem[a + 2]) << 8 | ctx.mem[a + 3];
}

/* ---- one case -------------------------------------------------------------- */

static ULONG load(M68KEmuContext &ctx, File &file)
{
    std::fill(host.begin(), host.begin() + ARENA, FILL);
    std::fill(host.begin() + ARENA, host.end(), BEYOND_BYTE);
    active = &ctx;
    return M68KEmu_LoadHunksFromMemory(&ctx, file.bytes.data(), static_cast<LONG>(file.bytes.size()));
}

/* Data address of the n-th block the heap handed out */
static ULONG data(M68KEmuContext &ctx, size_t n)
{
    CHECK(n < ctx.handed.size(), "the hunk was placed");
    return ctx.handed[n].first + 8;
}

/* No byte outside the blocks the heap handed out has changed */
static bool only_blocks_written(M68KEmuContext &ctx)
{
    std::vector<bool> mine(ARENA, false);
    for (const auto &block : ctx.handed)
        for (ULONG i = 0; i < block.second && block.first + i < ARENA; ++i)
            mine[block.first + i] = true;
    for (ULONG i = 0; i < ARENA; ++i)
        if (!mine[i] && host[i] != FILL) return false;
    return std::all_of(host.begin() + ARENA, host.end(), [](UBYTE b) { return b == BEYOND_BYTE; });
}

static void nothing_outside(M68KEmuContext &ctx)
{
    CHECK(ctx.clears_outside == 0 && ctx.copies_outside == 0, "no clear or copy outside containment was attempted");
    CHECK(only_blocks_written(ctx), "nothing outside the blocks of the heap was written");
    CHECK(ctx.bad_frees == 0, "only blocks of this load, with their own sizes, were given back");
}

static void program_state_kept(M68KEmuContext &ctx)
{
    CHECK(ctx.entry_point == 0x00011110 && ctx.seg_start == 0x00022220 &&
          ctx.load_lo == 0x00033330 && ctx.load_hi == 0x00044440,
          "the fields of the context are left as they were");
}

/* What every refusal has to leave behind */
static void refused_cleanly(M68KEmuContext &ctx, ULONG entry)
{
    CHECK(entry == 0, "the file is refused");
    nothing_outside(ctx);
    CHECK(ctx.live.empty(), "every block of this load is given back");
    program_state_kept(ctx);
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
    /* ---- files that are in order: behaviour that has to stay ---- */

    run("one code hunk", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0x11223344, 0x55667788, HUNK_END});
        ULONG entry = load(ctx, f), d0 = data(ctx, 0);
        CHECK(entry == d0 && ctx.entry_point == entry && ctx.seg_start == entry, "the entry is the data of the first hunk");
        CHECK(get32(ctx, d0) == 0x11223344 && get32(ctx, d0 + 4) == 0x55667788, "the block is copied");
        CHECK(get32(ctx, d0 - 8) == 16 && get32(ctx, d0 - 4) == 0, "size and link words before the data");
        CHECK(ctx.load_lo == d0 && ctx.load_hi == d0 + 8, "the load span is the hunk");
        CHECK(ctx.live.size() == 1 && ctx.heap_calls == 1, "one block");
        nothing_outside(ctx);
    });

    run("two hunks relocated against each other", [] {
        M68KEmuContext ctx;
        File f = header(2, 0, 1, {2, 2});
        f.ls({HUNK_CODE, 2, 0x00000004, 0, HUNK_RELOC32, 1, 1, 0, 0, HUNK_END});
        f.ls({HUNK_DATA, 2, 0, 0x00000004, HUNK_RELOC32, 1, 0, 4, 0, HUNK_END});
        ULONG entry = load(ctx, f), d0 = data(ctx, 0), d1 = data(ctx, 1);
        CHECK(entry == d0, "loaded");
        CHECK(get32(ctx, d0) == 4 + d1 && get32(ctx, d1 + 4) == 4 + d0, "each longword gets the address of its target hunk");
        CHECK(get32(ctx, d0 - 4) == (d1 - 4) >> 2 && get32(ctx, d1 - 4) == 0, "the hunks are chained");
        nothing_outside(ctx);
    });

    run("a relocation on the last longword of its hunk", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0x10, HUNK_RELOC32, 1, 0, 4, 0, HUNK_END});
        ULONG entry = load(ctx, f), d0 = data(ctx, 0);
        CHECK(entry == d0 && get32(ctx, d0 + 4) == 0x10 + d0, "relocated");
        nothing_outside(ctx);
    });

    run("the 16-bit relocation forms, with their padding", [] {
        M68KEmuContext ctx;
        File f = header(2, 0, 1, {2, 2});
        f.ls({HUNK_CODE, 2, 0, 4, HUNK_RELOC32SHORT}).ws({2, 1, 0, 4, 0, 0}).l(HUNK_END);
        f.ls({HUNK_DATA, 2, 0, 0, HUNK_DREL32}).ws({1, 0, 0, 0});
        f.l(HUNK_RELRELOC32).ws({1, 0, 4, 0}).l(HUNK_END);
        ULONG entry = load(ctx, f), d0 = data(ctx, 0), d1 = data(ctx, 1);
        CHECK(entry == d0, "loaded");
        CHECK(get32(ctx, d0) == d1 && get32(ctx, d0 + 4) == 4 + d1, "two short relocations to the second hunk");
        CHECK(get32(ctx, d1) == d0, "a data relocation to the first hunk");
        CHECK(get32(ctx, d1 + 4) == d0 - d1, "a relative relocation adds the distance to its target");
        nothing_outside(ctx);
    });

    run("a hunk without contents, memory attributes, symbols and debug data", [] {
        M68KEmuContext ctx;
        File f = header(3, 0, 2, {0x40000001, 3, 0xC0000002, MEMF_CHIP});
        f.ls({HUNK_CODE, 1, 0xAABBCCDD, HUNK_SYMBOL, 1, 0x666F6F00, 0x1234, 0, HUNK_DEBUG, 2, 0, 0, HUNK_END});
        f.ls({HUNK_BSS, 3, HUNK_END, HUNK_DATA, 2, 7, 9, HUNK_END});
        ULONG entry = load(ctx, f), d0 = data(ctx, 0), d1 = data(ctx, 1), d2 = data(ctx, 2);
        CHECK(entry == d0 && get32(ctx, d0) == 0xAABBCCDD, "the code hunk");
        CHECK(get32(ctx, d1 - 8) == 20 && get32(ctx, d1) == 0 && get32(ctx, d1 + 8) == 0, "the empty hunk is cleared");
        CHECK(get32(ctx, d2 - 8) == 16 && get32(ctx, d2) == 7 && get32(ctx, d2 + 4) == 9, "the hunk with an attribute longword");
        nothing_outside(ctx);
    });

    run("fewer blocks than the header announces are accepted (not changed here)", [] {
        M68KEmuContext ctx;
        File f = header(2, 0, 1, {1, 1});
        f.ls({HUNK_CODE, 1, 0x4E754E75, HUNK_END});
        ULONG entry = load(ctx, f);
        CHECK(entry == data(ctx, 0) && ctx.live.size() == 2, "loaded, the second hunk stays empty");
        nothing_outside(ctx);
    });

    run("a file that ends after a whole relocation block, without HUNK_END, is accepted (not changed here)", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 0, 0, 0});
        ULONG entry = load(ctx, f), d0 = data(ctx, 0);
        CHECK(entry == d0 && get32(ctx, d0) == d0, "loaded and relocated");
        nothing_outside(ctx);
    });

    run("a BSS block shorter than its hunk", [] {
        M68KEmuContext ctx;
        File f = header(2, 0, 1, {1, 3});
        f.ls({HUNK_CODE, 1, 0x4E754E75, HUNK_END, HUNK_BSS, 1, HUNK_END});
        ULONG entry = load(ctx, f), d1 = data(ctx, 1);
        CHECK(entry == data(ctx, 0) && get32(ctx, d1 - 8) == 20, "loaded, the hunk has the size of the header");
        CHECK(get32(ctx, d1) == 0 && get32(ctx, d1 + 4) == 0 && get32(ctx, d1 + 8) == 0, "the whole hunk is cleared");
        nothing_outside(ctx);
    });

    /* ---- refused by the old loader as well: has to stay refused, cleanly ---- */

    run("a hunk larger than containment", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {ARENA / 4 + 1});
        f.ls({HUNK_CODE, 1, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a relocation whose target is no hunk of the file", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 5, 0, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("an unsupported block", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_OVERLAY, 0});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a file cut inside a code block", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {4});
        f.ls({HUNK_CODE, 4, 1, 2});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a resident name longer than the file", [] {
        M68KEmuContext ctx;
        File f;
        f.ls({HUNK_HEADER, 0x10000, 0x41424344, 0, 1, 0, 0, 1});
        refused_cleanly(ctx, load(ctx, f));
        CHECK(ctx.heap_calls == 0, "nothing was allocated");
    });

    /* ---- sizes and places the file names ---- */

    run("a hunk size that wraps: 0x3FFFFFFF longwords", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {0x3FFFFFFF});
        f.ls({HUNK_CODE, 4, 1, 2, 3, 4, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
        CHECK(ctx.heap_calls == 0, "nothing is allocated for a hunk that cannot exist");
    });

    run("a hunk size that wraps: 0x3FFFFFFE longwords", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {0x3FFFFFFE});
        f.ls({HUNK_CODE, 4, 1, 2, 3, 4, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
        CHECK(ctx.heap_calls == 0, "nothing is allocated for a hunk that cannot exist");
    });

    run("a relocation that ends past its hunk", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 0, 6, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a relocation outside its hunk", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 0, 0x100, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a 16-bit relocation that ends past its hunk", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32SHORT}).ws({1, 0, 6, 0}).l(HUNK_END);
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a relocation block after the last hunk has ended", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_END, HUNK_RELOC32, 1, 0, 0, 0});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a debug block whose length wraps", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {1});
        f.ls({HUNK_CODE, 1, 0x4E754E75, HUNK_DEBUG, 0x40000001, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a heap that answers with a hunk outside containment", [] {
        M68KEmuContext ctx;
        ctx.wild_call = 1;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 1, 2, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    /* A relocation block ends with a zero count. A file that ends where the
       next count belongs is cut inside the block; one that ends after the
       zero count is the accepted case further up. */
    run("a relocation block cut where its next count belongs", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 0, 0});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a file that ends right after the word HUNK_RELOC32", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a BSS block longer than the size the header gives its hunk", [] {
        M68KEmuContext ctx;
        File f = header(2, 0, 1, {2, 1});
        f.ls({HUNK_CODE, 2, 1, 2, HUNK_END, HUNK_BSS, 2, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a BSS block after the last hunk has ended", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 1, 2, HUNK_END, HUNK_BSS, 1, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    /* Accepted, as before. The block is the last 8 bytes of containment, so
       the hunk of no bytes starts where containment ends: there is nothing
       to copy and no address to copy to. */
    run("a hunk without contents that the heap places at the very end of containment", [] {
        M68KEmuContext ctx;
        ctx.heap = ARENA - 8;
        File f = header(1, 0, 0, {0});
        f.ls({HUNK_CODE, 0, HUNK_END});
        ULONG entry = load(ctx, f);
        CHECK(entry == ARENA && ctx.live.size() == 1, "accepted: one block, the hunk has no byte");
        CHECK(get32(ctx, ARENA - 8) == 8 && get32(ctx, ARENA - 4) == 0, "size and link words in the block");
        nothing_outside(ctx);
    });

    /* ---- the hunks the header names ---- */

    run("a header that loads one hunk of a table of two", [] {
        M68KEmuContext ctx;
        File f = header(2, 0, 0, {2});
        f.ls({HUNK_CODE, 2, 1, 2, HUNK_END});
        ULONG entry = load(ctx, f);
        CHECK(entry != 0 && entry == data(ctx, 0), "loaded");
        CHECK(get32(ctx, entry) == 1 && get32(ctx, entry + 4) == 2 && ctx.live.size() == 1, "one hunk, with its contents");
        nothing_outside(ctx);
    });

    run("a header whose hunks are numbered from 1", [] {
        M68KEmuContext ctx;
        File f = header(3, 1, 2, {2, 2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 2, 0, 0, HUNK_END});
        f.ls({HUNK_DATA, 2, 0, 0, HUNK_RELOC32, 1, 1, 4, 0, HUNK_END});
        ULONG entry = load(ctx, f);
        CHECK(entry != 0 && ctx.live.size() == 2, "two hunks loaded");
        ULONG d0 = data(ctx, 0), d1 = data(ctx, 1);
        CHECK(entry == d0 && get32(ctx, d0) == d1 && get32(ctx, d1 + 4) == d0, "relocation targets are numbers of the table");
        nothing_outside(ctx);
    });

    run("a header whose hunks are numbered from 1, relocated with the 16-bit forms", [] {
        M68KEmuContext ctx;
        File f = header(3, 1, 2, {2, 2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32SHORT}).ws({1, 2, 0, 0}).l(HUNK_END);
        f.ls({HUNK_DATA, 2, 0, 0, HUNK_DREL32}).ws({1, 1, 4, 0});
        f.l(HUNK_RELRELOC32).ws({1, 1, 0, 0}).l(HUNK_END);
        ULONG entry = load(ctx, f);
        CHECK(entry != 0 && ctx.live.size() == 2, "two hunks loaded");
        ULONG d0 = data(ctx, 0), d1 = data(ctx, 1);
        CHECK(entry == d0 && get32(ctx, d0) == d1, "a short relocation to hunk 2 of the table, the second one loaded");
        CHECK(get32(ctx, d1 + 4) == d0, "a data relocation to hunk 1 of the table, the first one loaded");
        CHECK(get32(ctx, d1) == d0 - d1, "a relative relocation adds the distance to hunk 1 of the table");
        nothing_outside(ctx);
    });

    run("a header whose first hunk is above its last", [] {
        M68KEmuContext ctx;
        File f = header(1, 1, 0, {2});
        f.ls({HUNK_CODE, 2, 1, 2, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
        CHECK(ctx.heap_calls == 0, "nothing was allocated");
    });

    run("a header whose last hunk is outside its table", [] {
        M68KEmuContext ctx;
        File f = header(1, 0, 1, {2});
        f.ls({HUNK_CODE, 2, 1, 2, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
        CHECK(ctx.heap_calls == 0, "nothing was allocated");
    });

    /* ---- a target below the first hunk the file loads ----
       The old loader refuses these two files as well, for another reason:
       it ignores the range, takes the first word of the first block for a
       third size word and then meets a longword that is no block. So they
       pass on it too, and do not show that it lacks this refusal; nothing
       can, since it does not read a range at all. */

    run("a relocation to a hunk number below the first one loaded", [] {
        M68KEmuContext ctx;
        File f = header(3, 1, 2, {2, 2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32, 1, 0, 0, 0, HUNK_END});
        f.ls({HUNK_DATA, 2, 0, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    run("a 16-bit relocation to a hunk number below the first one loaded", [] {
        M68KEmuContext ctx;
        File f = header(3, 1, 2, {2, 2});
        f.ls({HUNK_CODE, 2, 0, 0, HUNK_RELOC32SHORT}).ws({1, 0, 0, 0}).l(HUNK_END);
        f.ls({HUNK_DATA, 2, 0, 0, HUNK_END});
        refused_cleanly(ctx, load(ctx, f));
    });

    std::printf("m68kemu hunk file loader: %u cases, %u failed, %u checks\n", cases, failed, checks);
    return failed ? 1 : 0;
}
