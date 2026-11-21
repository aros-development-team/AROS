/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library - the interface between the library (the host) and an
    optional 68k execution engine, version 1.0.

    An engine is a separate library. The specification is ENGINE.md beside
    this file; where a comment here is shorter, the specification governs.
*/

#ifndef M68KEMU_ENGINE_H
#define M68KEMU_ENGINE_H

#include <stddef.h>
#include <exec/types.h>

/* ---- identity and discovery ------------------------------------------- */

#define M68KENG_ABI_MAJOR   1      /* a different major is incompatible */
#define M68KENG_ABI_MINOR   0      /* later minors only append fields and bits */

/* An engine library has this function, the first after the standard
   vectors (LVO -30), declared by its .conf as
       APTR GetEngine(ULONG abiMajor) (D0)
   under a name of its own. It returns a const struct M68KEngOps * for that
   major, or NULL. The table stays valid while the host keeps the library
   open. */

/* Guest values are 32 bits wide on every target. */
typedef char M68KENG_ULONG_must_be_32_bits[(sizeof(ULONG) == 4) ? 1 : -1];
typedef char M68KENG_UWORD_must_be_16_bits[(sizeof(UWORD) == 2) ? 1 : -1];

/* ---- status of every operation ----------------------------------------- */

#define M68KENG_OK              0
#define M68KENG_ERR_VERSION     1  /* ABI major differs, or minor older than required */
#define M68KENG_ERR_SIZE        2  /* a structure's size_of is below its M68KENG_V1_SIZE_* */
#define M68KENG_ERR_CAPS        3  /* a required capability is missing */
#define M68KENG_ERR_NOMEM       4  /* allocation failed; nothing was created */
#define M68KENG_ERR_NOHOST      5  /* the engine cannot run on this machine */
#define M68KENG_ERR_ARG         6  /* an argument is invalid */
#define M68KENG_ERR_STATE       7  /* not allowed now (see the contract, section 7) */
#define M68KENG_ERR_UNSUPPORTED 8  /* outside the engine's model; nothing changed */

/* ---- capabilities ------------------------------------------------------- */

#define M68KENG_CAP_BASE   0x00000001UL  /* every mandatory v1 operation; always set */
/* Reserved for later minors; a v1.0 engine sets none of them and a v1.0
   host never requires them: */
#define M68KENG_CAP_IRQ    0x00000002UL  /* autovectored interrupts */
#define M68KENG_CAP_RTE    0x00000004UL  /* RTE and exception frames */
#define M68KENG_CAP_SRMODE 0x00000008UL  /* S, M and T changes, stack banks */
#define M68KENG_CAP_FPU    0x00000010UL
#define M68KENG_CAP_MMU    0x00000020UL
#define M68KENG_CAP_SMP    0x00000040UL  /* qualified on several cores */

/* ---- registers ---------------------------------------------------------- */

#define M68KENG_REG_D0    0    /* D0..D7 are 0..7 */
#define M68KENG_REG_A0    8    /* A0..A7 are 8..15; A7 is the active stack pointer */
#define M68KENG_REG_A7    15
#define M68KENG_REG_PC    16
#define M68KENG_REG_SR    17   /* only S=1, T=0, M=0 values: (sr & 0xF000) == 0x2000 */
#define M68KENG_REG_COUNT 18

#define M68KENG_SR_MODEL_MASK  0xF000
#define M68KENG_SR_MODEL_VALUE 0x2000
#define M68KENG_SR_DEFINED     0x271F  /* S, I2-I0, X, N, Z, V, C */
#define M68KENG_SR_AT_CREATE   0x2700

/* ---- guest memory --------------------------------------------------------- */

#define M68KENG_RANGE_READ_HOST  0x00000001UL  /* reads go to host->read; no fetch */
#define M68KENG_RANGE_WRITE_HOST 0x00000002UL  /* writes go to host->write */
#define M68KENG_MAX_RANGES       16

struct M68KEngRange
{
    ULONG lo;      /* first guest address */
    ULONG hi;      /* first address after the range; lo < hi <= map size */
    ULONG flags;   /* M68KENG_RANGE_*, at least one */
};

/* Owned by the host, which keeps it valid until domain_destroy returns and
   changes it only between engine calls, incrementing generation each time. */
struct M68KEngMap
{
    ULONG size_of;            /* sizeof(struct M68KEngMap) of the host */
    ULONG generation;
    UBYTE *base;              /* guest address 0; big-endian bytes */
    ULONG size;               /* guest [0, size) is memory; 0x1000 <= size */
    ULONG num_ranges;         /* <= M68KENG_MAX_RANGES; sorted by lo, disjoint */
    struct M68KEngRange ranges[M68KENG_MAX_RANGES];
};

/* ---- host services (m68kemu.library provides them) ----------------------- */

/* read and write are leaf calls made from inside run(): they must not call
   any engine operation. width is 1 or 2; a longword is two word calls,
   the word at addr first. The return value is 0 to continue, 1 to stop the
   run at once (M68KENG_STOP_HOST). pc is the address of the accessing
   instruction, for diagnostics only. */
struct M68KEngHost
{
    ULONG size_of;            /* sizeof(struct M68KEngHost) of the host */
    ULONG abi_major;          /* M68KENG_ABI_MAJOR */
    ULONG abi_minor;          /* M68KENG_ABI_MINOR the host was built with */
    ULONG (*read)(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width,
                  ULONG pc, ULONG *value);
    ULONG (*write)(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width,
                   ULONG value, ULONG pc);
    void  (*log)(APTR host_domain, CONST_STRPTR text);   /* may be NULL */
};

