/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kjit engine: the AArch64 translator. It decodes 68k instructions
    and emits AArch64 code for them: guest memory through the host's map,
    an exit to the host wherever the host has to act, translations shared
    by the CPUs of a domain.
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef M68KJIT_TEST_SEAMS
#include <stdio.h>              /* the encoder self test of the host checks */
#endif
#include "m68kjit_intern.h"
#include "m68kjit_platform.h"

/* translated code runs on AArch64, where the platform can run it */
#if defined(__aarch64__) && M68KJIT_PLAT_RUNS
#define HJ_RUNS 1
#else
#define HJ_RUNS 0
#endif

/* ------------------------------------------------------------------
   A64 encodings. Register 31 is SP or ZR as each form defines it.
   ------------------------------------------------------------------ */

enum { A64_EQ, A64_NE, A64_HS, A64_LO, A64_MI, A64_PL, A64_VS, A64_VC,
       A64_HI, A64_LS, A64_GE, A64_LT, A64_GT, A64_LE, A64_AL };

#define ZR 31
#define SP 31

static uint32_t a64_addsub_imm(int sf, int sub, int s, int rd, int rn, uint32_t imm12, int lsl12)
{
    return (uint32_t)sf << 31 | (uint32_t)sub << 30 | (uint32_t)s << 29 | 0x11000000u |
           (uint32_t)lsl12 << 22 | (imm12 & 0xFFFu) << 10 | (uint32_t)rn << 5 | (uint32_t)rd;
}

/* shift: 0 LSL, 1 LSR, 2 ASR */
static uint32_t a64_addsub_reg(int sf, int sub, int s, int rd, int rn, int rm, int shift, int amount)
{
    return (uint32_t)sf << 31 | (uint32_t)sub << 30 | (uint32_t)s << 29 | 0x0B000000u |
           (uint32_t)shift << 22 | (uint32_t)rm << 16 | (uint32_t)(amount & 63) << 10 |
           (uint32_t)rn << 5 | (uint32_t)rd;
}

/* opc: 0 AND, 1 ORR, 2 EOR, 3 ANDS; invert: BIC, ORN, EON, BICS */
static uint32_t a64_logic_reg(int sf, int opc, int invert, int rd, int rn, int rm, int shift, int amount)
{
    return (uint32_t)sf << 31 | (uint32_t)opc << 29 | 0x0A000000u | (uint32_t)shift << 22 |
           (uint32_t)invert << 21 | (uint32_t)rm << 16 | (uint32_t)(amount & 63) << 10 |
           (uint32_t)rn << 5 | (uint32_t)rd;
}

/* N:immr:imms of a bitmask immediate, or -1 if the value has none. */
static int a64_bitmask(uint64_t value, int sf, uint32_t *field)
{
    unsigned size = 64, ones, r;
    uint64_t emask, elem, run;

    if (!sf)
    {
        value &= 0xFFFFFFFFu;
        value |= value << 32;
    }
    if (value == 0 || value == ~(uint64_t)0)
        return -1;
    while (size > 2)
    {
        unsigned half = size / 2;
        uint64_t mask = ((uint64_t)1 << half) - 1;

        if ((value & mask) != ((value >> half) & mask))
            break;
        size = half;
    }
    emask = size == 64 ? ~(uint64_t)0 : ((uint64_t)1 << size) - 1;
    elem = value & emask;
    ones = 0;
    for (r = 0; r < size; r++)
        ones += (unsigned)((elem >> r) & 1);
    run = ((uint64_t)1 << ones) - 1;
    for (r = 0; r < size; r++)
    {
        uint64_t rot = r ? ((elem >> r) | (elem << (size - r))) & emask : elem;

        if (rot == run)
        {
            uint32_t immr = (size - r) % size;
            uint32_t imms = ((~(size - 1) << 1) & 0x3Fu) | (ones - 1);

            *field = (uint32_t)(size == 64) << 12 | immr << 6 | imms;
            return 0;
        }
    }
    return -1;
}

static uint32_t a64_logic_imm(int sf, int opc, int rd, int rn, uint32_t field)
{
    return (uint32_t)sf << 31 | (uint32_t)opc << 29 | 0x12000000u | field << 10 |
           (uint32_t)rn << 5 | (uint32_t)rd;
}

/* opc: 0 MOVN, 2 MOVZ, 3 MOVK */
static uint32_t a64_movw(int sf, int opc, int rd, uint32_t imm16, int hw)
{
    return (uint32_t)sf << 31 | (uint32_t)opc << 29 | 0x12800000u | (uint32_t)hw << 21 |
           (imm16 & 0xFFFFu) << 5 | (uint32_t)rd;
}

