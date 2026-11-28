/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Host fixture for CreateMsgPort / DeleteMsgPort shadow lifecycle, alias
    invalidation, and recycled-native guest-heap balance.
    Compiles verbatim production shadow_register/lookup/remove/create/destroy,
    msgport_is_embedded_proc_port, msgport_invalidate_aliases,
    m68k_msgport_own, m68k_msgport_destroy, and
    thunk_exec_FindPort/CreateMsgPort/DeleteMsgPort to test:
      1. Single CreateMsgPort/DeleteMsgPort cleanup (shadow_map + owned_ports + 68k heap + signal bit)
      2. 1200 (> M68KEMU_MAX_SHADOWS=1024) sequential cycles with distinct native ptrs
      3. Recycled native MsgPort address (no stale mapping hit or guest aliasing)
      4. Recycled native address reused by an unrelated object (no phantom MsgPort shadow)
      5. NULL port and double DeleteMsgPort with live neighbor preserved
      6. Wrong-type / NULL-native / raw containment addresses
      7. 16 simultaneous live ports + partial delete/recreate (swap-with-last compaction)
      8. Deduplicated CreateMsgPort -> AddPort/FindPort -> DeleteMsgPort positive
      9. Distinct-shadow alias (CreateMsgPort -> distinct alias p2 != p1 -> DeleteMsgPort(p1) -> alias use)
     10. Deleting via distinct FindPort alias handle p2 + mixed type-4 UserPort alias invalidation
     11. Ownership negatives (foreign FindPort, Process->pr_MsgPort), duplicate guest addr,
         stale foreign FindPort cleanup with strict guest-heap balance on recycled CreateMsgPort,
         repeated recycled-native FindPort/CreateMsgPort stress (zero guest-heap leak),
         and preservation of embedded type-4 & Process->pr_MsgPort blocks on native recycle
     12. CreateMsgPort rollback when owned_ports or shadow_map is full
*/
#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

using UBYTE = uint8_t;
using BYTE = int8_t;
using UWORD = uint16_t;
using WORD = int16_t;
using ULONG = uint32_t;
using LONG = int32_t;
using IPTR = uintptr_t;
using BOOL = int;
using APTR = void *;
using BPTR = uintptr_t;
using CONST_STRPTR = const char *;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define NT_MSGPORT 4
#define PA_SIGNAL  0

enum { SF_END = 0, SF_BYTE, SF_WORD, SF_LONG, SF_PTR };

#include "delport_consts.inc"

struct Node {
    struct Node *ln_Succ;
    struct Node *ln_Pred;
    UBYTE ln_Type;
    BYTE  ln_Pri;
    char *ln_Name;
};
struct List {
    struct Node *lh_Head;
    struct Node *lh_Tail;
    struct Node *lh_TailPred;
    UBYTE lh_Type;
    UBYTE l_pad;
};
struct MsgPort {
    struct Node mp_Node;
    UBYTE mp_Flags;
    UBYTE mp_SigBit;
    void *mp_SigTask;
    struct List mp_MsgList;
};
struct Task {
    ULONG tc_SigAlloc;
};
struct Screen   { int dummy; };
struct DrawInfo { int dummy; };
struct TextFont { int dummy; };

struct M68KStructLayout;
struct M68KFieldMap {
    UWORD m68k_off;
    UWORD native_off;
    UBYTE type;
    UWORD shadow_type;
    UWORD sub_alloc;
    const struct M68KStructLayout *sub_layout;
};
struct M68KStructLayout {
    const char *name;
    UWORD m68k_size;
    UWORD shadow_type;
    const struct M68KFieldMap *fields;
};

struct FakeCPU {
    ULONG d[8];
    ULONG a[8];
};
static ULONG M68KEmu_GetD(void *cpu, int n) { return static_cast<FakeCPU *>(cpu)->d[n]; }
static ULONG M68KEmu_GetA(void *cpu, int n) { return static_cast<FakeCPU *>(cpu)->a[n]; }
#define THUNK_D(n) M68KEmu_GetD(cpu, n)
#define THUNK_A(n) M68KEmu_GetA(cpu, n)

struct M68KFakeLibBase {
    ULONG m68k_addr;
};
struct M68KEmuContext {
    UBYTE *mem;
    ULONG  mem_size;
    ULONG  m68k_proc;
    char   program_name[64];
    BOOL   shadow_full_logged;
    M68KFakeLibBase libs[8];
    UWORD  num_libs;
    struct {
        ULONG m68k_addr;
        void *native_ptr;
        UWORD type;
        UBYTE streams_valid;
        BPTR  stream_native[3];
        ULONG stream_handle[3];
    } shadow_map[M68KEMU_MAX_SHADOWS];
    UWORD num_shadows;
    struct MsgPort *owned_ports[M68KEMU_MAX_OWNED_PORTS];
    UWORD num_owned_ports;
};

