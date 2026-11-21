/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Host-side test: the CPU driver with an engine under the interpreter
    (m68kemu_engine.h). Two small engines stand in for a real one. The
    first declines every instruction: each instruction goes through the
    hand-over of the registers and is then the interpreter's. The second
    executes MOVEQ itself and declines the rest: values and flags an
    engine writes come back to the interpreter. A program must end as it
    does without an engine.

    The first instruction of a CPU is always the interpreter's: the core
    starts with a check pending, and the engine takes over only at a plain
    instruction boundary. The counts below have it.
*/
#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"
#define M68KEMU_HOST_WITH_MOIRA
#include "m68kemu_host_stubs.inc"

/* Stub thunk tables */
const struct M68KThunkEntry m68kemu_thunks_exec[] = { { 0, nullptr } };
const ULONG m68kemu_thunks_exec_count = 0;
const struct M68KThunkEntry m68kemu_thunks_dos[] = { { 0, nullptr } };
const ULONG m68kemu_thunks_dos_count = 0;

extern "C" {
ULONG M68KEmu_OpenLibrary(struct M68KEmuContext *, const char *) { return 0; }
ULONG M68KEmu_LoadM68KLibrary(struct M68KEmuContext *, const char *, ULONG) { return 0; }
}

#include "../../rom/m68kemu/m68kemu_memory.c"
#include "../../rom/m68kemu/Moira/Moira/Moira.cpp"
#include "../../rom/m68kemu/Moira/Moira/MoiraDebugger.cpp"
#include "../../rom/m68kemu/m68kemu_moira.cpp"

#include <cstdio>
#include <cstring>

static int pass = 0, fail = 0;
#define TEST(n) printf("  %-58s ", n)
#define OK() do { printf("PASS\n"); pass++; } while(0)
#define NG(m) do { printf("FAIL: %s\n", m); fail++; return; } while(0)
#define CHK(c,m) do { if(!(c)) NG(m); } while(0)

/* ---- the two test engines ------------------------------------------------ */

struct TestDomain
{
    struct M68KEmuContext *ctx;
    bool  moveq;            /* execute MOVEQ, decline the rest */
    bool  no_cpu;           /* cpu_create fails */
    bool  no_run;           /* run() fails */
    ULONG cpus, made, runs, retired, invalidations, destroyed;
};

struct TestCPU
{
    TestDomain *dom;
    ULONG reg[M68KENG_REG_COUNT];
};

static LONG e_domain_create(const struct M68KEngHost *, APTR, struct M68KEngMap *, ULONG, struct M68KEngDomain **)
{
    return M68KENG_ERR_UNSUPPORTED;     /* the tests make their domain themselves */
}

static LONG e_domain_destroy(struct M68KEngDomain *domain)
{
    TestDomain *d = (TestDomain *)domain;

    if (d->cpus)
        return M68KENG_ERR_STATE;
    d->destroyed++;
    return M68KENG_OK;
}

static LONG e_invalidate(struct M68KEngDomain *domain, ULONG, ULONG)
{
    ((TestDomain *)domain)->invalidations++;
    return M68KENG_OK;
}

static LONG e_cpu_create(struct M68KEngDomain *domain, APTR, struct M68KEngCPU **cpu)
{
    TestDomain *d = (TestDomain *)domain;
    TestCPU *c;

    if (!d || !cpu)
        return M68KENG_ERR_ARG;
    *cpu = nullptr;
    if (d->no_cpu)
        return M68KENG_ERR_NOMEM;
    c = new TestCPU();
    c->dom = d;
    c->reg[M68KENG_REG_SR] = M68KENG_SR_AT_CREATE;
    d->cpus++;
    d->made++;
    *cpu = (struct M68KEngCPU *)c;
    return M68KENG_OK;
}

static LONG e_cpu_destroy(struct M68KEngCPU *cpu)
{
    TestCPU *c = (TestCPU *)cpu;

    c->dom->cpus--;
    delete c;
    return M68KENG_OK;
}

static LONG e_get_reg(struct M68KEngCPU *cpu, ULONG reg, ULONG *value)
{
    if (!cpu || !value || reg >= M68KENG_REG_COUNT)
        return M68KENG_ERR_ARG;
    *value = ((TestCPU *)cpu)->reg[reg];
    return M68KENG_OK;
}

static LONG e_set_reg(struct M68KEngCPU *cpu, ULONG reg, ULONG value)
{
    if (!cpu || reg >= M68KENG_REG_COUNT)
        return M68KENG_ERR_ARG;
    if (reg == M68KENG_REG_SR)
    {
        /* only a status register inside the model, and only its defined bits */
        if ((value & M68KENG_SR_MODEL_MASK) != M68KENG_SR_MODEL_VALUE)
            return M68KENG_ERR_UNSUPPORTED;
        value &= M68KENG_SR_DEFINED;
    }
    ((TestCPU *)cpu)->reg[reg] = value;
    return M68KENG_OK;
}

