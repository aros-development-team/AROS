/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kjit against Moira, one instruction at a time.

    Each case is one instruction with random extension words, random data
    and address registers, a random CCR and random memory. Moira, set up as
    m68kemu.library sets it up (68LC040), executes it; the engine gets the
    same state through its interface and runs it. Then the two are
    compared: D0-D7, A0-A7, the PC, the status register and every byte of
    the memory.

    - Where Moira completes the instruction, the engine either completes it
      with the same result, or declines it with nothing changed. Declining
      is what the interface lets an engine do: the host then executes the
      instruction, here Moira.
    - Where Moira takes an exception or stops, the engine must not have
      executed anything.

    One exception to Moira as the reference: what the divisions leave in
    the flags, and the signed division of 0x80000000 by -1. See "where the
    reference is the manual" below.

    Half of the opcodes are drawn from all 65536 words, half from the
    groups whose results are easy to get wrong: shifts and rotates, bit
    fields, ADDX, SUBX and NEGX, CAS and CAS2, the long multiplications and
    divisions, the word ones, MOVEP, the decimal instructions, the bit
    instructions, MOVEM, CHK and CMP2, Scc and DBcc.

    usage: m68kjit_moira_compare [cases [seed [examples [names]]]]
    It prints the counts and returns 0 when no case differs. Of the cases
    that differ it prints the first ones of each mnemonic (one, or as many
    as the third argument says) and their number by mnemonic. With a
    fourth argument it also counts by mnemonic the cases that are the same
    and the ones the engine declined.
*/
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#include "Moira/Moira.cpp"
#include "Moira/MoiraDebugger.cpp"

extern "C" {
#include "m68kjit_intern.h"
}

using namespace moira;

#define ARENA   0x10000u        /* guest memory of both */
#define CODE    0x8000u         /* where the instruction is */
#define WORDS   6               /* the opcode and five words after it */
#define ALINE   0xA000u         /* what ends the engine's run after the instruction */

/* ---- random numbers: xorshift64*, the same sequence for a seed everywhere ---- */

static uint64_t rng_state;

static uint64_t rnd64(void)
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 0x2545F4914F6CDD1DULL;
}

static uint32_t rnd(void) { return (uint32_t)(rnd64() >> 32); }

/* ---- the state both start from ---- */

struct Regs
{
    uint32_t d[8], a[8], pc;
    uint16_t sr;
};

static uint8_t image[ARENA];        /* the memory a case starts with */
static uint8_t refmem[ARENA];       /* Moira's */
static uint8_t engmem[ARENA];       /* the engine's */

/* ---- the reference: Moira as m68kemu.library configures it ---- */

class Ref : public Moira
{
public:
    int exception = 0;      /* the vector of an exception taken by the last instruction, or 0 */

    u8 read8(u32 addr) const override { return addr < ARENA ? refmem[addr] : 0; }
    u16 read16(u32 addr) const override { return (u16)(read8(addr) << 8 | read8(addr + 1)); }
    void write8(u32 addr, u8 val) const override { if (addr < ARENA) refmem[addr] = val; }
    void write16(u32 addr, u16 val) const override { write8(addr, (u8)(val >> 8)); write8(addr + 1, (u8)val); }
    void willExecute(M68kException exc, u16 vector) override { (void)exc; exception = vector ? vector : 255; }

    void start(void)
    {
        setModel(Model::M68LC040);
        reset();
        flags = 0;
    }

    /* One instruction from r; false when it took an exception or left the
       core in a state other than running (stopped, halted). */
    bool step(const Regs &r, Regs &out)
    {
        exception = 0;
        flags = 0;
        setSR(r.sr);
        flags = 0;
        for (int i = 0; i < 8; i++)
        {
            setD(i, r.d[i]);
            setA(i, r.a[i]);
        }
        setPC0(r.pc);
        setPC(r.pc);
        setIRD(read16(r.pc));
        setIRC(read16(r.pc + 2));
        try
        {
            execute();
        }
        catch (...)
        {
            exception = 254;        /* Moira gave up on it (a double fault) */
            flags = 0;
        }
        for (int i = 0; i < 8; i++)
        {
            out.d[i] = getD(i);
            out.a[i] = getA(i);
        }
        out.pc = getPC();
        out.sr = getSR();
        return !exception && !flags;
    }

