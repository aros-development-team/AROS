/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder

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

int main() {
    printf("m68kemu host-side tests\n=======================\n");
    t_ctx(); t_heap(); t_stress(); t_endian(); t_xlat(); t_libbase();
    printf("\n%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
