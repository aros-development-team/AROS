/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — the six math libraries for m68k programs, computed
    natively: mathffp/mathtrans (Motorola FFP), mathieeesingbas/trans
    (IEEE single in D0), mathieeedoubbas/trans (IEEE double in D0/D1).
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <string.h>
#include <math.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

/* condition codes the caller sees after the call */
#define CCR_Z 0x04
#define CCR_N 0x08

static void ccr(struct M68KEmuContext *ctx, BOOL neg, BOOL zero)
{
    ctx->ccr_val = (neg ? CCR_N : 0) | (zero ? CCR_Z : 0);
    ctx->ccr_valid = TRUE;
}

/* ── formats ─────────────────────────────────────────────────────── */

/* FFP: mantissa 31..8 (normalised, top bit set), sign 7, exponent 6..0 excess 64 */
static double ffp2d(ULONG f)
{
    ULONG mant = f >> 8;
    int exp = (int)(f & 0x7F) - 64;
    double v;

    if (!mant) return 0.0;
    v = ldexp((double)mant / 16777216.0, exp);
    return (f & 0x80) ? -v : v;
}

static ULONG d2ffp(double v)
{
    int e;
    double m;
    ULONG mant, sign = 0;

    if (v == 0.0 || isnan(v)) return 0;
    if (v < 0) { sign = 0x80; v = -v; }
    m = frexp(v, &e);                         /* v = m * 2^e, 0.5 <= m < 1 */
    mant = (ULONG)(m * 16777216.0 + 0.5);
    if (mant >= 0x1000000) { mant >>= 1; e++; }
    e += 64;
    if (e <= 0) return 0;                     /* underflow */
    if (e > 127) return 0xFFFFFF7F | sign;    /* overflow: largest magnitude */
    return (mant << 8) | sign | (ULONG)e;
}

static float l2f(ULONG l) { float f; memcpy(&f, &l, 4); return f; }
static ULONG f2l(float f) { ULONG l; memcpy(&l, &f, 4); return l; }

static double dreg(void *cpu, int hi)
{
    UQUAD q = ((UQUAD)M68KEmu_GetD(cpu, hi) << 32) | M68KEmu_GetD(cpu, hi + 1);
    double d;
    memcpy(&d, &q, 8);
    return d;
}

/* result double in D0 (high) / D1 (low); returns D0 */
static IPTR dret(struct M68KEmuContext *ctx, void *cpu, double d)
{
    UQUAD q;
    memcpy(&q, &d, 8);
    M68KEmu_SetD(cpu, 1, (ULONG)q);
    ccr(ctx, d < 0, d == 0);
    return (IPTR)(ULONG)(q >> 32);
}

static IPTR fret(struct M68KEmuContext *ctx, ULONG f, double v)
{
    ccr(ctx, v < 0, f == 0 || v == 0);
    return f;
}

static LONG to_long(double v)
{
    if (isnan(v)) return 0;
    if (v >= 2147483647.0) return 0x7FFFFFFF;
    if (v <= -2147483648.0) return (LONG)0x80000000;
    return (LONG)v;                           /* truncates toward zero, like the ROM */
}

static IPTR cmpret(struct M68KEmuContext *ctx, double a, double b)
{
    LONG r = a > b ? 1 : a < b ? -1 : 0;
    ccr(ctx, r > 0, r == 0);
    return (IPTR)(ULONG)r;
}

static IPTR tstret(struct M68KEmuContext *ctx, double a)
{
    LONG r = a > 0 ? 1 : a < 0 ? -1 : 0;
    ccr(ctx, r < 0, r == 0);
    return (IPTR)(ULONG)r;
}

#define D(n)  M68KEmu_GetD(cpu, n)
#define A(n)  M68KEmu_GetA(cpu, n)
#define T(name) static IPTR name(struct M68KEmuContext *ctx, void *cpu)

