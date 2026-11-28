/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The production helpers that keep the records of the BOOPSI classes a 68k
    program makes: the first block of records in the context, more blocks
    made as the program makes more classes. A record never moves while the
    table grows, since its native class keeps the record's address in its
    dispatcher hook; FreeClass moves the last record into the freed slot and
    the hook follows it.

    And the production m68k_MakeClass when the shadow table, which classes
    share with objects, ports and tasks, has no entry left: what it made is
    freed and the program gets 0. No CPU and no native BOOPSI run here: the
    registers are a plain structure and what MakeClass calls counts its calls.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <vector>
#include "class_constants.inc"

using UBYTE = unsigned char;
using UWORD = unsigned short;
using ULONG = unsigned int;
using BOOL = int;
using APTR = void *;
using IPTR = unsigned long;
using ClassID = UBYTE *;
using HOOKFUNC = ULONG (*)();
#define TRUE 1
#define FALSE 0
#define MEMF_ANY 0
#define MEMF_CLEAR (1 << 16)

struct Hook { HOOKFUNC h_Entry; APTR h_Data; };
struct IClass { Hook cl_Dispatcher; int tag; IClass *cl_Super; };
using Class = IClass;

struct M68KEmuContext;
struct M68KClassRec { M68KEmuContext *ctx; ULONG m68k; IClass *native; ULONG body; char *id; };
struct M68KEmuContext
{
    M68KClassRec m68kclasses[M68KEMU_CLASS_BLOCK] = {};
    M68KClassRec *m68kclass_more[M68KEMU_CLASS_BLOCKS - 1] = {};
    UWORD num_m68kclasses = 0;
    UBYTE *mem = nullptr;
    ULONG mem_size = 0;
};

/* AllocVec's result goes to any pointer without a cast, as a void * does in C */
struct Mem { APTR p; template <class T> operator T *() const { return static_cast<T *>(p); } };

static unsigned allocs, frees, fail_next;
static Mem AllocVec(size_t size, ULONG flags)
{
    if (fail_next) { --fail_next; return {nullptr}; }
    ++allocs;
    APTR p = std::malloc(size);
    if (p && (flags & MEMF_CLEAR)) std::memset(p, 0, size);
    return {p};
}
static void FreeVec(APTR p) { if (p) { ++frees; std::free(p); } }

#include "class_storage.inc"

/* What m68k_MakeClass reaches outside the records. Each part counts its calls
   and keeps what it was last given; the shadow table can be made full. */
struct Regs { ULONG d[8], a[8]; };
#define THUNK_D(n) (static_cast<Regs *>(cpu)->d[n])
#define THUNK_A(n) (static_cast<Regs *>(cpu)->a[n])

static std::deque<IClass> natives;      /* a deque: the classes made stay where they are */
static struct World
{
    unsigned made, freed, heap_allocs, heap_frees, registered;
    bool shadow_full;
    IClass *last_made, *last_freed;
    ULONG heap_next, heap_last, heap_freed, heap_freed_size;
    ULONG reg_m68k; void *reg_native; UWORD reg_type;
} world;

static void boopsi_init(M68KEmuContext *) {}
static Class *shadow_lookup(M68KEmuContext *, ULONG) { return nullptr; }
static ULONG shadow_register(M68KEmuContext *, ULONG m68k, void *native, UWORD type)
{
    if (world.shadow_full) return 0;
    ++world.registered; world.reg_m68k = m68k; world.reg_native = native; world.reg_type = type;
    return 1;
}
static Class *MakeClass(ClassID, ClassID, Class *super, ULONG, ULONG)
{
    natives.emplace_back();
    natives.back().cl_Super = super;
    ++world.made;
    return world.last_made = &natives.back();
}
static BOOL FreeClass(Class *cl) { ++world.freed; world.last_freed = cl; return TRUE; }
static ULONG M68KEmu_HeapAlloc(M68KEmuContext *, ULONG size, ULONG)
{
    ++world.heap_allocs;
    world.heap_last = world.heap_next ? world.heap_next : 0x1000;
    world.heap_next = world.heap_last + ((size + 7) & ~7u);
    return world.heap_last;
}
static void M68KEmu_HeapFree(M68KEmuContext *, ULONG addr, ULONG size)
{
    ++world.heap_frees; world.heap_freed = addr; world.heap_freed_size = size;
}
static ULONG class_body(M68KEmuContext *, Class *) { return 8; }
static ULONG m68k_class_of(M68KEmuContext *, Class *) { return 0x2000; }
static ULONG m68k_class_hook() { return 0; }
static void m68k_write16(M68KEmuContext *ctx, ULONG addr, ULONG v)
{
    ctx->mem[addr] = v >> 8; ctx->mem[addr + 1] = v;
}
static void m68k_write32(M68KEmuContext *ctx, ULONG addr, ULONG v)
{
    m68k_write16(ctx, addr, v >> 16); m68k_write16(ctx, addr + 2, v);
}
static ULONG read32(M68KEmuContext &ctx, ULONG addr)
{
    return (ULONG)ctx.mem[addr] << 24 | ctx.mem[addr + 1] << 16 | ctx.mem[addr + 2] << 8 | ctx.mem[addr + 3];
}

#include "class_makeclass.inc"

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

/* the program's MakeClass(NULL, NULL, NULL, size, 0): its registers */
static ULONG make68k(M68KEmuContext &ctx, ULONG size)
{
    Regs regs = {};
    regs.d[0] = size;
    return m68k_MakeClass(&ctx, &regs);
}

/* and MakeClass(name, NULL, NULL, size, 0): the name, a string literal whose
   size comes from its type, goes into the program's memory */
