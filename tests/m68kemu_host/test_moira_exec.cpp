/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder

    Host-side test: full Moira execution of m68k code.
    Tests the complete path: memory setup → load code → execute → trap → exit.
*/
#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

/* Stub thunk tables */
const struct M68KThunkEntry m68kemu_thunks_exec[] = { { 0, nullptr } };
const ULONG m68kemu_thunks_exec_count = 0;
const struct M68KThunkEntry m68kemu_thunks_dos[] = { { 0, nullptr } };
const ULONG m68kemu_thunks_dos_count = 0;

/* Include the C sources */
#include "../../rom/m68kemu/m68kemu_memory.c"

/* Include Moira */
#include "../../rom/m68kemu/Moira/Moira.cpp"
#include "../../rom/m68kemu/Moira/MoiraDebugger.cpp"

/* Include our Moira subclass */
#include "../../rom/m68kemu/m68kemu_moira.cpp"

#include <cstdio>
static int pass = 0, fail = 0;
#define TEST(n) printf("  %-50s ", n)
#define OK() do { printf("PASS\n"); pass++; } while(0)
#define NG(m) do { printf("FAIL: %s\n", m); fail++; return; } while(0)
#define CHK(c,m) do { if(!(c)) NG(m); } while(0)

void t_exec_moveq_rts()
{
    TEST("Execute MOVEQ #42,D0; RTS");
    struct M68KEmuLibBase base = {};
    auto *ctx = M68KEmu_CreateContext(&base);
    CHK(ctx, "ctx null");

    /* Write m68k code: MOVEQ #42,D0 (0x702A) + RTS (0x4E75) */
    ULONG code_addr = M68KEmu_HeapAlloc(ctx, 4, 0);
    CHK(code_addr, "alloc failed");
    m68k_write16(ctx, code_addr,     0x702A);  /* MOVEQ #42,D0 */
    m68k_write16(ctx, code_addr + 2, 0x4E75);  /* RTS */

    ctx->entry_point  = code_addr;
    ctx->m68k_argptr  = 0;
    ctx->m68k_argsize = 0;
    ctx->m68k_sysbase = 0;

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 42, "expected 42");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_exec_add()
{
    TEST("Execute MOVEQ #10,D0; ADDI.W #32,D0; RTS");
    struct M68KEmuLibBase base = {};
    auto *ctx = M68KEmu_CreateContext(&base);

    ULONG code_addr = M68KEmu_HeapAlloc(ctx, 8, 0);
    m68k_write16(ctx, code_addr,     0x700A);  /* MOVEQ #10,D0 */
    m68k_write16(ctx, code_addr + 2, 0x0640);  /* ADDI.W #xx,D0 */
    m68k_write16(ctx, code_addr + 4, 0x0020);  /* immediate: 32 */
    m68k_write16(ctx, code_addr + 6, 0x4E75);  /* RTS */

    ctx->entry_point = code_addr;
    ctx->m68k_argptr = ctx->m68k_argsize = ctx->m68k_sysbase = 0;

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 42, "expected 42");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_exec_loop()
{
    TEST("Execute loop: sum 1..10 = 55");
    struct M68KEmuLibBase base = {};
    auto *ctx = M68KEmu_CreateContext(&base);

    /* D0 = 0 (accumulator), D1 = 10 (counter)
       loop: ADD.W D1,D0; SUBQ.W #1,D1; BNE loop; RTS */
    ULONG a = M68KEmu_HeapAlloc(ctx, 16, 0);
    m68k_write16(ctx, a,      0x7000);  /* MOVEQ #0,D0 */
    m68k_write16(ctx, a + 2,  0x720A);  /* MOVEQ #10,D1 */
    /* loop: */
    m68k_write16(ctx, a + 4,  0xD041);  /* ADD.W D1,D0 */
    m68k_write16(ctx, a + 6,  0x5341);  /* SUBQ.W #1,D1 */
    m68k_write16(ctx, a + 8,  0x66FA);  /* BNE.S loop (-6) */
    m68k_write16(ctx, a + 10, 0x4E75);  /* RTS */

    ctx->entry_point = a;
    ctx->m68k_argptr = ctx->m68k_argsize = ctx->m68k_sysbase = 0;

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 55, "expected 55");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_exec_memory_access()
{
    TEST("Execute: write to memory, read back");
    struct M68KEmuLibBase base = {};
    auto *ctx = M68KEmu_CreateContext(&base);

    /* Allocate a data area and code area */
    ULONG data = M68KEmu_HeapAlloc(ctx, 4, 0);
    ULONG code = M68KEmu_HeapAlloc(ctx, 16, 0);

    /* LEA data,A0; MOVE.L #0xCAFE,D0; MOVE.L D0,(A0); MOVE.L (A0),D0; RTS */
    m68k_write16(ctx, code,      0x207C);  /* MOVEA.L #imm,A0 */
    m68k_write32(ctx, code + 2,  data);    /* immediate = data addr */
    m68k_write16(ctx, code + 6,  0x203C);  /* MOVE.L #imm,D0 */
    m68k_write32(ctx, code + 8,  0xCAFE);  /* immediate = 0xCAFE */
    m68k_write16(ctx, code + 12, 0x2080);  /* MOVE.L D0,(A0) */
    m68k_write16(ctx, code + 14, 0x2010);  /* MOVE.L (A0),D0 */
    m68k_write16(ctx, code + 16, 0x4E75);  /* RTS */

    ctx->entry_point = code;
    ctx->m68k_argptr = ctx->m68k_argsize = ctx->m68k_sysbase = 0;

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == (LONG)0xCAFE, "expected 0xCAFE");

    M68KEmu_DestroyContext(ctx);
    OK();
}

int main()
{
    printf("m68kemu Moira execution tests\n");
    printf("=============================\n");
    t_exec_moveq_rts();
    t_exec_add();
    t_exec_loop();
    t_exec_memory_access();
    printf("\n%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
