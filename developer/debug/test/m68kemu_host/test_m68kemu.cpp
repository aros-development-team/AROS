/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Host-side test for m68kemu core — no AROS required.
    Uses stub headers in stubs/ directory.
*/
#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

/* Stub thunk tables */
const struct M68KThunkEntry m68kemu_thunks_exec[] = { { 0, nullptr } };
const ULONG m68kemu_thunks_exec_count = 0;
const struct M68KThunkEntry m68kemu_thunks_dos[] = { { 0, nullptr } };
const ULONG m68kemu_thunks_dos_count = 0;

extern "C" {
ULONG M68KEmu_GetD(void *, int) { return 0; }
ULONG M68KEmu_GetA(void *, int) { return 0; }
void  M68KEmu_SetD(void *, int, ULONG) {}
void  M68KEmu_SetA(void *, int, ULONG) {}
ULONG M68KEmu_OpenLibrary(struct M68KEmuContext *, const char *) { return 0; }
ULONG M68KEmu_LoadM68KLibrary(struct M68KEmuContext *, const char *) { return 0; }
}

/* Include memory manager source */
#include "../../rom/m68kemu/m68kemu_memory.c"

#include <cstdio>
static int pass = 0, fail = 0;
#define TEST(n) printf("  %-40s ", n)
#define OK() do { printf("PASS\n"); pass++; } while(0)
#define NG(m) do { printf("FAIL: %s\n", m); fail++; return; } while(0)
#define CHK(c,m) do { if(!(c)) NG(m); } while(0)

void t_ctx() {
    TEST("Context create/destroy");
    struct M68KEmuLibBase b = {};
    auto *c = M68KEmu_CreateContext(&b);
    CHK(c && c->mem, "null"); CHK(c->mem_size == M68KEMU_MEM_SIZE, "size");
    M68KEmu_DestroyContext(c); OK();
}
void t_heap() {
    TEST("Heap alloc/free/reuse");
    struct M68KEmuLibBase b = {}; auto *c = M68KEmu_CreateContext(&b);
    ULONG a = M68KEmu_HeapAlloc(c, 100, 0); CHK(a, "a1");
    ULONG a2 = M68KEmu_HeapAlloc(c, 200, 0); CHK(a2 && a2!=a, "a2");
    M68KEmu_HeapFree(c, a, 100);
    ULONG a3 = M68KEmu_HeapAlloc(c, 64, 0); CHK(a3, "a3");
    M68KEmu_DestroyContext(c); OK();
}
void t_stress() {
    TEST("Heap stress 1000 alloc/free");
    struct M68KEmuLibBase b = {}; auto *c = M68KEmu_CreateContext(&b);
    ULONG addrs[1000];
    for(int i=0;i<1000;i++) { addrs[i]=M68KEmu_HeapAlloc(c,64,0); CHK(addrs[i],"oom"); }
    for(int i=0;i<1000;i++) M68KEmu_HeapFree(c,addrs[i],64);
    CHK(M68KEmu_HeapAlloc(c,100000,0), "post-free"); M68KEmu_DestroyContext(c); OK();
}
void t_endian() {
    TEST("Big-endian read/write");
    struct M68KEmuLibBase b = {}; auto *c = M68KEmu_CreateContext(&b);
    m68k_write16(c,0x1000,0xCAFE); CHK(m68k_read16(c,0x1000)==0xCAFE,"16");
    CHK(c->mem[0x1000]==0xCA,"hi");
    m68k_write32(c,0x2000,0xDEADBEEF); CHK(m68k_read32(c,0x2000)==0xDEADBEEF,"32");
    M68KEmu_DestroyContext(c); OK();
}
void t_xlat() {
    TEST("Pointer translation");
    struct M68KEmuLibBase b = {}; auto *c = M68KEmu_CreateContext(&b);
    CHK(m68k_to_host(c,0)==NULL,"0→null"); CHK(host_to_m68k(c,NULL)==0,"null→0");
    CHK(m68k_to_host(c,0x1000)==c->mem+0x1000,"fwd");
    CHK(host_to_m68k(c,c->mem+0x2000)==0x2000,"rev");
    M68KEmu_DestroyContext(c); OK();
}
void t_libbase() {
    TEST("Fake lib base + A-line traps");
    struct M68KEmuLibBase b = {}; auto *c = M68KEmu_CreateContext(&b);
    ULONG a = M68KEmu_SetupLibBase(c,0,"test.library",10,m68kemu_thunks_exec,0,NULL,0);
    CHK(a,"addr"); CHK(c->num_libs==1,"nlibs");
    UWORD op = m68k_read16(c, a-6);
    CHK((op&0xF000)==0xA000,"aline");
    M68KEmu_DestroyContext(c); OK();
}

/* Keep guest sizes explicitly 32-bit even on a 64-bit host. The whole
   production memory manager above operates on this small guarded arena. */
#include <algorithm>
#include <array>
#include <cstdint>
static_assert(sizeof(ULONG) == 4, "the guest byte count must be 32-bit");

struct HeapFixture
{
    M68KEmuContext ctx = {};
    std::array<UBYTE, 0x2000> bytes;
    explicit HeapFixture(ULONG size = 256)
    {
        bytes.fill(0xA5);
        ctx.mem = bytes.data();
        ctx.mem_size = bytes.size();
        ctx.heap_start = ctx.heap_free = 0x1000;
        ctx.heap_end = ctx.heap_start + size;
        m68k_write32(&ctx, ctx.heap_start, size);
        m68k_write32(&ctx, ctx.heap_start + 4, 0);
    }
    bool restored() const
    {
        auto *c = const_cast<M68KEmuContext *>(&ctx);
        return ctx.heap_free == ctx.heap_start &&
            m68k_read32(c, ctx.heap_start) == ctx.heap_end - ctx.heap_start &&
            m68k_read32(c, ctx.heap_start + 4) == 0;
    }
};

