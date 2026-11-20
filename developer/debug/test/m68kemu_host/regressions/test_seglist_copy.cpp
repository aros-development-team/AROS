/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The production copy of a LoadSeg() seglist into containment,
    M68KEmu_LoadHunks(), on seglists that are in order and on ones that are
    not. The relocation list of dos is answered by a double of
    GetSegListInfo(); its types, its kinds and the tag that asks for it are
    those of the headers of the tree. The heap is a recording fixture; no
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
#include <memory>
#include <string>
#include <utility>
#include <vector>

using UBYTE = uint8_t;
using UWORD = uint16_t;
using ULONG = uint32_t;
using LONG = int32_t;
using IPTR = uintptr_t;
using APTR = void *;
using BOOL = int;
/* a BPTR is a plain pointer on every target but m68k (AROS_FAST_BPTR) */
using BPTR = void *;
#define BADDR(a) ((APTR)(a))
#define TRUE 1
#define FALSE 0
#define MEMF_CLEAR (1u << 16)
/* the host of this test is little endian */
#define AROS_BE2LONG(x) __builtin_bswap32(x)
#define AROS_LONG2BE(x) __builtin_bswap32(x)

/* Tag, struct TagItem, TAG_DONE, TAG_USER, GSLI_68KRELOCS, struct
   HunkRelocGroup, HUNKRELOC_* and struct HunkRelocTable, as the tree has
   them */
#include "seglist_record.inc"

/* Containment is the first ARENA bytes of one host buffer, filled with one
   value; the bytes after it stand for native memory that is not the
   program's. Whatever the copy writes outside the blocks the heap handed
   out, inside containment or after it, changes a byte the fixture checks. */
static const ULONG ARENA = 0x40000;
static const size_t BEYOND = 0x100000;
static const UBYTE FILL = 0xCC, BEYOND_BYTE = 0xA5;
static const ULONG HEAP_BASE = 0x1000;
static std::vector<UBYTE> buffer(ARENA + BEYOND);

struct Failure { std::string what; };
static unsigned checks;
#define CHECK(c, label) do { ++checks; if (!(c)) throw Failure{label}; } while (0)

struct M68KEmuContext
{
    UBYTE *mem = buffer.data();
    ULONG mem_size = ARENA;
    ULONG entry_point = 0x00011110, seg_start = 0x00022220;
    bool low_guard = false;

    /* the fixture's own state */
    ULONG heap = HEAP_BASE;
    unsigned heap_calls = 0, fail_call = 0, wild_call = 0, bad_frees = 0, copies_outside = 0, messages = 0;
    std::map<ULONG, ULONG> live;                    /* address and size of every block in use */
    std::vector<std::pair<ULONG, ULONG>> handed;    /* every block ever handed out */
};
static M68KEmuContext *active;
static int tables_live;         /* AllocVec() not yet followed by its FreeVec() */
static unsigned info_calls;     /* calls of GetSegListInfo() */

static void bug(const char *, ...) { if (active) ++active->messages; }
static void m68k_low_write_trap(M68KEmuContext *, ULONG, const char *) { throw Failure{"write into the low memory guard"}; }

/* AllocVec() gives a pointer that C assigns to any pointer; the production
   line has no cast. */
struct AnyPointer
{
    void *p;
    template <typename T> operator T *() const { return static_cast<T *>(p); }
};
static AnyPointer AllocVec(size_t size, ULONG) { ++tables_live; return AnyPointer{std::calloc(1, size ? size : 1)}; }
static void FreeVec(APTR ptr) { if (ptr) { --tables_live; std::free(ptr); } }

/* A heap that only records. Blocks are never reused; MEMF_CLEAR clears the
   block, as the real heap does. A request that does not fit returns 0. The
   call a case names as failing returns 0; the call it names as wild answers
   with an address outside containment, as a heap whose lists a program has
   damaged could; that is no block of this heap. */
static ULONG M68KEmu_HeapAlloc(M68KEmuContext *ctx, ULONG size, ULONG flags)
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
    ctx->live[result] = size;
    ctx->handed.emplace_back(result, size);
    if (flags & MEMF_CLEAR)
        std::memset(buffer.data() + result, 0, size);
    return result;
}
static void M68KEmu_HeapFree(M68KEmuContext *ctx, ULONG addr, ULONG size)
{
    auto it = ctx->live.find(addr);
    if (it == ctx->live.end() || it->second != size) { ++ctx->bad_frees; return; }
    ctx->live.erase(it);
}