    void text(char *str, u32 pc) { disassemble(str, pc); }
};

/* ---- the engine's host: memory outside the arena reads as 0 and takes no writes ---- */

static ULONG host_read(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width, ULONG pc, ULONG *value)
{
    (void)host_domain; (void)host_cpu; (void)pc;
    ULONG hi = addr < ARENA ? engmem[addr] : 0;

    if (width == 1)
        *value = hi;
    else
    {
        ULONG next = addr + 1;

        *value = hi << 8 | (next < ARENA ? engmem[next] : 0);
    }
    return 0;
}

static ULONG host_write(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width, ULONG value, ULONG pc)
{
    (void)host_domain; (void)host_cpu; (void)pc;
    if (width == 1)
    {
        if (addr < ARENA)
            engmem[addr] = (UBYTE)value;
    }
    else
    {
        ULONG next = addr + 1;

        if (addr < ARENA)
            engmem[addr] = (UBYTE)(value >> 8);
        if (next < ARENA)
            engmem[next] = (UBYTE)value;
    }
    return 0;
}

/* ---- the cases ---- */

static uint16_t pick_opcode(void)
{
    static const struct { uint16_t bits, free; } group[] =
    {
        { 0xE000, 0x0FFF },     /* shifts and rotates, registers and memory; bit fields */
        { 0xE8C0, 0x073F },     /* bit fields */
        { 0xD100, 0x0ECF },     /* ADDX */
        { 0x9100, 0x0ECF },     /* SUBX */
        { 0x4000, 0x00FF },     /* NEGX */
        { 0x08C0, 0x063F },     /* CAS */
        { 0x0CFC, 0x0200 },     /* CAS2 */
        { 0x4C00, 0x007F },     /* MULU.L, MULS.L, DIVU.L, DIVS.L */
        { 0xC0C0, 0x0F3F },     /* MULU.W, MULS.W */
        { 0x80C0, 0x0F3F },     /* DIVU.W, DIVS.W */
        { 0x0108, 0x0EC7 },     /* MOVEP */
        { 0xC100, 0x0E0F },     /* ABCD */
        { 0x8100, 0x0E0F },     /* SBCD */
        { 0x4800, 0x003F },     /* NBCD */
        { 0x8140, 0x0E8F },     /* PACK, UNPK */
        { 0x0100, 0x0EFF },     /* BTST, BCHG, BCLR, BSET by a register */
        { 0x0800, 0x00FF },     /* the same by a number */
        { 0x4880, 0x047F },     /* MOVEM, EXT */
        { 0x4100, 0x0EBF },     /* CHK, LEA */
        { 0x00C0, 0x063F },     /* CMP2, CHK2 */
        { 0x50C0, 0x0F3F },     /* Scc, DBcc, TRAPcc */
        { 0x4AC0, 0x003F },     /* TAS */
        { 0x4840, 0x003F },     /* SWAP, PEA */
        { 0x4E40, 0x003F },     /* TRAP, LINK, UNLK, MOVE USP, the 4E7x group */
    };
    const unsigned groups = sizeof(group) / sizeof(group[0]);
    uint32_t r = rnd();
    uint16_t op;

    do
    {
        if (r & 1)
            op = (uint16_t)(r >> 8);
        else
        {
            unsigned g = (r >> 1) % groups;

            op = (uint16_t)(group[g].bits | ((r >> 8) & group[g].free));
        }
        r = rnd();
    } while ((op & 0xF000) == 0xA000);      /* an A-line opcode is the host's by the interface */
    return op;
}

static uint32_t pick_data(void)
{
    uint32_t r = rnd();

    switch (r & 15)
    {
    case 0: return 0;
    case 1: return 0xFFFFFFFFu;
    case 2: return 0x80000000u;
    case 3: return 0x7FFFFFFFu;
    case 4: return rnd() & 63;                  /* a shift count, a bit number */
    case 5: return rnd() & 0xFFFF;
    case 6: return 0xFFFF0000u | (rnd() & 0xFFFF);
    case 7: return rnd() & 0xFF;
    case 8: return (uint32_t)1 << (rnd() & 31);
    default: return rnd();
    }
}

