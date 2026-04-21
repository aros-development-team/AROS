/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * Author: Fabian Schmieder
 */
#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"
#include <cstdio>
#include <cstring>

static IPTR thunk_OpenLibrary(struct M68KEmuContext *ctx, void *cpu)
{
    const char *name = (const char *)m68k_to_host(ctx, M68KEmu_GetA(cpu, 1));
    printf("  [THUNK] OpenLibrary(\"%s\")\n", name ? name : "NULL");
    ULONG base = M68KEmu_FindLibBase(ctx, name);
    printf("  [THUNK] -> 0x%08X\n", base);
    return (IPTR)base;
}

static IPTR thunk_PutStr(struct M68KEmuContext *ctx, void *cpu)
{
    const char *str = (const char *)m68k_to_host(ctx, M68KEmu_GetD(cpu, 1));
    printf("  [THUNK] PutStr(\"%s\")\n", str ? str : "NULL");
    return 0;
}

const struct M68KThunkEntry m68kemu_thunks_exec[] = { { 552, thunk_OpenLibrary }, { 0, NULL } };
const ULONG m68kemu_thunks_exec_count = 1;

const struct M68KThunkEntry m68kemu_thunks_dos[] = { { 948, thunk_PutStr }, { 0, NULL } };
const ULONG m68kemu_thunks_dos_count = 1;

#include "../../rom/m68kemu/m68kemu_memory.c"
#include "../../rom/m68kemu/Moira/Moira.cpp"
#include "../../rom/m68kemu/Moira/MoiraDebugger.cpp"
#include "../../rom/m68kemu/m68kemu_moira.cpp"

int main()
{
    printf("=== test_hello thunk test ===\n");
    struct M68KEmuLibBase base = {};
    auto *ctx = M68KEmu_CreateContext(&base);

    ULONG sysbase = M68KEmu_SetupLibBase(ctx, 0, "exec.library", 512, m68kemu_thunks_exec, 1);
    printf("ExecBase=0x%08X jt_start=0x%08X\n", sysbase, ctx->libs[0].jt_start);

    ULONG dosbase = M68KEmu_SetupLibBase(ctx, 1, "dos.library", 512, m68kemu_thunks_dos, 1);
    printf("DOSBase=0x%08X jt_start=0x%08X\n", dosbase, ctx->libs[1].jt_start);

    m68k_write32(ctx, 4, sysbase);

    ULONG code = M68KEmu_HeapAlloc(ctx, 128, 0);
    ULONG p = code;

    m68k_write16(ctx, p, 0x2C78); p += 2;
    m68k_write16(ctx, p, 0x0004); p += 2;

    ULONG lea_a1 = p;
    m68k_write16(ctx, p, 0x43FA); p += 2;
    m68k_write16(ctx, p, 0x0000); p += 2;

    m68k_write16(ctx, p, 0x7000); p += 2;

    m68k_write16(ctx, p, 0x4EAE); p += 2;
    m68k_write16(ctx, p, 0xFDD8); p += 2;

    m68k_write16(ctx, p, 0x4A80); p += 2;

    ULONG beq = p;
    m68k_write16(ctx, p, 0x6700); p += 2;

    m68k_write16(ctx, p, 0x2C40); p += 2;

    ULONG lea_a0 = p;
    m68k_write16(ctx, p, 0x41FA); p += 2;
    m68k_write16(ctx, p, 0x0000); p += 2;

    m68k_write16(ctx, p, 0x2208); p += 2;

    m68k_write16(ctx, p, 0x4EAE); p += 2;
    m68k_write16(ctx, p, 0xFC4C); p += 2;

    m68k_write16(ctx, p, 0x7000); p += 2;
    m68k_write16(ctx, p, 0x4E75); p += 2;

    ULONG fail = p;
    m68k_write16(ctx, p, 0x7014); p += 2;
    m68k_write16(ctx, p, 0x4E75); p += 2;

    ULONG dosname = p;
    const char *dn = "dos.library";
    for (int i = 0; dn[i]; i++) ctx->mem[p++] = dn[i];
    ctx->mem[p++] = 0;

    ULONG hello = p;
    const char *hs = "Hello from m68k!\n";
    for (int i = 0; hs[i]; i++) ctx->mem[p++] = hs[i];
    ctx->mem[p++] = 0;

    int16_t off;
    off = (int16_t)(dosname - (lea_a1 + 2));
    m68k_write16(ctx, lea_a1 + 2, (uint16_t)off);
    off = (int16_t)(fail - (beq + 2));
    ctx->mem[beq + 1] = (uint8_t)(off & 0xFF);
    off = (int16_t)(hello - (lea_a0 + 2));
    m68k_write16(ctx, lea_a0 + 2, (uint16_t)off);

    ULONG ol_slot = sysbase - 552;
    printf("OpenLibrary slot @0x%08X: %04X\n", ol_slot, m68k_read16(ctx, ol_slot));

    ULONG ps_slot = dosbase - 948;
    printf("PutStr slot @0x%08X: %04X\n", ps_slot, m68k_read16(ctx, ps_slot));

    ctx->entry_point = code;
    ctx->m68k_argptr = 0;
    ctx->m68k_argsize = 0;
    ctx->m68k_sysbase = sysbase;

    printf("Executing...\n");
    LONG result = M68KEmu_Execute(ctx);
    printf("Result: %ld\n", result);

    M68KEmu_DestroyContext(ctx);
    return 0;
}