/* ── mathffp.library ─────────────────────────────────────────────── */
T(ffp_Fix)   { LONG r = to_long(ffp2d(D(0))); ccr(ctx, r < 0, r == 0); return (IPTR)(ULONG)r; }
T(ffp_Flt)   { double v = (double)(LONG)D(0); return fret(ctx, d2ffp(v), v); }
T(ffp_Cmp)   { return cmpret(ctx, ffp2d(D(1)), ffp2d(D(0))); }
T(ffp_Tst)   { return tstret(ctx, ffp2d(D(1))); }
T(ffp_Abs)   { double v = fabs(ffp2d(D(0)));  return fret(ctx, d2ffp(v), v); }
T(ffp_Neg)   { double v = -ffp2d(D(0));       return fret(ctx, d2ffp(v), v); }
T(ffp_Add)   { double v = ffp2d(D(1)) + ffp2d(D(0)); return fret(ctx, d2ffp(v), v); }
T(ffp_Sub)   { double v = ffp2d(D(1)) - ffp2d(D(0)); return fret(ctx, d2ffp(v), v); }
T(ffp_Mul)   { double v = ffp2d(D(1)) * ffp2d(D(0)); return fret(ctx, d2ffp(v), v); }
T(ffp_Div)   { double d = ffp2d(D(0)), v = d != 0 ? ffp2d(D(1)) / d : 0; return fret(ctx, d2ffp(v), v); }
T(ffp_Floor) { double v = floor(ffp2d(D(0))); return fret(ctx, d2ffp(v), v); }
T(ffp_Ceil)  { double v = ceil(ffp2d(D(0)));  return fret(ctx, d2ffp(v), v); }

/* ── mathtrans.library ───────────────────────────────────────────── */
#define FFP1(name, expr) T(name) { double x = ffp2d(D(0)); double v = (expr); (void)x; return fret(ctx, d2ffp(v), v); }
FFP1(fft_Atan, atan(x))
FFP1(fft_Sin, sin(x))
FFP1(fft_Cos, cos(x))
FFP1(fft_Tan, tan(x))
FFP1(fft_Sinh, sinh(x))
FFP1(fft_Cosh, cosh(x))
FFP1(fft_Tanh, tanh(x))
FFP1(fft_Exp, exp(x))
FFP1(fft_Log, x > 0 ? log(x) : 0)
FFP1(fft_Sqrt, x >= 0 ? sqrt(x) : 0)
FFP1(fft_Asin, asin(x))
FFP1(fft_Acos, acos(x))
FFP1(fft_Log10, x > 0 ? log10(x) : 0)
T(fft_Sincos) { double x = ffp2d(D(0)); if (D(1)) m68k_write32(ctx, D(1), d2ffp(cos(x))); return fret(ctx, d2ffp(sin(x)), sin(x)); }
T(fft_Pow)    { double v = pow(ffp2d(D(0)), ffp2d(D(1))); return fret(ctx, d2ffp(v), v); }   /* fnum2^fnum1 */
T(fft_Tieee)  { double v = ffp2d(D(0)); return fret(ctx, f2l((float)v), v); }
T(fft_Fieee)  { double v = l2f(D(0)); return fret(ctx, d2ffp(v), v); }

/* ── mathieeesingbas.library ─────────────────────────────────────── */
#define SP(v) f2l((float)(v))
T(sb_Fix)   { LONG r = to_long(l2f(D(0))); ccr(ctx, r < 0, r == 0); return (IPTR)(ULONG)r; }
T(sb_Flt)   { double v = (double)(LONG)D(0); return fret(ctx, SP(v), v); }
T(sb_Cmp)   { return cmpret(ctx, l2f(D(0)), l2f(D(1))); }
T(sb_Tst)   { return tstret(ctx, l2f(D(0))); }
T(sb_Abs)   { double v = fabs(l2f(D(0))); return fret(ctx, SP(v), v); }
T(sb_Neg)   { double v = -l2f(D(0));      return fret(ctx, SP(v), v); }
T(sb_Add)   { double v = (double)l2f(D(0)) + l2f(D(1)); return fret(ctx, SP(v), v); }
T(sb_Sub)   { double v = (double)l2f(D(0)) - l2f(D(1)); return fret(ctx, SP(v), v); }
T(sb_Mul)   { double v = (double)l2f(D(0)) * l2f(D(1)); return fret(ctx, SP(v), v); }
T(sb_Div)   { double v = (double)l2f(D(0)) / l2f(D(1)); return fret(ctx, SP(v), v); }
T(sb_Floor) { double v = floor(l2f(D(0))); return fret(ctx, SP(v), v); }
T(sb_Ceil)  { double v = ceil(l2f(D(0)));  return fret(ctx, SP(v), v); }