static void put16(uint8_t *mem, uint32_t at, uint16_t v)
{
    mem[at] = (uint8_t)(v >> 8);
    mem[at + 1] = (uint8_t)v;
}

static void make_case(Regs &r, uint16_t words[WORDS])
{
    int i;

    for (i = 0; i < 8; i++)
        r.d[i] = pick_data();
    for (i = 0; i < 7; i++)
    {
        r.a[i] = 0x1000u + (rnd() % 0xE000u);       /* inside the arena */
        if (rnd() & 7)
            r.a[i] &= ~1u;                          /* mostly even */
    }
    r.a[7] = (0xC000u + (rnd() % 0x2000u)) & ~1u;   /* the stack, always even */
    r.pc = CODE;
    r.sr = (uint16_t)(0x2700u | (rnd() & 0x1F));
    words[0] = pick_opcode();
    for (i = 1; i < WORDS; i++)
        words[i] = (rnd() & 3) ? (uint16_t)rnd() : (uint16_t)(rnd() & 0x00FF);
    if (rnd() & 3)
    {
        /* most of the time, an extension word without the bits that the
           instruction reserves: with them set an engine may decline it */
        uint16_t op = words[0];

        if ((op & 0xFF80) == 0x4C00)                            /* MUL.L, DIV.L: 0 Dq s z 0000000 Dr */
            words[1] &= 0x7C07;
        else if ((op & 0xF8C0) == 0xE8C0)                       /* bit fields: 0 Dn o offset w width */
            words[1] &= 0x7FFF;
        else if ((op & 0xFDFF) == 0x0CFC)                       /* CAS2: two words, Rn 000 Du 000 Dc */
        {
            words[1] &= 0xF1C7;
            words[2] &= 0xF1C7;
        }
        else if ((op & 0xF9C0) == 0x08C0 && (op & 0x0600))      /* CAS: 0000000 Du 000 Dc */
            words[1] &= 0x01C7;
        else if ((op & 0xF9C0) == 0x00C0)                       /* CMP2, CHK2: Rn c 00000000000 */
            words[1] &= 0xF800;
    }
    /* some new memory around the registers and at random */
    for (i = 0; i < 64; i++)
        image[rnd() % ARENA] = (uint8_t)rnd();
    for (i = 0; i < WORDS; i++)
        put16(image, CODE + 2u * (uint32_t)i, words[i]);
}

/* ---- where the reference is the manual ---- */

/* Moira, built as m68kemu.library builds it (MOIRA_MIMIC_MUSASHI), departs
   from the 68040 in three things after DIVU.W and DIVS.W (divuMusashi()
   and divsMusashi() in MoiraALU_cpp.h):

   - it sets Z when the quotient is NOT zero and clears it when it is: it
     assigns the quotient to a flag that is a bool;
   - on overflow it leaves C as it was, where the processor clears it;
   - it takes 0x80000000 / -1 as a division that gives 0 with N, Z, V and C
     clear, where the processor reports the overflow it is and leaves the
     register alone.

   And after DIVU.L and DIVS.L (execDivlMusashi() in MoiraExec_cpp.h,
   divluMusashi() and divlsMusashi() in MoiraALU_cpp.h):

   - on overflow it leaves C as it was;
   - it takes 0x80000000 / -1 with a dividend of 32 bits as a division that
     gives 0x80000000 with no remainder, N set and V clear, where the
     processor reports the overflow.

   The engine follows the manual (M68000 Family Programmer's Reference
   Manual, DIVS and DIVU: Z set if the quotient is zero, C always cleared,
   operands unaffected on overflow), and on overflow it leaves N and Z as
   they were. So for the divisions the reference is worked out here, from
   the quotient that Moira gives. */
static bool word_division(uint16_t op) { return (op & 0xF0C0) == 0x80C0; }
static bool long_division(const uint16_t w[]) { return (w[0] & 0xFFC0) == 0x4C40; }