template <std::size_t N>
static ULONG make68k(M68KEmuContext &ctx, const char (&name)[N], ULONG size)
{
    Regs regs = {};
    std::memcpy(ctx.mem + 0x100, name, N);
    regs.a[0] = 0x100;
    regs.d[0] = size;
    return m68k_MakeClass(&ctx, &regs);
}

/* the copies of the names, which FreeClass and the end of the program free */
static void drop_names(M68KEmuContext &ctx)
{
    for (int i = 0; i < ctx.num_m68kclasses; i++)
        FreeVec(class_slot(&ctx, i)->id);
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

    {   /* m68k_MakeClass with the shadow table full: 0, and what it made is freed */
        std::vector<UBYTE> mem(0x10000);
        M68KEmuContext ctx;
        ctx.mem = mem.data(); ctx.mem_size = mem.size();
        world = {}; allocs = frees = 0;

        ULONG first = make68k(ctx, "first.class", 12);
        IClass *cl1 = world.last_made;
        M68KClassRec *r1 = cl1 ? class_rec(&ctx, cl1) : nullptr;
        CHECK(first && ctx.num_m68kclasses == 1 && world.made == 1 && world.heap_allocs == 1 && !world.freed && !world.heap_frees,
              "a class is made: one native class, one block of the program's memory, nothing freed");
        CHECK(world.registered == 1 && world.reg_m68k == first && world.reg_native == cl1 && world.reg_type == SHADOW_BCLASS,
              "its shadow entry names the block and the native class");
        CHECK(r1 && r1->m68k == first && r1->native == cl1 && r1->body == 20 && cl1->cl_Dispatcher.h_Data == r1 &&
              cl1->cl_Dispatcher.h_Entry == m68k_class_hook,
              "its record names the block and the class, and the class's hook names the record");
        CHECK(r1 && r1->id && !std::strcmp(r1->id, "first.class") && allocs == 1 && !frees &&
              read32(ctx, first + CL_ID) == first + CL_SIZE && !std::memcmp(ctx.mem + first + CL_SIZE, "first.class", sizeof "first.class"),
              "its name is kept, natively and in the program's class");
        CHECK(read32(ctx, first + 24) == 0x2000, "the program's class names its superclass");

        world.shadow_full = true;
        ULONG refused = make68k(ctx, "second.class", 12);
        IClass *cl2 = world.last_made;
        CHECK(refused == 0, "with the shadow table full MakeClass returns 0");
        CHECK(world.made == 2 && cl2 != cl1 && world.freed == 1 && world.last_freed == cl2, "the native class it made is freed, once");
        CHECK(world.heap_allocs == 2 && world.heap_frees == 1 && world.heap_freed == world.heap_last &&
              world.heap_freed_size == CL_SIZE + 64, "its block of the program's memory is freed, once and whole");
        CHECK(allocs == 2 && frees == 1, "the copy of its name is freed");
        CHECK(ctx.num_m68kclasses == 1 && !class_rec(&ctx, cl2) && !cl2->cl_Dispatcher.h_Data && !cl2->cl_Dispatcher.h_Entry,
              "no record is made and the hook of the freed class is not set");
        CHECK(world.registered == 1 && class_rec(&ctx, cl1) == r1 && r1->m68k == first && r1->native == cl1 &&
              cl1->cl_Dispatcher.h_Data == r1 && !std::strcmp(r1->id, "first.class"), "the class made before is unchanged");

        world.shadow_full = false;
        ULONG second = make68k(ctx, "second.class", 12);
        IClass *cl3 = world.last_made;
        CHECK(second && second != first && ctx.num_m68kclasses == 2 && class_rec(&ctx, cl3) == &ctx.m68kclasses[1] &&
              world.registered == 2 && world.reg_m68k == second && world.reg_native == cl3,
              "with an entry free again the class is made, in the next record");
        CHECK(world.freed == 1 && world.heap_frees == 1 && allocs == 3 && frees == 1, "nothing more is freed");
        drop_names(ctx);
        CHECK(frees == 3, "the two names kept are the two left to free");
    }

    {   /* the same refusal on the 33rd class: the new block of records is kept and used later */
        std::vector<UBYTE> mem(0x10000);
        M68KEmuContext ctx;
        ctx.mem = mem.data(); ctx.mem_size = mem.size();
        world = {}; allocs = frees = 0;
        bool all = true;
        for (int i = 0; i < 32; i++) all = all && make68k(ctx, 4);
        CHECK(all && ctx.num_m68kclasses == 32 && allocs == 0, "32 classes without a name, in the first block");

        world.shadow_full = true;
        CHECK(make68k(ctx, 4) == 0 && ctx.num_m68kclasses == 32, "the 33rd is refused with the shadow table full");
        CHECK(world.made == 33 && world.freed == 1 && world.last_freed == world.last_made && world.heap_allocs == 33 &&
              world.heap_frees == 1 && world.heap_freed == world.heap_last, "its native class and its block are freed");
        CHECK(allocs == 1 && frees == 0 && ctx.m68kclass_more[0], "the block of records made for it is kept");

        world.shadow_full = false;
        ULONG later = make68k(ctx, 4);
        CHECK(later && ctx.num_m68kclasses == 33 && allocs == 1 && class_rec(&ctx, world.last_made) == &ctx.m68kclass_more[0][0],
              "the 33rd made later takes the first record of that block, which is not made again");
        CHECK(world.freed == 1 && world.heap_frees == 1, "nothing more is freed");
        teardown(ctx);
        CHECK(frees == 1, "teardown frees the one extra block");
    }

    std::printf("m68kemu class storage: %u checks, %u failed\n", checks, failed);
    return failed ? 1 : 0;
}