/* ── mathieeesingtrans.library ───────────────────────────────────── */
#define SP1(name, expr) T(name) { double x = l2f(D(0)); double v = (expr); return fret(ctx, SP(v), v); }
SP1(st_Atan, atan(x))
SP1(st_Sin, sin(x))
SP1(st_Cos, cos(x))
SP1(st_Tan, tan(x))
SP1(st_Sinh, sinh(x))
SP1(st_Cosh, cosh(x))
SP1(st_Tanh, tanh(x))
SP1(st_Exp, exp(x))
SP1(st_Log, log(x))
SP1(st_Sqrt, sqrt(x))
SP1(st_Asin, asin(x))
SP1(st_Acos, acos(x))
SP1(st_Log10, log10(x))
T(st_Sincos) { double x = l2f(D(0)); if (A(0)) m68k_write32(ctx, A(0), SP(cos(x))); return fret(ctx, SP(sin(x)), sin(x)); }
T(st_Pow)    { double v = pow(l2f(D(0)), l2f(D(1))); return fret(ctx, SP(v), v); }   /* y^x */
T(st_Tieee)  { double v = l2f(D(0)); return fret(ctx, D(0), v); }
T(st_Fieee)  { double v = l2f(D(0)); return fret(ctx, D(0), v); }

/* ── mathieeedoubbas.library ─────────────────────────────────────── */
T(db_Fix)   { LONG r = to_long(dreg(cpu, 0)); ccr(ctx, r < 0, r == 0); return (IPTR)(ULONG)r; }
T(db_Flt)   { return dret(ctx, cpu, (double)(LONG)D(0)); }
T(db_Cmp)   { return cmpret(ctx, dreg(cpu, 0), dreg(cpu, 2)); }
T(db_Tst)   { return tstret(ctx, dreg(cpu, 0)); }
T(db_Abs)   { return dret(ctx, cpu, fabs(dreg(cpu, 0))); }
T(db_Neg)   { return dret(ctx, cpu, -dreg(cpu, 0)); }
T(db_Add)   { return dret(ctx, cpu, dreg(cpu, 0) + dreg(cpu, 2)); }
T(db_Sub)   { return dret(ctx, cpu, dreg(cpu, 0) - dreg(cpu, 2)); }
T(db_Mul)   { return dret(ctx, cpu, dreg(cpu, 0) * dreg(cpu, 2)); }
T(db_Div)   { return dret(ctx, cpu, dreg(cpu, 0) / dreg(cpu, 2)); }
T(db_Floor) { return dret(ctx, cpu, floor(dreg(cpu, 0))); }
T(db_Ceil)  { return dret(ctx, cpu, ceil(dreg(cpu, 0))); }

/* ── mathieeedoubtrans.library ───────────────────────────────────── */
#define DP1(name, expr) T(name) { double x = dreg(cpu, 0); return dret(ctx, cpu, (expr)); }
DP1(dt_Atan, atan(x))
DP1(dt_Sin, sin(x))
DP1(dt_Cos, cos(x))
DP1(dt_Tan, tan(x))
DP1(dt_Sinh, sinh(x))
DP1(dt_Cosh, cosh(x))
DP1(dt_Tanh, tanh(x))
DP1(dt_Exp, exp(x))
DP1(dt_Log, log(x))
DP1(dt_Sqrt, sqrt(x))
DP1(dt_Asin, asin(x))
DP1(dt_Acos, acos(x))
DP1(dt_Log10, log10(x))
T(dt_Sincos)
{
    double x = dreg(cpu, 0), c = cos(x);
    UQUAD q;
    memcpy(&q, &c, 8);
    if (A(0)) { m68k_write32(ctx, A(0), (ULONG)(q >> 32)); m68k_write32(ctx, A(0) + 4, (ULONG)q); }
    return dret(ctx, cpu, sin(x));
}
T(dt_Pow)   { return dret(ctx, cpu, pow(dreg(cpu, 2), dreg(cpu, 0))); }   /* y^x, x in D0/D1 */
T(dt_Tieee) { double v = dreg(cpu, 0); ULONG f = f2l((float)v); ccr(ctx, v < 0, v == 0); return f; }
T(dt_Fieee) { return dret(ctx, cpu, (double)l2f(D(0))); }

#undef T

/* LVOs: the six libraries share the usual layout, first function at -30 */
static const struct M68KThunkEntry ffp_tab[] = {
    { 30, ffp_Fix }, { 36, ffp_Flt }, { 42, ffp_Cmp }, { 48, ffp_Tst }, { 54, ffp_Abs },
    { 60, ffp_Neg }, { 66, ffp_Add }, { 72, ffp_Sub }, { 78, ffp_Mul }, { 84, ffp_Div },
    { 90, ffp_Floor }, { 96, ffp_Ceil } };