/* DIVU.L and DIVS.L. False where the case cannot be used: a dividend of
   0x80000000 whose divisor, in memory, is 1 or -1, which Moira's result
   does not tell apart. */
static bool manual_long_division(const uint16_t w[], const Regs &before, Regs &want, bool &changed)
{
    const uint16_t X = 0x10, N = 0x08, Z = 0x04, V = 0x02, C = 0x01;
    unsigned dq = (w[1] >> 12) & 7, dr = w[1] & 7;
    bool signed32 = (w[1] & 0x0C00) == 0x0800;

    changed = false;
    if (signed32 && before.d[dq] == 0x80000000u && want.d[dq] == 0x80000000u && (dr == dq || want.d[dr] == 0) &&
        !(want.sr & V))
    {
        if (w[0] & 0x38)
            return false;
        if (before.d[w[0] & 7] == 0xFFFFFFFFu)
        {
            want.d[dr] = before.d[dr];
            want.d[dq] = before.d[dq];
            want.sr = (uint16_t)((want.sr & ~0x1F) | (before.sr & (X | N | Z)) | V);
            changed = true;
        }
        return true;
    }
    if ((want.sr & V) && (want.sr & C))
    {
        want.sr &= (uint16_t)~C;
        changed = true;
    }
    return true;
}

static void manual_word_division(uint16_t op, const Regs &before, Regs &want)
{
    const uint16_t X = 0x10, N = 0x08, Z = 0x04, V = 0x02;
    unsigned dn = (op >> 9) & 7;
    bool is_signed = (op & 0x0100) != 0;
    uint16_t ccr;

    if (is_signed && before.d[dn] == 0x80000000u && want.d[dn] == 0)
    {
        /* only 0x80000000 / -1 leaves 0 in the register */
        want.d[dn] = before.d[dn];
        ccr = (uint16_t)((before.sr & (X | N | Z)) | V);
    }
    else if (want.sr & V)
        ccr = (uint16_t)((before.sr & (X | N | Z)) | V);
    else
    {
        uint16_t q = (uint16_t)want.d[dn];

        ccr = (uint16_t)((before.sr & X) | ((q & 0x8000) ? N : 0) | (q ? 0 : Z));
    }
    want.sr = (uint16_t)((want.sr & ~0x1F) | ccr);
}

/* ---- comparing ---- */

static unsigned long cases_run, same, declined, exceptions, discarded, differ, by_the_manual;
static unsigned long declined_by_line[16];
static std::map<std::string, unsigned long> differ_by_name;    /* by mnemonic */
static std::map<std::string, unsigned long> same_by_name, declined_by_name;
static bool by_name;                                           /* count those two as well */

static std::string mnemonic(const char *str)
{
    std::string name(str);
    size_t end = name.find(' ');

    if (end != std::string::npos)
        name.resize(end);
    return name;
}
static unsigned long show_each = 1;                            /* examples printed for each mnemonic */

static void show(const char *what, Ref &ref, const Regs &before, const Regs &want, const Regs &got,
                 const struct M68KEngExit &x, const uint16_t words[WORDS])
{
    char str[128];
    int i;

    memcpy(refmem, image, ARENA);
    ref.text(str, before.pc);
    if (++differ_by_name[mnemonic(str)] > show_each)
        return;
    printf("DIFFERS: %s\n  %04x %04x %04x %04x %04x %04x   %s\n", what, words[0], words[1], words[2], words[3], words[4],
           words[5], str);
    printf("  before  SR %04x", before.sr);
    for (i = 0; i < 8; i++)
        printf(" D%d %08x", i, before.d[i]);
    printf("\n                 ");
    for (i = 0; i < 8; i++)
        printf(" A%d %08x", i, before.a[i]);
    printf("\n  Moira   SR %04x PC %08x", want.sr, want.pc);
    for (i = 0; i < 8; i++)
        if (want.d[i] != before.d[i] || got.d[i] != before.d[i])
            printf(" D%d %08x", i, want.d[i]);
    for (i = 0; i < 8; i++)
        if (want.a[i] != before.a[i] || got.a[i] != before.a[i])
            printf(" A%d %08x", i, want.a[i]);
    printf("\n  engine  SR %04x PC %08x", got.sr, got.pc);
    for (i = 0; i < 8; i++)
        if (want.d[i] != before.d[i] || got.d[i] != before.d[i])
            printf(" D%d %08x", i, got.d[i]);
    for (i = 0; i < 8; i++)
        if (want.a[i] != before.a[i] || got.a[i] != before.a[i])
            printf(" A%d %08x", i, got.a[i]);
    printf("\n          exit reason %u, retired %u, vector %u, text \"%s\"\n", (unsigned)x.reason, (unsigned)x.retired,
           (unsigned)x.vector, x.text);
}

