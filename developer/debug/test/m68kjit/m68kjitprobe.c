/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A native probe of m68kjit.library through its interface alone,
    without m68kemu.library: it opens the library, asks for the engine of
    interface version 1, makes a domain over 64 KiB of its own memory with
    one address range that goes to its callbacks, and runs small 68k
    programs whose results it knows:

      - arithmetic, ended by an A-line opcode;
      - a DBRA loop of 1000 turns, in one run and in runs of 100
        instructions;
      - RTS to an address outside memory;
      - a word read and a word written in the range of the callbacks;
      - changed code, before and after invalidate;
      - a division by zero and ILLEGAL, which the engine reports and does
        not execute;
      - 2500 blocks in a row, more than the engine's block table starts
        with, so that the engine takes larger memory.

    The counters of the engine show that it translated and ran code.
    Output: a line per check and a summary.
*/
#include <aros/debug.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "m68kemu_engine.h"

#define M68KJIT_LIBNAME "m68kjit.library"

#define GUEST_SIZE 0x10000UL
#define HOST_LO    0x8000UL
#define HOST_HI    0x9000UL
#define STACK      0x4000UL
#define GROW_AT    0xA000UL     /* 2500 blocks of 6 bytes from here end below GUEST_SIZE */
#define GROW_BLOCKS 2500UL
#define SENTINEL   0xFFDEAD00UL

static ULONG checks;
static ULONG failures;

/* What the callbacks saw. */
static ULONG reads, writes, read_addr, read_width, write_addr, write_width, write_value;

static void check(BOOL ok, CONST_STRPTR what)
{
    checks++;
    if (!ok)
    {
        bug("M68KJITPROBE FAIL: %s\n", what);
        failures++;
    }
    Printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    Flush(Output());
}

static ULONG host_read(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width, ULONG pc, ULONG *value)
{
    (void)host_domain;
    (void)host_cpu;
    (void)pc;
    reads++;
    read_addr = addr;
    read_width = width;
    *value = 0x1234;
    return 0;
}

static ULONG host_write(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width, ULONG value, ULONG pc)
{
    (void)host_domain;
    (void)host_cpu;
    (void)pc;
    writes++;
    write_addr = addr;
    write_width = width;
    write_value = value;
    return 0;
}

/* 68k words, big-endian, at a guest address. */
static void put_words(UBYTE *mem, ULONG addr, const UWORD *words, ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        mem[addr + 2 * i] = (UBYTE)(words[i] >> 8);
        mem[addr + 2 * i + 1] = (UBYTE)words[i];
    }
}

static ULONG reg(const struct M68KEngOps *ops, struct M68KEngCPU *cpu, ULONG index)
{
    ULONG value = 0xDEADBEEF;

    ops->get_reg(cpu, index, &value);
    return value;
}

/* One run from pc with a budget; the exit is left in ex. */
static LONG run_from(const struct M68KEngOps *ops, struct M68KEngCPU *cpu, ULONG pc, ULONG budget, struct M68KEngExit *ex)
{
    ex->size_of = sizeof(*ex);
    ex->reason = 0;
    ops->set_reg(cpu, M68KENG_REG_PC, pc);
    return ops->run(cpu, budget, ex);
}