/* exec's CopyMem(source, dest, size). A copy that does not lie in
   containment is counted and not carried out. */
static bool in_arena(const void *dst, size_t n)
{
    auto *p = static_cast<const UBYTE *>(dst);
    return p && p >= active->mem && n <= ARENA && size_t(p - active->mem) <= ARENA - n;
}
static void CopyMem(const void *src, void *dst, size_t n)
{
    if (!in_arena(dst, n)) { ++active->copies_outside; return; }
    std::memcpy(dst, src, n);
}

/* ---- what dos knows about a seglist ---------------------------------------- */

/* Whether dos has a node for the seglist, whether that node answers the
   relocation list, and the list itself. */
struct Record
{
    bool node = true, answers = true;
    HunkRelocTable table{};
    std::vector<HunkRelocGroup> groups;
    std::vector<std::vector<ULONG>> offsets;
    void seal()
    {
        for (size_t i = 0; i < groups.size(); ++i)
        {
            groups[i].hrg_Count = static_cast<ULONG>(offsets[i].size());
            groups[i].hrg_Offsets = offsets[i].data();
        }
        table.hrt_Groups = static_cast<ULONG>(groups.size());
        table.hrt_Group = groups.data();
    }
};
static std::map<BPTR, Record *> registry;

/* The double of dos.library's GetSegListInfo(): the number of tags it
   answered, 0 for a seglist it has no node for. A tag list is never
   followed past eight items. */
static ULONG GetSegListInfo(BPTR seglist, const TagItem *tags)
{
    ++info_calls;
    if (!seglist || !tags)
        return 0;
    auto it = registry.find(seglist);
    if (it == registry.end() || !it->second->node)
        return 0;
    ULONG count = 0;
    for (int i = 0; i < 8 && tags[i].ti_Tag != TAG_DONE; ++i)
    {
        if (tags[i].ti_Tag == GSLI_68KRELOCS && tags[i].ti_Data && it->second->answers)
        {
            *reinterpret_cast<HunkRelocTable **>(tags[i].ti_Data) = &it->second->table;
            ++count;
        }
    }
    return count;
}

#include "accessors.inc"
#include "seglist_copy.inc"
#ifndef SEGLIST_COPY_CAN_BE_GIVEN_BACK
/* a tree from before a copy could be given back: nothing is released */
static void M68KEmu_FreeHunks(M68KEmuContext *, ULONG) {}
#endif

/* ---- seglists -------------------------------------------------------------- */

/* A seglist as LoadSeg() leaves it: one allocation per segment that begins
   with a ULONG, the size of the allocation, then the BPTR of the next
   segment, then the data; the BPTR of a segment is the address of that
   link. dos puts the link four bytes after an eight-byte boundary. Here it
   lies on one, so that reading it through a BPTR pointer, as the copy and
   dos itself do, is an aligned read for the sanitizer. Nothing else of the
   layout differs. */
struct Segment
{
    std::unique_ptr<uint64_t[]> store;
    UBYTE *link = nullptr;
    ULONG bytes = 0;
    UBYTE *data() const { return link + sizeof(BPTR); }
};

struct SegList
{
    std::vector<Segment> segs;
    void add(ULONG n)
    {
        Segment s;
        s.store = std::make_unique<uint64_t[]>((n + 16 + 7) / 8 + 1);
        UBYTE *base = reinterpret_cast<UBYTE *>(s.store.get());
        ULONG total = n + static_cast<ULONG>(sizeof(ULONG) + sizeof(BPTR));
        s.link = base + 8;
        s.bytes = n;
        std::memcpy(base + 4, &total, sizeof total);
        if (!segs.empty())
        {
            BPTR next = s.link;
            std::memcpy(segs.back().link, &next, sizeof next);
        }
        segs.push_back(std::move(s));
    }
    BPTR bptr() const { return segs.empty() ? nullptr : segs[0].link; }
    /* the low 32 bits of the host address of a segment's data, which is
       what dos added to the relocated longwords */
    ULONG base32(size_t i) const { return static_cast<ULONG>(reinterpret_cast<IPTR>(segs[i].data())); }
    void put32(size_t i, ULONG off, ULONG v) const
    {
        UBYTE *p = segs[i].data() + off;
        p[0] = static_cast<UBYTE>(v >> 24); p[1] = static_cast<UBYTE>(v >> 16);
        p[2] = static_cast<UBYTE>(v >> 8); p[3] = static_cast<UBYTE>(v);
    }
    void size_word(size_t i, ULONG v) const { std::memcpy(segs[i].link - 4, &v, sizeof v); }
};