static inline void *m68k_to_host(struct M68KEmuContext *ctx, ULONG m68k_addr)
{
    if (!m68k_addr || m68k_addr >= ctx->mem_size)
        return nullptr;
    return ctx->mem + m68k_addr;
}

static inline void *m68k_to_host_or_shadow(struct M68KEmuContext *ctx, ULONG m68k_addr)
{
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == m68k_addr)
            return ctx->shadow_map[i].native_ptr;
    if (!m68k_addr || m68k_addr >= ctx->mem_size)
        return nullptr;
    return ctx->mem + m68k_addr;
}

static inline void m68k_write16(struct M68KEmuContext *ctx, ULONG addr, UWORD val)
{
    if (addr + 2 <= ctx->mem_size) {
        ctx->mem[addr]     = static_cast<UBYTE>(val >> 8);
        ctx->mem[addr + 1] = static_cast<UBYTE>(val & 0xFF);
    }
}

static inline void m68k_write32(struct M68KEmuContext *ctx, ULONG addr, ULONG val)
{
    if (addr + 4 <= ctx->mem_size) {
        ctx->mem[addr]     = static_cast<UBYTE>(val >> 24);
        ctx->mem[addr + 1] = static_cast<UBYTE>((val >> 16) & 0xFF);
        ctx->mem[addr + 2] = static_cast<UBYTE>((val >> 8) & 0xFF);
        ctx->mem[addr + 3] = static_cast<UBYTE>(val & 0xFF);
    }
}

/* Recording 68k heap stub with strict address/size tracking */
static ULONG g_m68k_heap_cur = 0x2000;
static std::map<ULONG, ULONG> g_m68k_alloc_map;
static std::set<ULONG> g_m68k_freed_addrs;
static int g_m68k_alloc_calls = 0;
static int g_m68k_free_calls = 0;
static int g_m68k_live_allocs = 0;
static int g_m68k_double_free_calls = 0;
static int g_m68k_bad_free_calls = 0;

static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx, ULONG size, ULONG)
{
    ULONG stride = ((size < 48 ? 48 : size) + 15) & ~15U;
    if (g_m68k_heap_cur + stride > ctx->mem_size)
        g_m68k_heap_cur = 0x2000;
    ULONG addr = g_m68k_heap_cur;
    g_m68k_heap_cur += stride;
    g_m68k_alloc_calls++;
    g_m68k_live_allocs++;
    g_m68k_alloc_map[addr] = size;
    g_m68k_freed_addrs.erase(addr);
    return addr;
}

static void M68KEmu_HeapFree(struct M68KEmuContext *, ULONG addr, ULONG size)
{
    if (!addr)
        return;
    g_m68k_free_calls++;
    auto it = g_m68k_alloc_map.find(addr);
    if (it == g_m68k_alloc_map.end()) {
        if (g_m68k_freed_addrs.count(addr))
            g_m68k_double_free_calls++;
        else
            g_m68k_bad_free_calls++;
        return;
    }
    if (it->second != size) {
        g_m68k_bad_free_calls++;
        return;
    }
    g_m68k_alloc_map.erase(it);
    g_m68k_freed_addrs.insert(addr);
    g_m68k_live_allocs--;
}

static int g_log_calls = 0;
static void m68k_log(struct M68KEmuContext *, const char *, ...) { g_log_calls++; }
#define D(x) do { (void)0; } while (0)

static void m68k_screen_extras(struct M68KEmuContext *, ULONG, struct Screen *) {}
static void m68k_drawinfo_extras(struct M68KEmuContext *, ULONG, struct DrawInfo *) {}
static void m68k_textfont_extras(struct M68KEmuContext *, ULONG, struct TextFont *) {}

/* Recording native Exec MsgPort + FindPort + signal-bit stub */
static struct Task g_task = { 0x0000FFFFU };
static std::set<struct MsgPort *> g_live_native_ports;
static std::vector<struct MsgPort *> g_retired_native_ports;
static std::map<std::string, struct MsgPort *> g_public_ports;
static bool g_recycle_mode = false;
static int g_native_create_calls = 0;
static int g_native_delete_calls = 0;
static int g_double_free_calls = 0;
static int g_bad_native_delete_calls = 0;
static int g_sig_exhausted_calls = 0;