static void programs(const struct M68KEngOps *ops, struct M68KEngDomain *domain, struct M68KEngCPU *cpu, UBYTE *mem)
{
    /* moveq #42,d0; addq.l #1,d0; A-line */
    static const UWORD arith[] = { 0x702A, 0x5280, 0xA000 };
    /* moveq #0,d0; move.w #999,d1; loop: addq.l #1,d0; dbra d1,loop; A-line */
    static const UWORD loop[] = { 0x7000, 0x323C, 0x03E7, 0x5280, 0x51C9, 0xFFFC, 0xA000 };
    static const UWORD rts[] = { 0x4E75 };
    /* move.w ($8000).l,d0; move.w d0,($8002).l; A-line */
    static const UWORD ranged[] = { 0x3039, 0x0000, 0x8000, 0x33C0, 0x0000, 0x8002, 0xA000 };
    static const UWORD one[] = { 0x7001, 0xA000 };      /* moveq #1,d0; A-line */
    static const UWORD two[] = { 0x7002, 0xA000 };      /* moveq #2,d0; A-line */
    /* moveq #10,d0; moveq #0,d1; divu.w d1,d0 */
    static const UWORD divzero[] = { 0x700A, 0x7200, 0x80C1 };
    static const UWORD illegal[] = { 0x4AFC };
    struct M68KEngExit ex;
    struct M68KEngStats st;
    ULONG code_bytes, at;
    ULONG total, turns;
    LONG rc;

    put_words(mem, 0x1000, arith, 3);
    rc = run_from(ops, cpu, 0x1000, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && ex.pc == 0x1004 && ex.opcode == 0xA000 && ex.retired == 2 &&
          reg(ops, cpu, M68KENG_REG_D0) == 43 && reg(ops, cpu, M68KENG_REG_PC) == 0x1004,
          "moveq and addq give 43 in D0 and the run ends at the A-line opcode, 2 instructions retired");
    Printf("     reason %ld, pc %lx, opcode %lx, retired %ld, D0 %ld\n", (LONG)ex.reason, (LONG)ex.pc, (LONG)ex.opcode,
           (LONG)ex.retired, (LONG)reg(ops, cpu, M68KENG_REG_D0));

    put_words(mem, 0x1100, loop, 7);
    rc = run_from(ops, cpu, 0x1100, 100000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && ex.pc == 0x110C && ex.retired == 2002 &&
          reg(ops, cpu, M68KENG_REG_D0) == 1000 && (reg(ops, cpu, M68KENG_REG_D0 + 1) & 0xFFFF) == 0xFFFF,
          "a DBRA loop of 1000 turns gives 1000 in D0, 2002 instructions retired in one run");
    Printf("     reason %ld, pc %lx, retired %ld, D0 %ld\n", (LONG)ex.reason, (LONG)ex.pc, (LONG)ex.retired,
           (LONG)reg(ops, cpu, M68KENG_REG_D0));

    /* the same loop in runs of 100 instructions: each run may retire up to
       100 + max_block_insns - 1, and the sum is the same */
    total = 0;
    turns = 0;
    rc = run_from(ops, cpu, 0x1100, 100, &ex);
    while (rc == M68KENG_OK && ex.reason == M68KENG_STOP_BUDGET && turns < 100)
    {
        total += ex.retired;
        turns++;
        ex.size_of = sizeof(ex);
        rc = ops->run(cpu, 100, &ex);
    }
    total += ex.retired;
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && total == 2002 && turns >= 1 &&
          reg(ops, cpu, M68KENG_REG_D0) == 1000,
          "the same loop in runs of 100 instructions: resumed until the A-line opcode, 2002 retired in all");
    Printf("     resumed %ld times, retired %ld in all\n", (LONG)turns, (LONG)total);

    /* RTS to the address on the stack, which is outside memory */
    put_words(mem, 0x1200, rts, 1);
    mem[STACK] = 0xFF;
    mem[STACK + 1] = 0xDE;
    mem[STACK + 2] = 0xAD;
    mem[STACK + 3] = 0x00;
    ops->set_reg(cpu, M68KENG_REG_A7, STACK);
    rc = run_from(ops, cpu, 0x1200, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_PC_OUTSIDE && ex.pc == SENTINEL && ex.address == SENTINEL &&
          ex.retired == 1 && reg(ops, cpu, M68KENG_REG_A7) == STACK + 4,
          "RTS to an address outside memory ends the run there, with the stack popped");
    Printf("     reason %ld, pc %lx, address %lx, A7 %lx\n", (LONG)ex.reason, (LONG)ex.pc, (LONG)ex.address,
           (LONG)reg(ops, cpu, M68KENG_REG_A7));

    /* a read and a write in the range of the callbacks */
    put_words(mem, 0x1300, ranged, 7);
    reads = 0;
    writes = 0;
    rc = run_from(ops, cpu, 0x1300, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && reads == 1 && read_addr == 0x8000 && read_width == 2 &&
          (reg(ops, cpu, M68KENG_REG_D0) & 0xFFFF) == 0x1234,
          "a word read in the host range comes from the callback, once");
    check(writes == 1 && write_addr == 0x8002 && write_width == 2 && (write_value & 0xFFFF) == 0x1234 &&
          mem[0x8002] == 0xEE && mem[0x8003] == 0xEE,
          "a word written in the host range goes to the callback, once, and not to memory");
    Printf("     reads %ld at %lx, writes %ld at %lx value %lx\n", (LONG)reads, (LONG)read_addr, (LONG)writes,
           (LONG)write_addr, (LONG)write_value);

    /* changed code: the new bytes run once the host has said so */
    put_words(mem, 0x2000, one, 2);
    rc = run_from(ops, cpu, 0x2000, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && reg(ops, cpu, M68KENG_REG_D0) == 1, "code at 2000 gives 1");
    put_words(mem, 0x2000, two, 2);
    rc = ops->invalidate(domain, 0x2000, 4);
    check(rc == M68KENG_OK, "invalidate of the changed bytes is accepted");
    rc = run_from(ops, cpu, 0x2000, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && reg(ops, cpu, M68KENG_REG_D0) == 2,
          "after invalidate the changed code gives 2");

    /* reported, not executed */
    put_words(mem, 0x2100, divzero, 3);
    rc = run_from(ops, cpu, 0x2100, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_EXCEPTION && ex.vector == 5 && ex.pc == 0x2104 && ex.retired == 2 &&
          reg(ops, cpu, M68KENG_REG_D0) == 10,
          "a division by zero is reported as vector 5 at the division, with D0 unchanged");
    Printf("     reason %ld, vector %ld, pc %lx, D0 %ld\n", (LONG)ex.reason, (LONG)ex.vector, (LONG)ex.pc,
           (LONG)reg(ops, cpu, M68KENG_REG_D0));
    put_words(mem, 0x2200, illegal, 1);
    rc = run_from(ops, cpu, 0x2200, 1000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_EXCEPTION && ex.vector == 4 && ex.pc == 0x2200 && ex.retired == 0,
          "ILLEGAL is reported as vector 4 and not executed");

    st.size_of = sizeof(st);
    rc = ops->get_stats(domain, &st);
    check(rc == M68KENG_OK && st.cpus == 1 && st.blocks_translated > 0 && st.insns_translated > 0 && st.code_bytes > 0 &&
          st.invalidations >= 1 && st.retired >= 2002 + 2002,
          "the counters show translated blocks, executable code and the instructions retired");
    Printf("     blocks %ld, instructions translated %ld, runs %ld, invalidations %ld, code bytes %ld, retired %ld\n",
           (LONG)st.blocks_translated, (LONG)st.insns_translated, (LONG)st.runs, (LONG)st.invalidations,
           (LONG)st.code_bytes, (LONG)st.retired);

    /* More blocks than the block table starts with: the engine takes a
       larger table and larger code memory when it can have them, drops
       what it translated, and the program still gives its result. moveq #0,d0; then 2500 times
       addq.l #1,d0; bra.s to the next one; nop; and an A-line opcode. */
    code_bytes = st.code_bytes;
    at = GROW_AT;
    mem[at] = 0x70; mem[at + 1] = 0x00;
    for (at += 2; at < GROW_AT + 2 + 6 * GROW_BLOCKS; at += 6)
    {
        static const UWORD unit[] = { 0x5280, 0x6002, 0x4E71 };

        put_words(mem, at, unit, 3);
    }
    mem[at] = 0xA0; mem[at + 1] = 0x00;
    rc = run_from(ops, cpu, GROW_AT, 100000, &ex);
    check(rc == M68KENG_OK && ex.reason == M68KENG_STOP_ALINE && ex.pc == at && reg(ops, cpu, M68KENG_REG_D0) == GROW_BLOCKS,
          "2500 blocks in a row give 2500");
    st.size_of = sizeof(st);
    rc = ops->get_stats(domain, &st);
    check(rc == M68KENG_OK && st.code_bytes >= code_bytes && st.code_resets >= 1,
          "the engine made room for them with a code reset");
    Printf("     code bytes %ld, then %ld (%s); code resets %ld\n", (LONG)code_bytes, (LONG)st.code_bytes,
           st.code_bytes > code_bytes ? "larger code memory" : "no memory for a larger one", (LONG)st.code_resets);
}

