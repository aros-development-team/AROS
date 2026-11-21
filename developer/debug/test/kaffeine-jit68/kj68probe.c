/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A native probe of kaffeine-jit68.library through its interface alone,
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
        not execute.

    The counters of the engine show that it translated and ran code.
    Output: a line per check and a summary.
*/
#include <aros/debug.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "kaffeine-jit68-abi-v1.h"

#define GUEST_SIZE 0x10000UL
#define HOST_LO    0x8000UL
#define HOST_HI    0x9000UL
#define STACK      0x4000UL
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
        bug("KJ68PROBE FAIL: %s\n", what);
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

static ULONG reg(const struct KJ68EngineOps *ops, struct KJ68CPU *cpu, ULONG index)
{
    ULONG value = 0xDEADBEEF;

    ops->get_reg(cpu, index, &value);
    return value;
}

/* One run from pc with a budget; the exit is left in ex. */
static LONG run_from(const struct KJ68EngineOps *ops, struct KJ68CPU *cpu, ULONG pc, ULONG budget, struct KJ68Exit *ex)
{
    ex->size_of = sizeof(*ex);
    ex->reason = 0;
    ops->set_reg(cpu, KJ68_REG_PC, pc);
    return ops->run(cpu, budget, ex);
}

static void programs(const struct KJ68EngineOps *ops, struct KJ68Domain *domain, struct KJ68CPU *cpu, UBYTE *mem)
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
    struct KJ68Exit ex;
    struct KJ68Stats st;
    ULONG total, turns;
    LONG rc;

    put_words(mem, 0x1000, arith, 3);
    rc = run_from(ops, cpu, 0x1000, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_ALINE && ex.pc == 0x1004 && ex.opcode == 0xA000 && ex.retired == 2 &&
          reg(ops, cpu, KJ68_REG_D0) == 43 && reg(ops, cpu, KJ68_REG_PC) == 0x1004,
          "moveq and addq give 43 in D0 and the run ends at the A-line opcode, 2 instructions retired");
    Printf("     reason %ld, pc %lx, opcode %lx, retired %ld, D0 %ld\n", (LONG)ex.reason, (LONG)ex.pc, (LONG)ex.opcode,
           (LONG)ex.retired, (LONG)reg(ops, cpu, KJ68_REG_D0));

    put_words(mem, 0x1100, loop, 7);
    rc = run_from(ops, cpu, 0x1100, 100000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_ALINE && ex.pc == 0x110C && ex.retired == 2002 &&
          reg(ops, cpu, KJ68_REG_D0) == 1000 && (reg(ops, cpu, KJ68_REG_D0 + 1) & 0xFFFF) == 0xFFFF,
          "a DBRA loop of 1000 turns gives 1000 in D0, 2002 instructions retired in one run");
    Printf("     reason %ld, pc %lx, retired %ld, D0 %ld\n", (LONG)ex.reason, (LONG)ex.pc, (LONG)ex.retired,
           (LONG)reg(ops, cpu, KJ68_REG_D0));

    /* the same loop in runs of 100 instructions: each run may retire up to
       100 + max_block_insns - 1, and the sum is the same */
    total = 0;
    turns = 0;
    rc = run_from(ops, cpu, 0x1100, 100, &ex);
    while (rc == KJ68_OK && ex.reason == KJ68_STOP_BUDGET && turns < 100)
    {
        total += ex.retired;
        turns++;
        ex.size_of = sizeof(ex);
        rc = ops->run(cpu, 100, &ex);
    }
    total += ex.retired;
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_ALINE && total == 2002 && turns >= 1 &&
          reg(ops, cpu, KJ68_REG_D0) == 1000,
          "the same loop in runs of 100 instructions: resumed until the A-line opcode, 2002 retired in all");
    Printf("     resumed %ld times, retired %ld in all\n", (LONG)turns, (LONG)total);

    /* RTS to the address on the stack, which is outside memory */
    put_words(mem, 0x1200, rts, 1);
    mem[STACK] = 0xFF;
    mem[STACK + 1] = 0xDE;
    mem[STACK + 2] = 0xAD;
    mem[STACK + 3] = 0x00;
    ops->set_reg(cpu, KJ68_REG_A7, STACK);
    rc = run_from(ops, cpu, 0x1200, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_PC_OUTSIDE && ex.pc == SENTINEL && ex.address == SENTINEL &&
          ex.retired == 1 && reg(ops, cpu, KJ68_REG_A7) == STACK + 4,
          "RTS to an address outside memory ends the run there, with the stack popped");
    Printf("     reason %ld, pc %lx, address %lx, A7 %lx\n", (LONG)ex.reason, (LONG)ex.pc, (LONG)ex.address,
           (LONG)reg(ops, cpu, KJ68_REG_A7));

    /* a read and a write in the range of the callbacks */
    put_words(mem, 0x1300, ranged, 7);
    reads = 0;
    writes = 0;
    rc = run_from(ops, cpu, 0x1300, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_ALINE && reads == 1 && read_addr == 0x8000 && read_width == 2 &&
          (reg(ops, cpu, KJ68_REG_D0) & 0xFFFF) == 0x1234,
          "a word read in the host range comes from the callback, once");
    check(writes == 1 && write_addr == 0x8002 && write_width == 2 && (write_value & 0xFFFF) == 0x1234 &&
          mem[0x8002] == 0xEE && mem[0x8003] == 0xEE,
          "a word written in the host range goes to the callback, once, and not to memory");
    Printf("     reads %ld at %lx, writes %ld at %lx value %lx\n", (LONG)reads, (LONG)read_addr, (LONG)writes,
           (LONG)write_addr, (LONG)write_value);

    /* changed code: the new bytes run once the host has said so */
    put_words(mem, 0x2000, one, 2);
    rc = run_from(ops, cpu, 0x2000, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_ALINE && reg(ops, cpu, KJ68_REG_D0) == 1, "code at 2000 gives 1");
    put_words(mem, 0x2000, two, 2);
    rc = ops->invalidate(domain, 0x2000, 4);
    check(rc == KJ68_OK, "invalidate of the changed bytes is accepted");
    rc = run_from(ops, cpu, 0x2000, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_ALINE && reg(ops, cpu, KJ68_REG_D0) == 2,
          "after invalidate the changed code gives 2");

    /* reported, not executed */
    put_words(mem, 0x2100, divzero, 3);
    rc = run_from(ops, cpu, 0x2100, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_EXCEPTION && ex.vector == 5 && ex.pc == 0x2104 && ex.retired == 2 &&
          reg(ops, cpu, KJ68_REG_D0) == 10,
          "a division by zero is reported as vector 5 at the division, with D0 unchanged");
    Printf("     reason %ld, vector %ld, pc %lx, D0 %ld\n", (LONG)ex.reason, (LONG)ex.vector, (LONG)ex.pc,
           (LONG)reg(ops, cpu, KJ68_REG_D0));
    put_words(mem, 0x2200, illegal, 1);
    rc = run_from(ops, cpu, 0x2200, 1000, &ex);
    check(rc == KJ68_OK && ex.reason == KJ68_STOP_EXCEPTION && ex.vector == 4 && ex.pc == 0x2200 && ex.retired == 0,
          "ILLEGAL is reported as vector 4 and not executed");

    st.size_of = sizeof(st);
    rc = ops->get_stats(domain, &st);
    check(rc == KJ68_OK && st.cpus == 1 && st.blocks_translated > 0 && st.insns_translated > 0 && st.code_bytes > 0 &&
          st.invalidations >= 1 && st.retired >= 2002 + 2002,
          "the counters show translated blocks, executable code and the instructions retired");
    Printf("     blocks %ld, instructions translated %ld, runs %ld, invalidations %ld, code bytes %ld, retired %ld\n",
           (LONG)st.blocks_translated, (LONG)st.insns_translated, (LONG)st.runs, (LONG)st.invalidations,
           (LONG)st.code_bytes, (LONG)st.retired);
}