static const struct M68KThunkEntry fft_tab[] = {
    { 30, fft_Atan }, { 36, fft_Sin }, { 42, fft_Cos }, { 48, fft_Tan }, { 54, fft_Sincos },
    { 60, fft_Sinh }, { 66, fft_Cosh }, { 72, fft_Tanh }, { 78, fft_Exp }, { 84, fft_Log },
    { 90, fft_Pow }, { 96, fft_Sqrt }, { 102, fft_Tieee }, { 108, fft_Fieee },
    { 114, fft_Asin }, { 120, fft_Acos }, { 126, fft_Log10 } };
static const struct M68KThunkEntry sb_tab[] = {
    { 30, sb_Fix }, { 36, sb_Flt }, { 42, sb_Cmp }, { 48, sb_Tst }, { 54, sb_Abs },
    { 60, sb_Neg }, { 66, sb_Add }, { 72, sb_Sub }, { 78, sb_Mul }, { 84, sb_Div },
    { 90, sb_Floor }, { 96, sb_Ceil } };
static const struct M68KThunkEntry st_tab[] = {
    { 30, st_Atan }, { 36, st_Sin }, { 42, st_Cos }, { 48, st_Tan }, { 54, st_Sincos },
    { 60, st_Sinh }, { 66, st_Cosh }, { 72, st_Tanh }, { 78, st_Exp }, { 84, st_Log },
    { 90, st_Pow }, { 96, st_Sqrt }, { 102, st_Tieee }, { 108, st_Fieee },
    { 114, st_Asin }, { 120, st_Acos }, { 126, st_Log10 } };
static const struct M68KThunkEntry db_tab[] = {
    { 30, db_Fix }, { 36, db_Flt }, { 42, db_Cmp }, { 48, db_Tst }, { 54, db_Abs },
    { 60, db_Neg }, { 66, db_Add }, { 72, db_Sub }, { 78, db_Mul }, { 84, db_Div },
    { 90, db_Floor }, { 96, db_Ceil } };
static const struct M68KThunkEntry dt_tab[] = {
    { 30, dt_Atan }, { 36, dt_Sin }, { 42, dt_Cos }, { 48, dt_Tan }, { 54, dt_Sincos },
    { 60, dt_Sinh }, { 66, dt_Cosh }, { 72, dt_Tanh }, { 78, dt_Exp }, { 84, dt_Log },
    { 90, dt_Pow }, { 96, dt_Sqrt }, { 102, dt_Tieee }, { 108, dt_Fieee },
    { 114, dt_Asin }, { 120, dt_Acos }, { 126, dt_Log10 } };

#define N(t) (sizeof(t) / sizeof(t[0]))
const struct M68KLibThunkSet m68kemu_math_libs[] = {
    { "mathffp.library",           ffp_tab, N(ffp_tab) },
    { "mathtrans.library",         fft_tab, N(fft_tab) },
    { "mathieeesingbas.library",   sb_tab,  N(sb_tab) },
    { "mathieeesingtrans.library", st_tab,  N(st_tab) },
    { "mathieeedoubbas.library",   db_tab,  N(db_tab) },
    { "mathieeedoubtrans.library", dt_tab,  N(dt_tab) },
    { NULL, NULL, 0 }
};
const ULONG m68kemu_math_libs_count = 6;

/* ── workbench.library ───────────────────────────────────────────────
   The App* calls register a 68k message port for AppMessages, which the
   emulator cannot deliver yet: they succeed with a token so programs that
   offer an AppIcon or AppWindow as an extra still run, and nothing
   arrives on the port. Workbench objects open natively. */
#include <proto/workbench.h>

#define T(name) static IPTR name(struct M68KEmuContext *ctx, void *cpu)
T(wb_AddApp)   { static ULONG token = 0x57420000; (void)ctx; (void)cpu; return ++token; }
T(wb_True)     { (void)ctx; (void)cpu; return TRUE; }
T(wb_False)    { (void)ctx; (void)cpu; return FALSE; }
T(wb_OpenObj)
{
    ULONG name = M68KEmu_GetA(cpu, 0);
    return name ? (IPTR)OpenWorkbenchObjectA((STRPTR)m68k_to_host(ctx, name), NULL) : FALSE;
}
#undef T