struct Many { size_t count; ULONG each; };
struct Reloc { ULONG hunk, target, kind, off, original; };

/* A seglist, what dos knows about it, and what each relocated longword held
   before dos relocated it. */
struct Program
{
    SegList segs;
    Record record;
    std::vector<Reloc> relocs;

    explicit Program(std::initializer_list<ULONG> sizes) { for (ULONG n : sizes) segs.add(n); attach(); }
    explicit Program(Many m) { for (size_t i = 0; i < m.count; ++i) segs.add(m.each); attach(); }
    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;
    ~Program() { registry.erase(segs.bptr()); }

    /* One group of the list. Each longword is given as its offset and the
       value the file had there; it is relocated here for the host
       addresses as dos/doshunks.h says each kind is. */
    Program &group(ULONG hunk, ULONG target, ULONG kind, std::initializer_list<std::pair<ULONG, ULONG>> longwords)
    {
        HunkRelocGroup g{};
        g.hrg_Hunk = hunk;
        g.hrg_Target = target;
        g.hrg_Kind = kind;
        record.groups.push_back(g);
        record.offsets.emplace_back();
        for (auto [off, original] : longwords)
        {
            record.offsets.back().push_back(off);
            if (hunk < segs.segs.size() && target < segs.segs.size() && off + 4 <= segs.segs[hunk].bytes)
            {
                ULONG v = segs.base32(target) + original;
                if (kind == HUNKRELOC_REL)
                    v -= segs.base32(hunk) + off;
                else if (kind == HUNKRELOC_BASE)
                    v -= segs.base32(hunk);
                segs.put32(hunk, off, v);
                relocs.push_back({hunk, target, kind, off, original});
            }
        }
        record.seal();
        return *this;
    }

private:
    void attach() { record.seal(); registry[segs.bptr()] = &record; }
};

static ULONG get32(M68KEmuContext &ctx, ULONG a)
{
    return ULONG(ctx.mem[a]) << 24 | ULONG(ctx.mem[a + 1]) << 16 | ULONG(ctx.mem[a + 2]) << 8 | ctx.mem[a + 3];
}

/* ---- one case -------------------------------------------------------------- */

static ULONG load(M68KEmuContext &ctx, Program &p)
{
    std::fill(buffer.begin(), buffer.begin() + ARENA, FILL);
    std::fill(buffer.begin() + ARENA, buffer.end(), BEYOND_BYTE);
    active = &ctx;
    tables_live = 0;
    info_calls = 0;
    return M68KEmu_LoadHunks(&ctx, p.segs.bptr());
}

/* Data address of the copy of the n-th segment */
static ULONG data(M68KEmuContext &ctx, size_t n)
{
    CHECK(n < ctx.handed.size(), "the segment was placed");
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
        if (!mine[i] && buffer[i] != FILL) return false;
    return std::all_of(buffer.begin() + ARENA, buffer.end(), [](UBYTE b) { return b == BEYOND_BYTE; });
}

static void nothing_outside(M68KEmuContext &ctx)
{
    CHECK(ctx.copies_outside == 0, "no copy outside containment was attempted");
    CHECK(only_blocks_written(ctx), "nothing outside the blocks of the heap was written");
    CHECK(ctx.bad_frees == 0, "only blocks of this copy, with their own sizes, were given back");
    CHECK(tables_live == 0, "the table of the copy was freed");
}