/* opc: 0 SBFM, 1 BFM, 2 UBFM */
static uint32_t a64_bitfield(int sf, int opc, int rd, int rn, int immr, int imms)
{
    return (uint32_t)sf << 31 | (uint32_t)opc << 29 | 0x13000000u | (uint32_t)sf << 22 |
           (uint32_t)(immr & 63) << 16 | (uint32_t)(imms & 63) << 10 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_extr(int sf, int rd, int rn, int rm, int lsb)
{
    return (uint32_t)sf << 31 | 0x13800000u | (uint32_t)sf << 22 | (uint32_t)rm << 16 |
           (uint32_t)(lsb & 63) << 10 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_rev32(int rd, int rn) { return 0x5AC00800u | (uint32_t)rn << 5 | (uint32_t)rd; }
static uint32_t a64_rev16(int rd, int rn) { return 0x5AC00400u | (uint32_t)rn << 5 | (uint32_t)rd; }
static uint32_t a64_clz(int rd, int rn) { return 0x5AC01000u | (uint32_t)rn << 5 | (uint32_t)rd; }

static uint32_t a64_csel(int sf, int rd, int rn, int rm, int cond)
{
    return (uint32_t)sf << 31 | 0x1A800000u | (uint32_t)rm << 16 | (uint32_t)cond << 12 |
           (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_csinc(int sf, int rd, int rn, int rm, int cond)
{
    return (uint32_t)sf << 31 | 0x1A800400u | (uint32_t)rm << 16 | (uint32_t)cond << 12 |
           (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_adc_sbc(int sf, int sub, int s, int rd, int rn, int rm)
{
    return (uint32_t)sf << 31 | (uint32_t)sub << 30 | (uint32_t)s << 29 | 0x1A000000u |
           (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd;
}

/* op: 0 LSLV, 1 LSRV, 2 ASRV, 3 RORV */
static uint32_t a64_shiftv(int sf, int op, int rd, int rn, int rm)
{
    return (uint32_t)sf << 31 | 0x1AC02000u | (uint32_t)rm << 16 |
           (uint32_t)(op & 3) << 10 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_lslv(int rd, int rn, int rm) { return a64_shiftv(0, 0, rd, rn, rm); }

static uint32_t a64_div(int sf, int is_signed, int rd, int rn, int rm)
{
    return (uint32_t)sf << 31 | 0x1AC00800u | (uint32_t)rm << 16 |
           (uint32_t)(is_signed ? 1 : 0) << 10 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_mul(int rd, int rn, int rm)
{
    return 0x1B007C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_msub(int sf, int rd, int rn, int rm, int ra)
{
    return (uint32_t)sf << 31 | 0x1B008000u | (uint32_t)rm << 16 |
           (uint32_t)ra << 10 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_smull(int rd, int rn, int rm)
{
    return 0x9B207C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_umull(int rd, int rn, int rm)
{
    return 0x9BA07C00u | (uint32_t)rm << 16 | (uint32_t)rn << 5 | (uint32_t)rd;
}

static uint32_t a64_mrs_nzcv(int rt) { return 0xD53B4200u | (uint32_t)rt; }

/* size: 0 byte, 1 halfword, 2 word; address [Xn, Wm, UXTW] */
static uint32_t a64_ldst_reg(int size, int load, int rt, int rn, int rm)
{
    return (uint32_t)size << 30 | 0x38200800u | (uint32_t)load << 22 | (uint32_t)rm << 16 |
           2u << 13 | (uint32_t)rn << 5 | (uint32_t)rt;
}

/* size: 0 byte to 3 doubleword; unsigned scaled offset in bytes */
static uint32_t a64_ldst_uimm(int size, int load, int rt, int rn, uint32_t offset)
{
    return (uint32_t)size << 30 | 0x39000000u | (uint32_t)load << 22 |
           ((offset >> size) & 0xFFFu) << 10 | (uint32_t)rn << 5 | (uint32_t)rt;
}

/* mode: 1 post-index, 2 signed offset, 3 pre-index */
static uint32_t a64_ldstp(int x64, int load, int mode, int rt, int rt2, int rn, int offset)
{
    int scale = x64 ? 3 : 2;

    return (uint32_t)(x64 ? 2 : 0) << 30 | 0x28000000u | (uint32_t)mode << 23 |
           (uint32_t)load << 22 | (uint32_t)((offset >> scale) & 0x7F) << 15 |
           (uint32_t)rt2 << 10 | (uint32_t)rn << 5 | (uint32_t)rt;
}

static uint32_t a64_b(int32_t words) { return 0x14000000u | ((uint32_t)words & 0x03FFFFFFu); }
static uint32_t a64_bl(int32_t words) { return 0x94000000u | ((uint32_t)words & 0x03FFFFFFu); }
static uint32_t a64_bcond(int cond, int32_t words)
{
    return 0x54000000u | (((uint32_t)words & 0x7FFFFu) << 5) | (uint32_t)cond;
}
static uint32_t a64_cbz(int sf, int nz, int rt, int32_t words)
{
    return (uint32_t)sf << 31 | 0x34000000u | (uint32_t)nz << 24 |
           (((uint32_t)words & 0x7FFFFu) << 5) | (uint32_t)rt;
}
static uint32_t a64_tbz(int nz, int rt, int bit, int32_t words)
{
    return (uint32_t)(bit >> 5) << 31 | 0x36000000u | (uint32_t)nz << 24 |
           (uint32_t)(bit & 31) << 19 | (((uint32_t)words & 0x3FFFu) << 5) | (uint32_t)rt;
}
static uint32_t a64_br(int rn) { return 0xD61F0000u | (uint32_t)rn << 5; }
static uint32_t a64_blr(int rn) { return 0xD63F0000u | (uint32_t)rn << 5; }
#define A64_RET 0xD65F03C0u

/* Aliases, 32-bit unless the name says otherwise. */
static uint32_t a64_mov(int rd, int rm) { return a64_logic_reg(0, 1, 0, rd, ZR, rm, 0, 0); }
static uint32_t a64_mov64(int rd, int rm) { return a64_logic_reg(1, 1, 0, rd, ZR, rm, 0, 0); }
static uint32_t a64_cmp(int rn, int rm) { return a64_addsub_reg(0, 1, 1, ZR, rn, rm, 0, 0); }
static uint32_t a64_tst(int rn, int rm) { return a64_logic_reg(0, 3, 0, ZR, rn, rm, 0, 0); }
static uint32_t a64_lsl(int rd, int rn, int s) { return a64_bitfield(0, 2, rd, rn, (32 - s) & 31, 31 - s); }
static uint32_t a64_lsr(int rd, int rn, int s) { return a64_bitfield(0, 2, rd, rn, s, 31); }
static uint32_t a64_asr(int rd, int rn, int s) { return a64_bitfield(0, 0, rd, rn, s, 31); }
static uint32_t a64_ubfx(int rd, int rn, int lsb, int width) { return a64_bitfield(0, 2, rd, rn, lsb, lsb + width - 1); }
static uint32_t a64_bfi(int rd, int rn, int lsb, int width) { return a64_bitfield(0, 1, rd, rn, (32 - lsb) & 31, width - 1); }
static uint32_t a64_sxtb(int rd, int rn) { return a64_bitfield(0, 0, rd, rn, 0, 7); }
static uint32_t a64_sxth(int rd, int rn) { return a64_bitfield(0, 0, rd, rn, 0, 15); }
static uint32_t a64_uxtb(int rd, int rn) { return a64_bitfield(0, 2, rd, rn, 0, 7); }
static uint32_t a64_uxth(int rd, int rn) { return a64_bitfield(0, 2, rd, rn, 0, 15); }
static uint32_t a64_ror(int rd, int rn, int s) { return a64_extr(0, rd, rn, rn, s); }
static uint32_t a64_cset(int rd, int cond) { return a64_csinc(0, rd, ZR, ZR, cond ^ 1); }

/* Logical immediates the translator uses, all encodable. */
static uint32_t a64_logic_const(int opc, int rd, int rn, uint32_t value)
{
    uint32_t field = 0;

    if (a64_bitmask(value, 0, &field) != 0)
        m68kjit_plat_dead_end();
    return a64_logic_imm(0, opc, rd, rn, field);
}

/* ------------------------------------------------------------------
   Code emission
   ------------------------------------------------------------------ */

struct HJEmit
{
    uint32_t *buf;
    unsigned n, cap;
    uintptr_t origin;   /* host address of buf[0] once copied */
    int bad;            /* too long, or a branch out of range */
};

static void put(struct HJEmit *e, uint32_t word)
{
    if (e->n < e->cap)
        e->buf[e->n] = word;
    else
        e->bad = 1;
    e->n++;
}

static int32_t words_to(const struct HJEmit *e, uintptr_t target)
{
    return (int32_t)(((intptr_t)target - (intptr_t)(e->origin + 4u * e->n)) / 4);
}

/* Point the local branch at index 'at' to index 'to'. */
static void patch(struct HJEmit *e, unsigned at, unsigned to)
{
    int32_t off = (int32_t)to - (int32_t)at;
    uint32_t w;

    if (at >= e->cap || to > e->cap)
    {
        e->bad = 1;
        return;
    }
    w = e->buf[at];
    if ((w & 0x7C000000u) == 0x14000000u)
        w = (w & 0xFC000000u) | ((uint32_t)off & 0x03FFFFFFu);
    else if ((w & 0x7E000000u) == 0x36000000u)
    {
        if (off < -8192 || off > 8191)
            e->bad = 1;
        w = (w & 0xFFF8001Fu) | (((uint32_t)off & 0x3FFFu) << 5);
    }
    else
    {
        if (off < -262144 || off > 262143)
            e->bad = 1;
        w = (w & 0xFF00001Fu) | (((uint32_t)off & 0x7FFFFu) << 5);
    }
    e->buf[at] = w;
}

static void emit_mov32(struct HJEmit *e, int rd, uint32_t v)
{
    uint32_t field;

    if ((v >> 16) == 0)
        put(e, a64_movw(0, 2, rd, v, 0));
    else if ((v & 0xFFFFu) == 0)
        put(e, a64_movw(0, 2, rd, v >> 16, 1));
    else if ((v >> 16) == 0xFFFFu)
        put(e, a64_movw(0, 0, rd, ~v & 0xFFFFu, 0));
    else if ((v & 0xFFFFu) == 0xFFFFu)
        put(e, a64_movw(0, 0, rd, (~v >> 16) & 0xFFFFu, 1));
    else if (a64_bitmask(v, 0, &field) == 0)
        put(e, a64_logic_imm(0, 1, rd, ZR, field));
    else
    {
        put(e, a64_movw(0, 2, rd, v & 0xFFFFu, 0));
        put(e, a64_movw(0, 3, rd, v >> 16, 1));
    }
}

/* ------------------------------------------------------------------
   State shared with translated code
   ------------------------------------------------------------------ */

/* Host registers inside translated code. D0-D7 live in w0-w7 and A0-A7 in
   w8-w15 from the enter trampoline to the exit trampoline. x16, x17 and
   x18 are caller-saved: C code called from translated code may change
   them, so the slow path glue keeps x16 and x18 around its call and takes
   the result in x17. x19 to x28 are callee-saved and the trampolines save
   them; x23 is saved with x24 and has no use. */
#define REG_D(n)  (n)
#define REG_A(n)  (8 + (n))
#define R_ADDR    16  /* guest address of an access; the slow path keeps it */
#define R_VAL     17  /* value loaded, or to be stored */
#define R_SRC     18  /* immediate operand; the slow path keeps it */
#define R_STATE   19
#define R_MEM     20
#define R_FLAGS   21  /* X N Z C V in bits 4..0: NZCV in the order of A64 */
#define R_BUDGET  22
#define R_LIMIT   24  /* mem_size - 4 */
#define R_SCR     25  /* scratch inside an access; slow path kind */
#define R_F1      26  /* flag scratch */
#define R_F2      27  /* flag scratch; guest PC for the slow path */
#define R_RES     28  /* result kept across a memory access */

#define HJF_V 0x01u
#define HJF_C 0x02u
#define HJF_Z 0x04u
#define HJF_N 0x08u
#define HJF_X 0x10u
#define HJF_NZVC 0x0Fu
#define HJF_ALL 0x1Fu

struct HJState
{
    uint32_t d[8];              /*   0 */
    uint32_t a[8];              /*  32 */
    uint32_t pc;                /*  64 */
    uint32_t flags;             /*  68 */
    uint32_t sr_high;           /*  72: SR bits 15..8 */
    uint32_t abort;             /*  76 */
    uint32_t limit;             /*  80 */
    uint32_t slice_budget;      /*  84 */
    UBYTE *mem;                 /*  88 */
    void *slow;                 /*  96 */
    struct HJCpu *backend;      /* 104 */
    uint64_t reserved[2];       /* 112 */
    UBYTE pages[];              /* 128: M68KJIT_PAGE_* of each 4 KiB guest page */
};

#define ST_PC     64
#define ST_FLAGS  68
#define ST_SRHI   72
#define ST_ABORT  76
#define ST_LIMIT  80
#define ST_BUDGET 84
#define ST_MEM    88
#define ST_SLOW   96
#define ST_PAGES 128

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
_Static_assert(offsetof(struct HJState, pc) == ST_PC, "state layout");
_Static_assert(offsetof(struct HJState, flags) == ST_FLAGS, "state layout");
_Static_assert(offsetof(struct HJState, sr_high) == ST_SRHI, "state layout");
_Static_assert(offsetof(struct HJState, abort) == ST_ABORT, "state layout");
_Static_assert(offsetof(struct HJState, limit) == ST_LIMIT, "state layout");
_Static_assert(offsetof(struct HJState, slice_budget) == ST_BUDGET, "state layout");
_Static_assert(offsetof(struct HJState, mem) == ST_MEM, "state layout");
_Static_assert(offsetof(struct HJState, slow) == ST_SLOW, "state layout");
_Static_assert(offsetof(struct HJState, pages) == ST_PAGES, "state layout");
#endif

/* Accesses through the C slow path, and the probes of a division's
   divisor: no access, only whether the read would reach the host. */
enum { SLOW_R8, SLOW_R16, SLOW_R32, SLOW_W8, SLOW_W16, SLOW_W32, SLOW_P16, SLOW_P32, SLOW_KINDS };

#define HJ_BUCKETS 4096u
#define HJ_MAX_INSNS 64
#define HJ_SCRATCH_WORDS 12288u
#define HJ_MAX_STUBS 1024u
#define HJ_BUDGET 65535u

/* The code memory, the block table and the guest-byte store of a domain
   start at these sizes and double together, HJ_GROW_STEPS times at most,
   whenever one of them is full (grow_code()): from 1 MiB of code, 2048
   blocks and 256 KiB of guest bytes to 16 MiB, 32768 blocks and 4 MiB. */
#define HJ_CODE_BYTES_START  (1u << 20)
#define HJ_MAX_BLOCKS_START  2048u
#define HJ_GUEST_BYTES_START (256u << 10)
#define HJ_GROW_STEPS 4

struct HJBlock
{
    struct HJBlock *next;   /* bucket chain */
    uint32_t pc, end;       /* guest bytes [pc, end) */
    uint32_t epoch;
    uint32_t words;
    const uint32_t *code;
    const UBYTE *guest;     /* the guest bytes when translated */
    const uint16_t *starts; /* the offset from pc of each instruction */
    uint16_t ninsns;        /* instructions in the block */
};

/* Exit reasons of translated code, as counted in HJStats.exits[]. */
enum
{
    HJ_EXIT_LOOKUP = 1, /* the block ended: the run loop finds the next one */
    HJ_EXIT_BUDGET,     /* back edges and lookups used up for this slice */
    HJ_EXIT_LINEA,      /* A-line opcode: to the bridge dispatcher */
    HJ_EXIT_FAULT,      /* instruction outside the subset, ILLEGAL, Line-F */
    HJ_EXIT_ABORT,      /* a slow access stopped the program */
    HJ_EXIT_COUNT
};

struct HJStats
{
    unsigned long blocks;        /* blocks translated */
    unsigned long guest_insns;   /* 68k instructions in them */
    unsigned long host_words;    /* AArch64 words emitted for them */
    unsigned long write_windows; /* code publication windows */
    unsigned long icache_syncs;  /* host instruction cache maintenance */
    unsigned long enters;        /* calls from C into translated code */
    unsigned long exits[HJ_EXIT_COUNT];
    unsigned long slow_accesses; /* guest accesses through the C slow path */
    unsigned long flushes;       /* local and shared-arena cache invalidations */
    unsigned long revalidated;   /* blocks kept after a flush, code unchanged */
    unsigned long resets;        /* code memory emptied */
    unsigned long max_depth;     /* deepest nesting of execute() */
};

_Static_assert(HJ_MAX_INSNS == M68KJIT_BLOCK_INSNS, "max_block_insns");

/* The translator state of a domain: code memory, glue, blocks and their
   guest bytes, shared by every CPU of the domain. The host makes one call
   on a domain at a time, so none of it needs a lock. */
struct HJBackend
{
    struct M68KEngDomain *dom;
    uint32_t *code;
    size_t code_bytes;          /* the code memory as it is now */
    size_t code_words, code_used, glue_words;
    uintptr_t g_enter, g_exit, g_lookup, g_budget, g_slow[SLOW_KINDS];
    uint32_t (*enter)(struct HJState *, const uint32_t *);
    struct HJBlock *buckets[HJ_BUCKETS];
    struct HJBlock *blocks;
    unsigned nblocks, max_blocks;
    UBYTE *guest_bytes;
    size_t guest_used, guest_cap;
    uint32_t epoch;
    uint32_t *scratch;
    struct HJTrans *trans;
    struct HJStats stats;
};

/* The translator state of a CPU: its registers and page flags, which
   translated code reads through R_STATE. */
struct HJCpu
{
    struct HJState *st;
    size_t st_bytes;
    struct M68KEngCPU *owner;
    struct HJBackend *b;
    uint32_t pages_generation;
    int declined;               /* a divisor probe found a host read */
};

/* ------------------------------------------------------------------
   68k decoding
   ------------------------------------------------------------------ */

enum
{
    EA_DN, EA_AN, EA_IND, EA_POST, EA_PRE, EA_DISP, EA_IDX,
    EA_ABS, EA_PCDISP, EA_PCIDX, EA_IMM
};

#define M(x) (1u << (x))
#define M_ALL   0x7FFu
#define M_DATA  (M_ALL & ~M(EA_AN))
#define M_ALT   (M(EA_DN) | M(EA_AN) | M(EA_IND) | M(EA_POST) | M(EA_PRE) | M(EA_DISP) | M(EA_IDX) | M(EA_ABS))
#define M_DALT  (M_ALT & ~M(EA_AN))
#define M_MALT  (M_DALT & ~M(EA_DN))
#define M_CTRL  (M(EA_IND) | M(EA_DISP) | M(EA_IDX) | M(EA_ABS) | M(EA_PCDISP) | M(EA_PCIDX))
#define M_CALT  (M(EA_IND) | M(EA_DISP) | M(EA_IDX) | M(EA_ABS))

struct HJEA
{
    uint8_t mode, reg;
    uint8_t xreg, xlong, scale;   /* index register 0-15, .L index, scale shift */
    uint8_t base_suppressed, index_suppressed;
    int32_t disp;
    uint32_t value;               /* address, immediate, or PC-relative address */
};

enum
{
    K_NOP, K_MOVEQ, K_MOVE, K_MOVEA, K_LEA, K_PEA,
    K_LINK, K_UNLK, K_EXG, K_CMPM,
    K_ALU,      /* <ea>,Dn: ADD SUB AND OR CMP */
    K_ALUM,     /* Dn,<ea>: ADD SUB AND OR EOR */
    K_ALUI,     /* #imm,<ea>: ORI ANDI SUBI ADDI EORI CMPI */
    K_ALUA,     /* ADDA SUBA CMPA */
    K_QUICK,    /* ADDQ SUBQ */
    K_ADDX,     /* ADDX SUBX NEGX */
    K_MULW,     /* MULU.W MULS.W */
    K_MULL,     /* MULU.L MULS.L */
    K_DIVW,     /* DIVU.W DIVS.W */
    K_DIVL,     /* DIVU.L DIVS.L DIVUL.L DIVSL.L */
    K_BIT,      /* BTST BCHG BCLR BSET */
    K_BF,       /* BFTST BFEXTU BFCHG BFEXTS BFCLR BFFFO BFSET BFINS */
    K_CAS,      /* CAS.B CAS.W CAS.L */
    K_MOVEP,    /* MOVEP.W MOVEP.L */
    K_TAS,      /* TAS <ea> */
    K_CLR, K_TST, K_NEG, K_NOT, K_SWAP, K_EXTW, K_EXTL, K_EXTBL,
    K_SHIFT,    /* ASL ASR LSL LSR ROL ROR ROXL ROXR */
    K_SCC,
    K_BCC, K_BSR, K_DBCC, K_JSR, K_JMP, K_RTS, K_RTR, K_RTD,
    K_MOVEM,
    K_MOVEFROMSR, K_MOVEFROMCCR, K_MOVETOCCR, K_MOVETOSR, K_LOGICSR,
    K_LINEA, K_FAULT
};
enum { OP_ADD, OP_SUB, OP_AND, OP_OR, OP_EOR, OP_CMP };
enum { SH_LSL, SH_LSR, SH_ASR, SH_ASL, SH_ROL, SH_ROR, SH_ROXL, SH_ROXR };
enum { BIT_TST, BIT_CHG, BIT_CLR, BIT_SET };
enum { BF_TST, BF_EXTU, BF_CHG, BF_EXTS, BF_CLR, BF_FFO, BF_SET, BF_INS };

struct HJInsn
{
    uint32_t pc, next, target;
    uint16_t opcode, mask;          /* MOVEM: registers in D0..A7 bit order */
    uint8_t kind, op, size, cond, count;
    uint8_t def, use, need;         /* HJF_* */
    struct HJEA src, dst;
    const char *why;                /* K_FAULT */
    uint32_t bad_at;                /* "fetch outside guest memory": the word */
};

struct HJDec
{
    const struct M68KEngDomain *dom;
    uint32_t at;
    int bad;
    uint32_t bad_at;
};

/* Instruction fetch is always direct: from guest memory and never from a
   READ_HOST range (contract, section 4). */
static int mj_fetchable(const struct M68KEngDomain *dom, uint32_t at)
{
    ULONG i;

    if (at > dom->size - 2)
        return 0;
    for (i = 0; i < dom->num_ranges; i++)
        if ((dom->ranges[i].flags & M68KENG_RANGE_READ_HOST) &&
            at + 2 > dom->ranges[i].lo && at < dom->ranges[i].hi)
            return 0;
    return 1;
}

static uint16_t fetch16(struct HJDec *d)
{
    uint16_t v = 0;

    if (!mj_fetchable(d->dom, d->at))
    {
        if (!d->bad)
            d->bad_at = d->at;
        d->bad = 1;
    }
    else
        v = (uint16_t)(d->dom->base[d->at] << 8 | d->dom->base[d->at + 1]);
    d->at += 2;
    return v;
}

static uint32_t fetch32(struct HJDec *d)
{
    uint32_t hi = fetch16(d);

    return hi << 16 | fetch16(d);
}

/* One effective address, if its kind is among 'allowed'. */
static int decode_ea(struct HJDec *d, int mode, int reg, int size, unsigned allowed, struct HJEA *ea)
{
    uint32_t base;
    uint16_t ext;

    memset(ea, 0, sizeof(*ea));
    ea->reg = (uint8_t)reg;
    switch (mode)
    {
    case 0: ea->mode = EA_DN; break;
    case 1: ea->mode = EA_AN; break;
    case 2: ea->mode = EA_IND; break;
    case 3: ea->mode = EA_POST; break;
    case 4: ea->mode = EA_PRE; break;
    case 5: ea->mode = EA_DISP; ea->disp = (int16_t)fetch16(d); break;
    case 6:
    case 7:
        if (mode == 7 && reg == 0) { ea->mode = EA_ABS; ea->value = (uint32_t)(int32_t)(int16_t)fetch16(d); break; }
        if (mode == 7 && reg == 1) { ea->mode = EA_ABS; ea->value = fetch32(d); break; }
        if (mode == 7 && reg == 2) { base = d->at; ea->mode = EA_PCDISP; ea->value = base + (uint32_t)(int32_t)(int16_t)fetch16(d); break; }
        if (mode == 7 && reg == 4)
        {
            ea->mode = EA_IMM;
            ea->value = size == 4 ? fetch32(d) : size == 2 ? fetch16(d) : (fetch16(d) & 0xFFu);
            break;
        }
        if (mode == 7 && reg != 3)
            return 0;
        base = d->at;
        ext = fetch16(d);
        ea->mode = mode == 6 ? EA_IDX : EA_PCIDX;
        ea->xreg = (uint8_t)(ext >> 12);
        ea->xlong = (uint8_t)((ext >> 11) & 1);
        ea->scale = (uint8_t)((ext >> 9) & 3);
        if (ext & 0x0100)
        {
            unsigned bd = (ext >> 4) & 3;

            /* 68020 full format, without memory indirection (IIS=000).
               BD=00 and bit 3 are reserved. Keep indirect forms outside
               the subset rather than treating their pointer as data. */
            if (!bd || (ext & 0x000F))
                return 0;
            ea->base_suppressed = (uint8_t)((ext >> 7) & 1);
            ea->index_suppressed = (uint8_t)((ext >> 6) & 1);
            ea->disp = bd == 2 ? (int16_t)fetch16(d) : bd == 3 ? (int32_t)fetch32(d) : 0;
        }
        else
            ea->disp = (int8_t)(ext & 0xFF);
        if (mode == 7)
            ea->value = (ea->base_suppressed ? 0 : base) + (uint32_t)ea->disp;
        break;
    }
    return (allowed & M(ea->mode)) != 0;
}

static const uint8_t cond_use[16] =
{
    0, 0, HJF_C | HJF_Z, HJF_C | HJF_Z, HJF_C, HJF_C, HJF_Z, HJF_Z,
    HJF_V, HJF_V, HJF_N, HJF_N, HJF_N | HJF_V, HJF_N | HJF_V,
    HJF_N | HJF_V | HJF_Z, HJF_N | HJF_V | HJF_Z
};

static int fault_insn(struct HJInsn *in, const char *why)
{
    in->kind = K_FAULT;
    in->why = why;
    in->def = 0;
    in->use = HJF_ALL;
    return 0;
}

static int std_size(uint16_t op)
{
    static const int sizes[4] = { 1, 2, 4, 0 };

    return sizes[(op >> 6) & 3];
}

/* Group 4: miscellaneous */
static int decode_misc(struct HJDec *d, uint16_t op, struct HJInsn *in)
{
    int mode = (op >> 3) & 7, reg = op & 7, size = std_size(op);

    if (op == 0x4E71) { in->kind = K_NOP; return 1; }
    if (op == 0x4E74) { in->kind = K_RTD; in->src.disp = (int16_t)fetch16(d); in->use = HJF_ALL; return 1; }
    if (op == 0x4E75) { in->kind = K_RTS; in->use = HJF_ALL; return 1; }
    if (op == 0x4E77) { in->kind = K_RTR; in->def = HJF_ALL; return 1; }
    if ((op & 0xFFF8) == 0x4E50)
    {
        in->kind = K_LINK; in->size = 2; in->dst.reg = (uint8_t)reg;
        in->src.disp = (int16_t)fetch16(d);
        return 1;
    }
    if ((op & 0xFFF8) == 0x4808)
    {
        in->kind = K_LINK; in->size = 4; in->dst.reg = (uint8_t)reg;
        in->src.disp = (int32_t)fetch32(d);
        return 1;
    }
    if ((op & 0xFFF8) == 0x4E58) { in->kind = K_UNLK; in->dst.reg = (uint8_t)reg; return 1; }
    if (op == 0x4AFC) return fault_insn(in, "ILLEGAL");
    if ((op & 0xFFC0) == 0x4E80 || (op & 0xFFC0) == 0x4EC0)
    {
        in->kind = (op & 0x0040) ? K_JMP : K_JSR;
        in->use = HJF_ALL;
        return decode_ea(d, mode, reg, 4, M_CTRL, &in->src) || fault_insn(in, "JSR/JMP mode");
    }
    if ((op & 0xFFF8) == 0x49C0) { in->kind = K_EXTBL; in->dst.reg = (uint8_t)reg; in->def = HJF_NZVC; return 1; }
    if ((op & 0xF1C0) == 0x41C0)
    {
        in->kind = K_LEA;
        in->dst.reg = (uint8_t)((op >> 9) & 7);
        return decode_ea(d, mode, reg, 4, M_CTRL, &in->src) || fault_insn(in, "LEA mode");
    }
    if ((op & 0xFFC0) == 0x40C0)
    {
        in->kind = K_MOVEFROMSR; in->size = 2; in->use = HJF_ALL;
        return decode_ea(d, mode, reg, 2, M_DALT, &in->dst) || fault_insn(in, "MOVE from SR mode");
    }
    if ((op & 0xFFC0) == 0x42C0)
    {
        in->kind = K_MOVEFROMCCR; in->size = 2; in->use = HJF_ALL;
        return decode_ea(d, mode, reg, 2, M_DALT, &in->dst) || fault_insn(in, "MOVE from CCR mode");
    }
    if ((op & 0xFFC0) == 0x44C0 || (op & 0xFFC0) == 0x46C0)
    {
        in->kind = (op & 0x0200) ? K_MOVETOSR : K_MOVETOCCR; in->size = 2; in->def = HJF_ALL;
        return decode_ea(d, mode, reg, 2, M_DATA, &in->src) || fault_insn(in, "MOVE to SR mode");
    }
    if (size && (op & 0xFF00) == 0x4000)
    {
        in->kind = K_ADDX; in->op = OP_SUB; in->count = 1; in->size = (uint8_t)size;
        in->use = HJF_X | HJF_Z; in->def = HJF_ALL;
        return decode_ea(d, mode, reg, size, M_DALT, &in->dst) || fault_insn(in, "NEGX mode");
    }
    if (size && ((op & 0xFF00) == 0x4200 || (op & 0xFF00) == 0x4400 ||
                 (op & 0xFF00) == 0x4600 || (op & 0xFF00) == 0x4A00))
    {
        unsigned allowed = M_DALT;

        switch (op & 0xFF00)
        {
        case 0x4200: in->kind = K_CLR; in->def = HJF_NZVC; break;
        case 0x4400: in->kind = K_NEG; in->def = HJF_ALL; break;
        case 0x4600: in->kind = K_NOT; in->def = HJF_NZVC; break;
        default:
            in->kind = K_TST; in->def = HJF_NZVC;
            allowed = size == 1 ? M_DATA : M_ALL;
            break;
        }
        in->size = (uint8_t)size;
        return decode_ea(d, mode, reg, size, allowed, &in->dst) || fault_insn(in, "unary mode");
    }
    if ((op & 0xFFC0) == 0x4AC0)
    {
        in->kind = K_TAS; in->size = 1; in->def = HJF_NZVC;
        return decode_ea(d, mode, reg, 1, M_DALT, &in->dst) || fault_insn(in, "TAS mode");
    }
    if ((op & 0xFFF8) == 0x4840) { in->kind = K_SWAP; in->dst.reg = (uint8_t)reg; in->def = HJF_NZVC; return 1; }
    if ((op & 0xFFC0) == 0x4840)
    {
        in->kind = K_PEA;
        return decode_ea(d, mode, reg, 4, M_CTRL, &in->src) || fault_insn(in, "PEA mode");
    }
    if ((op & 0xFFF8) == 0x4880) { in->kind = K_EXTW; in->dst.reg = (uint8_t)reg; in->def = HJF_NZVC; return 1; }
    if ((op & 0xFFF8) == 0x48C0) { in->kind = K_EXTL; in->dst.reg = (uint8_t)reg; in->def = HJF_NZVC; return 1; }
    if ((op & 0xFFC0) == 0x4C00)
    {
        uint16_t ext = fetch16(d);

        if (ext & 0x83F8u)
            return fault_insn(in, "MUL.L extension");
        in->kind = K_MULL;
        in->size = 4;
        in->op = (uint8_t)((ext >> 11) & 1);
        in->dst.reg = (uint8_t)((ext >> 12) & 7);
        in->count = (uint8_t)((ext & 7) | (((ext >> 10) & 1) << 3));
        in->def = HJF_NZVC;
        return decode_ea(d, mode, reg, 4, M_DATA, &in->src) || fault_insn(in, "MUL.L mode");
    }
    if ((op & 0xFFC0) == 0x4C40)
    {
        uint16_t ext = fetch16(d);

        if (ext & 0x83F8u)
            return fault_insn(in, "DIV.L extension");
        in->kind = K_DIVL;
        in->size = 4;
        in->op = (uint8_t)((ext >> 11) & 1);
        in->dst.reg = (uint8_t)((ext >> 12) & 7);
        in->count = (uint8_t)((ext & 7) | (((ext >> 10) & 1) << 3));
        in->def = HJF_NZVC;
        in->use = HJF_ALL;
        in->why = "divide by zero";
        return decode_ea(d, mode, reg, 4, M_DATA, &in->src) || fault_insn(in, "DIV.L mode");
    }
    if ((op & 0xFB80) == 0x4880)
    {
        uint16_t mask = fetch16(d);
        int load = (op >> 10) & 1;

        in->kind = K_MOVEM; in->count = (uint8_t)load; in->size = (op & 0x0040) ? 4 : 2;
        if (!decode_ea(d, mode, reg, in->size, load ? (M_CTRL | M(EA_POST)) : (M_CALT | M(EA_PRE)), &in->src))
            return fault_insn(in, "MOVEM mode");
        if (in->src.mode == EA_PRE)
        {
            uint16_t r = 0;
            int i;

            for (i = 0; i < 16; i++)
                if (mask & (1u << i))
                    r |= (uint16_t)(1u << (15 - i));
            mask = r;
        }
        in->mask = mask;
        if ((in->src.mode == EA_PRE || in->src.mode == EA_POST) && (mask & (1u << (8 + in->src.reg))))
            return fault_insn(in, "MOVEM with its address register");
        return 1;
    }
    return fault_insn(in, "opcode outside the subset");
}

static int decode_insn(const struct M68KEngDomain *dom, uint32_t pc, struct HJInsn *in)
{
    struct HJDec d = { dom, pc, 0, 0 };
    uint16_t op;
    int mode, reg, size, ok = 1;

    memset(in, 0, sizeof(*in));
    in->pc = pc;
    op = fetch16(&d);
    in->opcode = op;
    mode = (op >> 3) & 7;
    reg = op & 7;
    size = std_size(op);

    switch (op >> 12)
    {
    case 0x0:
    {
        static const int8_t ops[8] = { OP_OR, OP_AND, OP_SUB, OP_ADD, -1, OP_EOR, OP_CMP, -1 };
        int which = ops[(op >> 9) & 7];

        if (op & 0x0100)
        {
            int bop = (op >> 6) & 3;
            unsigned allowed = bop == BIT_TST ? M_DATA : M_DALT;

            if (mode == 1)
            {
                in->kind = K_MOVEP;
                in->op = (uint8_t)((op >> 7) & 1);
                in->size = (op & 0x0040) ? 4 : 2;
                in->dst.reg = (uint8_t)((op >> 9) & 7);
                in->src.reg = (uint8_t)reg;
                in->src.disp = (int16_t)fetch16(&d);
                break;
            }
            in->kind = K_BIT; in->op = (uint8_t)bop;
            in->size = mode == 0 ? 4 : 1;
            in->src.mode = EA_DN; in->src.reg = (uint8_t)((op >> 9) & 7);
            in->def = HJF_Z;
            ok = decode_ea(&d, mode, reg, 1, allowed, &in->dst) || fault_insn(in, "bit mode");
            break;
        }
        if ((op & 0xFF00) == 0x0800)
        {
            int bop = (op >> 6) & 3;
            unsigned allowed = bop == BIT_TST ? (M_DATA & ~M(EA_IMM)) : M_DALT;

            in->kind = K_BIT; in->op = (uint8_t)bop;
            in->size = mode == 0 ? 4 : 1;
            in->src.mode = EA_IMM; in->src.value = fetch16(&d) & 0xFFu;
            in->def = HJF_Z;
            ok = decode_ea(&d, mode, reg, 1, allowed, &in->dst) || fault_insn(in, "bit mode");
            break;
        }
        if ((op & 0x3F) == 0x3C && (which == OP_OR || which == OP_AND || which == OP_EOR) &&
            (size == 1 || size == 2))
        {
            in->kind = K_LOGICSR; in->op = (uint8_t)which; in->size = (uint8_t)size;
            in->src.value = fetch16(&d);
            in->use = HJF_ALL; in->def = HJF_ALL;
            break;
        }
        if (!size && (op & 0x3F) != 0x3C &&
            ((op & 0xFF00) == 0x0A00 || (op & 0xFF00) == 0x0C00 || (op & 0xFF00) == 0x0E00))
        {
            uint16_t ext = fetch16(&d);

            if (ext & 0xFE38u)
            {
                ok = fault_insn(in, "CAS extension");
                break;
            }
            in->kind = K_CAS;
            in->size = (op >> 9) == 5 ? 1 : (op >> 9) == 6 ? 2 : 4;
            in->src.reg = (uint8_t)(ext & 7);
            in->count = (uint8_t)((ext >> 6) & 7);
            in->def = HJF_NZVC;
            ok = decode_ea(&d, mode, reg, in->size, M_MALT, &in->dst) || fault_insn(in, "CAS mode");
            break;
        }
        if (which < 0 || !size || (op & 0x3F) == 0x3C)
        {
            ok = fault_insn(in, "opcode outside the subset");
            break;
        }
        in->kind = K_ALUI; in->op = (uint8_t)which; in->size = (uint8_t)size;
        in->src.mode = EA_IMM;
        in->src.value = size == 4 ? fetch32(&d) : size == 2 ? fetch16(&d) : (fetch16(&d) & 0xFFu);
        in->def = (which == OP_ADD || which == OP_SUB) ? HJF_ALL : HJF_NZVC;
        ok = decode_ea(&d, mode, reg, size,
                       M_DALT | (which == OP_CMP ? (M(EA_PCDISP) | M(EA_PCIDX)) : 0),
                       &in->dst) || fault_insn(in, "immediate mode");
        break;
    }
    case 0x1: case 0x2: case 0x3:
    {
        int msize = (op >> 12) == 1 ? 1 : (op >> 12) == 3 ? 2 : 4;
        int dmode = (op >> 6) & 7, dreg = (op >> 9) & 7;

        in->size = (uint8_t)msize;
        if (!decode_ea(&d, mode, reg, msize, msize == 1 ? M_DATA : M_ALL, &in->src))
        {
            ok = fault_insn(in, "MOVE source");
            break;
        }
        if (dmode == 1)
        {
            in->kind = K_MOVEA;
            in->dst.mode = EA_AN; in->dst.reg = (uint8_t)dreg;
            if (msize == 1)
                ok = fault_insn(in, "MOVEA.B");
            break;
        }
        in->kind = K_MOVE; in->def = HJF_NZVC;
        ok = decode_ea(&d, dmode, dreg, msize, M_DALT, &in->dst) || fault_insn(in, "MOVE destination");
        break;
    }
    case 0x4:
        ok = decode_misc(&d, op, in);
        break;
    case 0x5:
        if (size == 0)
        {
            in->cond = (uint8_t)((op >> 8) & 15);
            in->use = cond_use[in->cond];
            if (mode == 1)
            {
                uint32_t base = d.at;

                in->kind = K_DBCC; in->dst.reg = (uint8_t)reg;
                in->next = base + 2;
                in->target = base + (uint32_t)(int32_t)(int16_t)fetch16(&d);
                in->use = HJF_ALL;
                break;
            }
            in->kind = K_SCC; in->size = 1;
            ok = decode_ea(&d, mode, reg, 1, M_DALT, &in->dst) || fault_insn(in, "Scc mode");
            break;
        }
        in->kind = K_QUICK; in->op = (op & 0x0100) ? OP_SUB : OP_ADD;
        in->size = (uint8_t)size; in->count = (uint8_t)(((op >> 9) & 7) ? ((op >> 9) & 7) : 8);
        ok = decode_ea(&d, mode, reg, size, size == 1 ? M_DALT : M_ALT, &in->dst) || fault_insn(in, "quick mode");
        if (ok)
            in->def = in->dst.mode == EA_AN ? 0 : HJF_ALL;
        break;
    case 0x6:
    {
        uint32_t base = d.at;
        int32_t disp = (int8_t)(op & 0xFF);

        if ((op & 0xFF) == 0x00)
            disp = (int16_t)fetch16(&d);
        else if ((op & 0xFF) == 0xFF)
            disp = (int32_t)fetch32(&d);
        in->target = base + (uint32_t)disp;
        in->cond = (uint8_t)((op >> 8) & 15);
        if (in->cond == 1)
        {
            in->kind = K_BSR; in->use = HJF_ALL;
        }
        else
        {
            in->kind = K_BCC; in->use = HJF_ALL;
        }
        break;
    }
    case 0x7:
        if (op & 0x0100)
        {
            ok = fault_insn(in, "opcode outside the subset");
            break;
        }
        in->kind = K_MOVEQ; in->dst.reg = (uint8_t)((op >> 9) & 7);
        in->src.value = (uint32_t)(int32_t)(int8_t)(op & 0xFF);
        in->def = HJF_NZVC;
        break;
    case 0x8: case 0x9: case 0xB: case 0xC: case 0xD:
    {
        int group = op >> 12, opmode = (op >> 6) & 7, dn = (op >> 9) & 7;
        int alu = group == 0x8 ? OP_OR : group == 0x9 ? OP_SUB : group == 0xC ? OP_AND :
                  group == 0xD ? OP_ADD : OP_CMP;

        if (opmode == 3 || opmode == 7)
        {
            if (group == 0xC)
            {
                in->kind = K_MULW; in->op = (uint8_t)(opmode == 7); in->size = 2;
                in->dst.mode = EA_DN; in->dst.reg = (uint8_t)dn;
                in->def = HJF_NZVC;
                ok = decode_ea(&d, mode, reg, 2, M_DATA, &in->src) || fault_insn(in, "MUL.W mode");
                break;
            }
            if (group == 0x8)
            {
                in->kind = K_DIVW; in->op = (uint8_t)(opmode == 7); in->size = 2;
                in->dst.mode = EA_DN; in->dst.reg = (uint8_t)dn;
                in->def = HJF_NZVC;
                in->use = HJF_ALL;
                in->why = "divide by zero";
                ok = decode_ea(&d, mode, reg, 2, M_DATA, &in->src) || fault_insn(in, "DIV.W mode");
                break;
            }
            in->kind = K_ALUA; in->op = (uint8_t)alu; in->size = opmode == 3 ? 2 : 4;
            in->dst.mode = EA_AN; in->dst.reg = (uint8_t)dn;
            in->def = alu == OP_CMP ? HJF_NZVC : 0;
            ok = decode_ea(&d, mode, reg, in->size, M_ALL, &in->src) || fault_insn(in, "address mode");
            break;
        }
        in->size = (uint8_t)(1 << (opmode & 3));
        if (opmode < 3)
        {
            unsigned allowed = (alu == OP_OR || alu == OP_AND) ? M_DATA : (in->size == 1 ? M_DATA : M_ALL);

            in->kind = K_ALU; in->op = (uint8_t)alu;
            in->dst.mode = EA_DN; in->dst.reg = (uint8_t)dn;
            in->def = (alu == OP_ADD || alu == OP_SUB) ? HJF_ALL : HJF_NZVC;
            ok = decode_ea(&d, mode, reg, in->size, allowed, &in->src) || fault_insn(in, "source mode");
            break;
        }
        if (group == 0xB)
        {
            if (mode == 1)
            {
                in->kind = K_CMPM;
                in->src.reg = (uint8_t)reg;
                in->dst.reg = (uint8_t)dn;
                in->def = HJF_NZVC;
                break;
            }
            alu = OP_EOR;
        }
        if ((group == 0x9 || group == 0xD) && (mode == 0 || mode == 1))
        {
            in->kind = K_ADDX; in->op = (uint8_t)alu; in->count = 0;
            in->src.mode = mode ? EA_PRE : EA_DN; in->src.reg = (uint8_t)reg;
            in->dst.mode = mode ? EA_PRE : EA_DN; in->dst.reg = (uint8_t)dn;
            in->use = HJF_X | HJF_Z; in->def = HJF_ALL;
            break;
        }
        if (group == 0xC && ((opmode == 5 && (mode == 0 || mode == 1)) || (opmode == 6 && mode == 1)))
        {
            in->kind = K_EXG;
            in->src.reg = (uint8_t)(opmode == 5 && mode == 1 ? REG_A(dn) : REG_D(dn));
            in->dst.reg = (uint8_t)(mode == 1 ? REG_A(reg) : REG_D(reg));
            break;
        }
        in->kind = K_ALUM; in->op = (uint8_t)alu;
        in->src.mode = EA_DN; in->src.reg = (uint8_t)dn;
        in->def = (alu == OP_ADD || alu == OP_SUB) ? HJF_ALL : HJF_NZVC;
        ok = decode_ea(&d, mode, reg, in->size, alu == OP_EOR ? M_DALT : M_MALT, &in->dst) ||
             fault_insn(in, "ADDX/SUBX/ABCD/SBCD/EXG");
        break;
    }
    case 0xA:
        in->kind = K_LINEA; in->use = HJF_ALL;
        break;
    case 0xE:
    {
        static const uint8_t sops[4][2] = {
            { SH_ASR, SH_ASL }, { SH_LSR, SH_LSL },
            { SH_ROXR, SH_ROXL }, { SH_ROR, SH_ROL }
        };
        int left = (op >> 8) & 1, type;

        if (size == 0)
        {
            if (op & 0x0800)
            {
                uint16_t ext = fetch16(&d);
                int bf_op = (op >> 8) & 7;
                int do_reg = (ext >> 11) & 1, of = (ext >> 6) & 31;
                int dw_reg = (ext >> 5) & 1, wi = ext & 31;
                unsigned allowed = (bf_op == BF_TST || bf_op == BF_EXTU ||
                                    bf_op == BF_EXTS || bf_op == BF_FFO)
                                   ? (M(EA_DN) | M_CTRL) : (M(EA_DN) | M_CALT);

                if ((ext & 0x8000u) || ((bf_op & 1) == 0 && (ext & 0x7000u)) ||
                    (do_reg && (of & 0x18)) || (dw_reg && (wi & 0x18)))
                {
                    ok = fault_insn(in, "bitfield extension");
                    break;
                }
                in->kind = K_BF;
                in->op = (uint8_t)bf_op;
                in->mask = ext;
                in->def = HJF_NZVC;
                ok = decode_ea(&d, mode, reg, 4, allowed, &in->dst) || fault_insn(in, "bitfield mode");
                break;
            }
            type = (op >> 9) & 3;
            in->kind = K_SHIFT; in->size = 2; in->count = 1;
            in->src.mode = EA_IMM;
            in->op = sops[type][left];
            in->use = type == 2 ? HJF_X : 0;
            in->def = type == 3 ? HJF_NZVC : HJF_ALL;
            ok = decode_ea(&d, mode, reg, 2, M_MALT, &in->dst) || fault_insn(in, "shift mode");
            break;
        }
        type = (op >> 3) & 3;
        in->kind = K_SHIFT; in->size = (uint8_t)size;
        in->dst.mode = EA_DN; in->dst.reg = (uint8_t)reg;
        in->op = sops[type][left];
        if (op & 0x0020)
        {
            in->src.mode = EA_DN;
            in->src.reg = (uint8_t)((op >> 9) & 7);
            in->use = type == 3 ? 0 : HJF_X;
        }
        else
        {
            in->src.mode = EA_IMM;
            in->count = (uint8_t)(((op >> 9) & 7) ? ((op >> 9) & 7) : 8);
            in->use = type == 2 ? HJF_X : 0;
        }
        in->def = type == 3 ? HJF_NZVC : HJF_ALL;
        break;
    }
    default:
        ok = fault_insn(in, (op >> 12) == 0xF ? "Line-F" : "opcode outside the subset");
        break;
    }
    if (d.bad)
    {
        ok = fault_insn(in, "fetch outside guest memory");
        in->bad_at = d.bad_at;
    }
    if (!in->next)
        in->next = d.at;
    if (in->kind == K_BSR || in->kind == K_BCC)
        in->next = d.at;
    return ok;
}

/* Library execution cannot silently discard a stack-bank switch, trace
   or privilege exception. Keep full SR writes outside this first model;
   CCR-only instructions and native flag updates remain supported. */
static void decode_backend(const struct M68KEngDomain *dom, uint32_t pc,
                           struct HJInsn *in, int library_mode)
{
    decode_insn(dom, pc, in);
    if (library_mode && (in->kind == K_MOVETOSR ||
        (in->kind == K_LOGICSR && in->size == 2)))
        fault_insn(in, "full SR write is outside the library JIT model");
}

static int is_terminator(int kind)
{
    return kind == K_BCC || kind == K_BSR || kind == K_DBCC || kind == K_JSR ||
           kind == K_JMP || kind == K_RTS || kind == K_RTR || kind == K_RTD ||
           kind == K_LINEA || kind == K_FAULT;
}

/* Which flags each instruction must compute: those a later instruction of
   the block reads before another sets them. Everything is live at the end. */
static void flag_liveness(struct HJInsn *in, unsigned n)
{
    uint8_t live = HJF_ALL;

    while (n--)
    {
        in[n].need = in[n].def & live;
        live = (uint8_t)((live & ~in[n].def) | in[n].use);
    }
}

/* ------------------------------------------------------------------
   Translation of a block
   ------------------------------------------------------------------ */

#define HJ_NO_BRANCH (~0u)

struct HJStub
{
    unsigned br1, br2, br3, back;   /* br3: HJ_NO_BRANCH when there is none */
    uint8_t kind, areg;
    uint32_t pc;
};

struct HJTrans
{
    struct HJEmit e;
    uintptr_t g_exit, g_lookup, g_budget, g_slow[SLOW_KINDS];
    uint32_t start;
    const struct HJInsn *in;
    struct HJStub stub[HJ_MAX_STUBS];
    unsigned nstub;
    int library_mode;
};

static int step_of(int size, int reg) { return size == 1 && reg == 7 ? 2 : size; }

static void emit_glue_jump(struct HJTrans *t, uintptr_t glue)
{
    put(&t->e, a64_b(words_to(&t->e, glue)));
}

/* Continue at a known guest address: a back edge to this block stays in
   it while the budget lasts, anything else goes through the lookup. */
static void emit_exit_to(struct HJTrans *t, uint32_t target)
{
    struct HJEmit *e = &t->e;

    if (target == t->start)
    {
        put(e, a64_addsub_imm(0, 1, 1, R_BUDGET, R_BUDGET, 1, 0));
        put(e, a64_bcond(A64_NE, -(int32_t)e->n));
        emit_mov32(e, R_ADDR, target);
        emit_glue_jump(t, t->g_budget);
        return;
    }
    emit_mov32(e, R_ADDR, target);
    emit_glue_jump(t, t->g_lookup);
}

static void emit_exit_reason(struct HJTrans *t, uint32_t pc, int reason)
{
    emit_mov32(&t->e, R_ADDR, pc);
    emit_mov32(&t->e, R_VAL, (uint32_t)reason);
    emit_glue_jump(t, t->g_exit);
}

/* One guest access through host register 'areg'. Loads leave the value in
   w17; stores take it from w17. The access is made inline only when it
   lies at or below mem_size - 4, inside one 4 KiB page, on a page whose
   flags in the state (taken from the domain's map at every run) leave this
   kind of access direct. Every other access goes to an out-of-line call of
   the C slow path, which applies the contract's rule exactly: ranges to the
   byte, longwords as two word accesses. */
static void emit_access(struct HJTrans *t, int kind, int areg)
{
    struct HJEmit *e = &t->e;
    int store = kind >= SLOW_W8;
    int size = (kind == SLOW_R8 || kind == SLOW_W8) ? 0 : (kind == SLOW_R16 || kind == SLOW_W16) ? 1 : 2;
    int scr = store ? R_SCR : R_VAL;
    struct HJStub *s;

    if (t->nstub >= HJ_MAX_STUBS)
    {
        e->bad = 1;
        return;
    }
    s = &t->stub[t->nstub++];
    s->br3 = HJ_NO_BRANCH;
    put(e, a64_cmp(areg, R_LIMIT));
    s->br1 = e->n;
    put(e, a64_bcond(A64_HI, 0));
    if (size)
    {
        /* a word or long that ends in the next page */
        put(e, a64_logic_const(0, scr, areg, 0xFFFu));
        put(e, a64_addsub_imm(0, 1, 1, ZR, scr, 0x1000u - (1u << size), 0));
        s->br3 = e->n;
        put(e, a64_bcond(A64_HI, 0));
    }
    put(e, a64_lsr(scr, areg, M68KJIT_PAGE_SHIFT));
    put(e, a64_addsub_reg(1, 0, 0, scr, R_STATE, scr, 0, 0));
    put(e, a64_ldst_uimm(0, 1, scr, scr, ST_PAGES));
    s->br2 = e->n;
    put(e, a64_tbz(1, scr, store ? 1 : 0, 0));
    if (!store)
    {
        put(e, a64_ldst_reg(size, 1, R_VAL, R_MEM, areg));
        if (size == 2)
            put(e, a64_rev32(R_VAL, R_VAL));
        else if (size == 1)
            put(e, a64_rev16(R_VAL, R_VAL));
    }
    else if (size == 0)
        put(e, a64_ldst_reg(0, 0, R_VAL, R_MEM, areg));
    else
    {
        put(e, size == 2 ? a64_rev32(R_SCR, R_VAL) : a64_rev16(R_SCR, R_VAL));
        put(e, a64_ldst_reg(size, 0, R_SCR, R_MEM, areg));
    }
    s->back = e->n;
    s->kind = (uint8_t)kind;
    s->areg = (uint8_t)areg;
    s->pc = t->in->pc;
}

static void emit_stubs(struct HJTrans *t)
{
    struct HJEmit *e = &t->e;
    unsigned i;

    for (i = 0; i < t->nstub; i++)
    {
        const struct HJStub *s = &t->stub[i];

        patch(e, s->br1, e->n);
        patch(e, s->br2, e->n);
        if (s->br3 != HJ_NO_BRANCH)
            patch(e, s->br3, e->n);
        if (s->areg != R_ADDR)
            put(e, a64_mov(R_ADDR, s->areg));
        emit_mov32(e, R_F2, s->pc);
        put(e, a64_bl(words_to(e, t->g_slow[s->kind])));
        put(e, a64_b((int32_t)s->back - (int32_t)e->n));
    }
}

/* rd = rn + c, 32-bit wrap. */
static void emit_add_const(struct HJEmit *e, int rd, int rn, int32_t c)
{
    uint32_t mag = c < 0 ? (uint32_t)(-(int64_t)c) : (uint32_t)c;

    if (c == 0)
    {
        if (rd != rn)
            put(e, a64_mov(rd, rn));
    }
    else if (mag < 4096)
        put(e, a64_addsub_imm(0, c < 0, 0, rd, rn, mag, 0));
    else
    {
        emit_mov32(e, R_SCR, (uint32_t)c);
        put(e, a64_addsub_reg(0, 0, 0, rd, rn, R_SCR, 0, 0));
    }
}

static void emit_index(struct HJTrans *t, int base, const struct HJEA *ea)
{
    struct HJEmit *e = &t->e;
    int x = -1;

    if (!ea->index_suppressed)
    {
        x = ea->xreg < 8 ? REG_D(ea->xreg) : REG_A(ea->xreg - 8);
        if (!ea->xlong)
        {
            put(e, a64_sxth(R_SCR, x));
            x = R_SCR;
        }
    }
    if (base < 0 || ea->base_suppressed)
    {
        emit_mov32(e, R_ADDR, base < 0 ? ea->value : (uint32_t)ea->disp);
        if (x >= 0)
            put(e, a64_addsub_reg(0, 0, 0, R_ADDR, R_ADDR, x, 0, ea->scale));
        return;
    }
    if (x < 0)
        emit_add_const(e, R_ADDR, base, ea->disp);
    else
    {
        put(e, a64_addsub_reg(0, 0, 0, R_ADDR, base, x, 0, ea->scale));
        emit_add_const(e, R_ADDR, R_ADDR, ea->disp);
    }
}

/* The host register that holds the guest address of a memory operand.
   A predecrement happens here, a postincrement in emit_post(). */
static int emit_address(struct HJTrans *t, const struct HJEA *ea, int size)
{
    struct HJEmit *e = &t->e;
    int an = REG_A(ea->reg);

    switch (ea->mode)
    {
    case EA_IND:
    case EA_POST:
        return an;
    case EA_PRE:
        put(e, a64_addsub_imm(0, 1, 0, an, an, (uint32_t)step_of(size, ea->reg), 0));
        return an;
    case EA_DISP:
        emit_add_const(e, R_ADDR, an, ea->disp);
        return R_ADDR;
    case EA_IDX:
        emit_index(t, an, ea);
        return R_ADDR;
    case EA_PCIDX:
        emit_index(t, -1, ea);
        return R_ADDR;
    default:
        emit_mov32(e, R_ADDR, ea->value);
        return R_ADDR;
    }
}

static void emit_post(struct HJTrans *t, const struct HJEA *ea, int size)
{
    if (ea->mode == EA_POST)
        put(&t->e, a64_addsub_imm(0, 0, 0, REG_A(ea->reg), REG_A(ea->reg),
                                  (uint32_t)step_of(size, ea->reg), 0));
}

static int read_kind(int size) { return size == 1 ? SLOW_R8 : size == 2 ? SLOW_R16 : SLOW_R32; }
static int write_kind(int size) { return size == 1 ? SLOW_W8 : size == 2 ? SLOW_W16 : SLOW_W32; }

/* The host register with a source operand; its low 'size' bytes count. */
static int emit_read(struct HJTrans *t, const struct HJEA *ea, int size)
{
    int areg;

    switch (ea->mode)
    {
    case EA_DN: return REG_D(ea->reg);
    case EA_AN: return REG_A(ea->reg);
    case EA_IMM:
        emit_mov32(&t->e, R_SRC, ea->value);
        return R_SRC;
    default:
        areg = emit_address(t, ea, size);
        emit_access(t, read_kind(size), areg);
        emit_post(t, ea, size);
        return R_VAL;
    }
}

static void emit_insert(struct HJEmit *e, int dn, int val, int size)
{
    if (size == 4)
    {
        if (dn != val)
            put(e, a64_mov(dn, val));
    }
    else
        put(e, a64_bfi(dn, val, 0, 8 * size));
}

static void emit_write(struct HJTrans *t, const struct HJEA *ea, int size, int val)
{
    int areg;

    if (ea->mode == EA_DN)
    {
        emit_insert(&t->e, REG_D(ea->reg), val, size);
        return;
    }
    areg = emit_address(t, ea, size);
    if (val != R_VAL)
        put(&t->e, a64_mov(R_VAL, val));
    emit_access(t, write_kind(size), areg);
    emit_post(t, ea, size);
}

/* NZCV of A64 in bits 31..28 into the flags: bits 3..0 keep that order. */
static void emit_nzcv_to_flags(struct HJEmit *e, int sub, int setx)
{
    put(e, a64_mrs_nzcv(R_F1));
    put(e, a64_lsr(R_F1, R_F1, 28));
    if (sub)
        put(e, a64_logic_const(2, R_F1, R_F1, HJF_C));   /* borrow, not carry */
    put(e, a64_bfi(R_FLAGS, R_F1, 0, 4));
    if (setx)
    {
        put(e, a64_lsr(R_F2, R_F1, 1));
        put(e, a64_bfi(R_FLAGS, R_F2, 4, 1));
    }
}

/* After an ADDS or SUBS: what 'need' asks for. */
static void emit_flags_arith(struct HJEmit *e, int sub, unsigned need)
{
    if (need & HJF_NZVC)
        emit_nzcv_to_flags(e, sub, (need & HJF_X) != 0);
    else if (need & HJF_X)
    {
        put(e, a64_cset(R_F2, sub ? A64_LO : A64_HS));
        put(e, a64_bfi(R_FLAGS, R_F2, 4, 1));
    }
}

/* N and Z of the low 'size' bytes of reg; V and C cleared; X kept. */
static void emit_flags_logic(struct HJEmit *e, int reg, int size)
{
    if (size == 4)
        put(e, a64_tst(reg, reg));
    else
    {
        put(e, a64_lsl(R_F1, reg, 32 - 8 * size));
        put(e, a64_tst(R_F1, R_F1));
    }
    emit_nzcv_to_flags(e, 0, 0);
}

static void emit_flags_const(struct HJEmit *e, uint32_t nz)
{
    put(e, a64_logic_const(0, R_FLAGS, R_FLAGS, HJF_X));
    if (nz)
        put(e, a64_logic_const(1, R_FLAGS, R_FLAGS, nz));
}

/* out = dval OP sval on 'size' bytes. A D register out keeps its upper
   bytes; R_RES gets valid low bytes. CMP writes nothing. */
static void emit_alu(struct HJTrans *t, int op, int size, int dval, int sval, int out, unsigned need)
{
    struct HJEmit *e = &t->e;
    int arith = op == OP_ADD || op == OP_SUB || op == OP_CMP;
    int sub = op == OP_SUB || op == OP_CMP;
    unsigned want = need & (op == OP_CMP ? HJF_NZVC : HJF_ALL);
    int keep = op != OP_CMP, shift = 32 - 8 * size;
    int opc = op == OP_AND ? 0 : op == OP_OR ? 1 : 2;

    if (arith)
    {
        int s = want != 0;

        if (size == 4)
        {
            put(e, a64_addsub_reg(0, sub, s, keep ? out : ZR, dval, sval, 0, 0));
            emit_flags_arith(e, sub, want);
            return;
        }
        if (!s)
        {
            if (keep)
            {
                put(e, a64_addsub_reg(0, sub, 0, R_SCR, dval, sval, 0, 0));
                emit_insert(e, out, R_SCR, out == R_RES ? 4 : size);
            }
            return;
        }
        put(e, a64_lsl(R_SCR, dval, shift));
        put(e, a64_addsub_reg(0, sub, 1, keep ? R_SCR : ZR, R_SCR, sval, 0, shift));
        emit_flags_arith(e, sub, want);
        if (keep)
        {
            put(e, a64_lsr(R_SCR, R_SCR, shift));
            emit_insert(e, out, R_SCR, out == R_RES ? 4 : size);
        }
        return;
    }
    if (size == 4)
    {
        if (want && op == OP_AND)
        {
            put(e, a64_logic_reg(0, 3, 0, out, dval, sval, 0, 0));
            emit_nzcv_to_flags(e, 0, 0);
            return;
        }
        put(e, a64_logic_reg(0, opc, 0, out, dval, sval, 0, 0));
        if (want)
            emit_flags_logic(e, out, 4);
        return;
    }
    put(e, a64_logic_reg(0, opc, 0, R_SCR, dval, sval, 0, 0));
    if (want)
        emit_flags_logic(e, R_SCR, size);
    emit_insert(e, out, R_SCR, out == R_RES ? 4 : size);
}

/* A branch, still to be patched, taken when the 68k condition holds.
   Returns its index, or ~0u for F. */
static unsigned emit_cond_branch(struct HJEmit *e, int cond)
{
    unsigned at;
    static const int8_t bit[16] = { -1, -1, -1, -1, 1, 1, 2, 2, 0, 0, 3, 3, -1, -1, -1, -1 };

    switch (cond)
    {
    case 0:
        at = e->n;
        put(e, a64_b(0));
        return at;
    case 1:
        return ~0u;
    case 2: case 3:
        put(e, a64_logic_const(3, ZR, R_FLAGS, HJF_C | HJF_Z));
        at = e->n;
        put(e, a64_bcond(cond == 2 ? A64_EQ : A64_NE, 0));
        return at;
    case 12: case 13: case 14: case 15:
        put(e, a64_logic_reg(0, 2, 0, R_F1, R_FLAGS, R_FLAGS, 1, 3));      /* V ^ N */
        if (cond >= 14)
            put(e, a64_logic_reg(0, 1, 0, R_F1, R_F1, R_FLAGS, 1, 2));     /* | Z */
        at = e->n;
        put(e, a64_tbz(cond & 1, R_F1, 0, 0));
        return at;
    default:
        at = e->n;
        put(e, a64_tbz(cond & 1, R_FLAGS, bit[cond], 0));
        return at;
    }
}

static void emit_push(struct HJTrans *t, uint32_t value)
{
    struct HJEmit *e = &t->e;

    put(e, a64_addsub_imm(0, 1, 0, REG_A(7), REG_A(7), 4, 0));
    emit_mov32(e, R_VAL, value);
    emit_access(t, SLOW_W32, REG_A(7));
}

/* A Dn, the result register or w17 as the target of a RMW on 'ea'. */
static void emit_rmw(struct HJTrans *t, const struct HJInsn *in, int op, int sval)
{
    int areg = emit_address(t, &in->dst, in->size);

    emit_access(t, read_kind(in->size), areg);
    emit_alu(t, op, in->size, R_VAL, sval, R_RES, in->need);
    if (op != OP_CMP)
    {
        put(&t->e, a64_mov(R_VAL, R_RES));
        emit_access(t, write_kind(in->size), areg);
    }
    emit_post(t, &in->dst, in->size);
}

static void emit_addx(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int size = in->size, shift = 32 - 8 * size, sub = in->op == OP_SUB;
    int dval, sval, areg = -1;

    if (in->count == 1)
    {
        dval = ZR;
        if (in->dst.mode == EA_DN)
            sval = REG_D(in->dst.reg);
        else
        {
            areg = emit_address(t, &in->dst, size);
            emit_access(t, read_kind(size), areg);
            sval = R_VAL;
        }
    }
    else if (in->dst.mode == EA_DN)
    {
        sval = REG_D(in->src.reg);
        dval = REG_D(in->dst.reg);
    }
    else
    {
        int ay = REG_A(in->src.reg), ax = REG_A(in->dst.reg);

        put(e, a64_addsub_imm(0, 1, 0, ay, ay, (uint32_t)step_of(size, in->src.reg), 0));
        emit_access(t, read_kind(size), ay);
        put(e, a64_mov(R_SRC, R_VAL));
        put(e, a64_addsub_imm(0, 1, 0, ax, ax, (uint32_t)step_of(size, in->dst.reg), 0));
        emit_access(t, read_kind(size), ax);
        sval = R_SRC;
        dval = R_VAL;
        areg = ax;
    }
    put(e, a64_ubfx(R_F1, R_FLAGS, 4, 1));
    if (size == 4)
    {
        if (sub)
            put(e, a64_addsub_reg(0, 1, 1, ZR, ZR, R_F1, 0, 0));
        else
            put(e, a64_addsub_imm(0, 1, 1, ZR, R_F1, 1, 0));
        put(e, a64_adc_sbc(0, sub, 1, R_SCR, dval, sval));
    }
    else
    {
        put(e, a64_lsl(R_SCR, dval, shift));
        put(e, a64_lsl(R_F2, sval, shift));
        put(e, a64_logic_reg(0, 1, 0, R_F2, R_F2, R_F1, 0, shift - 1));
        if (!sub)
            put(e, a64_logic_reg(0, 1, 0, R_SCR, R_SCR, R_F1, 0, shift - 1));
        put(e, a64_addsub_reg(0, sub, 1, R_SCR, R_SCR, R_F2, 0, 0));
    }
    if (in->need)
    {
        put(e, a64_mrs_nzcv(R_F1));
        put(e, a64_lsr(R_F1, R_F1, 28));
        if (sub)
            put(e, a64_logic_const(2, R_F1, R_F1, HJF_C));
    }
    if (size < 4)
        put(e, a64_lsr(R_SCR, R_SCR, shift));
    if (in->need)
    {
        put(e, a64_tst(R_SCR, R_SCR));
        put(e, a64_cset(R_F2, A64_EQ));
        put(e, a64_logic_reg(0, 0, 0, R_F2, R_F2, R_FLAGS, 1, 2));
        put(e, a64_bfi(R_F1, R_F2, 2, 1));
        put(e, a64_bfi(R_FLAGS, R_F1, 0, 4));
        put(e, a64_lsr(R_F2, R_F1, 1));
        put(e, a64_bfi(R_FLAGS, R_F2, 4, 1));
    }
    if (in->dst.mode == EA_DN)
        emit_insert(e, REG_D(in->dst.reg), R_SCR, size);
    else
    {
        put(e, a64_mov(R_VAL, R_SCR));
        emit_access(t, write_kind(size), areg);
        if (in->count == 1)
            emit_post(t, &in->dst, size);
    }
}

static void emit_bit(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int v, areg;

    if (in->src.mode == EA_IMM)
    {
        unsigned bit = in->src.value & (in->dst.mode == EA_DN ? 31u : 7u);

        emit_mov32(e, R_RES, 1u << bit);
    }
    else
    {
        int sreg = REG_D(in->src.reg);

        if (in->dst.mode != EA_DN)
        {
            put(e, a64_logic_const(0, R_F1, sreg, 7u));
            sreg = R_F1;
        }
        emit_mov32(e, R_RES, 1u);
        put(e, a64_lslv(R_RES, R_RES, sreg));
    }
    if (in->op == BIT_TST)
    {
        v = emit_read(t, &in->dst, in->size);
        if (in->need & HJF_Z)
        {
            put(e, a64_tst(v, R_RES));
            put(e, a64_cset(R_F1, A64_EQ));
            put(e, a64_bfi(R_FLAGS, R_F1, 2, 1));
        }
        return;
    }
    v = in->dst.mode == EA_DN ? REG_D(in->dst.reg) : R_VAL;
    areg = in->dst.mode == EA_DN ? -1 : emit_address(t, &in->dst, 1);
    if (areg >= 0)
        emit_access(t, SLOW_R8, areg);
    if (in->need & HJF_Z)
    {
        put(e, a64_tst(v, R_RES));
        put(e, a64_cset(R_F1, A64_EQ));
        put(e, a64_bfi(R_FLAGS, R_F1, 2, 1));
    }
    if (in->op == BIT_CHG)
        put(e, a64_logic_reg(0, 2, 0, v, v, R_RES, 0, 0));
    else if (in->op == BIT_CLR)
        put(e, a64_logic_reg(0, 0, 1, v, v, R_RES, 0, 0));
    else
        put(e, a64_logic_reg(0, 1, 0, v, v, R_RES, 0, 0));
    if (areg >= 0)
    {
        emit_access(t, SLOW_W8, areg);
        emit_post(t, &in->dst, 1);
    }
}

static void emit_mull(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int dl = REG_D(in->dst.reg), dh = REG_D(in->count & 7);
    int quad = (in->count >> 3) & 1, is_signed = in->op;
    int v = emit_read(t, &in->src, 4);

    put(e, is_signed ? a64_smull(R_SCR, v, dl) : a64_umull(R_SCR, v, dl));
    if (quad)
    {
        put(e, a64_bitfield(1, 2, dh, R_SCR, 32, 63));
        put(e, a64_mov(dl, R_SCR));
        if (in->need & HJF_NZVC)
        {
            put(e, a64_logic_reg(1, 3, 0, ZR, R_SCR, R_SCR, 0, 0));
            emit_nzcv_to_flags(e, 0, 0);
        }
        return;
    }
    put(e, a64_mov(dl, R_SCR));
    if (in->need & HJF_NZVC)
    {
        put(e, a64_tst(dl, dl));
        put(e, a64_mrs_nzcv(R_F1));
        put(e, a64_lsr(R_F1, R_F1, 28));
        if (is_signed)
        {
            put(e, a64_bitfield(1, 0, R_F2, dl, 0, 31));
            put(e, a64_addsub_reg(1, 1, 1, ZR, R_SCR, R_F2, 0, 0));
        }
        else
        {
            put(e, a64_bitfield(1, 2, R_F2, R_SCR, 32, 63));
            put(e, a64_addsub_imm(0, 1, 1, ZR, R_F2, 0, 0));
        }
        put(e, a64_cset(R_F2, A64_NE));
        put(e, a64_bfi(R_F1, R_F2, 0, 1));
        put(e, a64_bfi(R_FLAGS, R_F1, 0, 4));
    }
}

/* A zero divisor ends the block at the instruction with nothing of it
   done (STOP_EXCEPTION, vector 5): on that path only, the postincrement
   or predecrement emit_read() made to the source address register is
   undone. Other modes changed no register. */
static void emit_undo_step(struct HJTrans *t, const struct HJEA *ea, int size)
{
    int an = REG_A(ea->reg);

    if (ea->mode == EA_POST)
        put(&t->e, a64_addsub_imm(0, 1, 0, an, an, (uint32_t)step_of(size, ea->reg), 0));
    else if (ea->mode == EA_PRE)
        put(&t->e, a64_addsub_imm(0, 0, 0, an, an, (uint32_t)step_of(size, ea->reg), 0));
}

/* Before a division reads its divisor from memory: when that read would
   reach the host (a READ_HOST range, or outside guest memory), the block
   ends at the division with nothing of it done and no host call made, as
   STOP_UNSUPPORTED, so that the host can execute the division itself,
   once. The inline test is the one of emit_access(); what it cannot decide
   goes to a probe in C, which applies the rule of the contract without an
   access. No mode of the subset reads memory to form the address. */
static void emit_div_probe(struct HJTrans *t, const struct HJInsn *in, int size)
{
    struct HJEmit *e = &t->e;
    const struct HJEA *ea = &in->src;
    struct HJStub *s;
    unsigned br_direct;
    int areg;

    if (ea->mode == EA_DN || ea->mode == EA_AN || ea->mode == EA_IMM)
        return;
    if (ea->mode == EA_PRE)
    {
        put(e, a64_addsub_imm(0, 1, 0, R_ADDR, REG_A(ea->reg), (uint32_t)step_of(size, ea->reg), 0));
        areg = R_ADDR;
    }
    else
        areg = emit_address(t, ea, size);   /* no guest register changes in these modes */
    if (t->nstub >= HJ_MAX_STUBS)
    {
        e->bad = 1;
        return;
    }
    s = &t->stub[t->nstub++];
    emit_mov32(e, R_VAL, 0);
    put(e, a64_cmp(areg, R_LIMIT));
    s->br1 = e->n;
    put(e, a64_bcond(A64_HI, 0));
    put(e, a64_logic_const(0, R_SCR, areg, 0xFFFu));
    put(e, a64_addsub_imm(0, 1, 1, ZR, R_SCR, 0x1000u - (uint32_t)size, 0));
    s->br3 = e->n;
    put(e, a64_bcond(A64_HI, 0));
    put(e, a64_lsr(R_SCR, areg, M68KJIT_PAGE_SHIFT));
    put(e, a64_addsub_reg(1, 0, 0, R_SCR, R_STATE, R_SCR, 0, 0));
    put(e, a64_ldst_uimm(0, 1, R_SCR, R_SCR, ST_PAGES));
    s->br2 = e->n;
    put(e, a64_tbz(1, R_SCR, 0, 0));
    s->back = e->n;
    s->kind = (uint8_t)(size == 2 ? SLOW_P16 : SLOW_P32);
    s->areg = (uint8_t)areg;
    s->pc = in->pc;
    br_direct = e->n;
    put(e, a64_cbz(0, 0, R_VAL, 0));
    emit_exit_reason(t, in->pc, HJ_EXIT_FAULT);
    patch(e, br_direct, e->n);
}

/* Dn / <ea>.W into remainder:quotient. The signed division is made in 64
   bits, so that 0x80000000 / -1 shows as a quotient that does not fit in a
   word, like any other. On overflow V is set and C cleared, and Dn, N and
   Z stay as they were. */
static void emit_divw(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int dn = REG_D(in->dst.reg), is_signed = in->op;
    unsigned br_nz, br_ovf, br_done;
    int v;

    emit_div_probe(t, in, 2);
    v = emit_read(t, &in->src, 2);

    put(e, is_signed ? a64_bitfield(1, 0, R_SRC, v, 0, 15) : a64_uxth(R_SRC, v));
    br_nz = e->n;
    put(e, a64_cbz(0, 1, R_SRC, 0));
    emit_undo_step(t, &in->src, 2);
    emit_exit_reason(t, in->pc, HJ_EXIT_FAULT);
    patch(e, br_nz, e->n);
    if (is_signed)
    {
        put(e, a64_bitfield(1, 0, R_RES, dn, 0, 31));
        put(e, a64_div(1, 1, R_SCR, R_RES, R_SRC));
        put(e, a64_msub(1, R_F1, R_SCR, R_SRC, R_RES));
        put(e, a64_bitfield(1, 0, R_F2, R_SCR, 0, 15));
        put(e, a64_addsub_reg(1, 1, 1, ZR, R_SCR, R_F2, 0, 0));
    }
    else
    {
        put(e, a64_div(0, 0, R_SCR, dn, R_SRC));
        put(e, a64_msub(0, R_F1, R_SCR, R_SRC, dn));
        put(e, a64_uxth(R_F2, R_SCR));
        put(e, a64_cmp(R_SCR, R_F2));
    }
    br_ovf = e->n;
    put(e, a64_bcond(A64_NE, 0));
    put(e, a64_uxth(dn, R_SCR));
    put(e, a64_bfi(dn, R_F1, 16, 16));
    if (in->need & HJF_NZVC)
        emit_flags_logic(e, dn, 2);
    br_done = e->n;
    put(e, a64_b(0));
    patch(e, br_ovf, e->n);
    if (in->need & HJF_C)
        put(e, a64_logic_const(0, R_FLAGS, R_FLAGS, ~HJF_C));
    if (in->need & HJF_V)
        put(e, a64_logic_const(1, R_FLAGS, R_FLAGS, HJF_V));
    patch(e, br_done, e->n);
}

/* Dr:Dq / <ea>.L, or Dq / <ea>.L, into Dq with the remainder in Dr. Only
   the unsigned division of 32 bits by 32 cannot overflow; the others are
   made in 64 bits and their quotient checked. */
static void emit_divl(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int dq = REG_D(in->dst.reg), dr = REG_D(in->count & 7);
    int quad = (in->count >> 3) & 1, is_signed = in->op;
    unsigned br_nz, br_ovf, br_done;
    int v;

    emit_div_probe(t, in, 4);
    v = emit_read(t, &in->src, 4);

    br_nz = e->n;
    put(e, a64_cbz(0, 1, v, 0));
    emit_undo_step(t, &in->src, 4);
    emit_exit_reason(t, in->pc, HJ_EXIT_FAULT);
    patch(e, br_nz, e->n);
    if (!quad && !is_signed)
    {
        /* 32 bits by 32 bits, unsigned: the quotient always fits */
        put(e, a64_div(0, 0, R_SCR, dq, v));
        put(e, a64_msub(0, R_F1, R_SCR, v, dq));
        put(e, a64_mov(dr, R_F1));
        put(e, a64_mov(dq, R_SCR));
        if (in->need & HJF_NZVC)
            emit_flags_logic(e, dq, 4);
        return;
    }
    if (quad)
    {
        put(e, a64_mov(R_RES, dq));
        put(e, a64_bitfield(1, 1, R_RES, dr, 32, 31));
    }
    else
        /* a signed dividend of 32 bits, divided in 64 like the long one:
           0x80000000 / -1 then shows as the quotient that does not fit */
        put(e, a64_bitfield(1, 0, R_RES, dq, 0, 31));
    if (is_signed)
        put(e, a64_bitfield(1, 0, R_SRC, v, 0, 31));
    else
        put(e, a64_mov(R_SRC, v));
    put(e, a64_div(1, is_signed, R_SCR, R_RES, R_SRC));
    put(e, a64_msub(1, R_F1, R_SCR, R_SRC, R_RES));
    if (is_signed)
    {
        put(e, a64_bitfield(1, 0, R_F2, R_SCR, 0, 31));
        put(e, a64_addsub_reg(1, 1, 1, ZR, R_SCR, R_F2, 0, 0));
    }
    else
    {
        put(e, a64_bitfield(1, 2, R_F2, R_SCR, 32, 63));
        put(e, a64_addsub_imm(0, 1, 1, ZR, R_F2, 0, 0));
    }
    br_ovf = e->n;
    put(e, a64_bcond(A64_NE, 0));
    put(e, a64_mov(dr, R_F1));
    put(e, a64_mov(dq, R_SCR));
    if (in->need & HJF_NZVC)
        emit_flags_logic(e, dq, 4);
    br_done = e->n;
    put(e, a64_b(0));
    patch(e, br_ovf, e->n);
    /* overflow, as in the word division: V set and C cleared; the
       registers, N and Z stay as they were */
    if (in->need & HJF_C)
        put(e, a64_logic_const(0, R_FLAGS, R_FLAGS, ~HJF_C));
    if (in->need & HJF_V)
        put(e, a64_logic_const(1, R_FLAGS, R_FLAGS, HJF_V));
    patch(e, br_done, e->n);
}

static void emit_bf(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    uint16_t ext = in->mask;
    int dn = (ext >> 12) & 7;
    int do_reg = (ext >> 11) & 1, of = (ext >> 6) & 31;
    int dw_reg = (ext >> 5) & 1, wi = ext & 31;
    int modifies = in->op == BF_CHG || in->op == BF_CLR ||
                   in->op == BF_SET || in->op == BF_INS;
    unsigned br_no5, br_no5_w;

    if (in->dst.mode != EA_DN)
    {
        int areg = emit_address(t, &in->dst, 1);

        if (areg != R_ADDR)
            put(e, a64_mov(R_ADDR, areg));
    }
    if (do_reg)
        put(e, a64_mov(R_SRC, REG_D(of & 7)));
    else
        emit_mov32(e, R_SRC, (uint32_t)of);
    if (dw_reg)
    {
        put(e, a64_addsub_imm(0, 1, 0, R_F1, REG_D(wi & 7), 1, 0));
        put(e, a64_logic_const(0, R_F1, R_F1, 31u));
        put(e, a64_addsub_imm(0, 0, 0, R_F1, R_F1, 1, 0));
    }
    else
        emit_mov32(e, R_F1, wi ? (uint32_t)wi : 32u);
    emit_mov32(e, R_F2, 32u);
    put(e, a64_addsub_reg(0, 1, 0, R_F2, R_F2, R_F1, 0, 0));

    if (in->dst.mode == EA_DN)
    {
        int dy = REG_D(in->dst.reg);

        put(e, a64_addsub_reg(0, 1, 0, R_SCR, ZR, R_SRC, 0, 0));
        put(e, a64_shiftv(0, 3, R_RES, dy, R_SCR));
        if (in->op == BF_INS)
        {
            put(e, a64_shiftv(0, 0, R_SCR, REG_D(dn), R_F2));
            put(e, a64_movw(0, 0, R_F1, 0, 0));
            put(e, a64_shiftv(0, 0, R_F1, R_F1, R_F2));
            put(e, a64_logic_reg(0, 0, 1, R_RES, R_RES, R_F1, 0, 0));
            put(e, a64_logic_reg(0, 1, 0, R_RES, R_RES, R_SCR, 0, 0));
            put(e, a64_shiftv(0, 3, dy, R_RES, R_SRC));
            if (in->need & HJF_NZVC)
                emit_flags_logic(e, R_SCR, 4);
            return;
        }
        put(e, a64_shiftv(0, 1, R_SCR, R_RES, R_F2));
        put(e, a64_shiftv(0, 0, R_SCR, R_SCR, R_F2));
        switch (in->op)
        {
        case BF_TST:
            break;
        case BF_EXTU:
            put(e, a64_shiftv(0, 1, REG_D(dn), R_RES, R_F2));
            break;
        case BF_EXTS:
            put(e, a64_shiftv(0, 2, REG_D(dn), R_RES, R_F2));
            break;
        case BF_FFO:
            put(e, a64_clz(R_F2, R_SCR));
            put(e, a64_cmp(R_F2, R_F1));
            put(e, a64_csel(0, R_F2, R_F2, R_F1, A64_LO));
            put(e, a64_addsub_reg(0, 0, 0, REG_D(dn), R_SRC, R_F2, 0, 0));
            break;
        default:
            put(e, a64_movw(0, 0, R_F1, 0, 0));
            put(e, a64_shiftv(0, 0, R_F1, R_F1, R_F2));
            if (in->op == BF_CHG)
                put(e, a64_logic_reg(0, 2, 0, R_RES, R_RES, R_F1, 0, 0));
            else if (in->op == BF_CLR)
                put(e, a64_logic_reg(0, 0, 1, R_RES, R_RES, R_F1, 0, 0));
            else
                put(e, a64_logic_reg(0, 1, 0, R_RES, R_RES, R_F1, 0, 0));
            put(e, a64_shiftv(0, 3, dy, R_RES, R_SRC));
            break;
        }
        if (in->need & HJF_NZVC)
            emit_flags_logic(e, R_SCR, 4);
        return;
    }

    put(e, a64_addsub_reg(0, 0, 0, R_ADDR, R_ADDR, R_SRC, 2, 3));
    emit_access(t, SLOW_R32, R_ADDR);
    put(e, a64_mov(R_RES, R_VAL));
    put(e, a64_bitfield(1, 2, R_RES, R_RES, 56, 55));
    if (!modifies)
    {
        put(e, a64_logic_const(0, R_SCR, R_SRC, 7u));
        put(e, a64_addsub_reg(0, 0, 0, R_SCR, R_SCR, R_F1, 0, 0));
        put(e, a64_addsub_imm(0, 1, 1, ZR, R_SCR, 32, 0));
        br_no5 = e->n;
        put(e, a64_bcond(A64_LS, 0));
        put(e, a64_addsub_imm(0, 0, 0, R_ADDR, R_ADDR, 4, 0));
        emit_access(t, SLOW_R8, R_ADDR);
        put(e, a64_addsub_imm(0, 1, 0, R_ADDR, R_ADDR, 4, 0));
        put(e, a64_logic_reg(1, 1, 0, R_RES, R_RES, R_VAL, 0, 0));
        patch(e, br_no5, e->n);
    }
    emit_mov32(e, R_F2, 32u);
    put(e, a64_addsub_reg(0, 1, 0, R_F2, R_F2, R_F1, 0, 0));

    if (in->op == BF_TST || in->op == BF_EXTU || in->op == BF_EXTS || in->op == BF_FFO)
    {
        put(e, a64_logic_const(0, R_SCR, R_SRC, 7u));
        put(e, a64_shiftv(1, 0, R_SCR, R_RES, R_SCR));
        put(e, a64_bitfield(1, 2, R_SCR, R_SCR, 8, 63));
        put(e, a64_shiftv(0, 1, R_SCR, R_SCR, R_F2));
        if (in->op == BF_EXTU)
            put(e, a64_mov(REG_D(dn), R_SCR));
        put(e, a64_shiftv(0, 0, R_SCR, R_SCR, R_F2));
        if (in->op == BF_EXTS)
            put(e, a64_shiftv(0, 2, REG_D(dn), R_SCR, R_F2));
        else if (in->op == BF_FFO)
        {
            put(e, a64_clz(R_F2, R_SCR));
            put(e, a64_cmp(R_F2, R_F1));
            put(e, a64_csel(0, R_F2, R_F2, R_F1, A64_LO));
            put(e, a64_addsub_reg(0, 0, 0, REG_D(dn), R_SRC, R_F2, 0, 0));
        }
        if (in->need & HJF_NZVC)
            emit_flags_logic(e, R_SCR, 4);
        return;
    }

    if (in->op == BF_INS)
        put(e, a64_shiftv(0, 0, R_SCR, REG_D(dn), R_F2));
    else
    {
        put(e, a64_logic_const(0, R_SCR, R_SRC, 7u));
        put(e, a64_shiftv(1, 0, R_SCR, R_RES, R_SCR));
        put(e, a64_bitfield(1, 2, R_SCR, R_SCR, 8, 63));
        put(e, a64_shiftv(0, 1, R_SCR, R_SCR, R_F2));
        put(e, a64_shiftv(0, 0, R_SCR, R_SCR, R_F2));
    }
    if (in->need & HJF_NZVC)
    {
        put(e, a64_tst(R_SCR, R_SCR));
        put(e, a64_mrs_nzcv(R_VAL));
        put(e, a64_lsr(R_VAL, R_VAL, 28));
        put(e, a64_bfi(R_FLAGS, R_VAL, 0, 4));
    }
    put(e, a64_movw(0, 0, R_VAL, 0, 0));
    put(e, a64_shiftv(0, 0, R_VAL, R_VAL, R_F2));
    put(e, a64_bitfield(1, 2, R_VAL, R_VAL, 56, 55));
    put(e, a64_logic_const(0, R_F2, R_SRC, 7u));
    put(e, a64_shiftv(1, 1, R_VAL, R_VAL, R_F2));
    if (in->op == BF_CHG)
        put(e, a64_logic_reg(1, 2, 0, R_RES, R_RES, R_VAL, 0, 0));
    else if (in->op == BF_CLR)
        put(e, a64_logic_reg(1, 0, 1, R_RES, R_RES, R_VAL, 0, 0));
    else if (in->op == BF_SET)
        put(e, a64_logic_reg(1, 1, 0, R_RES, R_RES, R_VAL, 0, 0));
    else
    {
        put(e, a64_bitfield(1, 2, R_SCR, R_SCR, 56, 55));
        put(e, a64_shiftv(1, 1, R_SCR, R_SCR, R_F2));
        put(e, a64_logic_reg(1, 0, 1, R_RES, R_RES, R_VAL, 0, 0));
        put(e, a64_logic_reg(1, 1, 0, R_RES, R_RES, R_SCR, 0, 0));
    }
    put(e, a64_bitfield(1, 2, R_VAL, R_RES, 8, 63));
    emit_access(t, SLOW_W32, R_ADDR);
    put(e, a64_logic_const(0, R_SCR, R_SRC, 7u));
    put(e, a64_addsub_reg(0, 0, 0, R_SCR, R_SCR, R_F1, 0, 0));
    put(e, a64_addsub_imm(0, 1, 1, ZR, R_SCR, 32, 0));
    br_no5_w = e->n;
    put(e, a64_bcond(A64_LS, 0));
    put(e, a64_addsub_imm(0, 0, 0, R_ADDR, R_ADDR, 4, 0));
    /* Moira commits the leading longword before it reads or writes the
       fifth byte. A callback fault here must preserve that completed
       prefix. The trailing byte also contributes to the original field's
       Z flag (or the inserted field for BFINS). */
    emit_access(t, SLOW_R8, R_ADDR);
    put(e, a64_logic_const(0, R_SCR, R_SRC, 7u));
    put(e, a64_addsub_reg(0, 0, 0, R_SCR, R_SCR, R_F1, 0, 0));
    emit_mov32(e, R_F2, 40u);
    put(e, a64_addsub_reg(0, 1, 0, R_F2, R_F2, R_SCR, 0, 0));
    put(e, a64_movw(0, 0, R_RES, 0, 0));
    put(e, a64_shiftv(0, 0, R_RES, R_RES, R_F2));
    put(e, a64_logic_const(0, R_RES, R_RES, 255u));
    if (in->op == BF_INS)
    {
        put(e, a64_shiftv(0, 0, R_SCR, REG_D(dn), R_F2));
        put(e, a64_logic_const(0, R_SCR, R_SCR, 255u));
    }
    else
        put(e, a64_logic_reg(0, 0, 0, R_SCR, R_VAL, R_RES, 0, 0));
    if (in->need & HJF_Z)
    {
        unsigned br_zero = e->n;
        put(e, a64_cbz(0, 0, R_SCR, 0));
        put(e, a64_logic_const(0, R_FLAGS, R_FLAGS, ~HJF_Z));
        patch(e, br_zero, e->n);
    }
    if (in->op == BF_CHG)
        put(e, a64_logic_reg(0, 2, 0, R_VAL, R_VAL, R_RES, 0, 0));
    else if (in->op == BF_CLR)
        put(e, a64_logic_reg(0, 0, 1, R_VAL, R_VAL, R_RES, 0, 0));
    else if (in->op == BF_SET)
        put(e, a64_logic_reg(0, 1, 0, R_VAL, R_VAL, R_RES, 0, 0));
    else
    {
        put(e, a64_logic_reg(0, 0, 1, R_VAL, R_VAL, R_RES, 0, 0));
        put(e, a64_logic_reg(0, 1, 0, R_VAL, R_VAL, R_SCR, 0, 0));
    }
    emit_access(t, SLOW_W8, R_ADDR);
    patch(e, br_no5_w, e->n);
}

static void emit_cas(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int areg = emit_address(t, &in->dst, in->size);
    int dc = REG_D(in->src.reg), du = REG_D(in->count);
    int shift = 32 - 8 * in->size;
    unsigned br_ne, br_done;

    emit_access(t, read_kind(in->size), areg);
    put(e, a64_mov(R_RES, R_VAL));
    if (in->size == 4)
        put(e, a64_addsub_reg(0, 1, 1, ZR, R_RES, dc, 0, 0));
    else
    {
        put(e, a64_lsl(R_SCR, R_RES, shift));
        put(e, a64_addsub_reg(0, 1, 1, ZR, R_SCR, dc, 0, shift));
    }
    if (in->need & HJF_NZVC)
        emit_nzcv_to_flags(e, 1, 0);
    br_ne = e->n;
    put(e, a64_bcond(A64_NE, 0));
    put(e, a64_mov(R_VAL, du));
    emit_access(t, write_kind(in->size), areg);
    br_done = e->n;
    put(e, a64_b(0));
    patch(e, br_ne, e->n);
    emit_insert(e, dc, R_RES, in->size);
    patch(e, br_done, e->n);
    emit_post(t, &in->dst, in->size);
}

static void emit_movep(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int dx = REG_D(in->dst.reg), size = in->size, i;

    emit_add_const(e, R_ADDR, REG_A(in->src.reg), in->src.disp);
    if (in->op)
    {
        for (i = 0; i < size; i++)
        {
            int shift = 8 * (size - 1 - i);

            if (shift)
                put(e, a64_lsr(R_VAL, dx, shift));
            else
                put(e, a64_mov(R_VAL, dx));
            emit_access(t, SLOW_W8, R_ADDR);
            if (i + 1 < size)
                put(e, a64_addsub_imm(0, 0, 0, R_ADDR, R_ADDR, 2, 0));
        }
        return;
    }
    for (i = 0; i < size; i++)
    {
        int shift = 8 * (size - 1 - i);

        emit_access(t, SLOW_R8, R_ADDR);
        if (i == 0)
            put(e, a64_lsl(R_RES, R_VAL, shift));
        else
            put(e, a64_bfi(R_RES, R_VAL, shift, 8));
        if (i + 1 < size)
            put(e, a64_addsub_imm(0, 0, 0, R_ADDR, R_ADDR, 2, 0));
    }
    emit_insert(e, dx, R_RES, size);
}

static void emit_shift(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int dn = REG_D(in->dst.reg), n = in->count;
    unsigned need = in->need;

    if (in->size == 4 && in->src.mode == EA_IMM && in->dst.mode == EA_DN &&
        in->op != SH_ASL && in->op != SH_ROXL && in->op != SH_ROXR)
    {
        switch (in->op)
        {
        case SH_LSL:
            if (need) put(e, a64_ubfx(R_F2, dn, 32 - n, 1));
            put(e, a64_lsl(dn, dn, n));
            break;
        case SH_LSR:
        case SH_ASR:
            if (need) put(e, a64_ubfx(R_F2, dn, n - 1, 1));
            put(e, in->op == SH_LSR ? a64_lsr(dn, dn, n) : a64_asr(dn, dn, n));
            break;
        case SH_ROL:
            put(e, a64_ror(dn, dn, 32 - n));
            if (need) put(e, a64_ubfx(R_F2, dn, 0, 1));
            break;
        default:
            put(e, a64_ror(dn, dn, n));
            if (need) put(e, a64_lsr(R_F2, dn, 31));
            break;
        }
        if (need & HJF_NZVC)
        {
            put(e, a64_tst(dn, dn));
            put(e, a64_mrs_nzcv(R_F1));
            put(e, a64_lsr(R_F1, R_F1, 28));
            put(e, a64_bfi(R_F1, R_F2, 1, 1));
            put(e, a64_bfi(R_FLAGS, R_F1, 0, 4));
        }
        if ((need & HJF_X) && in->op != SH_ROL && in->op != SH_ROR)
            put(e, a64_bfi(R_FLAGS, R_F2, 4, 1));
        return;
    }
    {
        int w = 8 * in->size, shift = 32 - w, areg = -1;
        unsigned br_zero, loop_top;

        if (in->dst.mode == EA_DN)
            put(e, in->size == 1 ? a64_uxtb(R_RES, dn) :
                   in->size == 2 ? a64_uxth(R_RES, dn) : a64_mov(R_RES, dn));
        else
        {
            areg = emit_address(t, &in->dst, 2);
            emit_access(t, SLOW_R16, areg);
            put(e, a64_uxth(R_RES, R_VAL));
        }
        if (in->src.mode == EA_IMM)
            emit_mov32(e, R_SRC, in->count);
        else
            put(e, a64_logic_const(0, R_SRC, REG_D(in->src.reg), 0x3Fu));
        if (in->op == SH_ROXL || in->op == SH_ROXR)
            put(e, a64_ubfx(R_F1, R_FLAGS, 4, 1));
        else
            put(e, a64_mov(R_F1, ZR));
        if (in->op == SH_ASL)
            put(e, a64_mov(R_F2, ZR));
        br_zero = e->n;
        put(e, a64_cbz(0, 0, R_SRC, 0));
        loop_top = e->n;
        switch (in->op)
        {
        case SH_LSL:
            put(e, a64_ubfx(R_F1, R_RES, w - 1, 1));
            put(e, a64_lsl(R_RES, R_RES, 1));
            break;
        case SH_ASL:
            put(e, a64_ubfx(R_F1, R_RES, w - 1, 1));
            put(e, a64_lsl(R_SCR, R_RES, 1));
            put(e, a64_logic_reg(0, 2, 0, R_SCR, R_RES, R_SCR, 0, 0));
            put(e, a64_logic_reg(0, 1, 0, R_F2, R_F2, R_SCR, 0, 0));
            put(e, a64_lsl(R_RES, R_RES, 1));
            break;
        case SH_ROL:
            put(e, a64_ubfx(R_F1, R_RES, w - 1, 1));
            put(e, a64_lsl(R_RES, R_RES, 1));
            put(e, a64_logic_reg(0, 1, 0, R_RES, R_RES, R_F1, 0, 0));
            break;
        case SH_ROXL:
            put(e, a64_mov(R_SCR, R_F1));
            put(e, a64_ubfx(R_F1, R_RES, w - 1, 1));
            put(e, a64_lsl(R_RES, R_RES, 1));
            put(e, a64_logic_reg(0, 1, 0, R_RES, R_RES, R_SCR, 0, 0));
            break;
        case SH_LSR:
            put(e, a64_ubfx(R_F1, R_RES, 0, 1));
            put(e, a64_lsr(R_RES, R_RES, 1));
            break;
        case SH_ASR:
            put(e, a64_ubfx(R_F1, R_RES, 0, 1));
            put(e, a64_ubfx(R_SCR, R_RES, w - 1, 1));
            put(e, a64_lsr(R_RES, R_RES, 1));
            put(e, a64_bfi(R_RES, R_SCR, w - 1, 1));
            break;
        case SH_ROR:
            put(e, a64_ubfx(R_F1, R_RES, 0, 1));
            put(e, a64_lsr(R_RES, R_RES, 1));
            put(e, a64_bfi(R_RES, R_F1, w - 1, 1));
            break;
        default:
            put(e, a64_mov(R_SCR, R_F1));
            put(e, a64_ubfx(R_F1, R_RES, 0, 1));
            put(e, a64_lsr(R_RES, R_RES, 1));
            put(e, a64_bfi(R_RES, R_SCR, w - 1, 1));
            break;
        }
        put(e, a64_addsub_imm(0, 1, 1, R_SRC, R_SRC, 1, 0));
        put(e, a64_bcond(A64_NE, (int32_t)loop_top - (int32_t)e->n));
        if ((need & HJF_X) && in->op != SH_ROL && in->op != SH_ROR)
            put(e, a64_bfi(R_FLAGS, R_F1, 4, 1));
        patch(e, br_zero, e->n);
        if (need & HJF_NZVC)
        {
            if (in->size == 4)
                put(e, a64_tst(R_RES, R_RES));
            else
            {
                put(e, a64_lsl(R_SCR, R_RES, shift));
                put(e, a64_tst(R_SCR, R_SCR));
            }
            put(e, a64_mrs_nzcv(R_SCR));
            put(e, a64_lsr(R_SCR, R_SCR, 28));
            put(e, a64_bfi(R_SCR, R_F1, 1, 1));
            if (in->op == SH_ASL)
            {
                put(e, a64_ubfx(R_F2, R_F2, w - 1, 1));
                put(e, a64_bfi(R_SCR, R_F2, 0, 1));
            }
            put(e, a64_bfi(R_FLAGS, R_SCR, 0, 4));
        }
        if (in->dst.mode == EA_DN)
            emit_insert(e, dn, R_RES, in->size);
        else
        {
            put(e, a64_mov(R_VAL, R_RES));
            emit_access(t, SLOW_W16, areg);
            emit_post(t, &in->dst, 2);
        }
    }
}

static void emit_movem(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int r, an = REG_A(in->src.reg), step = in->size;
    int rkind = read_kind(step), wkind = write_kind(step);

    if (in->count == 0 && in->src.mode == EA_PRE)
    {
        for (r = 15; r >= 0; r--)
            if (in->mask & (1u << r))
            {
                put(e, a64_addsub_imm(0, 1, 0, an, an, (uint32_t)step, 0));
                put(e, a64_mov(R_VAL, r));
                emit_access(t, wkind, an);
            }
        return;
    }
    if (in->count == 1 && in->src.mode == EA_POST)
    {
        for (r = 0; r < 16; r++)
            if (in->mask & (1u << r))
            {
                emit_access(t, rkind, an);
                put(e, a64_addsub_imm(0, 0, 0, an, an, (uint32_t)step, 0));
                put(e, step == 2 ? a64_sxth(r, R_VAL) : a64_mov(r, R_VAL));
            }
        return;
    }
    put(e, a64_mov(R_RES, emit_address(t, &in->src, step)));
    for (r = 0; r < 16; r++)
        if (in->mask & (1u << r))
        {
            if (in->count == 0)
            {
                put(e, a64_mov(R_VAL, r));
                emit_access(t, wkind, R_RES);
            }
            else
            {
                emit_access(t, rkind, R_RES);
                put(e, step == 2 ? a64_sxth(r, R_VAL) : a64_mov(r, R_VAL));
            }
            put(e, a64_addsub_imm(0, 0, 0, R_RES, R_RES, (uint32_t)step, 0));
        }
}

/* 68k CCR (C bit 0, V bit 1) to and from the flags (V bit 0, C bit 1). */
static uint32_t swap_cv(uint32_t f) { return (f & 0x1Cu) | ((f & 1u) << 1) | ((f >> 1) & 1u); }

static void emit_ccr_to_flags(struct HJEmit *e, int src)
{
    put(e, a64_logic_const(0, R_F1, src, 0x1C));
    put(e, a64_ubfx(R_F2, src, 0, 1));
    put(e, a64_logic_reg(0, 1, 0, R_F1, R_F1, R_F2, 0, 1));
    put(e, a64_ubfx(R_F2, src, 1, 1));
    put(e, a64_logic_reg(0, 1, 0, R_FLAGS, R_F1, R_F2, 0, 0));
}

static void emit_insn(struct HJTrans *t, const struct HJInsn *in)
{
    struct HJEmit *e = &t->e;
    int v, an, size = in->size;
    unsigned br, br2;

    t->in = in;
    switch (in->kind)
    {
    case K_NOP:
        break;
    case K_MOVEQ:
        emit_mov32(e, REG_D(in->dst.reg), in->src.value);
        if (in->need)
            emit_flags_const(e, in->src.value == 0 ? HJF_Z : (in->src.value >> 31) ? HJF_N : 0);
        break;
    case K_MOVE:
        if (in->src.mode == EA_IMM && in->dst.mode == EA_DN && size == 4)
        {
            emit_mov32(e, REG_D(in->dst.reg), in->src.value);
            if (in->need)
                emit_flags_const(e, in->src.value == 0 ? HJF_Z : (in->src.value >> 31) ? HJF_N : 0);
            break;
        }
        v = emit_read(t, &in->src, size);
        if (in->src.mode == EA_AN && (in->dst.mode == EA_PRE || in->dst.mode == EA_POST) &&
            in->dst.reg == in->src.reg)
        {
            put(e, a64_mov(R_SRC, v));
            v = R_SRC;
        }
        if (in->need)
            emit_flags_logic(e, v, size);
        emit_write(t, &in->dst, size, v);
        break;
    case K_MOVEA:
        v = emit_read(t, &in->src, size);
        an = REG_A(in->dst.reg);
        put(e, size == 2 ? a64_sxth(an, v) : a64_mov(an, v));
        break;
    case K_LEA:
        an = emit_address(t, &in->src, 4);
        if (an != REG_A(in->dst.reg))
            put(e, a64_mov(REG_A(in->dst.reg), an));
        break;
    case K_PEA:
        an = emit_address(t, &in->src, 4);
        put(e, a64_mov(R_RES, an));
        put(e, a64_addsub_imm(0, 1, 0, REG_A(7), REG_A(7), 4, 0));
        put(e, a64_mov(R_VAL, R_RES));
        emit_access(t, SLOW_W32, REG_A(7));
        break;
    case K_LINK:
        put(e, a64_addsub_imm(0, 1, 0, REG_A(7), REG_A(7), 4, 0));
        put(e, a64_mov(R_VAL, REG_A(in->dst.reg)));
        emit_access(t, SLOW_W32, REG_A(7));
        if (in->dst.reg != 7)
            put(e, a64_mov(REG_A(in->dst.reg), REG_A(7)));
        emit_add_const(e, REG_A(7), REG_A(7), in->src.disp);
        break;
    case K_UNLK:
        if (in->dst.reg != 7)
            put(e, a64_mov(REG_A(7), REG_A(in->dst.reg)));
        emit_access(t, SLOW_R32, REG_A(7));
        if (in->dst.reg != 7)
            put(e, a64_addsub_imm(0, 0, 0, REG_A(7), REG_A(7), 4, 0));
        put(e, a64_mov(REG_A(in->dst.reg), R_VAL));
        break;
    case K_EXG:
        if (in->src.reg != in->dst.reg)
        {
            put(e, a64_mov(R_SCR, in->src.reg));
            put(e, a64_mov(in->src.reg, in->dst.reg));
            put(e, a64_mov(in->dst.reg, R_SCR));
        }
        break;
    case K_CMPM:
    {
        int ay = REG_A(in->src.reg), ax = REG_A(in->dst.reg);

        emit_access(t, read_kind(size), ay);
        put(e, a64_addsub_imm(0, 0, 0, ay, ay, (uint32_t)step_of(size, in->src.reg), 0));
        put(e, a64_mov(R_SRC, R_VAL));
        emit_access(t, read_kind(size), ax);
        put(e, a64_addsub_imm(0, 0, 0, ax, ax, (uint32_t)step_of(size, in->dst.reg), 0));
        emit_alu(t, OP_CMP, size, R_VAL, R_SRC, ZR, in->need);
        break;
    }
    case K_ALU:
        v = emit_read(t, &in->src, size);
        emit_alu(t, in->op, size, REG_D(in->dst.reg), v, REG_D(in->dst.reg), in->need);
        break;
    case K_ALUM:
        if (in->dst.mode == EA_DN)
            emit_alu(t, in->op, size, REG_D(in->dst.reg), REG_D(in->src.reg), REG_D(in->dst.reg), in->need);
        else
            emit_rmw(t, in, in->op, REG_D(in->src.reg));
        break;
    case K_ALUI:
        emit_mov32(e, R_SRC, in->src.value);
        if (in->dst.mode == EA_DN)
            emit_alu(t, in->op, size, REG_D(in->dst.reg), R_SRC, REG_D(in->dst.reg), in->need);
        else
            emit_rmw(t, in, in->op, R_SRC);
        break;
    case K_QUICK:
        if (in->dst.mode == EA_AN)
        {
            an = REG_A(in->dst.reg);
            put(e, a64_addsub_imm(0, in->op == OP_SUB, 0, an, an, in->count, 0));
            break;
        }
        emit_mov32(e, R_SRC, in->count);
        if (in->dst.mode == EA_DN)
            emit_alu(t, in->op, size, REG_D(in->dst.reg), R_SRC, REG_D(in->dst.reg), in->need);
        else
            emit_rmw(t, in, in->op, R_SRC);
        break;
    case K_ADDX:
        emit_addx(t, in);
        break;
    case K_MULW:
        v = emit_read(t, &in->src, 2);
        an = REG_D(in->dst.reg);
        put(e, in->op ? a64_sxth(R_SCR, v) : a64_uxth(R_SCR, v));
        put(e, in->op ? a64_sxth(R_F1, an) : a64_uxth(R_F1, an));
        put(e, a64_mul(an, R_SCR, R_F1));
        if (in->need)
            emit_flags_logic(e, an, 4);
        break;
    case K_MULL:
        emit_mull(t, in);
        break;
    case K_DIVW:
        emit_divw(t, in);
        break;
    case K_DIVL:
        emit_divl(t, in);
        break;
    case K_BIT:
        emit_bit(t, in);
        break;
    case K_BF:
        emit_bf(t, in);
        break;
    case K_CAS:
        emit_cas(t, in);
        break;
    case K_MOVEP:
        emit_movep(t, in);
        break;
    case K_TAS:
        if (in->dst.mode == EA_DN)
        {
            int dn = REG_D(in->dst.reg);

            if (in->need)
                emit_flags_logic(e, dn, 1);
            put(e, a64_logic_const(1, dn, dn, 0x80u));
            break;
        }
        an = emit_address(t, &in->dst, 1);
        emit_access(t, SLOW_R8, an);
        if (in->need)
            emit_flags_logic(e, R_VAL, 1);
        put(e, a64_logic_const(1, R_VAL, R_VAL, 0x80u));
        emit_access(t, SLOW_W8, an);
        emit_post(t, &in->dst, 1);
        break;
    case K_ALUA:
        v = emit_read(t, &in->src, size);
        if (size == 2)
        {
            put(e, a64_sxth(R_SRC, v));
            v = R_SRC;
        }
        an = REG_A(in->dst.reg);
        if (in->op == OP_CMP)
        {
            put(e, a64_addsub_reg(0, 1, in->need != 0, ZR, an, v, 0, 0));
            emit_flags_arith(e, 1, in->need & HJF_NZVC);
        }
        else
            put(e, a64_addsub_reg(0, in->op == OP_SUB, 0, an, an, v, 0, 0));
        break;
    case K_CLR:
        if (in->dst.mode == EA_DN)
        {
            int dn = REG_D(in->dst.reg);

            if (size == 4)
                put(e, a64_mov(dn, ZR));
            else
                put(e, a64_logic_const(0, dn, dn, size == 2 ? 0xFFFF0000u : 0xFFFFFF00u));
        }
        else
            emit_write(t, &in->dst, size, ZR);
        if (in->need)
            emit_flags_const(e, HJF_Z);
        break;
    case K_TST:
        v = emit_read(t, &in->dst, size);
        if (in->need)
            emit_flags_logic(e, v, size);
        break;
    case K_NEG:
    case K_NOT:
    {
        int dn = in->dst.mode == EA_DN ? REG_D(in->dst.reg) : -1;
        int areg = -1, val;

        if (dn < 0)
        {
            areg = emit_address(t, &in->dst, size);
            emit_access(t, read_kind(size), areg);
            val = R_VAL;
        }
        else
            val = dn;
        if (in->kind == K_NEG)
            emit_alu(t, OP_SUB, size, ZR, val, dn < 0 ? R_RES : dn, in->need);
        else
        {
            put(e, a64_logic_reg(0, 1, 1, R_SCR, ZR, val, 0, 0));
            if (in->need)
                emit_flags_logic(e, R_SCR, size);
            emit_insert(e, dn < 0 ? R_RES : dn, R_SCR, dn < 0 ? 4 : size);
        }
        if (dn < 0)
        {
            put(e, a64_mov(R_VAL, R_RES));
            emit_access(t, write_kind(size), areg);
            emit_post(t, &in->dst, size);
        }
        break;
    }
    case K_SWAP:
        put(e, a64_ror(REG_D(in->dst.reg), REG_D(in->dst.reg), 16));
        if (in->need)
            emit_flags_logic(e, REG_D(in->dst.reg), 4);
        break;
    case K_EXTW:
        put(e, a64_sxtb(R_SCR, REG_D(in->dst.reg)));
        put(e, a64_bfi(REG_D(in->dst.reg), R_SCR, 0, 16));
        if (in->need)
            emit_flags_logic(e, R_SCR, 4);
        break;
    case K_EXTL:
        put(e, a64_sxth(REG_D(in->dst.reg), REG_D(in->dst.reg)));
        if (in->need)
            emit_flags_logic(e, REG_D(in->dst.reg), 4);
        break;
    case K_EXTBL:
        put(e, a64_sxtb(REG_D(in->dst.reg), REG_D(in->dst.reg)));
        if (in->need)
            emit_flags_logic(e, REG_D(in->dst.reg), 4);
        break;
    case K_SHIFT:
        emit_shift(t, in);
        break;
    case K_SCC:
        br = emit_cond_branch(e, in->cond);
        put(e, a64_mov(R_RES, ZR));
        br2 = e->n;
        put(e, a64_b(0));
        if (br != ~0u)
            patch(e, br, e->n);
        emit_mov32(e, R_RES, 0xFF);
        patch(e, br2, e->n);
        emit_write(t, &in->dst, 1, R_RES);
        break;
    case K_MOVEFROMSR:
    case K_MOVEFROMCCR:
        put(e, a64_logic_const(0, R_F1, R_FLAGS, 0x1C));
        put(e, a64_ubfx(R_F2, R_FLAGS, 1, 1));
        put(e, a64_logic_reg(0, 1, 0, R_F1, R_F1, R_F2, 0, 0));
        put(e, a64_ubfx(R_F2, R_FLAGS, 0, 1));
        put(e, a64_logic_reg(0, 1, 0, R_RES, R_F1, R_F2, 0, 1));
        if (in->kind == K_MOVEFROMSR)
        {
            put(e, a64_ldst_uimm(2, 1, R_F2, R_STATE, ST_SRHI));
            put(e, a64_logic_reg(0, 1, 0, R_RES, R_RES, R_F2, 0, 8));
        }
        emit_write(t, &in->dst, 2, R_RES);
        break;
    case K_MOVETOSR:
    case K_MOVETOCCR:
        v = emit_read(t, &in->src, 2);
        if (in->kind == K_MOVETOSR)
        {
            put(e, a64_ubfx(R_F1, v, 8, 8));
            put(e, a64_logic_const(0, R_F1, R_F1, ~0x08u));   /* no SR bit 11 */
            put(e, a64_ldst_uimm(2, 0, R_F1, R_STATE, ST_SRHI));
        }
        emit_ccr_to_flags(e, v);
        break;
    case K_LOGICSR:
    {
        int opc = in->op == OP_AND ? 0 : in->op == OP_OR ? 1 : 2;
        uint32_t fmask = swap_cv(in->src.value & 0x1Fu);

        emit_mov32(e, R_F1, fmask);
        put(e, a64_logic_reg(0, opc, 0, R_FLAGS, R_FLAGS, R_F1, 0, 0));
        if (size == 2)
        {
            uint32_t smask = (in->src.value >> 8) & 0xF7u;

            put(e, a64_ldst_uimm(2, 1, R_F1, R_STATE, ST_SRHI));
            emit_mov32(e, R_F2, smask);
            put(e, a64_logic_reg(0, opc, 0, R_F1, R_F1, R_F2, 0, 0));
            put(e, a64_logic_const(0, R_F1, R_F1, ~0x08u));
            put(e, a64_ldst_uimm(2, 0, R_F1, R_STATE, ST_SRHI));
        }
        break;
    }
    case K_MOVEM:
        emit_movem(t, in);
        break;
    case K_BCC:
        if (in->cond == 0)
        {
            emit_exit_to(t, in->target);
            break;
        }
        br = emit_cond_branch(e, in->cond);
        emit_exit_to(t, in->next);
        if (br != ~0u)
        {
            patch(e, br, e->n);
            emit_exit_to(t, in->target);
        }
        break;
    case K_BSR:
        emit_push(t, in->next);
        emit_exit_to(t, in->target);
        break;
    case K_DBCC:
    {
        int dn = REG_D(in->dst.reg);

        br = emit_cond_branch(e, in->cond);
        put(e, a64_logic_const(3, ZR, dn, 0xFFFF));
        put(e, a64_addsub_imm(0, 1, 0, R_F1, dn, 1, 0));
        put(e, a64_bfi(dn, R_F1, 0, 16));
        br2 = e->n;
        put(e, a64_bcond(A64_EQ, 0));
        emit_exit_to(t, in->target);
        if (br != ~0u)
            patch(e, br, e->n);
        patch(e, br2, e->n);
        emit_exit_to(t, in->next);
        break;
    }
    case K_JSR:
    case K_JMP:
        if (in->src.mode == EA_ABS || in->src.mode == EA_PCDISP)
        {
            if (in->kind == K_JSR)
                emit_push(t, in->next);
            emit_exit_to(t, in->src.value);
            break;
        }
        put(e, a64_mov(R_RES, emit_address(t, &in->src, 4)));
        if (in->kind == K_JSR)
            emit_push(t, in->next);
        put(e, a64_mov(R_ADDR, R_RES));
        emit_glue_jump(t, t->g_lookup);
        break;
    case K_RTS:
        emit_access(t, SLOW_R32, REG_A(7));
        put(e, a64_addsub_imm(0, 0, 0, REG_A(7), REG_A(7), 4, 0));
        put(e, a64_mov(R_ADDR, R_VAL));
        emit_glue_jump(t, t->g_lookup);
        break;
    case K_RTR:
        emit_access(t, SLOW_R16, REG_A(7));
        put(e, a64_addsub_imm(0, 0, 0, REG_A(7), REG_A(7), 2, 0));
        emit_ccr_to_flags(e, R_VAL);
        emit_access(t, SLOW_R32, REG_A(7));
        put(e, a64_addsub_imm(0, 0, 0, REG_A(7), REG_A(7), 4, 0));
        put(e, a64_mov(R_ADDR, R_VAL));
        emit_glue_jump(t, t->g_lookup);
        break;
    case K_RTD:
        emit_access(t, SLOW_R32, REG_A(7));
        emit_add_const(e, REG_A(7), REG_A(7), 4 + in->src.disp);
        put(e, a64_mov(R_ADDR, R_VAL));
        emit_glue_jump(t, t->g_lookup);
        break;
    case K_LINEA:
        emit_exit_reason(t, in->pc, HJ_EXIT_LINEA);
        break;
    default:
        emit_exit_reason(t, in->pc, HJ_EXIT_FAULT);
        break;
    }
}

/* Decode and emit a block of at most 'max' instructions at pc into t->e.
   Returns the number of 68k instructions, sets *end, and gives in starts
   the offset from pc of each instruction. */
static unsigned translate_into(const struct M68KEngDomain *dom, struct HJTrans *t, uint32_t pc,
                               unsigned max, uint32_t *end, uint16_t *starts)
{
    struct HJInsn insns[HJ_MAX_INSNS];
    unsigned n = 0, i;
    uint32_t at = pc;

    while (n < max)
    {
        decode_backend(dom, at, &insns[n], t->library_mode);
        at = insns[n].next;
        if (is_terminator(insns[n++].kind))
            break;
    }
    flag_liveness(insns, n);
    t->start = pc;
    t->nstub = 0;
    for (i = 0; i < n; i++)
    {
        starts[i] = (uint16_t)(insns[i].pc - pc);
        emit_insn(t, &insns[i]);
    }
    if (!is_terminator(insns[n - 1].kind))
        emit_exit_to(t, insns[n - 1].next);
    emit_stubs(t);
    /* After every word decoded, the extension words of a faulting
       instruction too: a new one can make it valid. translate() keeps the
       end inside guest memory. */
    *end = insns[n - 1].next;
    return n;
}

/* ------------------------------------------------------------------
   Code memory
   ------------------------------------------------------------------ */

static void code_write_begin(struct HJBackend *b)
{
    b->stats.write_windows++;
#if HJ_RUNS
    m68kjit_plat_code_write_begin(b->code, b->code_bytes);
#endif
}

static void code_write_end(struct HJBackend *b, const uint32_t *start, size_t words)
{
    b->stats.icache_syncs++;
#if HJ_RUNS
    m68kjit_plat_code_write_end(b->code, b->code_bytes, (void *)(uintptr_t)start, words * sizeof(*start));
#else
    (void)start; (void)words;
#endif
}

#if HJ_RUNS
static void *code_alloc(size_t bytes)
{
    void *code;

#ifdef M68KJIT_TEST_SEAMS
    if (mj_test_fail_next())
        return NULL;
#endif
    code = m68kjit_plat_code_alloc(bytes);
#ifdef M68KJIT_TEST_SEAMS
    if (code)
        m68kjit_test_exec_live += (long)bytes;
#endif
    return code;
}

static void code_free(void *code, size_t bytes)
{
    if (!code)
        return;
#ifdef M68KJIT_TEST_SEAMS
    m68kjit_test_exec_live -= (long)bytes;
#endif
    m68kjit_plat_code_free(code, bytes);
}
#endif

/* Empties the code memory of the domain. */
static void reset_code(struct HJBackend *b)
{
    b->code_used = b->glue_words;
    memset(b->buckets, 0, sizeof(b->buckets));
    b->nblocks = 0;
    b->guest_used = 0;
    b->stats.resets++;
    mj_count(&b->dom->counters.code_resets, 1);
}

static void glue_regs(struct HJEmit *e, int load)
{
    int i;

    for (i = 0; i < 16; i += 2)
        put(e, a64_ldstp(0, load, 2, i, i + 1, R_STATE, 4 * i));
}

static void emit_glue(struct HJBackend *b, struct HJEmit *e)
{
    unsigned abort_at, i;
    uintptr_t common;

    /* enter(state, entry) */
    b->g_enter = e->origin + 4u * e->n;
    put(e, a64_ldstp(1, 0, 3, 29, 30, SP, -96));
    put(e, a64_addsub_imm(1, 0, 0, 29, SP, 0, 0));
    for (i = 0; i < 5; i++)
        put(e, a64_ldstp(1, 0, 2, 19 + 2 * i, 20 + 2 * i, SP, 16 + 16 * (int)i));
    put(e, a64_mov64(R_STATE, 0));
    put(e, a64_mov64(R_VAL, 1));
    put(e, a64_ldst_uimm(3, 1, R_MEM, R_STATE, ST_MEM));
    put(e, a64_ldst_uimm(2, 1, R_FLAGS, R_STATE, ST_FLAGS));
    put(e, a64_ldst_uimm(2, 1, R_LIMIT, R_STATE, ST_LIMIT));
    put(e, a64_ldst_uimm(2, 1, R_BUDGET, R_STATE, ST_BUDGET));
    glue_regs(e, 1);
    put(e, a64_br(R_VAL));

    /* exit: w16 = next PC, w17 = reason */
    b->g_exit = e->origin + 4u * e->n;
    put(e, a64_ldst_uimm(2, 0, R_ADDR, R_STATE, ST_PC));
    glue_regs(e, 0);
    put(e, a64_ldst_uimm(2, 0, R_FLAGS, R_STATE, ST_FLAGS));
    put(e, a64_mov(0, R_VAL));
    for (i = 0; i < 5; i++)
        put(e, a64_ldstp(1, 1, 2, 19 + 2 * i, 20 + 2 * i, SP, 16 + 16 * (int)i));
    put(e, a64_ldstp(1, 1, 1, 29, 30, SP, 96));
    put(e, A64_RET);

    b->g_budget = e->origin + 4u * e->n;
    emit_mov32(e, R_VAL, HJ_EXIT_BUDGET);
    put(e, a64_b(words_to(e, b->g_exit)));

    /* lookup: w16 = guest PC. The block ended at an address it does not
       continue at by itself: back to the run loop, which finds or
       translates the block there. With a slice budget of 1
       (mj_tr_cpu_create) every block ends its slice here or at its back
       edge, so the run loop sees each block and counts its instructions. */
    b->g_lookup = e->origin + 4u * e->n;
    put(e, a64_addsub_imm(0, 1, 1, R_BUDGET, R_BUDGET, 1, 0));
    put(e, a64_bcond(A64_EQ, words_to(e, b->g_budget)));
    emit_mov32(e, R_VAL, HJ_EXIT_LOOKUP);
    put(e, a64_b(words_to(e, b->g_exit)));

    /* slow access: w16 address (kept), w17 value, w25 kind, w27 guest PC */
    common = e->origin + 4u * e->n;
    put(e, a64_ldstp(1, 0, 3, 29, 30, SP, -32));
    put(e, a64_addsub_imm(1, 0, 0, 29, SP, 0, 0));
    put(e, a64_ldst_uimm(3, 0, R_ADDR, SP, 16));
    put(e, a64_ldst_uimm(3, 0, R_SRC, SP, 24));
    glue_regs(e, 0);
    put(e, a64_mov64(0, R_STATE));
    put(e, a64_mov(1, R_ADDR));
    put(e, a64_mov(2, R_VAL));
    put(e, a64_mov(3, R_SCR));
    put(e, a64_mov(4, R_F2));
    put(e, a64_ldst_uimm(3, 1, R_ADDR, R_STATE, ST_SLOW));
    put(e, a64_blr(R_ADDR));
    put(e, a64_mov(R_VAL, 0));
    glue_regs(e, 1);
    put(e, a64_ldst_uimm(2, 1, R_ADDR, R_STATE, ST_ABORT));
    abort_at = e->n;
    put(e, a64_cbz(0, 1, R_ADDR, 0));
    put(e, a64_ldst_uimm(3, 1, R_ADDR, SP, 16));
    put(e, a64_ldst_uimm(3, 1, R_SRC, SP, 24));
    put(e, a64_ldstp(1, 1, 1, 29, 30, SP, 32));
    put(e, A64_RET);
    patch(e, abort_at, e->n);
    put(e, a64_ldstp(1, 1, 1, 29, 30, SP, 32));
    put(e, a64_mov(R_ADDR, R_F2));   /* the PC of the access the host stopped */
    emit_mov32(e, R_VAL, HJ_EXIT_ABORT);
    put(e, a64_b(words_to(e, b->g_exit)));
    for (i = 0; i < SLOW_KINDS; i++)
    {
        b->g_slow[i] = e->origin + 4u * e->n;
        emit_mov32(e, R_SCR, i);
        put(e, a64_b(words_to(e, common)));
    }
}

#if HJ_RUNS
/* The glue for code memory at origin, into the scratch buffer. FALSE when
   it does not fit there; its size does not depend on origin. */
static int glue_emit(struct HJBackend *b, struct HJEmit *e, const uint32_t *origin)
{
    memset(e, 0, sizeof(*e));
    e->buf = b->scratch;
    e->cap = HJ_SCRATCH_WORDS;
    e->origin = (uintptr_t)origin;
    emit_glue(b, e);
    return !e->bad;
}

/* The glue that glue_emit() left in the scratch buffer goes to the start
   of the code memory, which holds nothing else after this. */
static void glue_publish(struct HJBackend *b, const struct HJEmit *e)
{
    uintptr_t entry = b->g_enter;

    b->code_words = b->code_bytes / 4;
    code_write_begin(b);
    memcpy(b->code, b->scratch, 4u * e->n);
    code_write_end(b, b->code, e->n);
    b->glue_words = b->code_used = e->n;
    memcpy(&b->enter, &entry, sizeof(entry));
}

/* The blocks of the table that a lookup still finds. A block whose guest
   bytes changed leaves its chain when it is looked up again (get_block()),
   and its place in the table stays used until the next reset. */
static unsigned linked_blocks(const struct HJBackend *b)
{
    const struct HJBlock *blk;
    unsigned i, n = 0;

    for (i = 0; i < HJ_BUCKETS; i++)
        for (blk = b->buckets[i]; blk; blk = blk->next)
            n++;
    return n;
}

/* The next size of the code memory, the block table and the guest-byte
   store, when there is one, at least half of the blocks are still in use,
   and there is memory for all three; otherwise they stay as they are. A
   program that changes a little code again and again fills them with
   blocks nothing finds any more: emptying them is enough for it. The
   caller empties them next (reset_code()), so nothing they held is
   carried over, and no translated code is running (translate()). */
static void grow_code(struct HJBackend *b)
{
    size_t code_bytes = b->code_bytes * 2, guest_cap = b->guest_cap * 2;
    unsigned max_blocks = b->max_blocks * 2;
    uint32_t *code;
    struct HJBlock *blocks;
    UBYTE *guest_bytes;
    struct HJEmit e;

    if (b->code_bytes >= ((size_t)HJ_CODE_BYTES_START << HJ_GROW_STEPS) || linked_blocks(b) * 2 < b->nblocks)
        return;
    code = code_alloc(code_bytes);
    blocks = mj_alloc(max_blocks * sizeof(*blocks));
    guest_bytes = mj_alloc(guest_cap);
    if (!code || !blocks || !guest_bytes)
    {
        code_free(code, code_bytes);
        mj_free(blocks, max_blocks * sizeof(*blocks));
        mj_free(guest_bytes, guest_cap);
        return;
    }
    code_free(b->code, b->code_bytes);
    mj_free(b->blocks, b->max_blocks * sizeof(*b->blocks));
    mj_free(b->guest_bytes, b->guest_cap);
    b->code = code;
    b->code_bytes = code_bytes;
    b->blocks = blocks;
    b->max_blocks = max_blocks;
    b->guest_bytes = guest_bytes;
    b->guest_cap = guest_cap;
    /* the glue has the size it had in mj_tr_domain_create(), where it fit */
    glue_emit(b, &e, b->code);
    glue_publish(b, &e);
}
#endif

/* ------------------------------------------------------------------
   The C side: slow accesses, blocks
   ------------------------------------------------------------------ */

/* An access is direct when it lies inside guest memory and touches no
   range that sends this kind of access to the host (contract, section 4). */
static int mj_direct(const struct M68KEngDomain *dom, uint32_t addr, uint32_t width, ULONG flag)
{
    ULONG i;

    if ((uint64_t)addr + width > dom->size)
        return 0;
    for (i = 0; i < dom->num_ranges; i++)
        if ((dom->ranges[i].flags & flag) &&
            addr < dom->ranges[i].hi && dom->ranges[i].lo < (uint64_t)addr + width)
            return 0;
    return 1;
}

/* A byte or word read from guest memory or from the host. A host answer
   of 1 stops the run at once: no other host access is made in it. */
static uint32_t mj_read(struct HJCpu *c, uint32_t addr, uint32_t width, uint32_t pc)
{
    const struct M68KEngDomain *dom = c->owner->domain;
    ULONG value = 0;

    if (c->st->abort)
        return 0;
    if (mj_direct(dom, addr, width, M68KENG_RANGE_READ_HOST))
        return width == 1 ? dom->base[addr] : (uint32_t)(dom->base[addr] << 8 | dom->base[addr + 1]);
    if (dom->host->read(dom->host_domain, c->owner->host_cpu, addr, width, pc, &value))
    {
        c->st->abort = 1;
        return 0;
    }
    return width == 1 ? (value & 0xFFu) : (value & 0xFFFFu);
}

static void mj_write(struct HJCpu *c, uint32_t addr, uint32_t width, uint32_t value, uint32_t pc)
{
    const struct M68KEngDomain *dom = c->owner->domain;

    if (c->st->abort)
        return;
    if (mj_direct(dom, addr, width, M68KENG_RANGE_WRITE_HOST))
    {
        if (width == 1)
            dom->base[addr] = (UBYTE)value;
        else
        {
            dom->base[addr] = (UBYTE)(value >> 8);
            dom->base[addr + 1] = (UBYTE)value;
        }
        return;
    }
    if (dom->host->write(dom->host_domain, c->owner->host_cpu, addr, width, value, pc))
        c->st->abort = 1;
}

/* Called from translated code. A longword is two word accesses, the word
   at addr first, each routed by itself. */
static uint32_t slow_access(struct HJState *st, uint32_t addr, uint32_t value, uint32_t kind, uint32_t pc)
{
    struct HJCpu *c = st->backend;
    uint32_t hi;

    if (kind == SLOW_P16 || kind == SLOW_P32)
    {
        /* a divisor probe: no access and no host call */
        const struct M68KEngDomain *dom = c->owner->domain;

        c->declined = !mj_direct(dom, addr, 2, M68KENG_RANGE_READ_HOST) ||
                      (kind == SLOW_P32 && !mj_direct(dom, addr + 2, 2, M68KENG_RANGE_READ_HOST));
        return (uint32_t)c->declined;
    }
    c->b->stats.slow_accesses++;
    switch (kind)
    {
    case SLOW_R8: return mj_read(c, addr, 1, pc);
    case SLOW_R16: return mj_read(c, addr, 2, pc);
    case SLOW_R32:
        hi = mj_read(c, addr, 2, pc);
        return hi << 16 | mj_read(c, addr + 2, 2, pc);
    case SLOW_W8: mj_write(c, addr, 1, value & 0xFFu, pc); return 0;
    case SLOW_W16: mj_write(c, addr, 2, value & 0xFFFFu, pc); return 0;
    default:
        mj_write(c, addr, 2, value >> 16, pc);
        mj_write(c, addr + 2, 2, value & 0xFFFFu, pc);
        return 0;
    }
}

static unsigned bucket_of(uint32_t pc) { return (pc >> 1) & (HJ_BUCKETS - 1); }

/* Called only from mj_tr_run(), between blocks: no translated code is
   running, so the code memory can be emptied here. Not reentrant. */
static struct HJBlock *translate(struct HJBackend *b, uint32_t pc)
{
    struct HJTrans *t = b->trans;
    unsigned max = HJ_MAX_INSNS, n = 0, attempt;
    uint32_t end = pc;
    uint16_t starts[HJ_MAX_INSNS];
    struct HJBlock *blk;

    for (attempt = 0; attempt < 2; attempt++)
    {
        if (b->code_words - b->code_used < HJ_SCRATCH_WORDS || b->nblocks >= b->max_blocks ||
            b->guest_cap - b->guest_used < 4u * HJ_MAX_INSNS * 10u)
        {
#if HJ_RUNS
            grow_code(b);
#endif
            reset_code(b);
        }
        for (;;)
        {
            memset(&t->e, 0, sizeof(t->e));
            t->e.buf = b->scratch;
            t->e.cap = HJ_SCRATCH_WORDS;
            t->e.origin = (uintptr_t)(b->code + b->code_used);
            t->g_exit = b->g_exit; t->g_lookup = b->g_lookup; t->g_budget = b->g_budget;
            t->library_mode = 1;
            memcpy(t->g_slow, b->g_slow, sizeof(t->g_slow));
            n = translate_into(b->dom, t, pc, max, &end, starts);
            if (!t->e.bad || max == 1)
                break;
            max = n > 1 ? n / 2 : 1;
        }
        if (!t->e.bad)
            break;
    }
    if (end > b->dom->size)
        end = b->dom->size;
    if (t->e.bad || end <= pc)
        return NULL;

    code_write_begin(b);
    memcpy(b->code + b->code_used, b->scratch, 4u * t->e.n);
    code_write_end(b, b->code + b->code_used, t->e.n);

    blk = &b->blocks[b->nblocks++];
    blk->pc = pc;
    blk->end = end;
    blk->epoch = b->epoch;
    blk->words = t->e.n;
    blk->code = b->code + b->code_used;
    b->code_used += t->e.n;
    memcpy(b->guest_bytes + b->guest_used, b->dom->base + pc, end - pc);
    blk->guest = b->guest_bytes + b->guest_used;
    b->guest_used += (end - pc + 3u) & ~3u;
    /* the instruction offsets beside the guest bytes; the room check above
       leaves space for both */
    memcpy(b->guest_bytes + b->guest_used, starts, n * sizeof(starts[0]));
    blk->starts = (const uint16_t *)(const void *)(b->guest_bytes + b->guest_used);
    blk->ninsns = (uint16_t)n;
    b->guest_used += (n * sizeof(starts[0]) + 3u) & ~3u;
    blk->next = b->buckets[bucket_of(pc)];
    b->buckets[bucket_of(pc)] = blk;

    b->stats.blocks++;
    b->stats.guest_insns += n;
    b->stats.host_words += t->e.n;
    mj_count(&b->dom->counters.blocks, 1);
    mj_count(&b->dom->counters.insns, n);
    return blk;
}

/* A block for pc that matches the guest code, translated if needed. A
   block from before a flush is kept when its guest bytes are unchanged. */
static struct HJBlock *get_block(struct HJCpu *c, uint32_t pc)
{
    struct HJBackend *b = c->b;
    struct HJBlock **link = &b->buckets[bucket_of(pc)], *blk;

    for (blk = *link; blk; link = &blk->next, blk = blk->next)
        if (blk->pc == pc)
            break;
    if (blk && blk->epoch != b->epoch)
    {
        if (memcmp(b->dom->base + blk->pc, blk->guest, blk->end - blk->pc) == 0)
        {
            blk->epoch = b->epoch;
            b->stats.revalidated++;
        }
        else
        {
            *link = blk->next;
            blk = NULL;
        }
    }
    if (!blk)
        blk = translate(b, pc);
    return blk;
}

/* ------------------------------------------------------------------
   Public functions
   ------------------------------------------------------------------ */

#ifdef M68KJIT_TEST_SEAMS
/* For the host checks. Words that clang assembled from the same text
   (Apple clang 21, -arch arm64): every encoder and alias above, with
   negative offsets, shifted operands, bitmask immediates and branches both
   ways. */
static int HJ_EncoderSelfTest(char *message, unsigned long size)
{
    const struct { uint32_t want, got; const char *text; } enc[] =
    {
        { 0x113ffc41, a64_addsub_imm(0, 0, 0, 1, 2, 4095, 0), "add w1, w2, #4095" },
        { 0x51000483, a64_addsub_imm(0, 1, 0, 3, 4, 1, 0), "sub w3, w4, #1" },
        { 0x31001cc5, a64_addsub_imm(0, 0, 1, 5, 6, 7, 0), "adds w5, w6, #7" },
        { 0x710006d6, a64_addsub_imm(0, 1, 1, 22, 22, 1, 0), "subs w22, w22, #1" },
        { 0x910003fd, a64_addsub_imm(1, 0, 0, 29, SP, 0, 0), "add x29, sp, #0" },
        { 0x5177fe19, a64_addsub_imm(0, 1, 0, 25, 16, 0xDFF, 1), "sub w25, w16, #0xdff, lsl #12" },
        { 0x5177fd11, a64_addsub_imm(0, 1, 0, 17, 8, 0xDFF, 1), "sub w17, w8, #0xdff, lsl #12" },
        { 0x11001108, a64_addsub_imm(0, 0, 0, 8, 8, 4, 0), "add w8, w8, #4" },
        { 0x11000a31, a64_addsub_imm(0, 0, 0, 17, 17, 2, 0), "add w17, w17, #2" },
        { 0x7120063f, a64_addsub_imm(0, 1, 1, ZR, 17, 0x801, 0), "cmp w17, #0x801" },
        { 0x0b030041, a64_addsub_reg(0, 0, 0, 1, 2, 3, 0, 0), "add w1, w2, w3" },
        { 0x4b0600a4, a64_addsub_reg(0, 1, 0, 4, 5, 6, 0, 0), "sub w4, w5, w6" },
        { 0x2b116339, a64_addsub_reg(0, 0, 1, 25, 25, 17, 0, 24), "adds w25, w25, w17, lsl #24" },
        { 0x6b03433f, a64_addsub_reg(0, 1, 1, ZR, 25, 3, 0, 16), "subs wzr, w25, w3, lsl #16" },
        { 0x6b18011f, a64_cmp(8, 24), "cmp w8, w24" },
        { 0xeb10033f, a64_addsub_reg(1, 1, 1, ZR, 25, 16, 0, 0), "cmp x25, x16" },
        { 0x8b191231, a64_addsub_reg(1, 0, 0, 17, 17, 25, 0, 4), "add x17, x17, x25, lsl #4" },
        { 0x0b190910, a64_addsub_reg(0, 0, 0, 16, 8, 25, 0, 2), "add w16, w8, w25, lsl #2" },
        { 0x4b0343f9, a64_addsub_reg(0, 1, 0, 25, ZR, 3, 0, 16), "neg w25, w3, lsl #16" },
        { 0x6b1103fc, a64_addsub_reg(0, 1, 1, 28, ZR, 17, 0, 0), "negs w28, w17" },
        { 0x0a030041, a64_logic_reg(0, 0, 0, 1, 2, 3, 0, 0), "and w1, w2, w3" },
        { 0x2a0203e1, a64_mov(1, 2), "mov w1, w2" },
        { 0x2a1f03f1, a64_mov(17, ZR), "mov w17, wzr" },
        { 0x4a550eba, a64_logic_reg(0, 2, 0, 26, 21, 21, 1, 3), "eor w26, w21, w21, lsr #3" },
        { 0x2a550b5a, a64_logic_reg(0, 1, 0, 26, 26, 21, 1, 2), "orr w26, w26, w21, lsr #2" },
        { 0x6a03007f, a64_tst(3, 3), "tst w3, w3" },
        { 0x6a0600a4, a64_logic_reg(0, 3, 0, 4, 5, 6, 0, 0), "ands w4, w5, w6" },
        { 0x2a3103f9, a64_logic_reg(0, 1, 1, 25, ZR, 17, 0, 0), "mvn w25, w17" },
        { 0xaa0003f3, a64_mov64(19, 0), "mov x19, x0" },
        { 0xaa0103f1, a64_mov64(17, 1), "mov x17, x1" },
        { 0x2a1b075a, a64_logic_reg(0, 1, 0, 26, 26, 27, 0, 1), "orr w26, w26, w27, lsl #1" },
        { 0x2a1b235c, a64_logic_reg(0, 1, 0, 28, 26, 27, 0, 8), "orr w28, w26, w27, lsl #8" },
        { 0x2a1b0355, a64_logic_reg(0, 1, 0, 21, 26, 27, 0, 0), "orr w21, w26, w27" },
        { 0x121c02b5, a64_logic_const(0, 21, 21, 0x10), "and w21, w21, #0x10" },
        { 0x321d02b5, a64_logic_const(1, 21, 21, 0x8), "orr w21, w21, #0x8" },
        { 0x321e02b5, a64_logic_const(1, 21, 21, 0x4), "orr w21, w21, #0x4" },
        { 0x521f035a, a64_logic_const(2, 26, 26, 0x2), "eor w26, w26, #0x2" },
        { 0x721f06bf, a64_logic_const(3, ZR, 21, 0x6), "tst w21, #0x6" },
        { 0x72003c7f, a64_logic_const(3, ZR, 3, 0xFFFF), "tst w3, #0xffff" },
        { 0x12103c63, a64_logic_const(0, 3, 3, 0xFFFF0000u), "and w3, w3, #0xffff0000" },
        { 0x12185c63, a64_logic_const(0, 3, 3, 0xFFFFFF00u), "and w3, w3, #0xffffff00" },
        { 0x121e0a3a, a64_logic_const(0, 26, 17, 0x1C), "and w26, w17, #0x1c" },
        { 0x121c7b5a, a64_logic_const(0, 26, 26, ~0x08u), "and w26, w26, #0xfffffff7" },
        { 0x32089fe1, a64_logic_const(1, 1, ZR, 0xFF00FF00u), "orr w1, wzr, #0xff00ff00" },
        { 0x3200f3e1, a64_logic_const(1, 1, ZR, 0x55555555u), "orr w1, wzr, #0x55555555" },
        { 0x12010442, a64_logic_const(0, 2, 2, 0x80000001u), "and w2, w2, #0x80000001" },
        { 0x52824681, a64_movw(0, 2, 1, 0x1234, 0), "movz w1, #0x1234" },
        { 0x52a24681, a64_movw(0, 2, 1, 0x1234, 1), "movz w1, #0x1234, lsl #16" },
        { 0x12824681, a64_movw(0, 0, 1, 0x1234, 0), "movn w1, #0x1234" },
        { 0x12aacf02, a64_movw(0, 0, 2, 0x5678, 1), "movn w2, #0x5678, lsl #16" },
        { 0x72b579a3, a64_movw(0, 3, 3, 0xABCD, 1), "movk w3, #0xabcd, lsl #16" },
        { 0x529ffff6, a64_movw(0, 2, 22, 0xFFFF, 0), "movz w22, #0xffff" },
        { 0x53081c41, a64_lsl(1, 2, 24), "lsl w1, w2, #24" },
        { 0x531f7800, a64_lsl(0, 0, 1), "lsl w0, w0, #1" },
        { 0x53103ffa, a64_lsl(26, ZR, 16), "lsl w26, wzr, #16" },
        { 0x531c7f5a, a64_lsr(26, 26, 28), "lsr w26, w26, #28" },
        { 0x53087c00, a64_lsr(0, 0, 8), "lsr w0, w0, #8" },
        { 0x53017e31, a64_lsr(17, 17, 1), "lsr w17, w17, #1" },
        { 0x13037c41, a64_asr(1, 2, 3), "asr w1, w2, #3" },
        { 0x531f7c7b, a64_ubfx(27, 3, 31, 1), "ubfx w27, w3, #31, #1" },
        { 0x5300007b, a64_ubfx(27, 3, 0, 1), "ubfx w27, w3, #0, #1" },
        { 0x53083e3a, a64_ubfx(26, 17, 8, 8), "ubfx w26, w17, #8, #8" },
        { 0xd3413219, a64_bitfield(1, 2, 25, 16, 1, 12), "ubfx x25, x16, #1, #12" },
        { 0x33000f55, a64_bfi(21, 26, 0, 4), "bfi w21, w26, #0, #4" },
        { 0x331c0375, a64_bfi(21, 27, 4, 1), "bfi w21, w27, #4, #1" },
        { 0x331f037a, a64_bfi(26, 27, 1, 1), "bfi w26, w27, #1, #1" },
        { 0x33003f23, a64_bfi(3, 25, 0, 16), "bfi w3, w25, #0, #16" },
        { 0x33001f23, a64_bfi(3, 25, 0, 8), "bfi w3, w25, #0, #8" },
        { 0x13001c79, a64_sxtb(25, 3), "sxtb w25, w3" },
        { 0x13003ee8, a64_sxth(8, 23), "sxth w8, w23" },
        { 0x53001c7c, a64_uxtb(28, 3), "uxtb w28, w3" },
        { 0x53003e3c, a64_uxth(28, 17), "uxth w28, w17" },
        { 0x93407c1b, a64_bitfield(1, 0, 27, 0, 0, 31), "sxtw x27, w0" },
        { 0xd360ff3b, a64_bitfield(1, 2, 27, 25, 32, 63), "lsr x27, x25, #32" },
        { 0xd378df9c, a64_bitfield(1, 2, 28, 28, 56, 55), "lsl x28, x28, #8" },
        { 0xd348ff39, a64_bitfield(1, 2, 25, 25, 8, 63), "lsr x25, x25, #8" },
        { 0xb3607c1c, a64_bitfield(1, 1, 28, 0, 32, 31), "bfi x28, x0, #32, #32" },
        { 0x0b970e10, a64_addsub_reg(0, 0, 0, 16, 16, 23, 2, 3), "add w16, w16, w23, asr #3" },
        { 0xea19033f, a64_logic_reg(1, 3, 0, ZR, 25, 25, 0, 0), "tst x25, x25" },
        { 0x0a550b7b, a64_logic_reg(0, 0, 0, 27, 27, 21, 1, 2), "and w27, w27, w21, lsr #2" },
        { 0x0a3c0000, a64_logic_reg(0, 0, 1, 0, 0, 28, 0, 0), "bic w0, w0, w28" },
        { 0x13834063, a64_ror(3, 3, 16), "ror w3, w3, #16" },
        { 0x13837c63, a64_ror(3, 3, 31), "ror w3, w3, #31" },
        { 0x5ac00a31, a64_rev32(17, 17), "rev w17, w17" },
        { 0x5ac00639, a64_rev16(25, 17), "rev16 w25, w17" },
        { 0x5ac01339, a64_clz(25, 25), "clz w25, w25" },
        { 0x1a9a3339, a64_csel(0, 25, 25, 26, A64_LO), "csel w25, w25, w26, lo" },
        { 0x1a9f37fb, a64_cset(27, A64_HS), "cset w27, hs" },
        { 0x1a9f27fb, a64_cset(27, A64_LO), "cset w27, lo" },
        { 0x1a9f17fa, a64_cset(26, A64_EQ), "cset w26, eq" },
        { 0x1a9f07fb, a64_cset(27, A64_NE), "cset w27, ne" },
        { 0x3a020039, a64_adc_sbc(0, 0, 1, 25, 1, 2), "adcs w25, w1, w2" },
        { 0x7a020039, a64_adc_sbc(0, 1, 1, 25, 1, 2), "sbcs w25, w1, w2" },
        { 0x1ada239c, a64_lslv(28, 28, 26), "lsl w28, w28, w26" },
        { 0x1adb2799, a64_shiftv(0, 1, 25, 28, 27), "lsr w25, w28, w27" },
        { 0x1adb2b80, a64_shiftv(0, 2, 0, 28, 27), "asr w0, w28, w27" },
        { 0x1ad92c1c, a64_shiftv(0, 3, 28, 0, 25), "ror w28, w0, w25" },
        { 0x9adb2339, a64_shiftv(1, 0, 25, 25, 27), "lsl x25, x25, x27" },
        { 0x9ad72739, a64_shiftv(1, 1, 25, 25, 23), "lsr x25, x25, x23" },
        { 0x1ad10819, a64_div(0, 0, 25, 0, 17), "udiv w25, w0, w17" },
        { 0x1ad10c19, a64_div(0, 1, 25, 0, 17), "sdiv w25, w0, w17" },
        { 0x9ad70b99, a64_div(1, 0, 25, 28, 23), "udiv x25, x28, x23" },
        { 0x9ad70f99, a64_div(1, 1, 25, 28, 23), "sdiv x25, x28, x23" },
        { 0x1b1a7f20, a64_mul(0, 25, 26), "mul w0, w25, w26" },
        { 0x1b11833a, a64_msub(0, 26, 25, 17, 0), "msub w26, w25, w17, w0" },
        { 0x9b17f33a, a64_msub(1, 26, 25, 23, 28), "msub x26, x25, x23, x28" },
        { 0x9b207e39, a64_smull(25, 17, 0), "smull x25, w17, w0" },
        { 0x9ba07e39, a64_umull(25, 17, 0), "umull x25, w17, w0" },
        { 0xd53b421a, a64_mrs_nzcv(26), "mrs x26, nzcv" },
        { 0xb8684a91, a64_ldst_reg(2, 1, 17, 20, 8), "ldr w17, [x20, w8, uxtw]" },
        { 0x78704a91, a64_ldst_reg(1, 1, 17, 20, 16), "ldrh w17, [x20, w16, uxtw]" },
        { 0x387c4a91, a64_ldst_reg(0, 1, 17, 20, 28), "ldrb w17, [x20, w28, uxtw]" },
        { 0xb82f4a99, a64_ldst_reg(2, 0, 25, 20, 15), "str w25, [x20, w15, uxtw]" },
        { 0x78304a99, a64_ldst_reg(1, 0, 25, 20, 16), "strh w25, [x20, w16, uxtw]" },
        { 0x38284a91, a64_ldst_reg(0, 0, 17, 20, 8), "strb w17, [x20, w8, uxtw]" },
        { 0xf9402e74, a64_ldst_uimm(3, 1, 20, 19, 88), "ldr x20, [x19, #88]" },
        { 0xb9404675, a64_ldst_uimm(2, 1, 21, 19, 68), "ldr w21, [x19, #68]" },
        { 0xb9004270, a64_ldst_uimm(2, 0, 16, 19, 64), "str w16, [x19, #64]" },
        { 0xf9000bf0, a64_ldst_uimm(3, 0, 16, SP, 16), "str x16, [sp, #16]" },
        { 0xf9403270, a64_ldst_uimm(3, 1, 16, 19, 96), "ldr x16, [x19, #96]" },
        { 0xb9404a7b, a64_ldst_uimm(2, 1, 27, 19, 72), "ldr w27, [x19, #72]" },
        { 0xb9004a7a, a64_ldst_uimm(2, 0, 26, 19, 72), "str w26, [x19, #72]" },
        { 0xa9ba7bfd, a64_ldstp(1, 0, 3, 29, 30, SP, -96), "stp x29, x30, [sp, #-96]!" },
        { 0xa8c67bfd, a64_ldstp(1, 1, 1, 29, 30, SP, 96), "ldp x29, x30, [sp], #96" },
        { 0xa90153f3, a64_ldstp(1, 0, 2, 19, 20, SP, 16), "stp x19, x20, [sp, #16]" },
        { 0xa94573fb, a64_ldstp(1, 1, 2, 27, 28, SP, 80), "ldp x27, x28, [sp, #80]" },
        { 0x29000660, a64_ldstp(0, 0, 2, 0, 1, 19, 0), "stp w0, w1, [x19]" },
        { 0x29473e6e, a64_ldstp(0, 1, 2, 14, 15, 19, 56), "ldp w14, w15, [x19, #56]" },
        { 0xa9404639, a64_ldstp(1, 1, 2, 25, 17, 17, 0), "ldp x25, x17, [x17]" },
        { 0xa9be7bfd, a64_ldstp(1, 0, 3, 29, 30, SP, -32), "stp x29, x30, [sp, #-32]!" },
        { 0xa8c27bfd, a64_ldstp(1, 1, 1, 29, 30, SP, 32), "ldp x29, x30, [sp], #32" },
        { 0x14000002, a64_b(2), "b .+8" },
        { 0x17ffffff, a64_b(-1), "b .-4" },
        { 0x94000064, a64_bl(100), "bl .+400" },
        { 0x54ffffa1, a64_bcond(A64_NE, -3), "b.ne .-12" },
        { 0x54000080, a64_bcond(A64_EQ, 4), "b.eq .+16" },
        { 0x54000208, a64_bcond(A64_HI, 16), "b.hi .+64" },
        { 0x54000063, a64_bcond(A64_LO, 3), "b.lo .+12" },
        { 0x34000111, a64_cbz(0, 0, 17, 8), "cbz w17, .+32" },
        { 0x35000070, a64_cbz(0, 1, 16, 3), "cbnz w16, .+12" },
        { 0x360800b5, a64_tbz(0, 21, 1, 5), "tbz w21, #1, .+20" },
        { 0x3707ffda, a64_tbz(1, 26, 0, -2), "tbnz w26, #0, .-8" },
        { 0x37180035, a64_tbz(1, 21, 3, 1), "tbnz w21, #3, .+4" },
        { 0xd61f0220, a64_br(17), "br x17" },
        { 0xd63f0200, a64_blr(16), "blr x16" },
        { 0xd65f03c0, A64_RET, "ret" },
    };
    unsigned i, bad = 0;

    for (i = 0; i < sizeof(enc) / sizeof(enc[0]); i++)
        if (enc[i].want != enc[i].got && bad++ == 0 && message)
            snprintf(message, size, "%s: %08lx, want %08lx", enc[i].text,
                     (unsigned long)enc[i].got, (unsigned long)enc[i].want);
    if (!bad && message)
        snprintf(message, size, "%u encodings", i);
    return (int)bad;
}

static const char *HJ_ExecMemoryKind(void)
{
#if HJ_RUNS
    return m68kjit_plat_code_method();
#else
    return "none";
#endif
}
#endif

static int HJ_Available(void)
{
    return HJ_RUNS;
}

/* ------------------------------------------------------------------
   The engine side (m68kjit_intern.h), called by m68kjit_engine.c
   ------------------------------------------------------------------ */

int mj_tr_runs(void)
{
    return HJ_Available();
}

#ifdef M68KJIT_TEST_SEAMS
const char *mj_tr_exec_memory_kind(void)
{
    return HJ_ExecMemoryKind();
}

int mj_tr_encoder_self_test(char *message, unsigned long size)
{
    return HJ_EncoderSelfTest(message, size);
}
#endif

#if HJ_RUNS
static void hj_free_code(struct HJBackend *b)
{
    code_free(b->code, b->code_bytes);
    mj_free(b->blocks, b->max_blocks * sizeof(*b->blocks));
    mj_free(b->guest_bytes, b->guest_cap);
    mj_free(b->scratch, 4u * HJ_SCRATCH_WORDS);
    mj_free(b->trans, sizeof(*b->trans));
    mj_free(b, sizeof(*b));
}
#endif

/* The code memory, block table, guest-byte store and the glue of a domain,
   shared by its CPUs. */
LONG mj_tr_domain_create(struct M68KEngDomain *dom)
{
#if HJ_RUNS
    struct HJBackend *b;
    struct HJEmit e;

    b = mj_alloc(sizeof(*b));
    if (!b)
        return M68KENG_ERR_NOMEM;
    b->dom = dom;
    b->code_bytes = HJ_CODE_BYTES_START;
    b->max_blocks = HJ_MAX_BLOCKS_START;
    b->guest_cap = HJ_GUEST_BYTES_START;
    b->code = code_alloc(b->code_bytes);
    b->blocks = mj_alloc(b->max_blocks * sizeof(*b->blocks));
    b->guest_bytes = mj_alloc(b->guest_cap);
    b->scratch = mj_alloc(4u * HJ_SCRATCH_WORDS);
    b->trans = mj_alloc(sizeof(*b->trans));
    if (!b->code || !b->blocks || !b->guest_bytes || !b->scratch || !b->trans)
    {
        hj_free_code(b);
        return M68KENG_ERR_NOMEM;
    }
    if (!glue_emit(b, &e, b->code))
    {
        /* not reachable with these sizes: the glue fits the scratch buffer */
        hj_free_code(b);
        return M68KENG_ERR_NOMEM;
    }
    glue_publish(b, &e);
    b->stats.write_windows = b->stats.icache_syncs = 0;
    dom->code = b;
    return M68KENG_OK;
#else
    (void)dom;
    (void)slow_access;
    (void)emit_glue;
    return M68KENG_ERR_NOHOST;
#endif
}

void mj_tr_domain_destroy(struct M68KEngDomain *dom)
{
#if HJ_RUNS
    if (dom->code)
        hj_free_code(dom->code);
#endif
    dom->code = NULL;
}

ULONG mj_tr_code_bytes(const struct M68KEngDomain *dom)
{
    return dom->code ? (ULONG)dom->code->code_bytes : 0;
}

/* Registers as cpu_create leaves them: 0, PC 0, SR 0x2700. */
LONG mj_tr_cpu_create(struct M68KEngCPU *owner)
{
    struct M68KEngDomain *dom = owner->domain;
    struct HJCpu *c = mj_alloc(sizeof(*c));
    struct HJState *st;

    if (!c)
        return M68KENG_ERR_NOMEM;
    c->st_bytes = sizeof(struct HJState) + dom->npages;
    c->st = mj_alloc(c->st_bytes);
    if (!c->st)
    {
        mj_free(c, sizeof(*c));
        return M68KENG_ERR_NOMEM;
    }
    st = c->st;
    c->owner = owner;
    c->b = dom->code;
    st->mem = dom->base;
    st->limit = dom->size - 4;
    st->slow = (void *)(uintptr_t)slow_access;
    st->backend = c;
    st->slice_budget = 1;      /* one block per entry: the run loop counts */
    st->sr_high = (M68KENG_SR_AT_CREATE >> 8) & 0x27u;
    memcpy(st->pages, dom->pages, dom->npages);
    c->pages_generation = dom->pages_generation;
    owner->tr = c;
    return M68KENG_OK;
}

void mj_tr_cpu_destroy(struct M68KEngCPU *owner)
{
    struct HJCpu *c = owner->tr;

    if (!c)
        return;
    mj_free(c->st, c->st_bytes);
    mj_free(c, sizeof(*c));
    owner->tr = NULL;
}

ULONG mj_tr_get_reg(const struct M68KEngCPU *owner, ULONG reg)
{
    const struct HJState *st = owner->tr->st;

    if (reg < 8)
        return st->d[reg];
    if (reg < 16)
        return st->a[reg - 8];
    if (reg == M68KENG_REG_PC)
        return st->pc;
    return (ULONG)((st->sr_high << 8 | swap_cv(st->flags)) & M68KENG_SR_DEFINED);
}

/* Version 1.0 has one mode: S set, T and M clear. */
LONG mj_tr_set_reg(struct M68KEngCPU *owner, ULONG reg, ULONG value)
{
    struct HJState *st = owner->tr->st;

    if (reg < 8)
        st->d[reg] = value;
    else if (reg < 16)
        st->a[reg - 8] = value;
    else if (reg == M68KENG_REG_PC)
        st->pc = value;
    else
    {
        if ((value & M68KENG_SR_MODEL_MASK) != M68KENG_SR_MODEL_VALUE)
            return M68KENG_ERR_UNSUPPORTED;
        st->sr_high = (value >> 8) & 0x27u;
        st->flags = swap_cv(value & 0x1Fu);
    }
    return M68KENG_OK;
}

/* A new epoch makes every block compare its guest bytes before it runs
   again. */
void mj_tr_invalidate(struct M68KEngDomain *dom)
{
    struct HJBackend *b = dom->code;

    if (++b->epoch == 0)
        reset_code(b);
    b->stats.flushes++;
}

/* A new map generation. Every block was decoded under the old fetch rules:
   an extension word or a later opcode of it may now lie in a READ_HOST
   range, or a fetch that failed may now succeed. The code memory of the
   domain is emptied, with no revalidation by guest bytes, so every CPU of
   the domain translates again under the new map. Counted as a code reset. */
void mj_tr_map_changed(struct M68KEngDomain *dom)
{
    reset_code(dom->code);
}

/* Instructions of the block that start before pc. */
static ULONG count_before(const struct HJBlock *blk, uint32_t pc)
{
    ULONG n = 0;

    while (n < (ULONG)blk->ninsns && blk->pc + blk->starts[n] < pc)
        n++;
    return n;
}

static void mj_exit(struct M68KEngExit *x, ULONG reason, uint32_t pc, ULONG retired)
{
    x->reason = reason;
    x->pc = pc;
    x->retired = retired;
}

static void mj_text(struct M68KEngExit *x, const char *s)
{
    size_t i;

    for (i = 0; s[i] && i + 1 < M68KENG_TEXT_SIZE; i++)
        x->text[i] = s[i];
    x->text[i] = 0;
}

static ULONG mj_opcode(const struct M68KEngDomain *dom, uint32_t pc)
{
    return mj_fetchable(dom, pc) ? (ULONG)(dom->base[pc] << 8 | dom->base[pc + 1]) : 0;
}

/* The F-line encodings of 68040 integer-unit instructions: CINV, CPUSH,
   MOVE16. They are no Line-F exception on a 68040; this engine does not
   execute them. */
static int mj_040_integer_fline(uint16_t op)
{
    return (op & 0xFF00u) == 0xF400u || (op & 0xFFE0u) == 0xF600u || (op & 0xFFF8u) == 0xF620u;
}

/* A fault exit: the instruction at pc was not executed. An exception is
   reported only where it is certain; anything else is UNSUPPORTED, which
   the contract always allows. */
static void mj_classify(const struct M68KEngDomain *dom, struct M68KEngExit *x, uint32_t pc, ULONG retired)
{
    struct HJInsn in;
    uint16_t op = (uint16_t)mj_opcode(dom, pc);

    decode_backend(dom, pc, &in, 1);
    if (in.kind == K_FAULT && in.why && strcmp(in.why, "fetch outside guest memory") == 0)
    {
        mj_exit(x, M68KENG_STOP_PC_OUTSIDE, pc, retired);
        x->address = in.bad_at;
        return;
    }
    mj_exit(x, M68KENG_STOP_EXCEPTION, pc, retired);
    x->opcode = op;
    if (in.kind == K_DIVW || in.kind == K_DIVL)
    {
        x->vector = 5;
        mj_text(x, "divide by zero");
    }
    else if (op == 0x4AFCu)
    {
        x->vector = 4;
        mj_text(x, "ILLEGAL");
    }
    else if ((op & 0xFFF0u) == 0x4E40u)
    {
        x->vector = 32u + (op & 15u);
        mj_text(x, "TRAP");
    }
    else if ((op & 0xF000u) == 0xF000u && !mj_040_integer_fline(op))
    {
        x->vector = 11;
        mj_text(x, "Line-F");
    }
    else
    {
        x->reason = M68KENG_STOP_UNSUPPORTED;
        mj_text(x, in.why ? in.why : "unexpected fault exit");
    }
}

/* One run: blocks are entered one at a time (slice budget 1), so the count
   of retired instructions is exact: a whole block when it ends through the
   lookup or the budget, the instructions before the exit PC otherwise. No
   block starts once budget instructions have retired. */
void mj_tr_run(struct M68KEngCPU *owner, ULONG budget, struct M68KEngExit *x)
{
    struct HJCpu *c = owner->tr;
    struct HJBackend *b = c->b;
    const struct M68KEngDomain *dom = owner->domain;
    struct HJState *st = c->st;
    ULONG retired = 0;

    if (c->pages_generation != dom->pages_generation)
    {
        memcpy(st->pages, dom->pages, dom->npages);
        c->pages_generation = dom->pages_generation;
    }
    st->abort = 0;
    c->declined = 0;
#if HJ_RUNS
    __asm__ __volatile__("isb" ::: "memory");
#endif
    for (;;)
    {
        uint32_t pc = st->pc, reason;
        struct HJBlock *blk;

        if (pc & 1)
        {
            mj_exit(x, M68KENG_STOP_EXCEPTION, pc, retired);
            x->vector = 3;
            mj_text(x, "odd PC");
            return;
        }
        if (!mj_fetchable(dom, pc))
        {
            mj_exit(x, M68KENG_STOP_PC_OUTSIDE, pc, retired);
            x->address = pc;
            return;
        }
        blk = get_block(c, pc);
        if (!blk)
        {
            mj_exit(x, M68KENG_STOP_FAULT, pc, retired);
            mj_text(x, "translation failed");
            return;
        }
        b->stats.enters++;
        reason = b->enter(st, blk->code);
        if (reason < HJ_EXIT_COUNT)
            b->stats.exits[reason]++;
        if (reason == HJ_EXIT_BUDGET || reason == HJ_EXIT_LOOKUP)
        {
            retired += blk->ninsns;
            if (retired >= budget)
            {
                mj_exit(x, M68KENG_STOP_BUDGET, st->pc, retired);
                return;
            }
            continue;
        }
        retired += count_before(blk, st->pc);
        if (reason == HJ_EXIT_LINEA)
        {
            mj_exit(x, M68KENG_STOP_ALINE, st->pc, retired);
            x->opcode = mj_opcode(dom, st->pc);
            return;
        }
        if (reason == HJ_EXIT_ABORT)
        {
            mj_exit(x, M68KENG_STOP_HOST, st->pc, retired);
            mj_text(x, "a host access asked to stop");
            return;
        }
        if (c->declined)
        {
            /* a divisor probe ended the block at the division: nothing of
               it done and no host call made (contract, section 6) */
            c->declined = 0;
            mj_exit(x, M68KENG_STOP_UNSUPPORTED, st->pc, retired);
            x->opcode = mj_opcode(dom, st->pc);
            mj_text(x, "divisor read through the host");
            return;
        }
        mj_classify(dom, x, st->pc, retired);
        return;
    }
}
