/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The production helpers that keep the records of the BOOPSI classes a 68k
    program makes: the first block of records in the context, more blocks
    made as the program makes more classes. A record never moves while the
    table grows, since its native class keeps the record's address in its
    dispatcher hook; FreeClass moves the last record into the freed slot and
    the hook follows it. No CPU and no native BOOPSI run here.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "class_constants.inc"

using UWORD = unsigned short;
using ULONG = unsigned int;
using BOOL = int;
using APTR = void *;
#define TRUE 1
#define FALSE 0
#define MEMF_ANY 0
#define MEMF_CLEAR (1 << 16)

struct Hook { APTR h_Data; };
struct IClass { Hook cl_Dispatcher; int tag; };
using Class = IClass;

struct M68KEmuContext;
struct M68KClassRec { M68KEmuContext *ctx; ULONG m68k; IClass *native; ULONG body; char *id; };
struct M68KEmuContext
{
    M68KClassRec m68kclasses[M68KEMU_CLASS_BLOCK] = {};
    M68KClassRec *m68kclass_more[M68KEMU_CLASS_BLOCKS - 1] = {};
    UWORD num_m68kclasses = 0;
};

static unsigned allocs, frees, fail_next;
static APTR AllocVec(size_t size, ULONG flags)
{
    if (fail_next) { --fail_next; return nullptr; }
    ++allocs;
    APTR p = std::malloc(size);
    if (p && (flags & MEMF_CLEAR)) std::memset(p, 0, size);
    return p;
}
static void FreeVec(APTR p) { if (p) { ++frees; std::free(p); } }

#include "class_storage.inc"

static unsigned checks, failed;
#define CHECK(c, label) do { ++checks; if (!(c)) { ++failed; std::printf("FAIL %s (line %d)\n", label, __LINE__); } } while (0)

/* MakeClass's part: room, then the next record, its class and its hook */
static M68KClassRec *make(M68KEmuContext &ctx, IClass &cl, int tag)
{
    if (!class_room(&ctx))
        return nullptr;
    M68KClassRec *rec = class_slot(&ctx, ctx.num_m68kclasses++);
    rec->ctx = &ctx; rec->m68k = 0x10000 + tag; rec->native = &cl; rec->body = tag; rec->id = nullptr;
    cl.tag = tag;
    cl.cl_Dispatcher.h_Data = rec;
    return rec;
}

static void teardown(M68KEmuContext &ctx)
{
    for (int i = 0; i < M68KEMU_CLASS_BLOCKS - 1; i++)
        FreeVec(ctx.m68kclass_more[i]);
}

/* every live class is found, by its record, which its hook names */
static bool consistent(M68KEmuContext &ctx, std::vector<IClass> &cls, std::vector<bool> &live)
{
    for (size_t i = 0; i < cls.size(); i++)
    {
        M68KClassRec *r = class_rec(&ctx, &cls[i]);
        if (live[i] != (r != nullptr)) return false;
        if (r && (r->native != &cls[i] || cls[i].cl_Dispatcher.h_Data != r || (int)r->body != cls[i].tag)) return false;
    }
    return true;
}

int main()
{
    {   /* across the boundary: 40 classes, the first 32 records never move */
        M68KEmuContext ctx;
        std::vector<IClass> cls(40);
        std::vector<bool> live(40, false);
        std::vector<M68KClassRec *> at(40);
        allocs = frees = 0;
        for (int i = 0; i < 40; i++) { at[i] = make(ctx, cls[i], i); live[i] = at[i] != nullptr; }
        CHECK(ctx.num_m68kclasses == 40 && allocs == 1, "40 classes, one more block made");
        bool same = true;
        for (int i = 0; i < 40; i++) same = same && class_rec(&ctx, &cls[i]) == at[i];
        CHECK(same, "every record is where it was made, the first 32 included, after the table grew");
        CHECK(at[31] == &ctx.m68kclasses[31] && at[32] == &ctx.m68kclass_more[0][0], "the 33rd record is the first of the new block");
        CHECK(consistent(ctx, cls, live), "every class found, its hook naming its record");

        /* FreeClass of a class in the first block: the last record (in the second) takes its place */
        M68KClassRec *freed = class_rec(&ctx, &cls[5]);
        class_drop(&ctx, freed); live[5] = false;
        CHECK(ctx.num_m68kclasses == 39 && class_rec(&ctx, &cls[39]) == &ctx.m68kclasses[5], "the last record moved into the freed slot");
        CHECK(consistent(ctx, cls, live), "after FreeClass: the moved class's hook follows its record, the others unchanged");

        /* free down below 32, then grow again: the block is reused, not made again */
        for (int i = 6; i < 14; i++) { class_drop(&ctx, class_rec(&ctx, &cls[i])); live[i] = false; }
        CHECK(ctx.num_m68kclasses == 31 && consistent(ctx, cls, live), "31 live after freeing nine");
        std::vector<IClass> more(4);
        for (int i = 0; i < 4; i++) make(ctx, more[i], 100 + i);
        CHECK(ctx.num_m68kclasses == 35 && allocs == 1, "growing past 32 again reuses the kept block");
        CHECK(consistent(ctx, cls, live), "the older classes are unchanged");
        teardown(ctx);
        CHECK(frees == 1, "teardown frees the extra block");
    }

    {   /* no memory for a new block: refused, the 32 existing records usable */
        M68KEmuContext ctx;
        std::vector<IClass> cls(33);
        std::vector<bool> live(33, false);
        allocs = frees = 0;
        for (int i = 0; i < 32; i++) live[i] = make(ctx, cls[i], i) != nullptr;
        fail_next = 1;
        CHECK(make(ctx, cls[32], 32) == nullptr, "the 33rd is refused without memory for a block");
        CHECK(ctx.num_m68kclasses == 32 && ctx.m68kclass_more[0] == nullptr, "nothing changed");
        CHECK(consistent(ctx, cls, live), "the 32 existing classes are found as before");
        CHECK(make(ctx, cls[32], 32) != nullptr && ctx.num_m68kclasses == 33, "with memory again, the 33rd is made");
        live[32] = true;
        CHECK(consistent(ctx, cls, live), "all 33 found");
        teardown(ctx);
    }

    {   /* the last block full: refused at M68KEMU_CLASS_BLOCK * M68KEMU_CLASS_BLOCKS */
        M68KEmuContext ctx;
        const int max = M68KEMU_CLASS_BLOCK * M68KEMU_CLASS_BLOCKS;
        std::vector<IClass> cls(max + 1);
        std::vector<bool> live(max + 1, false);
        allocs = frees = 0;
        for (int i = 0; i < max; i++) live[i] = make(ctx, cls[i], i) != nullptr;
        CHECK(ctx.num_m68kclasses == max && allocs == M68KEMU_CLASS_BLOCKS - 1, "the whole table, in its blocks");
        CHECK(make(ctx, cls[max], max) == nullptr && ctx.num_m68kclasses == max, "one more is refused");
        CHECK(consistent(ctx, cls, live), "every class found");
        teardown(ctx);
        CHECK(frees == M68KEMU_CLASS_BLOCKS - 1, "teardown frees every extra block");
    }

    std::printf("m68kemu class storage: %u checks, %u failed\n", checks, failed);
    return failed ? 1 : 0;
}