static struct MsgPort *CreateMsgPort(void)
{
    g_native_create_calls++;
    int sigbit = -1;
    for (int b = 16; b < 32; b++) {
        if ((g_task.tc_SigAlloc & (1U << b)) == 0) {
            sigbit = b;
            g_task.tc_SigAlloc |= (1U << b);
            break;
        }
    }
    if (sigbit < 0) {
        g_sig_exhausted_calls++;
        return nullptr;
    }
    struct MsgPort *mp = nullptr;
    if (g_recycle_mode && !g_retired_native_ports.empty()) {
        mp = g_retired_native_ports.back();
        g_retired_native_ports.pop_back();
        std::memset(mp, 0, sizeof(*mp));
    } else {
        mp = static_cast<struct MsgPort *>(std::calloc(1, sizeof(struct MsgPort)));
    }
    mp->mp_Node.ln_Type = NT_MSGPORT;
    mp->mp_Flags = PA_SIGNAL;
    mp->mp_SigBit = static_cast<UBYTE>(sigbit);
    mp->mp_SigTask = &g_task;
    mp->mp_MsgList.lh_Head = reinterpret_cast<struct Node *>(&mp->mp_MsgList.lh_Tail);
    mp->mp_MsgList.lh_Tail = nullptr;
    mp->mp_MsgList.lh_TailPred = reinterpret_cast<struct Node *>(&mp->mp_MsgList.lh_Head);
    g_live_native_ports.insert(mp);
    return mp;
}

static void DeleteMsgPort(struct MsgPort *port)
{
    if (!port)
        return;
    g_native_delete_calls++;
    auto it = g_live_native_ports.find(port);
    if (it == g_live_native_ports.end()) {
        if (std::find(g_retired_native_ports.begin(), g_retired_native_ports.end(), port)
                != g_retired_native_ports.end())
            g_double_free_calls++;
        else
            g_bad_native_delete_calls++;
        return;
    }
    g_live_native_ports.erase(it);
    if (port->mp_SigBit >= 16 && port->mp_SigBit < 32)
        g_task.tc_SigAlloc &= ~(1U << port->mp_SigBit);
    g_retired_native_ports.push_back(port);
}

static struct MsgPort *FindPort(CONST_STRPTR name)
{
    if (!name)
        return nullptr;
    auto it = g_public_ports.find(name);
    return (it == g_public_ports.end()) ? nullptr : it->second;
}

#include "delport_extracted.inc"

static unsigned g_checks = 0;
static unsigned g_failed = 0;
#define CHECK(expr) do { \
    g_checks++; \
    if (!(expr)) { \
        std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); \
        g_failed++; \
    } \
} while (0)

static void cleanup_ports(void)
{
    for (struct MsgPort *p : g_live_native_ports) std::free(p);
    g_live_native_ports.clear();
    for (struct MsgPort *p : g_retired_native_ports) std::free(p);
    g_retired_native_ports.clear();
    g_public_ports.clear();
}

static void reset_env(struct M68KEmuContext *ctx, std::vector<UBYTE> &mem)
{
    cleanup_ports();
    std::memset(ctx, 0, sizeof(*ctx));
    std::fill(mem.begin(), mem.end(), 0);
    ctx->mem = mem.data();
    ctx->mem_size = static_cast<ULONG>(mem.size());
    std::snprintf(ctx->program_name, sizeof(ctx->program_name), "test_delport");
    g_m68k_heap_cur = 0x2000;
    g_m68k_alloc_map.clear();
    g_m68k_freed_addrs.clear();
    g_m68k_alloc_calls = 0;
    g_m68k_free_calls = 0;
    g_m68k_live_allocs = 0;
    g_m68k_double_free_calls = 0;
    g_m68k_bad_free_calls = 0;
    g_log_calls = 0;
    g_task.tc_SigAlloc = 0x0000FFFFU;
    g_recycle_mode = false;
    g_native_create_calls = 0;
    g_native_delete_calls = 0;
    g_double_free_calls = 0;
    g_bad_native_delete_calls = 0;
    g_sig_exhausted_calls = 0;
}

static ULONG call_CreateMsgPort(struct M68KEmuContext *ctx)
{
    FakeCPU cpu = {};
    return static_cast<ULONG>(thunk_exec_CreateMsgPort(ctx, &cpu));
}

static void call_DeleteMsgPort(struct M68KEmuContext *ctx, ULONG m68k_port)
{
    FakeCPU cpu = {};
    cpu.a[0] = m68k_port;
    (void)thunk_exec_DeleteMsgPort(ctx, &cpu);
}

/* The name is a string literal: its size, terminator included, comes from its type. */
template <std::size_t N>
static ULONG call_FindPort(struct M68KEmuContext *ctx, const char (&name)[N])
{
    const ULONG name_addr = 0x1000;
    std::memcpy(ctx->mem + name_addr, name, N);
    FakeCPU cpu = {};
    cpu.a[1] = name_addr;
    return static_cast<ULONG>(thunk_exec_FindPort(ctx, &cpu));
}