/* ---- the end of a run ----------------------------------------------------- */

#define M68KENG_STOP_BUDGET      1  /* resumable: budget reached or slice ended */
#define M68KENG_STOP_ALINE       2  /* resumable: PC at an A-line opcode, not executed */
#define M68KENG_STOP_PC_OUTSIDE  3  /* resumable: PC even but not fetchable */
#define M68KENG_STOP_EXCEPTION   4  /* resumable: instruction at PC raises vector */
#define M68KENG_STOP_UNSUPPORTED 5  /* resumable: instruction at PC not executed */
#define M68KENG_STOP_HOST        6  /* terminal: a host access asked to stop */
#define M68KENG_STOP_FAULT       7  /* terminal: engine failure, see text */

#define M68KENG_TEXT_SIZE 64

/* run() accepts 1 <= budget <= M68KENG_MAX_BUDGET; an engine declares
   1 <= max_block_insns <= M68KENG_MAX_BLOCK_INSNS. A run then retires at most
   budget + max_block_insns - 1 <= 0xFFFFFFFF instructions, so retired is
   always exact in 32 bits. */
#define M68KENG_MAX_BUDGET      0xFFFF0000UL
#define M68KENG_MAX_BLOCK_INSNS 0x00010000UL

struct M68KEngExit
{
    ULONG size_of;            /* the host sets sizeof(struct M68KEngExit) */
    ULONG reason;             /* M68KENG_STOP_* */
    ULONG pc;                 /* the CPU's PC after the exit */
    ULONG retired;            /* instructions completed by this run */
    ULONG opcode;             /* first word at pc: ALINE, EXCEPTION, UNSUPPORTED */
    ULONG vector;             /* EXCEPTION: 68k vector number */
    ULONG address;            /* PC_OUTSIDE: the fetch address that failed */
    char  text[M68KENG_TEXT_SIZE];   /* NUL-terminated ASCII, "" if none */
};

/* ---- counters, for evidence of what the engine did ------------------------ */

/* Every field saturates at its largest value instead of wrapping. An engine
   that does not translate leaves blocks_translated, insns_translated,
   code_resets and code_bytes at 0. */
struct M68KEngStats
{
    ULONG size_of;            /* the host sets sizeof(struct M68KEngStats) */
    ULONG cpus;               /* live CPU objects of the domain */
    ULONG blocks_translated;  /* since domain_create */
    ULONG insns_translated;
    ULONG runs;               /* run() calls that returned M68KENG_OK */
    ULONG invalidations;
    ULONG code_resets;
    ULONG code_bytes;         /* executable bytes held now */
    UQUAD retired;            /* instructions retired by all runs */
};

/* ---- the engine ------------------------------------------------------------ */

struct M68KEngDomain;            /* one per guest memory, engine-owned, opaque */
struct M68KEngCPU;               /* one per guest register set, engine-owned, opaque */

struct M68KEngOps
{
    ULONG size_of;            /* sizeof(struct M68KEngOps) of the engine */
    ULONG abi_major;
    ULONG abi_minor;
    ULONG caps;               /* M68KENG_CAP_* */
    ULONG max_block_insns;    /* 1..M68KENG_MAX_BLOCK_INSNS; a run retires < budget + max_block_insns */
    CONST_STRPTR name;        /* engine name and version, static ASCII */

    LONG (*domain_create)(const struct M68KEngHost *host, APTR host_domain,
                          struct M68KEngMap *map, ULONG required_caps,
                          struct M68KEngDomain **domain);
    LONG (*domain_destroy)(struct M68KEngDomain *domain);
    LONG (*invalidate)(struct M68KEngDomain *domain, ULONG addr, ULONG size);
    LONG (*cpu_create)(struct M68KEngDomain *domain, APTR host_cpu, struct M68KEngCPU **cpu);
    LONG (*cpu_destroy)(struct M68KEngCPU *cpu);
    LONG (*get_reg)(struct M68KEngCPU *cpu, ULONG reg, ULONG *value);
    LONG (*set_reg)(struct M68KEngCPU *cpu, ULONG reg, ULONG value);
    LONG (*run)(struct M68KEngCPU *cpu, ULONG budget, struct M68KEngExit *exit);
    LONG (*get_stats)(struct M68KEngDomain *domain, struct M68KEngStats *stats);
};

#define M68KENG_INVALIDATE_ALL 0xFFFFFFFFUL  /* size with addr 0: all guest memory */

/* The version 1.0 prefix of each structure: the byte just past its last
   version 1.0 member, as this target's compiler lays it out (trailing
   padding not included). A later minor appends members after it and never
   moves these, so the values below never change for a target. A size_of
   below the prefix is M68KENG_ERR_SIZE; at or above it is accepted. */
#define M68KENG_V1_END(type, member) \
    ((ULONG)(offsetof(type, member) + sizeof(((type *)0)->member)))
#define M68KENG_V1_SIZE_OPS M68KENG_V1_END(struct M68KEngOps, get_stats)
#define M68KENG_V1_SIZE_HOST      M68KENG_V1_END(struct M68KEngHost, log)
#define M68KENG_V1_SIZE_MAP       M68KENG_V1_END(struct M68KEngMap, ranges)
#define M68KENG_V1_SIZE_EXIT      M68KENG_V1_END(struct M68KEngExit, text)
#define M68KENG_V1_SIZE_STATS     M68KENG_V1_END(struct M68KEngStats, retired)

#endif /* M68KEMU_ENGINE_H */