int main(void)
{
    struct Library *base;
    const struct M68KEngOps *ops;
    struct M68KEngHost host;
    struct M68KEngMap map;
    struct M68KEngDomain *domain = NULL;
    struct M68KEngCPU *cpu = NULL;
    UBYTE *mem;
    BOOL nohost = FALSE;

    base = OpenLibrary(M68KJIT_LIBNAME, 1);
    check(base != NULL, "m68kjit.library opens");
    if (!base)
    {
        Printf("m68kjitprobe: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
        return RETURN_FAIL;
    }
    Printf("     version %ld.%ld\n", (LONG)base->lib_Version, (LONG)base->lib_Revision);

    ops = AROS_LC1(APTR, M68KJit_GetEngine, AROS_LCA(ULONG, M68KENG_ABI_MAJOR + 1, D0), struct Library *, base, 5, M68KJit);
    check(ops == NULL, "no engine for interface major 2");
    ops = AROS_LC1(APTR, M68KJit_GetEngine, AROS_LCA(ULONG, M68KENG_ABI_MAJOR, D0), struct Library *, base, 5, M68KJit);
    check(ops != NULL && ops->size_of >= M68KENG_V1_SIZE_OPS && ops->abi_major == M68KENG_ABI_MAJOR &&
          (ops->caps & M68KENG_CAP_BASE) && ops->max_block_insns >= 1 && ops->max_block_insns <= M68KENG_MAX_BLOCK_INSNS &&
          ops->name != NULL,
          "the engine of interface major 1 is there, with sizes, version and capabilities as the interface asks");

    mem = AllocMem(GUEST_SIZE, MEMF_ANY);
    check(mem != NULL, "64 KiB of guest memory");
    if (ops && ops->name && mem)
    {
        ULONG i;
        LONG rc;

        Printf("     %s, interface %ld.%ld, blocks of at most %ld instructions\n", ops->name, (LONG)ops->abi_major,
               (LONG)ops->abi_minor, (LONG)ops->max_block_insns);
        for (i = 0; i < GUEST_SIZE; i++)
            mem[i] = 0xEE;

        host.size_of = sizeof(host);
        host.abi_major = M68KENG_ABI_MAJOR;
        host.abi_minor = M68KENG_ABI_MINOR;
        host.read = host_read;
        host.write = host_write;
        host.log = NULL;
        map.size_of = sizeof(map);
        map.generation = 1;
        map.base = mem;
        map.size = GUEST_SIZE;
        map.num_ranges = 1;
        map.ranges[0].lo = HOST_LO;
        map.ranges[0].hi = HOST_HI;
        map.ranges[0].flags = M68KENG_RANGE_READ_HOST | M68KENG_RANGE_WRITE_HOST;

        rc = ops->domain_create(&host, NULL, &map, M68KENG_CAP_BASE, &domain);
        if (rc == M68KENG_ERR_NOHOST)
        {
            /* a hosted AROS: the engine is there and runs no code */
            Printf("     the engine cannot run code on this machine: its programs are not run\n");
            nohost = TRUE;
        }
        else
        {
            check(rc == M68KENG_OK && domain != NULL, "a domain over that memory");
            if (rc != M68KENG_OK)
                Printf("     domain_create gives %ld\n", rc);
        }
        if (domain)
        {
            rc = ops->cpu_create(domain, NULL, &cpu);
            check(rc == M68KENG_OK && cpu != NULL && reg(ops, cpu, M68KENG_REG_SR) == M68KENG_SR_AT_CREATE &&
                  reg(ops, cpu, M68KENG_REG_PC) == 0 && reg(ops, cpu, M68KENG_REG_D0) == 0,
                  "a CPU, with its registers as the interface says they start");
            if (cpu)
            {
                ops->set_reg(cpu, M68KENG_REG_A7, STACK);
                programs(ops, domain, cpu, mem);
                check(ops->cpu_destroy(cpu) == M68KENG_OK, "the CPU is destroyed");
            }
            check(ops->domain_destroy(domain) == M68KENG_OK, "the domain is destroyed");
        }
    }
    if (mem)
        FreeMem(mem, GUEST_SIZE);
    CloseLibrary(base);

    Printf("m68kjitprobe: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
    Flush(Output());
    return failures ? RETURN_FAIL : nohost ? RETURN_WARN : RETURN_OK;
}