static const struct M68KThunkEntry wb_tab[] = {
    { 30, wb_False },    /* UpdateWorkbench */
    { 48, wb_AddApp },   /* AddAppWindowA */
    { 54, wb_True },     /* RemoveAppWindow */
    { 60, wb_AddApp },   /* AddAppIconA */
    { 66, wb_True },     /* RemoveAppIcon */
    { 72, wb_AddApp },   /* AddAppMenuItemA */
    { 78, wb_True },     /* RemoveAppMenuItem */
    { 90, wb_False },    /* WBInfo */
    { 96, wb_OpenObj },  /* OpenWorkbenchObjectA */
    { 102, wb_False },   /* CloseWorkbenchObjectA */
    { 108, wb_False },   /* WorkbenchControlA */
    { 114, wb_AddApp },  /* AddAppWindowDropZoneA */
    { 120, wb_True },    /* RemoveAppWindowDropZone */
    { 126, wb_False },   /* ChangeWorkbenchSelectionA */
    { 132, wb_False },   /* MakeWorkbenchObjectVisibleA */
};

const struct M68KLibThunkSet m68kemu_wb_lib = { "workbench.library", wb_tab, N(wb_tab) };

/* ── commodities.library: the calls that take m68k-layout data ───────
   NewBroker and CreateCxObj's arguments carry 32-bit big-endian
   pointers; they are translated here. The rest stay generated. */
#include <libraries/commodities.h>
#include <proto/commodities.h>

/* thunks.c */
APTR shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);

static APTR cx_str(struct M68KEmuContext *ctx, ULONG a)
{
    return a ? (APTR)m68k_to_host(ctx, a) : NULL;
}

/* -36: CxBroker(A0=nb, D0=error) */
static IPTR cx_Broker(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG nb = M68KEmu_GetA(cpu, 0), errp = M68KEmu_GetD(cpu, 0);
    struct NewBroker n;
    LONG err = 0;
    CxObj *co;

    if (!nb) return 0;
    memset(&n, 0, sizeof(n));
    n.nb_Version = ctx->mem[nb + 0];
    n.nb_Name    = cx_str(ctx, m68k_read32(ctx, nb + 2));
    n.nb_Title   = cx_str(ctx, m68k_read32(ctx, nb + 6));
    n.nb_Descr   = cx_str(ctx, m68k_read32(ctx, nb + 10));
    n.nb_Unique  = (WORD)m68k_read16(ctx, nb + 14);
    n.nb_Flags   = (WORD)m68k_read16(ctx, nb + 16);
    n.nb_Pri     = (BYTE)ctx->mem[nb + 18];
    n.nb_Port    = (struct MsgPort *)shadow_lookup(ctx, m68k_read32(ctx, nb + 20));
    n.nb_ReservedChannel = (WORD)m68k_read16(ctx, nb + 24);
    co = CxBroker(&n, &err);
    if (errp && errp < ctx->mem_size) m68k_write32(ctx, errp, (ULONG)err);
    return (IPTR)co;           /* a CxObj is opaque to the program: the handle goes back as is */
}

/* -30: CreateCxObj(D0=type, A0=arg1, A1=arg2) */
static IPTR cx_Create(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG type = M68KEmu_GetD(cpu, 0), a1 = M68KEmu_GetA(cpu, 0), a2 = M68KEmu_GetA(cpu, 1);

    switch (type)
    {
    case CX_FILTER:     return (IPTR)CreateCxObj(type, (IPTR)cx_str(ctx, a1), a2);
    case CX_TYPEFILTER: return (IPTR)CreateCxObj(type, a1, a2);
    case CX_SEND:       return (IPTR)CreateCxObj(type, (IPTR)shadow_lookup(ctx, a1), a2);
    case CX_SIGNAL:     return (IPTR)CreateCxObj(type, (IPTR)FindTask(NULL), a2);   /* the program's task is this process */
    case CX_TRANSLATE:  return (IPTR)CreateCxObj(type, 0, 0);                        /* NULL translation: swallow */
    case CX_DEBUG:      return (IPTR)CreateCxObj(type, a1, 0);
    default:
        m68k_log(ctx, "%s: commodities object type %lu needs a 68k callback, not supported yet",
                 ctx->program_name[0] ? ctx->program_name : "68k program", (unsigned long)type);
        return 0;
    }
}

const struct M68KThunkEntry m68kemu_cx_manual[] = {
    { 30, cx_Create },
    { 36, cx_Broker },
};
const ULONG m68kemu_cx_manual_count = 2;
