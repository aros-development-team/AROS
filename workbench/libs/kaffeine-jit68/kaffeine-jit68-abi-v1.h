/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    kaffeine-jit68.library: version 1 of the interface between
    m68kemu.library (the host) and an optional 68k execution engine.

    Interface version 1.0, source revision 2. The kaffeine-jit68 engine
    implements it and has passed its checks on an AArch64 host and, through
    a probe of its library, on AROS AArch64 under QEMU. The specification is
    kaffeine-jit68-abi-v1.md beside this file; where a comment here is
    shorter, the specification governs.
*/

#ifndef KAFFEINE_JIT68_ABI_V1_H
#define KAFFEINE_JIT68_ABI_V1_H

#include <stddef.h>
#include <exec/types.h>

/* ---- identity and discovery ------------------------------------------- */

#define KJ68_LIBNAME     "kaffeine-jit68.library"
#define KJ68_LIBVERSION  1      /* lowest library version that has KJ68_GetEngine */
#define KJ68_ABI_MAJOR   1      /* a different major is incompatible */
#define KJ68_ABI_MINOR   0      /* later minors only append fields and bits */

/* The engine library has one function, the first after the standard
   vectors (LVO -30), declared by its .conf as
       APTR KJ68_GetEngine(ULONG abiMajor) (D0)
   It returns a const struct KJ68EngineOps * for that major, or NULL. The
   table stays valid while the host keeps the library open. */

/* Guest values are 32 bits wide on every target. */
typedef char KJ68_ULONG_must_be_32_bits[(sizeof(ULONG) == 4) ? 1 : -1];
typedef char KJ68_UWORD_must_be_16_bits[(sizeof(UWORD) == 2) ? 1 : -1];

/* ---- status of every operation ----------------------------------------- */

#define KJ68_OK              0
#define KJ68_ERR_VERSION     1  /* ABI major differs, or minor older than required */
#define KJ68_ERR_SIZE        2  /* a structure's size_of is below its KJ68_V1_SIZE_* */
#define KJ68_ERR_CAPS        3  /* a required capability is missing */
#define KJ68_ERR_NOMEM       4  /* allocation failed; nothing was created */
#define KJ68_ERR_NOHOST      5  /* the engine cannot run on this machine */
#define KJ68_ERR_ARG         6  /* an argument is invalid */
#define KJ68_ERR_STATE       7  /* not allowed now (see the contract, section 7) */
#define KJ68_ERR_UNSUPPORTED 8  /* outside the engine's model; nothing changed */

/* ---- capabilities ------------------------------------------------------- */

#define KJ68_CAP_BASE   0x00000001UL  /* every mandatory v1 operation; always set */
/* Reserved for later minors; a v1.0 engine sets none of them and a v1.0
   host never requires them: */
#define KJ68_CAP_IRQ    0x00000002UL  /* autovectored interrupts */
#define KJ68_CAP_RTE    0x00000004UL  /* RTE and exception frames */
#define KJ68_CAP_SRMODE 0x00000008UL  /* S, M and T changes, stack banks */
#define KJ68_CAP_FPU    0x00000010UL
#define KJ68_CAP_MMU    0x00000020UL
#define KJ68_CAP_SMP    0x00000040UL  /* qualified on several cores */

/* ---- registers ---------------------------------------------------------- */

#define KJ68_REG_D0    0    /* D0..D7 are 0..7 */
#define KJ68_REG_A0    8    /* A0..A7 are 8..15; A7 is the active stack pointer */
#define KJ68_REG_A7    15
#define KJ68_REG_PC    16
#define KJ68_REG_SR    17   /* only S=1, T=0, M=0 values: (sr & 0xF000) == 0x2000 */
#define KJ68_REG_COUNT 18

#define KJ68_SR_MODEL_MASK  0xF000
#define KJ68_SR_MODEL_VALUE 0x2000
#define KJ68_SR_DEFINED     0x271F  /* S, I2-I0, X, N, Z, V, C */
#define KJ68_SR_AT_CREATE   0x2700

/* ---- guest memory --------------------------------------------------------- */

#define KJ68_RANGE_READ_HOST  0x00000001UL  /* reads go to host->read; no fetch */
#define KJ68_RANGE_WRITE_HOST 0x00000002UL  /* writes go to host->write */
#define KJ68_MAX_RANGES       16

struct KJ68Range
{
    ULONG lo;      /* first guest address */
    ULONG hi;      /* first address after the range; lo < hi <= map size */
    ULONG flags;   /* KJ68_RANGE_*, at least one */
};

/* Owned by the host, which keeps it valid until domain_destroy returns and
   changes it only between engine calls, incrementing generation each time. */
struct KJ68MemoryMap
{
    ULONG size_of;            /* sizeof(struct KJ68MemoryMap) of the host */
    ULONG generation;
    UBYTE *base;              /* guest address 0; big-endian bytes */
    ULONG size;               /* guest [0, size) is memory; 0x1000 <= size */
    ULONG num_ranges;         /* <= KJ68_MAX_RANGES; sorted by lo, disjoint */
    struct KJ68Range ranges[KJ68_MAX_RANGES];
};

/* ---- host services (m68kemu.library provides them) ----------------------- */

/* read and write are leaf calls made from inside run(): they must not call
   any engine operation. width is 1 or 2; a longword is two word calls,
   the word at addr first. The return value is 0 to continue, 1 to stop the
   run at once (KJ68_STOP_HOST). pc is the address of the accessing
   instruction, for diagnostics only. */