/* What a copy that succeeded has to look like */
static void whole(M68KEmuContext &ctx, Program &p, ULONG entry)
{
    size_t n = p.segs.segs.size();

    CHECK(entry != 0 && ctx.live.size() == n && ctx.handed.size() == n, "copied, one block for each segment");
    CHECK(entry == data(ctx, 0) && ctx.entry_point == entry && ctx.seg_start == entry,
          "the entry is the data of the first segment, and the context says so");
    for (size_t i = 0; i < n; ++i)
    {
        const Segment &s = p.segs.segs[i];
        ULONG d = data(ctx, i);
        std::vector<bool> relocated(s.bytes, false);

        CHECK(get32(ctx, d - 8) == s.bytes + 8, "the size word of each copy");
        CHECK(get32(ctx, d - 4) == (i + 1 < n ? (data(ctx, i + 1) - 4) >> 2 : 0), "the copies are chained as the segments were");
        for (const Reloc &r : p.relocs)
            if (r.hunk == i)
                for (ULONG b = 0; b < 4; ++b) relocated[r.off + b] = true;
        for (ULONG b = 0; b < s.bytes; ++b)
            if (!relocated[b])
                CHECK(ctx.mem[d + b] == s.data()[b], "every other byte is the segment's");
    }
    /* by the definitions of dos/doshunks.h, for the addresses in containment */
    for (const Reloc &r : p.relocs)
    {
        ULONG holder = data(ctx, r.hunk), target = data(ctx, r.target);
        ULONG want = target + r.original;

        if (r.kind == HUNKRELOC_REL)
            want -= holder + r.off;
        else if (r.kind == HUNKRELOC_BASE)
            want -= holder;
        CHECK(get32(ctx, holder + r.off) == want, "every relocated longword holds its value for containment");
    }
    nothing_outside(ctx);
}