static LONG e_run(struct M68KEngCPU *cpu, ULONG budget, struct M68KEngExit *x)
{
    TestCPU *c = (TestCPU *)cpu;
    TestDomain *d;
    ULONG n = 0, pc;

    if (!c || !x || !budget || budget > M68KENG_MAX_BUDGET)
        return M68KENG_ERR_ARG;
    if (x->size_of < M68KENG_V1_SIZE_EXIT)
        return M68KENG_ERR_SIZE;
    d = c->dom;
    if (d->no_run)
        return M68KENG_ERR_STATE;
    d->runs++;
    x->opcode = x->vector = x->address = 0;
    x->text[0] = 0;
    for (;;)
    {
        UWORD op;

        pc = c->reg[M68KENG_REG_PC];
        if (n >= budget)
        {
            x->reason = M68KENG_STOP_BUDGET;
            break;
        }
        if (pc & 1)
        {
            x->reason = M68KENG_STOP_EXCEPTION;     /* address error */
            x->vector = 3;
            break;
        }
        if (pc >= d->ctx->mem_size - 1)
        {
            x->reason = M68KENG_STOP_PC_OUTSIDE;
            x->address = pc;
            break;
        }
        op = m68k_read16(d->ctx, pc);
        if (!d->moveq || (op & 0xF100) != 0x7000)
        {
            x->reason = (op & 0xF000) == 0xA000 ? M68KENG_STOP_ALINE : M68KENG_STOP_UNSUPPORTED;
            x->opcode = op;
            if (x->reason == M68KENG_STOP_UNSUPPORTED)
                strcpy(x->text, "declined");
            break;
        }
        /* MOVEQ #imm,Dn: N and Z from the value, V and C clear, X as it was */
        LONG v = (BYTE)(op & 0xFF);
        ULONG sr = c->reg[M68KENG_REG_SR] & ~0x000FUL;

        c->reg[M68KENG_REG_D0 + ((op >> 9) & 7)] = (ULONG)v;
        c->reg[M68KENG_REG_SR] = sr | (v < 0 ? 8 : 0) | (v == 0 ? 4 : 0);
        c->reg[M68KENG_REG_PC] = pc + 2;
        n++;
    }
    d->retired += n;
    x->pc = pc;
    x->retired = n;
    return M68KENG_OK;
}

static LONG e_get_stats(struct M68KEngDomain *domain, struct M68KEngStats *stats)
{
    TestDomain *d = (TestDomain *)domain;

    if (!d || !stats)
        return M68KENG_ERR_ARG;
    if (stats->size_of < M68KENG_V1_SIZE_STATS)
        return M68KENG_ERR_SIZE;
    /* these engines translate nothing */
    stats->blocks_translated = stats->insns_translated = stats->code_resets = stats->code_bytes = 0;
    stats->cpus = d->cpus;
    stats->runs = d->runs;
    stats->invalidations = d->invalidations;
    stats->retired = d->retired;
    return M68KENG_OK;
}

static const struct M68KEngOps test_ops =
{
    sizeof(struct M68KEngOps), M68KENG_ABI_MAJOR, M68KENG_ABI_MINOR, M68KENG_CAP_BASE, 16,
    "test engine",
    e_domain_create, e_domain_destroy, e_invalidate, e_cpu_create, e_cpu_destroy,
    e_get_reg, e_set_reg, e_run, e_get_stats
};

/* a context whose program runs with the test engine, as M68KEMU_CPU would set it */
static struct M68KEmuContext *with_engine(struct M68KEmuLibBase *base, TestDomain *d, int mode)
{
    struct M68KEmuContext *ctx = M68KEmu_CreateContext(base);

    if (!ctx)
        return nullptr;
    d->ctx = ctx;
    ctx->eng_ops = &test_ops;
    ctx->eng_domain = (struct M68KEngDomain *)d;
    ctx->eng_mode = mode;
    ctx->eng_run = TRUE;
    ctx->m68k_argptr = ctx->m68k_argsize = ctx->m68k_sysbase = 0;
    return ctx;
}

static ULONG words(struct M68KEmuContext *ctx, const UWORD *w, unsigned n)
{
    ULONG a = M68KEmu_HeapAlloc(ctx, 2 * n, 0);

    for (unsigned i = 0; a && i < n; i++)
        m68k_write16(ctx, a + 2 * i, w[i]);
    return a;
}