static bool same_regs(const Regs &a, const Regs &b, bool with_pc)
{
    return memcmp(a.d, b.d, sizeof(a.d)) == 0 && memcmp(a.a, b.a, sizeof(a.a)) == 0 && a.sr == b.sr &&
           (!with_pc || a.pc == b.pc);
}

int main(int argc, char **argv)
{
    unsigned long cases = argc > 1 ? strtoul(argv[1], NULL, 0) : 1000000ul;
    uint64_t seed = argc > 2 ? strtoull(argv[2], NULL, 0) : 0x6D36386B6A697421ULL;
    const struct M68KEngOps *ops = M68KJit_GetEngineTable(M68KENG_ABI_MAJOR);
    struct M68KEngHost host;
    struct M68KEngMap map;
    struct M68KEngDomain *domain = NULL;
    struct M68KEngCPU *cpu = NULL;
    Ref ref;
    unsigned long n;
    unsigned i;

    if (!ops || !mj_tr_runs())
    {
        printf("SKIP: translated code runs only on an AArch64 host\n");
        return 1;
    }
    if (!seed)
        seed = 1;
    rng_state = seed;
    for (i = 0; i < ARENA; i++)
        image[i] = (uint8_t)rnd();
    memset(&host, 0, sizeof(host));
    host.size_of = sizeof(host);
    host.abi_major = M68KENG_ABI_MAJOR;
    host.abi_minor = M68KENG_ABI_MINOR;
    host.read = host_read;
    host.write = host_write;
    memset(&map, 0, sizeof(map));
    map.size_of = sizeof(map);
    map.generation = 1;
    map.base = engmem;
    map.size = ARENA;
    if (ops->domain_create(&host, NULL, &map, M68KENG_CAP_BASE, &domain) != M68KENG_OK ||
        ops->cpu_create(domain, NULL, &cpu) != M68KENG_OK)
    {
        printf("no domain or no CPU\n");
        return 1;
    }
    memcpy(refmem, image, ARENA);
    ref.start();

    if (argc > 3)
        show_each = strtoul(argv[3], NULL, 0);
    by_name = argc > 4;
    for (n = 0; n < cases; n++)
    {
        Regs before, want, got, probe;
        uint16_t words[WORDS];
        struct M68KEngExit x;
        bool completes, mem_same;
        ULONG v = 0;

        make_case(before, words);

        /* Moira once, to see where the instruction ends: an A-line opcode
           goes there, which ends the engine's run after one instruction */
        memcpy(refmem, image, ARENA);
        completes = ref.step(before, probe);
        if (completes && !(probe.pc & 1) && probe.pc < ARENA - 1)
        {
            put16(image, probe.pc, ALINE);
            memcpy(refmem, image, ARENA);
            completes = ref.step(before, want);
            if (!completes || want.pc != probe.pc)
            {
                /* the instruction reads the place where it ends: not a case */
                discarded++;
                continue;
            }
        }
        else if (completes)
            want = probe;
        if (completes && word_division(words[0]))
        {
            manual_word_division(words[0], before, want);
            by_the_manual++;
        }
        else if (completes && long_division(words))
        {
            bool changed;

            if (!manual_long_division(words, before, want, changed))
            {
                discarded++;
                continue;
            }
            if (changed)
                by_the_manual++;
        }

        memcpy(engmem, image, ARENA);
        ops->invalidate(domain, 0, M68KENG_INVALIDATE_ALL);
        for (i = 0; i < 8; i++)
        {
            ops->set_reg(cpu, M68KENG_REG_D0 + i, before.d[i]);
            ops->set_reg(cpu, M68KENG_REG_A0 + i, before.a[i]);
        }
        ops->set_reg(cpu, M68KENG_REG_SR, before.sr);
        ops->set_reg(cpu, M68KENG_REG_PC, before.pc);
        memset(&x, 0, sizeof(x));
        x.size_of = sizeof(x);
        if (ops->run(cpu, 1, &x) != M68KENG_OK)
        {
            printf("DIFFERS: run() refused case %lu\n", n);
            differ++;
            continue;
        }
        for (i = 0; i < 8; i++)
        {
            ops->get_reg(cpu, M68KENG_REG_D0 + i, &v);
            got.d[i] = v;
            ops->get_reg(cpu, M68KENG_REG_A0 + i, &v);
            got.a[i] = v;
        }
        ops->get_reg(cpu, M68KENG_REG_PC, &v);
        got.pc = v;
        ops->get_reg(cpu, M68KENG_REG_SR, &v);
        got.sr = (uint16_t)v;
        cases_run++;

        if (x.retired == 0)
        {
            /* the engine did not execute it: nothing may have changed */
            mem_same = memcmp(engmem, image, ARENA) == 0;
            if (!same_regs(got, before, true) || !mem_same || x.reason == M68KENG_STOP_BUDGET ||
                x.reason == M68KENG_STOP_HOST || x.reason == M68KENG_STOP_FAULT)
            {
                show(mem_same ? "the engine reports nothing executed, and its registers or its exit say otherwise"
                              : "the engine reports nothing executed, and memory changed",
                     ref, before, completes ? want : before, got, x, words);
                differ++;
            }
            else if (completes)
            {
                declined++;
                declined_by_line[words[0] >> 12]++;
                if (by_name)
                {
                    char str[128];

                    memcpy(refmem, image, ARENA);
                    ref.text(str, before.pc);
                    declined_by_name[mnemonic(str)]++;
                }
            }
            else
                exceptions++;
            continue;
        }
        if (!completes)
        {
            show("Moira takes an exception or stops, and the engine executed", ref, before, before, got, x, words);
            differ++;
            continue;
        }
        mem_same = memcmp(engmem, refmem, ARENA) == 0;
        if (x.retired != 1 || !same_regs(got, want, true) || !mem_same)
        {
            show(mem_same ? "registers, PC or status register" : "memory", ref, before, want, got, x, words);
            differ++;
            continue;
        }
        same++;
        if (by_name)
        {
            char str[128];

            memcpy(refmem, image, ARENA);
            ref.text(str, before.pc);
            same_by_name[mnemonic(str)]++;
        }
    }

    printf("m68kjit against Moira, seed %016llx: %lu cases; %lu the same, %lu declined by the engine with nothing changed, "
           "%lu exceptions or stops that the engine did not execute either, %lu not usable, %lu differ\n",
           (unsigned long long)seed, cases_run, same, declined, exceptions, discarded, differ);
    printf("  %lu divisions had their flags, or their overflow, from the manual, where Moira departs from it\n", by_the_manual);
    printf("  declined, by the first four bits of the opcode:");
    for (i = 0; i < 16; i++)
        if (declined_by_line[i])
            printf(" %X:%lu", i, declined_by_line[i]);
    printf("\n");
    if (differ)
    {
        printf("  differ, by mnemonic:");
        for (const auto &e : differ_by_name)
            printf(" %s:%lu", e.first.c_str(), e.second);
        printf("\n");
    }
    if (by_name)
    {
        printf("  the same, by mnemonic:");
        for (const auto &e : same_by_name)
            printf(" %s:%lu", e.first.c_str(), e.second);
        printf("\n  declined, by mnemonic:");
        for (const auto &e : declined_by_name)
            printf(" %s:%lu", e.first.c_str(), e.second);
        printf("\n");
    }
    ops->cpu_destroy(cpu);
    ops->domain_destroy(domain);
    return differ ? 1 : 0;
}