int main(void)
{
    struct Library *base;
    const struct KJ68EngineOps *ops;
    struct KJ68HostServices host;
    struct KJ68MemoryMap map;
    struct KJ68Domain *domain = NULL;
    struct KJ68CPU *cpu = NULL;
    UBYTE *mem;
    ULONG i;
    LONG rc;

    base = OpenLibrary(KJ68_LIBNAME, KJ68_LIBVERSION);
    check(base != NULL, "kaffeine-jit68.library opens");
    if (!base)
    {
        Printf("kj68probe: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
        return RETURN_FAIL;
    }
    Printf("     version %ld.%ld\n", (LONG)base->lib_Version, (LONG)base->lib_Revision);

    ops = AROS_LC1(APTR, KJ68_GetEngine, AROS_LCA(ULONG, KJ68_ABI_MAJOR + 1, D0), struct Library *, base, 5, KJ68);
    check(ops == NULL, "no engine for interface major 2");
    ops = AROS_LC1(APTR, KJ68_GetEngine, AROS_LCA(ULONG, KJ68_ABI_MAJOR, D0), struct Library *, base, 5, KJ68);
    check(ops != NULL && ops->size_of >= KJ68_V1_SIZE_ENGINEOPS && ops->abi_major == KJ68_ABI_MAJOR &&
          (ops->caps & KJ68_CAP_BASE) && ops->max_block_insns >= 1 && ops->max_block_insns <= KJ68_MAX_BLOCK_INSNS &&
          ops->name != NULL,
          "the engine of interface major 1 is there, with sizes, version and capabilities as the interface asks");

    mem = AllocMem(GUEST_SIZE, MEMF_ANY);
    check(mem != NULL, "64 KiB of guest memory");
    if (ops && ops->name && mem)
    {
        Printf("     %s, interface %ld.%ld, blocks of at most %ld instructions\n", ops->name, (LONG)ops->abi_major,
               (LONG)ops->abi_minor, (LONG)ops->max_block_insns);
        for (i = 0; i < GUEST_SIZE; i++)
            mem[i] = 0xEE;

        host.size_of = sizeof(host);
        host.abi_major = KJ68_ABI_MAJOR;
        host.abi_minor = KJ68_ABI_MINOR;
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
        map.ranges[0].flags = KJ68_RANGE_READ_HOST | KJ68_RANGE_WRITE_HOST;

        rc = ops->domain_create(&host, NULL, &map, KJ68_CAP_BASE, &domain);
        check(rc == KJ68_OK && domain != NULL, "a domain over that memory");
        if (rc != KJ68_OK)
            Printf("     domain_create gives %ld\n", rc);
        if (domain)
        {
            rc = ops->cpu_create(domain, NULL, &cpu);
            check(rc == KJ68_OK && cpu != NULL && reg(ops, cpu, KJ68_REG_SR) == KJ68_SR_AT_CREATE &&
                  reg(ops, cpu, KJ68_REG_PC) == 0 && reg(ops, cpu, KJ68_REG_D0) == 0,
                  "a CPU, with its registers as the interface says they start");
            if (cpu)
            {
                ops->set_reg(cpu, KJ68_REG_A7, STACK);
                programs(ops, domain, cpu, mem);
                check(ops->cpu_destroy(cpu) == KJ68_OK, "the CPU is destroyed");
            }
            check(ops->domain_destroy(domain) == KJ68_OK, "the domain is destroyed");
        }
    }
    if (mem)
        FreeMem(mem, GUEST_SIZE);
    CloseLibrary(base);

    Printf("kj68probe: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
    Flush(Output());
    return failures ? RETURN_FAIL : RETURN_OK;
}