struct KJ68HostServices
{
    ULONG size_of;            /* sizeof(struct KJ68HostServices) of the host */
    ULONG abi_major;          /* KJ68_ABI_MAJOR */
    ULONG abi_minor;          /* KJ68_ABI_MINOR the host was built with */
    ULONG (*read)(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width,
                  ULONG pc, ULONG *value);
    ULONG (*write)(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width,
                   ULONG value, ULONG pc);
    void  (*log)(APTR host_domain, CONST_STRPTR text);   /* may be NULL */
};

/* ---- the end of a run ----------------------------------------------------- */

#define KJ68_STOP_BUDGET      1  /* resumable: budget reached or slice ended */
#define KJ68_STOP_ALINE       2  /* resumable: PC at an A-line opcode, not executed */
#define KJ68_STOP_PC_OUTSIDE  3  /* resumable: PC even but not fetchable */
#define KJ68_STOP_EXCEPTION   4  /* resumable: instruction at PC raises vector */
#define KJ68_STOP_UNSUPPORTED 5  /* resumable: instruction at PC not executed */
#define KJ68_STOP_HOST        6  /* terminal: a host access asked to stop */
#define KJ68_STOP_FAULT       7  /* terminal: engine failure, see text */

#define KJ68_TEXT_SIZE 64

/* run() accepts 1 <= budget <= KJ68_MAX_BUDGET; an engine declares
   1 <= max_block_insns <= KJ68_MAX_BLOCK_INSNS. A run then retires at most
   budget + max_block_insns - 1 <= 0xFFFFFFFF instructions, so retired is
   always exact in 32 bits. */
#define KJ68_MAX_BUDGET      0xFFFF0000UL
#define KJ68_MAX_BLOCK_INSNS 0x00010000UL

struct KJ68Exit
{
    ULONG size_of;            /* the host sets sizeof(struct KJ68Exit) */
    ULONG reason;             /* KJ68_STOP_* */
    ULONG pc;                 /* the CPU's PC after the exit */
    ULONG retired;            /* instructions completed by this run */
    ULONG opcode;             /* first word at pc: ALINE, EXCEPTION, UNSUPPORTED */
    ULONG vector;             /* EXCEPTION: 68k vector number */
    ULONG address;            /* PC_OUTSIDE: the fetch address that failed */
    char  text[KJ68_TEXT_SIZE];   /* NUL-terminated ASCII, "" if none */
};

/* ---- counters, for evidence of what the engine did ------------------------ */

/* Every field saturates at its largest value instead of wrapping. */
struct KJ68Stats
{
    ULONG size_of;            /* the host sets sizeof(struct KJ68Stats) */
    ULONG cpus;               /* live CPU objects of the domain */
    ULONG blocks_translated;  /* since domain_create */
    ULONG insns_translated;
    ULONG runs;               /* run() calls that returned KJ68_OK */
    ULONG invalidations;
    ULONG code_resets;
    ULONG code_bytes;         /* executable bytes held now */
    UQUAD retired;            /* instructions retired by all runs */
};

/* ---- the engine ------------------------------------------------------------ */

struct KJ68Domain;            /* one per guest memory, engine-owned, opaque */
struct KJ68CPU;               /* one per guest register set, engine-owned, opaque */

struct KJ68EngineOps
{
    ULONG size_of;            /* sizeof(struct KJ68EngineOps) of the engine */
    ULONG abi_major;
    ULONG abi_minor;
    ULONG caps;               /* KJ68_CAP_* */
    ULONG max_block_insns;    /* 1..KJ68_MAX_BLOCK_INSNS; a run retires < budget + max_block_insns */
    CONST_STRPTR name;        /* engine name and version, static ASCII */

    LONG (*domain_create)(const struct KJ68HostServices *host, APTR host_domain,
                          struct KJ68MemoryMap *map, ULONG required_caps,
                          struct KJ68Domain **domain);
    LONG (*domain_destroy)(struct KJ68Domain *domain);
    LONG (*invalidate)(struct KJ68Domain *domain, ULONG addr, ULONG size);
    LONG (*cpu_create)(struct KJ68Domain *domain, APTR host_cpu, struct KJ68CPU **cpu);
    LONG (*cpu_destroy)(struct KJ68CPU *cpu);
    LONG (*get_reg)(struct KJ68CPU *cpu, ULONG reg, ULONG *value);
    LONG (*set_reg)(struct KJ68CPU *cpu, ULONG reg, ULONG value);
    LONG (*run)(struct KJ68CPU *cpu, ULONG budget, struct KJ68Exit *exit);
    LONG (*get_stats)(struct KJ68Domain *domain, struct KJ68Stats *stats);
};

#define KJ68_INVALIDATE_ALL 0xFFFFFFFFUL  /* size with addr 0: all guest memory */

/* The version 1.0 prefix of each structure: the byte just past its last
   version 1.0 member, as this target's compiler lays it out (trailing
   padding not included). A later minor appends members after it and never
   moves these, so the values below never change for a target. A size_of
   below the prefix is KJ68_ERR_SIZE; at or above it is accepted. */
#define KJ68_V1_END(type, member) \
    ((ULONG)(offsetof(type, member) + sizeof(((type *)0)->member)))
#define KJ68_V1_SIZE_ENGINEOPS KJ68_V1_END(struct KJ68EngineOps, get_stats)
#define KJ68_V1_SIZE_HOST      KJ68_V1_END(struct KJ68HostServices, log)
#define KJ68_V1_SIZE_MAP       KJ68_V1_END(struct KJ68MemoryMap, ranges)
#define KJ68_V1_SIZE_EXIT      KJ68_V1_END(struct KJ68Exit, text)
#define KJ68_V1_SIZE_STATS     KJ68_V1_END(struct KJ68Stats, retired)

#endif /* KAFFEINE_JIT68_ABI_V1_H */