static const UWORD prog_42[] = { 0x702A, 0x4E75 };                 /* MOVEQ #42,D0; RTS */
/* MOVEQ #0,D0; MOVEQ #10,D1; loop: ADD.W D1,D0; SUBQ.W #1,D1; BNE.S loop; RTS */
static const UWORD prog_sum[] = { 0x7000, 0x720A, 0xD041, 0x5341, 0x66FA, 0x4E75 };
/* NOP; MOVEQ #-1,D1; BPL.S bad; MOVEQ #0,D1; BNE.S bad; BMI.S bad; MOVEQ #7,D0; RTS; bad: MOVEQ #99,D0; RTS */
static const UWORD prog_flags[] = { 0x4E71, 0x72FF, 0x6A0A, 0x7200, 0x6606, 0x6B04, 0x7007, 0x4E75, 0x7063, 0x4E75 };

/* ---- the tests ------------------------------------------------------------ */

void t_decline_42()
{
    TEST("Declining engine: MOVEQ #42,D0; RTS");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    auto *ctx = with_engine(&base, &d, M68KCPU_ENGINE);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_42, 2);
    CHK(ctx->entry_point, "alloc failed");

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 42, "expected 42");
    CHK(d.made == 1 && d.runs == 1 && d.retired == 0, "one engine CPU, one declined run for the RTS, nothing retired");
    CHK(ctx->eng_interp == 2 && !ctx->eng_fault && !ctx->eng_refused, "both instructions were the interpreter's");

    M68KEmu_DestroyContext(ctx);
    CHK(d.cpus == 0 && d.destroyed == 1, "the engine CPU went before the domain");
    OK();
}

void t_decline_loop()
{
    TEST("Declining engine: loop, sum 1..10 = 55");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    auto *ctx = with_engine(&base, &d, M68KCPU_ENGINE);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_sum, 6);

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 55, "expected 55");
    /* two instructions, ten turns of three, the RTS: 33, the first without a run */
    CHK(d.runs == 32 && d.retired == 0 && ctx->eng_interp == 33, "a declined run for each instruction after the first");

    M68KEmu_DestroyContext(ctx);
    CHK(d.cpus == 0 && d.destroyed == 1, "teardown");
    OK();
}

void t_moveq_values()
{
    TEST("MOVEQ engine: its values come back");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    d.moveq = true;
    auto *ctx = with_engine(&base, &d, M68KCPU_ENGINE);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_sum, 6);

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 55, "expected 55");
    CHK(d.retired == 1, "the engine executed the second MOVEQ");
    /* one run for that MOVEQ up to the ADD, then one for each of the 30 instructions after the ADD */
    CHK(d.runs == 31 && ctx->eng_interp == 32, "31 runs, 32 instructions of the interpreter");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_moveq_flags()
{
    TEST("MOVEQ engine: its flags come back");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    d.moveq = true;
    auto *ctx = with_engine(&base, &d, M68KCPU_ENGINE);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_flags, 10);

    /* N set by the first MOVEQ, then Z set and N clear by the second */
    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 7, "a branch on a flag of the engine went the wrong way");
    /* a run for each MOVEQ and its branch, and one that declines the BMI */
    CHK(d.retired == 3 && d.runs == 4, "the three MOVEQ on the way were the engine's");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_no_cpu_auto()
{
    TEST("No engine CPU, auto: the interpreter runs the program");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    d.no_cpu = true;
    auto *ctx = with_engine(&base, &d, M68KCPU_AUTO);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_42, 2);

    LONG result = M68KEmu_Execute(ctx);
    CHK(result == 42, "expected 42");
    CHK(d.runs == 0 && !ctx->eng_run && !ctx->eng_refused && ctx->eng_why, "no engine run, and a reason");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_no_cpu_jit()
{
    TEST("No engine CPU, jit: the program is not run");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    d.no_cpu = true;
    auto *ctx = with_engine(&base, &d, M68KCPU_ENGINE);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_42, 2);

    LONG result = M68KEmu_Execute(ctx);
    CHK(ctx->eng_refused && d.runs == 0, "refused before an instruction");
    CHK(result == -1, "a refused run gives -1");

    M68KEmu_DestroyContext(ctx);
    OK();
}

void t_run_fails()
{
    TEST("run() fails: the program stops as a fault");
    struct M68KEmuLibBase base = {};
    TestDomain d = {};
    d.no_run = true;
    auto *ctx = with_engine(&base, &d, M68KCPU_ENGINE);
    CHK(ctx, "ctx null");
    ctx->entry_point = words(ctx, prog_42, 2);

    LONG result = M68KEmu_Execute(ctx);
    CHK(ctx->eng_fault, "no fault recorded");
    CHK(result == 20, "a fault of the engine gives 20");

    M68KEmu_DestroyContext(ctx);
    CHK(d.cpus == 0 && d.destroyed == 1, "teardown after a fault");
    OK();
}

int main()
{
    printf("m68kemu engine tests\n");
    t_decline_42();
    t_decline_loop();
    t_moveq_values();
    t_moveq_flags();
    t_no_cpu_auto();
    t_no_cpu_jit();
    t_run_fails();
    printf("%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
