/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

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

extern "C" {
ULONG M68KEmu_OpenLibrary(struct M68KEmuContext *, const char *) { return 0; }
ULONG M68KEmu_LoadM68KLibrary(struct M68KEmuContext *, const char *) { return 0; }
}

/* Include the C sources */
#include "../../rom/m68kemu/m68kemu_memory.c"

/* Include Moira */
#include "../../rom/m68kemu/Moira/Moira/Moira.cpp"
#include "../../rom/m68kemu/Moira/Moira/MoiraDebugger.cpp"

/* Include our Moira subclass */
#include "../../rom/m68kemu/m68kemu_moira.cpp"

#include <cstdio>
#include <array>
#include <cstring>
#include <vector>
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

/* The M68040 containment address is 32 bits. Only $00DFF000-$00DFFFFF
   belongs to custom registers; the page at $01DFF000 is ordinary RAM. */
static std::array<ULONG, 3> beam_state(const M68KEmuContext &ctx)
{
    return {ctx.beam_v, ctx.beam_h, ctx.beam_lof};
}

void t_custom_address_decode()
{
    std::vector<UBYTE> memory(M68KEMU_MEM_SIZE, 0);
    struct M68KEmuContext ctx = {};
    ctx.mem = memory.data();
    ctx.mem_size = memory.size();
    AROSMoira cpu(&ctx);
    cpu.setModel(Model::M68040);
    const ULONG high = 0x01DFF016, outside = 0x02DFF016;
    const auto idle_beam = beam_state(ctx);
    auto check = [](const char *name, bool ok) {
        TEST(name);
        if (ok) { OK(); }
        else { printf("FAIL\n"); fail++; }
    };

    m68k_write16(&ctx, high, 0xA55A);
    m68k_write16(&ctx, 0x01DFFFFE, 0x6C93);
    check("High RAM: byte reads and lanes",
          cpu.read8(high) == 0xA5 && cpu.read8(high + 1) == 0x5A &&
          cpu.read8(0x01DFFFFF) == 0x93 && beam_state(ctx) == idle_beam);
    ctx.beam_v = ctx.beam_h = 0;
    check("High RAM: word reads and page end",
          cpu.read16(high) == 0xA55A && cpu.read16(0x01DFFFFE) == 0x6C93 &&
          beam_state(ctx) == idle_beam);
    ctx.beam_v = ctx.beam_h = 0;
    cpu.write8(high, 0x12);
    cpu.write8(high + 1, 0x34);
    check("High RAM: byte writes and lanes", m68k_read16(&ctx, high) == 0x1234);
    cpu.write16(high + 0x14, 0x0001); /* same low bits as VPOSW */
    check("High RAM: word write does not touch VPOSW",
          m68k_read16(&ctx, high + 0x14) == 1 && beam_state(ctx) == idle_beam);

    ctx.beam_v = ctx.beam_h = 0;
    check("Beyond arena: byte aliases read zero",
          cpu.read8(outside) == 0 && cpu.read8(0xFFDFF016) == 0 &&
          beam_state(ctx) == idle_beam);
    ctx.beam_v = ctx.beam_h = 0;
    check("Beyond arena: word aliases read zero",
          cpu.read16(outside) == 0 && cpu.read16(0xFFDFF016) == 0 &&
          beam_state(ctx) == idle_beam);
    ctx.beam_v = ctx.beam_h = 0;
    const auto before = memory;
    cpu.write8(outside, 0x56);
    cpu.write8(0xFFDFF017, 0x78);
    cpu.write16(outside + 0x14, 1);
    cpu.write16(0xFFDFF02A, 1);
    check("Beyond arena: writes have no RAM or chip effect",
          std::memcmp(ctx.mem, before.data(), before.size()) == 0 &&
          beam_state(ctx) == idle_beam);

    /* The real custom page must retain its register semantics even though
       these addresses are numerically inside the containment arena. */
    m68k_write16(&ctx, CUSTOM_BASE + 0x16, 0xA55A);
    check("Custom page: POTGOR byte and word reads",
          cpu.read8(CUSTOM_BASE + 0x16) == 0xFF &&
          cpu.read8(CUSTOM_BASE + 0x17) == 0 && cpu.read16(CUSTOM_BASE + 0x16) == 0xFF00);
    cpu.write8(CUSTOM_BASE + 0x16, 0x12);
    cpu.write8(CUSTOM_BASE + 0x17, 0x34);
    cpu.write16(CUSTOM_BASE + 0x16, 0x5678);
    ctx.beam_v = ctx.beam_h = 0;
    cpu.write16(CUSTOM_BASE + 0x2A, 1);
    check("Custom page: writes stay on custom registers",
          m68k_read16(&ctx, CUSTOM_BASE + 0x16) == 0xA55A &&
          m68k_read16(&ctx, CUSTOM_BASE + 0x2A) == 0 && ctx.beam_v == 0x100);
}

void t_exec_high_ram()
{
    TEST("Execute M68040 word write/read in high RAM");
    struct M68KEmuLibBase base = {};
    auto *ctx = M68KEmu_CreateContext(&base);
    CHK(ctx, "ctx null");
    const ULONG data = 0x01DFF016;
    ULONG code = M68KEmu_HeapAlloc(ctx, 16, 0);
    if (!code) { M68KEmu_DestroyContext(ctx); NG("alloc failed"); }
    /* MOVE.W #$1234,$01DFF016; MOVE.W $01DFF016,D0; RTS */
    const UWORD words[] = {0x33FC, 0x1234, 0x01DF, 0xF016,
                           0x3039, 0x01DF, 0xF016, 0x4E75};
    for (unsigned i = 0; i < sizeof(words)/sizeof(words[0]); i++)
        m68k_write16(ctx, code + i*2, words[i]);
    m68k_write16(ctx, data, 0xA55A);
    ctx->entry_point = code;
    ctx->m68k_argptr = ctx->m68k_argsize = ctx->m68k_sysbase = 0;
    LONG result = M68KEmu_Execute(ctx);
    bool ok = result == 0x1234 && m68k_read16(ctx, data) == 0x1234;
    M68KEmu_DestroyContext(ctx);
    CHK(ok, "high RAM was intercepted as custom registers");
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
    t_custom_address_decode();
    t_exec_high_ram();
    printf("\n%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