int main(void)
{
    std::vector<UBYTE> mem(128 * 1024, 0);
    struct M68KEmuContext ctx = {};

    /* Generator configuration guard */
    CHECK(THUNKGEN_HAS_CREATEMSGPORT_OWN == 1);
    CHECK(THUNKGEN_HAS_DELETEMSGPORT_DESTROY == 1);

    /* Case 1: Single CreateMsgPort / DeleteMsgPort full lifecycle */
    {
        reset_env(&ctx, mem);
        ULONG p1 = call_CreateMsgPort(&ctx);
        CHECK(p1 != 0);
        CHECK(ctx.num_shadows == 1);
        CHECK(ctx.num_owned_ports == 1);
        CHECK(shadow_lookup(&ctx, p1) != nullptr);
        CHECK(g_m68k_live_allocs == 1);
        CHECK(g_live_native_ports.size() == 1);
        CHECK(g_task.tc_SigAlloc != 0x0000FFFFU);

        call_DeleteMsgPort(&ctx, p1);
        CHECK(ctx.num_shadows == 0);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);
        CHECK(g_m68k_live_allocs == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(g_live_native_ports.empty());
        CHECK(g_native_delete_calls == 1);
        CHECK(g_double_free_calls == 0);
        CHECK(g_bad_native_delete_calls == 0);
        CHECK(g_task.tc_SigAlloc == 0x0000FFFFU);
    }

    /* Case 2: 1200 (> M68KEMU_MAX_SHADOWS=1024) sequential cycles, distinct native ptrs */
    {
        reset_env(&ctx, mem);
        int failures = 0;
        for (int i = 0; i < 1200; i++)
        {
            ULONG p = call_CreateMsgPort(&ctx);
            if (!p) { failures++; break; }
            call_DeleteMsgPort(&ctx, p);
        }
        CHECK(failures == 0);
        CHECK(ctx.num_shadows == 0);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(ctx.shadow_full_logged == FALSE);
        CHECK(g_m68k_live_allocs == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(g_live_native_ports.empty());
        CHECK(g_task.tc_SigAlloc == 0x0000FFFFU);
        CHECK(g_sig_exhausted_calls == 0);
        CHECK(g_native_create_calls == 1200 && g_native_delete_calls == 1200);
    }

    /* Case 3: Recycled native MsgPort address (no stale mapping hit or guest aliasing) */
    {
        reset_env(&ctx, mem);
        g_recycle_mode = true;
        ULONG p1 = call_CreateMsgPort(&ctx);
        void *np1 = shadow_lookup(&ctx, p1);
        CHECK(p1 != 0 && np1 != nullptr);
        call_DeleteMsgPort(&ctx, p1);
        CHECK(ctx.num_shadows == 0);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);

        int allocs_before = g_m68k_alloc_calls;
        ULONG p2 = call_CreateMsgPort(&ctx);
        void *np2 = shadow_lookup(&ctx, p2);
        CHECK(p2 != 0 && np2 == np1);
        CHECK(p2 != p1);
        CHECK(g_m68k_alloc_calls == allocs_before + 1);
        CHECK(ctx.num_shadows == 1);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);
        CHECK(shadow_lookup(&ctx, p2) == np2);

        call_DeleteMsgPort(&ctx, p2);
        CHECK(ctx.num_shadows == 0 && ctx.num_owned_ports == 0);
        CHECK(g_m68k_live_allocs == 0 && g_m68k_bad_free_calls == 0 && g_live_native_ports.empty());
        CHECK(g_double_free_calls == 0 && g_bad_native_delete_calls == 0);
    }

    /* Case 4: Recycled native address reused by an unrelated object */
    {
        reset_env(&ctx, mem);
        g_recycle_mode = true;
        ULONG p1 = call_CreateMsgPort(&ctx);
        void *np = shadow_lookup(&ctx, p1);
        CHECK(p1 != 0 && np != nullptr);
        call_DeleteMsgPort(&ctx, p1);

        const ULONG win_addr = 0x5000;
        CHECK(shadow_register(&ctx, win_addr, np, 1 /* Window */) == win_addr);
        int msgport_matches = 0;
        const struct M68KStructLayout *pl = shadow_find_layout("MsgPort");
        for (UWORD i = 0; i < ctx.num_shadows; i++)
            if (ctx.shadow_map[i].native_ptr == np && ctx.shadow_map[i].type == pl->shadow_type)
                msgport_matches++;
        CHECK(msgport_matches == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);
        CHECK(shadow_lookup(&ctx, win_addr) == np);
    }

    /* Case 5: NULL and double DeleteMsgPort with live neighbor preserved */
    {
        reset_env(&ctx, mem);
        ULONG p_live = call_CreateMsgPort(&ctx);
        void *np_live = shadow_lookup(&ctx, p_live);
        ULONG p1 = call_CreateMsgPort(&ctx);
        CHECK(p_live != 0 && p1 != 0 && ctx.num_shadows == 2 && ctx.num_owned_ports == 2);

        call_DeleteMsgPort(&ctx, 0);
        CHECK(g_native_delete_calls == 0);
        CHECK(ctx.num_shadows == 2);

        call_DeleteMsgPort(&ctx, p1);
        CHECK(ctx.num_shadows == 1 && ctx.num_owned_ports == 1);
        CHECK(shadow_lookup(&ctx, p_live) == np_live);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);

        call_DeleteMsgPort(&ctx, p1);
        CHECK(g_native_delete_calls == 1);
        CHECK(g_double_free_calls == 0);
        CHECK(g_bad_native_delete_calls == 0);
        CHECK(g_m68k_double_free_calls == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(ctx.num_shadows == 1 && shadow_lookup(&ctx, p_live) == np_live);

        call_DeleteMsgPort(&ctx, p_live);
        CHECK(ctx.num_shadows == 0 && ctx.num_owned_ports == 0 && g_m68k_live_allocs == 0);
        CHECK(g_live_native_ports.empty() && g_task.tc_SigAlloc == 0x0000FFFFU);
    }

    /* Case 6: Wrong-type, NULL-native, and raw containment addresses */
    {
        reset_env(&ctx, mem);
        const struct M68KStructLayout *pl = shadow_find_layout("MsgPort");
        struct MsgPort fake_port = {};
        struct MsgPort other_obj = {};

        CHECK(shadow_register(&ctx, 0x2800, &fake_port, 4) == 0x2800);
        CHECK(shadow_register(&ctx, 0x2880, &fake_port, 0) == 0x2880);
        CHECK(shadow_register(&ctx, 0x2900, &other_obj, 1) == 0x2900);
        CHECK(shadow_register(&ctx, 0x2980, nullptr, pl->shadow_type) == 0x2980);
        UWORD before_shadows = ctx.num_shadows;

        call_DeleteMsgPort(&ctx, 0x2800);
        call_DeleteMsgPort(&ctx, 0x2880);
        call_DeleteMsgPort(&ctx, 0x2900);
        call_DeleteMsgPort(&ctx, 0x2980);
        call_DeleteMsgPort(&ctx, 0x3000); /* raw containment address */
        call_DeleteMsgPort(&ctx, 0xDEADBEEFU); /* out-of-range */

        CHECK(g_native_delete_calls == 0);
        CHECK(g_bad_native_delete_calls == 0);
        CHECK(g_double_free_calls == 0);
        CHECK(g_m68k_free_calls == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(ctx.num_shadows == before_shadows);
        CHECK(shadow_lookup(&ctx, 0x2800) == &fake_port);
        CHECK(shadow_lookup(&ctx, 0x2900) == &other_obj);
    }

    /* Case 7: 16 simultaneous live ports + partial delete/recreate (swap compaction) */
    {
        reset_env(&ctx, mem);
        ULONG ports[16];
        void *natives[16];
        for (int i = 0; i < 16; i++)
        {
            ports[i] = call_CreateMsgPort(&ctx);
            CHECK(ports[i] != 0);
            natives[i] = shadow_lookup(&ctx, ports[i]);
            CHECK(natives[i] != nullptr);
        }
        CHECK(ctx.num_shadows == 16);
        CHECK(ctx.num_owned_ports == 16);
        CHECK(g_task.tc_SigAlloc == 0xFFFFFFFFU);

        for (int i = 1; i < 16; i += 2)
            call_DeleteMsgPort(&ctx, ports[i]);
        CHECK(ctx.num_shadows == 8);
        CHECK(ctx.num_owned_ports == 8);
        for (int i = 0; i < 16; i += 2)
            CHECK(shadow_lookup(&ctx, ports[i]) == natives[i]);
        for (int i = 1; i < 16; i += 2)
            CHECK(shadow_lookup(&ctx, ports[i]) == nullptr);

        ULONG ports2[8];
        for (int i = 0; i < 8; i++)
        {
            ports2[i] = call_CreateMsgPort(&ctx);
            CHECK(ports2[i] != 0);
        }
        CHECK(ctx.num_shadows == 16);
        CHECK(g_task.tc_SigAlloc == 0xFFFFFFFFU);

        for (int i = 0; i < 16; i += 2) call_DeleteMsgPort(&ctx, ports[i]);
        for (int i = 0; i < 8; i++) call_DeleteMsgPort(&ctx, ports2[i]);
        CHECK(ctx.num_shadows == 0);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(g_m68k_live_allocs == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(g_live_native_ports.empty());
        CHECK(g_task.tc_SigAlloc == 0x0000FFFFU);
    }

    /* Case 8: Positive; CreateMsgPort -> AddPort -> FindPort (deduplicated) -> DeleteMsgPort */
    {
        reset_env(&ctx, mem);
        ULONG p1 = call_CreateMsgPort(&ctx);
        struct MsgPort *np1 = static_cast<struct MsgPort *>(shadow_lookup(&ctx, p1));
        CHECK(p1 != 0 && np1 != nullptr);
        g_public_ports["MYPORT"] = np1;

        ULONG p_found = call_FindPort(&ctx, "MYPORT");
        CHECK(p_found == p1);
        CHECK(ctx.num_shadows == 1);
        CHECK(ctx.num_owned_ports == 1);

        g_public_ports.erase("MYPORT");
        call_DeleteMsgPort(&ctx, p_found);
        CHECK(ctx.num_shadows == 0);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);
        CHECK(g_native_delete_calls == 1);
        CHECK(g_m68k_live_allocs == 0 && g_m68k_bad_free_calls == 0);
        CHECK(call_FindPort(&ctx, "MYPORT") == 0);
    }

    /* Case 9: Core alias-lifetime case; CreateMsgPort -> distinct second MsgPort shadow alias p2 != p1
       -> DeleteMsgPort(p1) MUST invalidate BOTH p1 and p2 (and preserve live neighbor) */
    {
        reset_env(&ctx, mem);
        g_recycle_mode = true;
        const struct M68KStructLayout *pl = shadow_find_layout("MsgPort");
        ULONG p1 = call_CreateMsgPort(&ctx);
        struct MsgPort *np1 = static_cast<struct MsgPort *>(shadow_lookup(&ctx, p1));
        ULONG p_other = call_CreateMsgPort(&ctx);
        void *np_other = shadow_lookup(&ctx, p_other);

        ULONG p2 = M68KEmu_HeapAlloc(&ctx, pl->m68k_size, 0);
        CHECK(p1 != 0 && p2 != 0 && p2 != p1 && np1 != nullptr && np_other != nullptr);
        CHECK(shadow_register(&ctx, p2, np1, pl->shadow_type) == p2);
        CHECK(ctx.num_shadows == 3);
        CHECK(shadow_lookup(&ctx, p1) == np1);
        CHECK(shadow_lookup(&ctx, p2) == np1);

        call_DeleteMsgPort(&ctx, p1);
        CHECK(g_native_delete_calls == 1);
        CHECK(g_double_free_calls == 0);
        CHECK(g_m68k_double_free_calls == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);
        CHECK(shadow_lookup(&ctx, p2) == nullptr);
        CHECK(ctx.num_shadows == 1 && shadow_lookup(&ctx, p_other) == np_other);
        CHECK(ctx.num_owned_ports == 1);
        CHECK(g_m68k_live_allocs == 1);

        call_DeleteMsgPort(&ctx, p2);
        CHECK(g_native_delete_calls == 1);
        CHECK(g_double_free_calls == 0);
        CHECK(g_bad_native_delete_calls == 0);
        CHECK(shadow_lookup(&ctx, p2) == nullptr);

        ULONG p3 = call_CreateMsgPort(&ctx);
        CHECK(p3 != 0 && shadow_lookup(&ctx, p3) == np1);
        call_DeleteMsgPort(&ctx, p3);
        call_DeleteMsgPort(&ctx, p_other);
        CHECK(ctx.num_shadows == 0 && ctx.num_owned_ports == 0 && g_m68k_live_allocs == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(g_live_native_ports.empty() && g_task.tc_SigAlloc == 0x0000FFFFU);
    }

    /* Case 10: Deleting via distinct FindPort alias p2 + mixed type-4 UserPort alias invalidation */
    {
        reset_env(&ctx, mem);
        const struct M68KStructLayout *pl = shadow_find_layout("MsgPort");
        ULONG p1 = call_CreateMsgPort(&ctx);
        struct MsgPort *np1 = static_cast<struct MsgPort *>(shadow_lookup(&ctx, p1));
        ULONG p2 = M68KEmu_HeapAlloc(&ctx, pl->m68k_size, 0);
        CHECK(p1 != 0 && p2 != 0 && p2 != p1 && np1 != nullptr);
        CHECK(shadow_register(&ctx, p2, np1, pl->shadow_type) == p2);
        const ULONG p_win = 0x5800;
        CHECK(shadow_register(&ctx, p_win, np1, 4) == p_win);
        CHECK(ctx.num_shadows == 3);

        call_DeleteMsgPort(&ctx, p_win);
        CHECK(g_native_delete_calls == 0);
        CHECK(ctx.num_shadows == 3);

        call_DeleteMsgPort(&ctx, p2);
        CHECK(g_native_delete_calls == 1);
        CHECK(g_double_free_calls == 0);
        CHECK(g_m68k_double_free_calls == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(ctx.num_shadows == 0);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);
        CHECK(shadow_lookup(&ctx, p2) == nullptr);
        CHECK(shadow_lookup(&ctx, p_win) == nullptr);
        CHECK(g_m68k_live_allocs == 0);
        CHECK(g_m68k_free_calls == g_m68k_alloc_calls);

        call_DeleteMsgPort(&ctx, p1);
        CHECK(g_native_delete_calls == 1 && g_double_free_calls == 0);
    }

    /* Case 11: Ownership negatives (foreign FindPort, Process->pr_MsgPort), duplicate m68k_addr,
       recycled-native stale FindPort guest-heap balance, and preserved embedded type-4 / pr_MsgPort blocks */
    {
        reset_env(&ctx, mem);
        const struct M68KStructLayout *pl = shadow_find_layout("MsgPort");
        const struct M68KStructLayout *tl = shadow_find_layout("Task");
        CHECK(pl != nullptr && tl != nullptr);

        /* 11a: Foreign system port found by FindPort() is NOT owned -> DeleteMsgPort refused */
        struct MsgPort foreign_port = {};
        foreign_port.mp_Node.ln_Type = NT_MSGPORT;
        g_public_ports["AREXX"] = &foreign_port;
        ULONG pf = call_FindPort(&ctx, "AREXX");
        CHECK(pf != 0 && shadow_lookup(&ctx, pf) == &foreign_port);
        CHECK(ctx.num_owned_ports == 0);
        call_DeleteMsgPort(&ctx, pf);
        CHECK(g_native_delete_calls == 0);
        CHECK(shadow_lookup(&ctx, pf) == &foreign_port);

        /* 11b: Embedded Process->pr_MsgPort (type 193, not owned) -> DeleteMsgPort refused */
        struct MsgPort proc_port = {};
        const ULONG m_proc_main = 0x6000;
        const ULONG m_pr_msgport = m_proc_main + M68K_PR_MSGPORT;
        ctx.m68k_proc = m_proc_main;
        CHECK(shadow_register(&ctx, m_pr_msgport, &proc_port, pl->shadow_type) == m_pr_msgport);
        int frees_before = g_m68k_free_calls;
        call_DeleteMsgPort(&ctx, m_pr_msgport);
        CHECK(g_native_delete_calls == 0);
        CHECK(g_m68k_free_calls == frees_before);
        CHECK(shadow_lookup(&ctx, m_pr_msgport) == &proc_port);

        /* 11c: Duplicate shadow_map entries with identical (p1, np, 193) -> single 68k HeapFree */
        ULONG p1 = call_CreateMsgPort(&ctx);
        struct MsgPort *np1 = static_cast<struct MsgPort *>(shadow_lookup(&ctx, p1));
        CHECK(p1 != 0 && np1 != nullptr);
        CHECK(shadow_register(&ctx, p1, np1, pl->shadow_type) == p1);
        call_DeleteMsgPort(&ctx, p1);
        CHECK(g_native_delete_calls == 1);
        CHECK(g_m68k_double_free_calls == 0);
        CHECK(g_m68k_bad_free_calls == 0);
        CHECK(shadow_lookup(&ctx, p1) == nullptr);

        /* 11d: Stale foreign FindPort entry at recycled native address is unmapped AND its guest block freed */
        g_recycle_mode = true;
        int live_before_11d = g_m68k_live_allocs;
        ULONG p_seed = call_CreateMsgPort(&ctx);
        struct MsgPort *np_recycled = static_cast<struct MsgPort *>(shadow_lookup(&ctx, p_seed));
        call_DeleteMsgPort(&ctx, p_seed);
        CHECK(g_m68k_live_allocs == live_before_11d);

        g_public_ports["DEAD_FOREIGN"] = np_recycled;
        ULONG p_stale_foreign = call_FindPort(&ctx, "DEAD_FOREIGN");
        CHECK(p_stale_foreign != 0 && shadow_lookup(&ctx, p_stale_foreign) == np_recycled);
        CHECK(g_m68k_live_allocs == live_before_11d + 1);
        g_public_ports.erase("DEAD_FOREIGN");

        ULONG p_fresh = call_CreateMsgPort(&ctx);
        CHECK(p_fresh != 0 && p_fresh != p_stale_foreign);
        CHECK(shadow_lookup(&ctx, p_fresh) == np_recycled);
        CHECK(shadow_lookup(&ctx, p_stale_foreign) == nullptr);
        /* Stale FindPort guest block was freed; only p_fresh is now live above baseline */
        CHECK(g_m68k_live_allocs == live_before_11d + 1);
        CHECK(g_m68k_bad_free_calls == 0 && g_m68k_double_free_calls == 0);
        call_DeleteMsgPort(&ctx, p_fresh);
        CHECK(g_m68k_live_allocs == live_before_11d);
        CHECK(ctx.num_owned_ports == 0 && g_live_native_ports.empty());
        CHECK(g_task.tc_SigAlloc == 0x0000FFFFU);

        /* 11e: Repeated recycled-native FindPort -> CreateMsgPort -> DeleteMsgPort balance (0 leak)
           while preserving embedded type-4 blocks AND embedded Process->pr_MsgPort blocks */
        int live_before_11e = g_m68k_live_allocs;
        int allocs_before_11e = g_m68k_alloc_calls;
        int frees_before_11e = g_m68k_free_calls;
        for (int iter = 0; iter < 64; iter++)
        {
            g_public_ports["RECYCLED_FOREIGN"] = np_recycled;
            ULONG p_stale = call_FindPort(&ctx, "RECYCLED_FOREIGN");
            CHECK(p_stale != 0);
            g_public_ports.erase("RECYCLED_FOREIGN");

            ULONG p_new = call_CreateMsgPort(&ctx);
            CHECK(p_new != 0 && shadow_lookup(&ctx, p_new) == np_recycled);
            CHECK(shadow_lookup(&ctx, p_stale) == nullptr);
            call_DeleteMsgPort(&ctx, p_new);
        }
        CHECK(g_m68k_live_allocs == live_before_11e);
        CHECK((g_m68k_free_calls - frees_before_11e) == (g_m68k_alloc_calls - allocs_before_11e));
        CHECK(g_m68k_bad_free_calls == 0 && g_m68k_double_free_calls == 0);

        /* Stale embedded type-4 twin/UserPort + stale embedded child Process->pr_MsgPort at recycled native:
           both mappings MUST be invalidated, while neither embedded block may be HeapFree'd! */
        int dead_child_proc_dummy = 0;
        const ULONG m_emb_type4 = 0x5900;
        const ULONG m_dead_proc = 0x6400;
        const ULONG m_dead_pr_port = m_dead_proc + M68K_PR_MSGPORT;
        CHECK(shadow_register(&ctx, m_emb_type4, np_recycled, 4) == m_emb_type4);
        CHECK(shadow_register(&ctx, m_dead_proc, &dead_child_proc_dummy, tl->shadow_type) == m_dead_proc);
        CHECK(shadow_register(&ctx, m_dead_pr_port, np_recycled, pl->shadow_type) == m_dead_pr_port);
        g_public_ports["STALE_WITH_EMBEDDED"] = np_recycled;
        ULONG p_stale_mix = call_FindPort(&ctx, "STALE_WITH_EMBEDDED");
        /* shadow_create reused m_dead_pr_port (same type 193); add a distinct standalone FindPort shadow too */
        ULONG p_stale_standalone = M68KEmu_HeapAlloc(&ctx, pl->m68k_size, 0);
        CHECK(p_stale_mix == m_dead_pr_port && p_stale_standalone != 0);
        CHECK(shadow_register(&ctx, p_stale_standalone, np_recycled, pl->shadow_type) == p_stale_standalone);
        g_public_ports.erase("STALE_WITH_EMBEDDED");

        int live_before_mix = g_m68k_live_allocs; /* includes p_stale_standalone */
        ULONG p_after_mix = call_CreateMsgPort(&ctx);
        CHECK(p_after_mix != 0 && shadow_lookup(&ctx, p_after_mix) == np_recycled);
        CHECK(shadow_lookup(&ctx, m_emb_type4) == nullptr);
        CHECK(shadow_lookup(&ctx, m_dead_pr_port) == nullptr);
        CHECK(shadow_lookup(&ctx, p_stale_standalone) == nullptr);
        /* Task entry itself stays intact; only p_stale_standalone was freed, never m_emb_type4 or m_dead_pr_port */
        CHECK(shadow_lookup(&ctx, m_dead_proc) == &dead_child_proc_dummy);
        CHECK(g_m68k_bad_free_calls == 0 && g_m68k_double_free_calls == 0);
        CHECK(g_m68k_live_allocs == live_before_mix); /* -1 stale_standalone +1 p_after_mix */
        call_DeleteMsgPort(&ctx, p_after_mix);
        CHECK(g_m68k_live_allocs == live_before_11e);
        CHECK(g_m68k_bad_free_calls == 0 && g_m68k_double_free_calls == 0);
    }

    /* Case 12: CreateMsgPort failure rollback when owned_ports or shadow_map is full */
    {
        reset_env(&ctx, mem);
        ctx.num_owned_ports = M68KEMU_MAX_OWNED_PORTS;
        ULONG p_fail1 = call_CreateMsgPort(&ctx);
        CHECK(p_fail1 == 0);
        CHECK(g_live_native_ports.empty());
        CHECK(g_task.tc_SigAlloc == 0x0000FFFFU);
        CHECK(ctx.num_shadows == 0);
        CHECK(g_m68k_live_allocs == 0 && g_m68k_bad_free_calls == 0);

        reset_env(&ctx, mem);
        ctx.num_shadows = M68KEMU_MAX_SHADOWS;
        ULONG p_fail2 = call_CreateMsgPort(&ctx);
        CHECK(p_fail2 == 0);
        CHECK(g_live_native_ports.empty());
        CHECK(g_task.tc_SigAlloc == 0x0000FFFFU);
        CHECK(ctx.num_owned_ports == 0);
        CHECK(g_m68k_live_allocs == 0 && g_m68k_bad_free_calls == 0);
    }

    cleanup_ports();
    std::printf("m68kemu deletemsgport shadow: %u checks, %u failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