/* What every refusal has to leave behind */
static void refused_cleanly(M68KEmuContext &ctx, ULONG entry)
{
    CHECK(entry == 0, "the seglist is refused");
    nothing_outside(ctx);
    CHECK(ctx.live.empty(), "every block of this copy is given back");
    CHECK(ctx.entry_point == 0x00011110 && ctx.seg_start == 0x00022220, "the fields of the context are left as they were");
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
    /* ---- seglists that are in order: behaviour that has to stay ---- */

    run("one segment without relocations", [] {
        M68KEmuContext ctx;
        Program p{8};
        p.segs.put32(0, 0, 0x11223344);
        p.segs.put32(0, 4, 0x55667788);
        ULONG entry = load(ctx, p);
        whole(ctx, p, entry);
        CHECK(get32(ctx, entry) == 0x11223344 && get32(ctx, entry + 4) == 0x55667788, "the data is copied");
        CHECK(info_calls == 1, "dos is asked for the list once");
    });

    run("two segments relocated against each other", [] {
        M68KEmuContext ctx;
        Program p{8, 8};
        p.group(0, 1, HUNKRELOC_ABS, {{0, 4}}).group(1, 0, HUNKRELOC_ABS, {{4, 0}});
        whole(ctx, p, load(ctx, p));
    });

    run("a relative and a base-relative relocation", [] {
        M68KEmuContext ctx;
        Program p{12, 8};
        p.group(0, 1, HUNKRELOC_REL, {{4, 2}}).group(1, 0, HUNKRELOC_BASE, {{0, 8}});
        whole(ctx, p, load(ctx, p));
    });

    run("a relocation on the last longword of its segment", [] {
        M68KEmuContext ctx;
        Program p{8};
        p.group(0, 0, HUNKRELOC_ABS, {{4, 0x10}});
        whole(ctx, p, load(ctx, p));
    });

    run("a segment without contents between two others", [] {
        M68KEmuContext ctx;
        Program p{4, 0, 4};
        p.group(0, 2, HUNKRELOC_ABS, {{0, 0}});
        whole(ctx, p, load(ctx, p));
    });

    run("several groups, two of them for the same segment", [] {
        M68KEmuContext ctx;
        Program p{16, 8};
        p.group(0, 1, HUNKRELOC_ABS, {{0, 0}, {8, 4}}).group(0, 0, HUNKRELOC_ABS, {{4, 12}});
        p.group(1, 0, HUNKRELOC_REL, {{0, 0}});
        whole(ctx, p, load(ctx, p));
    });

    /* ---- refused by the old code as well: has to stay refused, cleanly ---- */

    run("a segment larger than containment", [] {
        M68KEmuContext ctx;
        Program p{ARENA};
        refused_cleanly(ctx, load(ctx, p));
    });

    /* ---- the relocation list. It is read to the end before anything is
       placed, so a refusal for it leaves the heap unasked. ---- */

    run("a seglist dos has no node for", [] {
        M68KEmuContext ctx;
        Program p{8};
        p.record.node = false;
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("a node that answers no relocation list", [] {
        M68KEmuContext ctx;
        Program p{8};
        p.record.answers = false;
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("a relocation list that was lost", [] {
        M68KEmuContext ctx;
        Program p{8, 8};
        p.record.table.hrt_Groups = 0xFFFFFFFF;
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("a relocation of an unknown kind", [] {
        M68KEmuContext ctx;
        Program p{8, 8};
        p.group(0, 1, 3, {{0, 0}});
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("a group for a segment the seglist does not have", [] {
        M68KEmuContext ctx;
        Program p{8, 8};
        p.group(2, 0, HUNKRELOC_ABS, {{0, 0}});
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("a group whose target the seglist does not have", [] {
        M68KEmuContext ctx;
        Program p{8, 8};
        p.group(0, 5, HUNKRELOC_ABS, {{0, 0}});
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("an offset that ends past its segment", [] {
        M68KEmuContext ctx;
        Program p{8};
        p.group(0, 0, HUNKRELOC_ABS, {{6, 0}});
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("a bad group after a good one: nothing is placed", [] {
        M68KEmuContext ctx;
        Program p{8, 8};
        p.group(0, 1, HUNKRELOC_ABS, {{0, 0}}).group(1, 0, HUNKRELOC_ABS, {{8, 0}});
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    /* ---- the segments and the heap ---- */

    run("a segment whose size is smaller than its own two words", [] {
        M68KEmuContext ctx;
        Program p{8};
        p.segs.size_word(0, 4);
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("more segments than containment can hold", [] {
        M68KEmuContext ctx;
        Program p{Many{ARENA / 8 + 1, 0}};
        refused_cleanly(ctx, load(ctx, p));
        CHECK(ctx.heap_calls == 0, "nothing was placed");
    });

    run("no memory for the second segment: the first is given back", [] {
        M68KEmuContext ctx;
        ctx.fail_call = 2;
        Program p{8, 8};
        p.group(0, 1, HUNKRELOC_ABS, {{0, 0}});
        refused_cleanly(ctx, load(ctx, p));
    });

    run("a heap that answers with a block outside containment", [] {
        M68KEmuContext ctx;
        ctx.wild_call = 1;
        Program p{8};
        refused_cleanly(ctx, load(ctx, p));
    });

    /* Accepted, as before. The block is the last 8 bytes of containment, so
       the segment of no bytes starts where containment ends: there is
       nothing to copy and no address to copy to. */
    run("a segment without contents that the heap places at the very end of containment", [] {
        M68KEmuContext ctx;
        ctx.heap = ARENA - 8;
        Program p{0};
        ULONG entry = load(ctx, p);
        CHECK(entry == ARENA && ctx.live.size() == 1, "accepted: one block, the segment has no byte");
        CHECK(get32(ctx, ARENA - 8) == 8 && get32(ctx, ARENA - 4) == 0, "size and link words in the block");
        nothing_outside(ctx);
    });

    /* ---- giving a copy back ---- */

    run("a copy that is not going to be run is given back", [] {
        M68KEmuContext ctx;
        Program p{8, 0, 12};
        p.group(0, 2, HUNKRELOC_ABS, {{0, 4}});
        ULONG entry = load(ctx, p);
        whole(ctx, p, entry);
        M68KEmu_FreeHunks(&ctx, entry);
        CHECK(ctx.live.empty() && ctx.bad_frees == 0, "every block of the copy is given back, each with its own size");
    });

    run("a copy of more than a hundred segments is given back whole", [] {
        M68KEmuContext ctx;
        Program p{Many{120, 4}};
        ULONG entry = load(ctx, p);
        CHECK(entry != 0 && ctx.live.size() == 120, "copied");
        M68KEmu_FreeHunks(&ctx, entry);
        CHECK(ctx.live.empty() && ctx.bad_frees == 0, "all of it is given back");
    });

    std::printf("m68kemu seglist copy: %u cases, %u failed, %u checks\n", cases, failed, checks);
    return failed ? 1 : 0;
}