void t_heap_overflow()
{
    TEST("Heap size overflow rejected atomically");
    bool ok = true;
    const ULONG requirement_sets[] = {0, MEMF_CLEAR, (ULONG)-1};
    const ULONG sizes[] = {
        0x7FFFFFFFu, 0x80000000u, 0xFFFFFFF0u, 0xFFFFFFF1u,
        0xFFFFFFF2u, 0xFFFFFFF3u, 0xFFFFFFF4u, 0xFFFFFFF5u,
        0xFFFFFFF6u, 0xFFFFFFF7u, 0xFFFFFFF8u, 0xFFFFFFF9u,
        0xFFFFFFFAu, 0xFFFFFFFBu, 0xFFFFFFFCu, 0xFFFFFFFDu,
        0xFFFFFFFEu, 0xFFFFFFFFu
    };
    for (ULONG requirements : requirement_sets)
        for (ULONG size : sizes)
        {
            HeapFixture f;
            const auto before = f.bytes;
            const ULONG head = f.ctx.heap_free;
            const ULONG result = M68KEmu_HeapAlloc(&f.ctx, size, requirements);
            if (result || f.ctx.heap_free != head || f.bytes != before)
            {
                std::printf("\n    size %08x flags %08x -> %08x, changed=%d",
                            unsigned(size), unsigned(requirements), unsigned(result),
                            f.ctx.heap_free != head || f.bytes != before);
                ok = false;
            }
        }
    CHK(ok, "an impossible allocation changed the heap or adjacent bytes");
    OK();
}

void t_heap_rounding()
{
    TEST("Heap rounding, exact fit and OOM");
    const ULONG sizes[] = {
        0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 31, 32, 231, 232, 233,
        235, 236, 237, 240, 244, 245, 248, 249, 250, 251, 252, 256
    };
    for (ULONG size : sizes)
    {
        HeapFixture f;
        const auto before = f.bytes;
        const uint64_t needed = ((uint64_t(size) + 3) / 4) * 4 + M68KEMU_HEAP_HDR;
        const ULONG result = M68KEmu_HeapAlloc(&f.ctx, size, 0);
        if (needed > 256)
        {
            CHK(!result && f.restored() && f.bytes == before, "normal OOM must not mutate the heap");
            continue;
        }
        const ULONG remainder = 256 - ULONG(needed);
        const ULONG actual = remainder >= M68KEMU_HEAP_MIN_SPLIT ? ULONG(needed) : 256;
        const ULONG header = remainder >= M68KEMU_HEAP_MIN_SPLIT ? f.ctx.heap_end - actual : f.ctx.heap_start;
        CHK(result == header + M68KEMU_HEAP_HDR && !(result & 3), "rounded allocation address");
        CHK(m68k_read32(&f.ctx, header) == actual && uint64_t(result) + size <= f.ctx.heap_end,
            "rounded size must hold all requested payload bytes");
        CHK(std::equal(before.begin(), before.begin() + f.ctx.heap_start, f.bytes.begin()) &&
            std::equal(before.begin() + f.ctx.heap_end, before.end(), f.bytes.begin() + f.ctx.heap_end),
            "allocator touched bytes outside its heap");
        M68KEmu_HeapFree(&f.ctx, result, size);
        CHK(f.restored(), "free restores the original free chunk");
    }
    OK();
}

void t_heap_zero_and_coalescence()
{
    TEST("Heap internal zero and coalescence");
    {
        HeapFixture f;
        const ULONG zero = M68KEmu_HeapAlloc(&f.ctx, 0, 0);
        CHK(zero == f.ctx.heap_end && m68k_read32(&f.ctx, zero - M68KEMU_HEAP_HDR) == M68KEMU_HEAP_HDR,
            "internal zero payload still reserves its header");
        CHK(m68k_read32(&f.ctx, f.ctx.heap_start) == 256 - M68KEMU_HEAP_HDR,
            "zero consumes only the header");
        M68KEmu_HeapFree(&f.ctx, zero, 0);
        CHK(f.restored(), "internal zero is still freeable");
    }
    {
        HeapFixture f;
        const ULONG a = M68KEmu_HeapAlloc(&f.ctx, 5, 0);
        const ULONG b = M68KEmu_HeapAlloc(&f.ctx, 1, 0);
        const ULONG z = M68KEmu_HeapAlloc(&f.ctx, 0, 0);
        CHK(a && b && z && a != b && a != z && b != z, "distinct live header and payload reservations");
        M68KEmu_HeapFree(&f.ctx, b, 1);
        M68KEmu_HeapFree(&f.ctx, a, 5);
        M68KEmu_HeapFree(&f.ctx, z, 0);
        CHK(f.restored(), "out-of-order frees coalesce both sides including a zero payload");
    }
    OK();
}

int main() {
    printf("m68kemu host-side tests\n=======================\n");
    t_ctx(); t_heap(); t_stress(); t_endian(); t_xlat(); t_libbase();
    t_heap_overflow(); t_heap_rounding(); t_heap_zero_and_coalescence();
    printf("\n%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
