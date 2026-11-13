/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: RISC-V disassembler for debug.library.

    A self-contained decoder for the instruction sets this port runs on
    (I, M, A, F, D, C, Zicsr, and the common Zba/Zbb operations),
    feeding exec's alert output. Anything it does not know is shown as
    a raw .insn word, which still disassembles offline.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <libraries/debug.h>

#include <proto/exec.h>
#include <proto/debug.h>

struct RVDisCtx
{
    struct Library *DebugBase;
    const UBYTE *start;
    const UBYTE *end;
    const UBYTE *pc;
    const UBYTE *cur;       /* Next instruction to decode     */
    const UBYTE *insn;      /* Last decoded instruction       */
    ULONG   insnlen;
    char    hexbuf[12];
    char    asmbuf[128];
};

static const char * const xreg[32] =
{
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "fp",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static const char * const freg[32] =
{
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
    "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
    "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
    "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

/* Small append helpers - the buffer is bounded by the context */

static char *putstr(char *p, char *end, const char *s)
{
    while (*s && p < end - 1)
        *p++ = *s++;
    *p = 0;
    return p;
}

static char *putdec(char *p, char *end, LONG val)
{
    char tmp[24];
    int i = 0;
    ULONG u;

    if (val < 0)
    {
        p = putstr(p, end, "-");
        u = (ULONG)-val;
    }
    else
        u = (ULONG)val;

    do
    {
        tmp[i++] = '0' + (u % 10);
        u /= 10;
    } while (u);
    while (i && p < end - 1)
        *p++ = tmp[--i];
    *p = 0;
    return p;
}

static char *puthex(char *p, char *end, UQUAD val)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[16];
    int i = 0;

    p = putstr(p, end, "0x");
    do
    {
        tmp[i++] = digits[val & 0xf];
        val >>= 4;
    } while (val);
    while (i && p < end - 1)
        *p++ = tmp[--i];
    *p = 0;
    return p;
}

/* "addr <symbol+offset>" for jump and branch targets */
static char *puttarget(struct RVDisCtx *ctx, char *p, char *end, IPTR addr)
{
    struct Library *DebugBase = ctx->DebugBase;
    const char *symname = NULL;
    void *symstart = NULL;
    struct TagItem tags[] =
    {
        { DL_SymbolName,  (IPTR)&symname  },
        { DL_SymbolStart, (IPTR)&symstart },
        { TAG_DONE,       0               }
    };

    p = puthex(p, end, addr);
    if (DecodeLocationA((APTR)addr, tags) && symname)
    {
        p = putstr(p, end, " <");
        p = putstr(p, end, symname);
        if (symstart && addr != (IPTR)symstart)
        {
            p = putstr(p, end, "+");
            p = puthex(p, end, addr - (IPTR)symstart);
        }
        p = putstr(p, end, ">");
    }
    return p;
}

static const char *csrname(ULONG csr)
{
    switch (csr)
    {
    case 0x100: return "sstatus";
    case 0x104: return "sie";
    case 0x105: return "stvec";
    case 0x140: return "sscratch";
    case 0x141: return "sepc";
    case 0x142: return "scause";
    case 0x143: return "stval";
    case 0x144: return "sip";
    case 0x180: return "satp";
    case 0xc00: return "cycle";
    case 0xc01: return "time";
    case 0xc02: return "instret";
    case 0x001: return "fflags";
    case 0x002: return "frm";
    case 0x003: return "fcsr";
    }
    return NULL;
}

/* rd,rs1,rs2 */
static char *put_r(char *p, char *end, const char *op, ULONG rd, ULONG rs1,
                   ULONG rs2)
{
    p = putstr(p, end, op);
    p = putstr(p, end, " ");
    p = putstr(p, end, xreg[rd]);
    p = putstr(p, end, ",");
    p = putstr(p, end, xreg[rs1]);
    p = putstr(p, end, ",");
    p = putstr(p, end, xreg[rs2]);
    return p;
}

/* rd,rs1,imm */
static char *put_i(char *p, char *end, const char *op, ULONG rd, ULONG rs1,
                   LONG imm)
{
    p = putstr(p, end, op);
    p = putstr(p, end, " ");
    p = putstr(p, end, xreg[rd]);
    p = putstr(p, end, ",");
    p = putstr(p, end, xreg[rs1]);
    p = putstr(p, end, ",");
    p = putdec(p, end, imm);
    return p;
}

/* reg,imm(base) */
static char *put_mem(char *p, char *end, const char *op, const char *reg,
                     LONG imm, ULONG base)
{
    p = putstr(p, end, op);
    p = putstr(p, end, " ");
    p = putstr(p, end, reg);
    p = putstr(p, end, ",");
    p = putdec(p, end, imm);
    p = putstr(p, end, "(");
    p = putstr(p, end, xreg[base]);
    p = putstr(p, end, ")");
    return p;
}

static LONG sext(ULONG val, int bits)
{
    ULONG m = 1UL << (bits - 1);

    return (LONG)((val ^ m) - m);
}

/* The 32-bit encodings */
static void rvDecode32(struct RVDisCtx *ctx, ULONG in)
{
    char *p = ctx->asmbuf;
    char *end = ctx->asmbuf + sizeof(ctx->asmbuf);
    ULONG opcode = in & 0x7f;
    ULONG rd = (in >> 7) & 0x1f;
    ULONG f3 = (in >> 12) & 7;
    ULONG rs1 = (in >> 15) & 0x1f;
    ULONG rs2 = (in >> 20) & 0x1f;
    ULONG f7 = (in >> 25) & 0x7f;
    LONG imm_i = sext(in >> 20, 12);
    LONG imm_s = sext(((in >> 25) << 5) | ((in >> 7) & 0x1f), 12);

    ctx->asmbuf[0] = 0;

    switch (opcode)
    {
    case 0x37:  /* LUI */
        p = putstr(p, end, "lui ");
        p = putstr(p, end, xreg[rd]);
        p = putstr(p, end, ",");
        p = puthex(p, end, in >> 12);
        break;

    case 0x17:  /* AUIPC */
        p = putstr(p, end, "auipc ");
        p = putstr(p, end, xreg[rd]);
        p = putstr(p, end, ",");
        p = puthex(p, end, in >> 12);
        break;

    case 0x6f:  /* JAL */
    {
        LONG off = sext((((in >> 31) & 1) << 20) | (((in >> 12) & 0xff) << 12) |
                        (((in >> 20) & 1) << 11) | (((in >> 21) & 0x3ff) << 1),
                        21);

        p = putstr(p, end, rd == 0 ? "j " : (rd == 1 ? "jal " : "jal ?, "));
        if (rd > 1)
        {
            p = ctx->asmbuf;
            p = putstr(p, end, "jal ");
            p = putstr(p, end, xreg[rd]);
            p = putstr(p, end, ",");
        }
        p = puttarget(ctx, p, end, (IPTR)ctx->insn + off);
        break;
    }

    case 0x67:  /* JALR */
        if (in == 0x00008067)
            p = putstr(p, end, "ret");
        else if (rd <= 1 && imm_i == 0)
        {
            p = putstr(p, end, rd ? "jalr " : "jr ");
            p = putstr(p, end, xreg[rs1]);
        }
        else
            p = put_mem(p, end, "jalr", xreg[rd], imm_i, rs1);
        break;

    case 0x63:  /* branches */
    {
        static const char * const br[8] =
            { "beq", "bne", NULL, NULL, "blt", "bge", "bltu", "bgeu" };
        LONG off = sext((((in >> 31) & 1) << 12) | (((in >> 7) & 1) << 11) |
                        (((in >> 25) & 0x3f) << 5) | (((in >> 8) & 0xf) << 1),
                        13);

        if (!br[f3])
            goto unknown;
        p = putstr(p, end, br[f3]);
        p = putstr(p, end, " ");
        p = putstr(p, end, xreg[rs1]);
        p = putstr(p, end, ",");
        p = putstr(p, end, xreg[rs2]);
        p = putstr(p, end, ",");
        p = puttarget(ctx, p, end, (IPTR)ctx->insn + off);
        break;
    }

    case 0x03:  /* loads */
    {
        static const char * const ld[8] =
            { "lb", "lh", "lw", "ld", "lbu", "lhu", "lwu", NULL };

        if (!ld[f3])
            goto unknown;
        p = put_mem(p, end, ld[f3], xreg[rd], imm_i, rs1);
        break;
    }

    case 0x23:  /* stores */
    {
        static const char * const st[8] =
            { "sb", "sh", "sw", "sd", NULL, NULL, NULL, NULL };

        if (!st[f3])
            goto unknown;
        p = put_mem(p, end, st[f3], xreg[rs2], imm_s, rs1);
        break;
    }

    case 0x07:  /* FP loads */
        if (f3 != 2 && f3 != 3)
            goto unknown;
        p = put_mem(p, end, f3 == 2 ? "flw" : "fld", freg[rd], imm_i, rs1);
        break;

    case 0x27:  /* FP stores */
        if (f3 != 2 && f3 != 3)
            goto unknown;
        p = put_mem(p, end, f3 == 2 ? "fsw" : "fsd", freg[rs2], imm_s, rs1);
        break;

    case 0x13:  /* OP-IMM */
        switch (f3)
        {
        case 0:
            if (in == 0x00000013)
                p = putstr(p, end, "nop");
            else if (rs1 == 0)
            {
                p = putstr(p, end, "li ");
                p = putstr(p, end, xreg[rd]);
                p = putstr(p, end, ",");
                p = putdec(p, end, imm_i);
            }
            else if (imm_i == 0)
            {
                p = putstr(p, end, "mv ");
                p = putstr(p, end, xreg[rd]);
                p = putstr(p, end, ",");
                p = putstr(p, end, xreg[rs1]);
            }
            else
                p = put_i(p, end, "addi", rd, rs1, imm_i);
            break;
        case 1:
            if ((in >> 26) == 0)
                p = put_i(p, end, "slli", rd, rs1, (in >> 20) & 0x3f);
            else if ((in >> 26) == 0x1a && rs2 == 0)
                p = put_i(p, end, "clz", rd, rs1, 0);
            else if ((in >> 26) == 0x1a && rs2 == 1)
                p = put_i(p, end, "ctz", rd, rs1, 0);
            else if ((in >> 26) == 0x1a && rs2 == 2)
                p = put_i(p, end, "cpop", rd, rs1, 0);
            else
                goto unknown;
            break;
        case 2: p = put_i(p, end, "slti", rd, rs1, imm_i); break;
        case 3: p = put_i(p, end, "sltiu", rd, rs1, imm_i); break;
        case 4: p = put_i(p, end, "xori", rd, rs1, imm_i); break;
        case 5:
            if ((in >> 26) == 0)
                p = put_i(p, end, "srli", rd, rs1, (in >> 20) & 0x3f);
            else if ((in >> 26) == 0x10)
                p = put_i(p, end, "srai", rd, rs1, (in >> 20) & 0x3f);
            else if ((in >> 26) == 0x18)
                p = put_i(p, end, "rori", rd, rs1, (in >> 20) & 0x3f);
            else if (in == 0x6b855013 || ((in >> 20) == 0x698))
                p = put_i(p, end, "rev8", rd, rs1, 0);
            else if ((in >> 20) == 0x287)
                p = put_i(p, end, "orc.b", rd, rs1, 0);
            else
                goto unknown;
            break;
        case 6: p = put_i(p, end, "ori", rd, rs1, imm_i); break;
        case 7: p = put_i(p, end, "andi", rd, rs1, imm_i); break;
        }
        break;

    case 0x1b:  /* OP-IMM-32 */
        switch (f3)
        {
        case 0:
            if (imm_i == 0)
            {
                p = putstr(p, end, "sext.w ");
                p = putstr(p, end, xreg[rd]);
                p = putstr(p, end, ",");
                p = putstr(p, end, xreg[rs1]);
            }
            else
                p = put_i(p, end, "addiw", rd, rs1, imm_i);
            break;
        case 1:
            if (f7 == 0x02)
                p = put_i(p, end, "slli.uw", rd, rs1, (in >> 20) & 0x3f);
            else
                p = put_i(p, end, "slliw", rd, rs1, rs2);
            break;
        case 5:
            p = put_i(p, end, f7 == 0x20 ? "sraiw" : "srliw", rd, rs1, rs2);
            break;
        default:
            goto unknown;
        }
        break;

    case 0x33:  /* OP */
    {
        static const char * const base0[8] =
            { "add", "sll", "slt", "sltu", "xor", "srl", "or", "and" };
        static const char * const muldiv[8] =
            { "mul", "mulh", "mulhsu", "mulhu", "div", "divu", "rem", "remu" };

        if (f7 == 0x00)
            p = put_r(p, end, base0[f3], rd, rs1, rs2);
        else if (f7 == 0x01)
            p = put_r(p, end, muldiv[f3], rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 0)
            p = put_r(p, end, "sub", rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 5)
            p = put_r(p, end, "sra", rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 4)
            p = put_r(p, end, "xnor", rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 6)
            p = put_r(p, end, "orn", rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 7)
            p = put_r(p, end, "andn", rd, rs1, rs2);
        else if (f7 == 0x10 && f3 == 2)
            p = put_r(p, end, "sh1add", rd, rs1, rs2);
        else if (f7 == 0x10 && f3 == 4)
            p = put_r(p, end, "sh2add", rd, rs1, rs2);
        else if (f7 == 0x10 && f3 == 6)
            p = put_r(p, end, "sh3add", rd, rs1, rs2);
        else if (f7 == 0x05 && f3 == 4)
            p = put_r(p, end, "min", rd, rs1, rs2);
        else if (f7 == 0x05 && f3 == 5)
            p = put_r(p, end, "minu", rd, rs1, rs2);
        else if (f7 == 0x05 && f3 == 6)
            p = put_r(p, end, "max", rd, rs1, rs2);
        else if (f7 == 0x05 && f3 == 7)
            p = put_r(p, end, "maxu", rd, rs1, rs2);
        else if (f7 == 0x30 && f3 == 1)
            p = put_r(p, end, "rol", rd, rs1, rs2);
        else if (f7 == 0x30 && f3 == 5)
            p = put_r(p, end, "ror", rd, rs1, rs2);
        else
            goto unknown;
        break;
    }

    case 0x3b:  /* OP-32 */
    {
        static const char * const mul32[8] =
            { "mulw", NULL, NULL, NULL, "divw", "divuw", "remw", "remuw" };

        if (f7 == 0x00 && f3 == 0)
            p = put_r(p, end, "addw", rd, rs1, rs2);
        else if (f7 == 0x00 && f3 == 1)
            p = put_r(p, end, "sllw", rd, rs1, rs2);
        else if (f7 == 0x00 && f3 == 5)
            p = put_r(p, end, "srlw", rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 0)
            p = put_r(p, end, "subw", rd, rs1, rs2);
        else if (f7 == 0x20 && f3 == 5)
            p = put_r(p, end, "sraw", rd, rs1, rs2);
        else if (f7 == 0x01 && mul32[f3])
            p = put_r(p, end, mul32[f3], rd, rs1, rs2);
        else if (f7 == 0x04 && f3 == 0)
        {
            if (rs2 == 0)
            {
                p = putstr(p, end, "zext.w ");
                p = putstr(p, end, xreg[rd]);
                p = putstr(p, end, ",");
                p = putstr(p, end, xreg[rs1]);
            }
            else
                p = put_r(p, end, "add.uw", rd, rs1, rs2);
        }
        else if (f7 == 0x10 && f3 == 2)
            p = put_r(p, end, "sh1add.uw", rd, rs1, rs2);
        else if (f7 == 0x10 && f3 == 4)
            p = put_r(p, end, "sh2add.uw", rd, rs1, rs2);
        else if (f7 == 0x10 && f3 == 6)
            p = put_r(p, end, "sh3add.uw", rd, rs1, rs2);
        else if (f7 == 0x30 && f3 == 1)
            p = put_r(p, end, "rolw", rd, rs1, rs2);
        else if (f7 == 0x30 && f3 == 5)
            p = put_r(p, end, "rorw", rd, rs1, rs2);
        else
            goto unknown;
        break;
    }

    case 0x0f:
        p = putstr(p, end, f3 == 1 ? "fence.i" : "fence");
        break;

    case 0x73:  /* SYSTEM */
        if (in == 0x00000073)
            p = putstr(p, end, "ecall");
        else if (in == 0x00100073)
            p = putstr(p, end, "ebreak");
        else if (in == 0x10200073)
            p = putstr(p, end, "sret");
        else if (in == 0x10500073)
            p = putstr(p, end, "wfi");
        else if (f3 && f3 != 4)
        {
            static const char * const csrops[8] =
                { NULL, "csrrw", "csrrs", "csrrc",
                  NULL, "csrrwi", "csrrsi", "csrrci" };
            const char *csr = csrname(in >> 20);

            p = putstr(p, end, csrops[f3]);
            p = putstr(p, end, " ");
            p = putstr(p, end, xreg[rd]);
            p = putstr(p, end, ",");
            if (csr)
                p = putstr(p, end, csr);
            else
                p = puthex(p, end, in >> 20);
            p = putstr(p, end, ",");
            if (f3 >= 5)
                p = putdec(p, end, rs1);
            else
                p = putstr(p, end, xreg[rs1]);
        }
        else
            goto unknown;
        break;

    case 0x2f:  /* AMO */
    {
        static const char * const amo[32] =
        {
            [0x00] = "amoadd", [0x01] = "amoswap", [0x02] = "lr",
            [0x03] = "sc",     [0x04] = "amoxor",  [0x08] = "amoor",
            [0x0c] = "amoand", [0x10] = "amomin",  [0x14] = "amomax",
            [0x18] = "amominu",[0x1c] = "amomaxu"
        };
        const char *op = (f3 == 2 || f3 == 3) ? amo[f7 >> 2] : NULL;

        if (!op)
            goto unknown;
        p = putstr(p, end, op);
        p = putstr(p, end, f3 == 2 ? ".w " : ".d ");
        p = putstr(p, end, xreg[rd]);
        p = putstr(p, end, ",");
        if ((f7 >> 2) != 0x02)  /* lr has no rs2 */
        {
            p = putstr(p, end, xreg[rs2]);
            p = putstr(p, end, ",");
        }
        p = putstr(p, end, "(");
        p = putstr(p, end, xreg[rs1]);
        p = putstr(p, end, ")");
        break;
    }

    case 0x53:  /* FP OP */
    {
        const char *sz = ((in >> 25) & 3) ? ".d" : ".s";
        ULONG op5 = in >> 27;
        static const char * const fops[4] =
            { "fadd", "fsub", "fmul", "fdiv" };

        if (op5 < 4)
        {
            p = putstr(p, end, fops[op5]);
            p = putstr(p, end, sz);
            p = putstr(p, end, " ");
            p = putstr(p, end, freg[rd]);
            p = putstr(p, end, ",");
            p = putstr(p, end, freg[rs1]);
            p = putstr(p, end, ",");
            p = putstr(p, end, freg[rs2]);
        }
        else
        {
            /* The rest matters less for crash reading; name the class */
            p = putstr(p, end, "f-op");
            p = putstr(p, end, sz);
            p = putstr(p, end, " (");
            p = puthex(p, end, in);
            p = putstr(p, end, ")");
        }
        break;
    }

    case 0x43: case 0x47: case 0x4b: case 0x4f:
    {
        static const char * const fma[4] =
            { "fmadd", "fmsub", "fnmsub", "fnmadd" };

        p = putstr(p, end, fma[(opcode >> 2) & 3]);
        p = putstr(p, end, ((in >> 25) & 3) ? ".d" : ".s");
        break;
    }

    default:
    unknown:
        p = putstr(p, end, ".insn ");
        p = puthex(p, end, in);
        break;
    }
}

/* The compressed (16-bit) encodings */
static void rvDecode16(struct RVDisCtx *ctx, ULONG in)
{
    char *p = ctx->asmbuf;
    char *end = ctx->asmbuf + sizeof(ctx->asmbuf);
    ULONG q = in & 3;
    ULONG f3 = (in >> 13) & 7;
    ULONG rd = (in >> 7) & 0x1f;
    ULONG rs2 = (in >> 2) & 0x1f;
    ULONG rdp = 8 + ((in >> 2) & 7);    /* rd'/rs2'  */
    ULONG rs1p = 8 + ((in >> 7) & 7);   /* rs1'/rd'  */

    ctx->asmbuf[0] = 0;

    if (q == 0)
    {
        switch (f3)
        {
        case 0:
        {
            ULONG imm = (((in >> 7) & 0xf) << 6) | (((in >> 11) & 3) << 4) |
                        (((in >> 5) & 1) << 3) | (((in >> 6) & 1) << 2);

            if (!imm)
                break;
            p = putstr(p, end, "c.addi4spn ");
            p = putstr(p, end, xreg[rdp]);
            p = putstr(p, end, ",sp,");
            p = putdec(p, end, imm);
            return;
        }
        case 1:
            p = put_mem(p, end, "c.fld", freg[rdp],
                        (((in >> 10) & 7) << 3) | (((in >> 5) & 3) << 6), rs1p);
            return;
        case 2:
            p = put_mem(p, end, "c.lw", xreg[rdp],
                        (((in >> 10) & 7) << 3) | (((in >> 6) & 1) << 2) |
                        (((in >> 5) & 1) << 6), rs1p);
            return;
        case 3:
            p = put_mem(p, end, "c.ld", xreg[rdp],
                        (((in >> 10) & 7) << 3) | (((in >> 5) & 3) << 6), rs1p);
            return;
        case 5:
            p = put_mem(p, end, "c.fsd", freg[rdp],
                        (((in >> 10) & 7) << 3) | (((in >> 5) & 3) << 6), rs1p);
            return;
        case 6:
            p = put_mem(p, end, "c.sw", xreg[rdp],
                        (((in >> 10) & 7) << 3) | (((in >> 6) & 1) << 2) |
                        (((in >> 5) & 1) << 6), rs1p);
            return;
        case 7:
            p = put_mem(p, end, "c.sd", xreg[rdp],
                        (((in >> 10) & 7) << 3) | (((in >> 5) & 3) << 6), rs1p);
            return;
        }
    }
    else if (q == 1)
    {
        LONG imm6 = sext((((in >> 12) & 1) << 5) | ((in >> 2) & 0x1f), 6);

        switch (f3)
        {
        case 0:
            if (in == 0x0001)
                p = putstr(p, end, "c.nop");
            else
                p = put_i(p, end, "c.addi", rd, rd, imm6);
            return;
        case 1:
            p = put_i(p, end, "c.addiw", rd, rd, imm6);
            return;
        case 2:
            p = putstr(p, end, "c.li ");
            p = putstr(p, end, xreg[rd]);
            p = putstr(p, end, ",");
            p = putdec(p, end, imm6);
            return;
        case 3:
            if (rd == 2)
            {
                LONG imm = sext((((in >> 12) & 1) << 9) |
                                (((in >> 3) & 3) << 7) |
                                (((in >> 5) & 1) << 6) |
                                (((in >> 2) & 1) << 5) |
                                (((in >> 6) & 1) << 4), 10);

                p = putstr(p, end, "c.addi16sp sp,");
                p = putdec(p, end, imm);
            }
            else
            {
                p = putstr(p, end, "c.lui ");
                p = putstr(p, end, xreg[rd]);
                p = putstr(p, end, ",");
                p = puthex(p, end, (ULONG)imm6 & 0xfffff);
            }
            return;
        case 4:
        {
            ULONG sub = (in >> 10) & 3;

            if (sub == 0)
                p = put_i(p, end, "c.srli", rs1p, rs1p,
                          (((in >> 12) & 1) << 5) | ((in >> 2) & 0x1f));
            else if (sub == 1)
                p = put_i(p, end, "c.srai", rs1p, rs1p,
                          (((in >> 12) & 1) << 5) | ((in >> 2) & 0x1f));
            else if (sub == 2)
                p = put_i(p, end, "c.andi", rs1p, rs1p, imm6);
            else
            {
                static const char * const alu[8] =
                    { "c.sub", "c.xor", "c.or", "c.and",
                      "c.subw", "c.addw", NULL, NULL };
                ULONG op = ((in >> 5) & 3) | (((in >> 12) & 1) << 2);

                if (!alu[op])
                    break;
                p = putstr(p, end, alu[op]);
                p = putstr(p, end, " ");
                p = putstr(p, end, xreg[rs1p]);
                p = putstr(p, end, ",");
                p = putstr(p, end, xreg[rdp]);
            }
            return;
        }
        case 5:
        {
            LONG off = sext((((in >> 12) & 1) << 11) | (((in >> 8) & 1) << 10) |
                            (((in >> 9) & 3) << 8) | (((in >> 6) & 1) << 7) |
                            (((in >> 7) & 1) << 6) | (((in >> 2) & 1) << 5) |
                            (((in >> 11) & 1) << 4) | (((in >> 3) & 7) << 1),
                            12);

            p = putstr(p, end, "c.j ");
            p = puttarget(ctx, p, end, (IPTR)ctx->insn + off);
            return;
        }
        case 6:
        case 7:
        {
            LONG off = sext((((in >> 12) & 1) << 8) | (((in >> 5) & 3) << 6) |
                            (((in >> 2) & 1) << 5) | (((in >> 10) & 3) << 3) |
                            (((in >> 3) & 3) << 1), 9);

            p = putstr(p, end, f3 == 6 ? "c.beqz " : "c.bnez ");
            p = putstr(p, end, xreg[rs1p]);
            p = putstr(p, end, ",");
            p = puttarget(ctx, p, end, (IPTR)ctx->insn + off);
            return;
        }
        }
    }
    else if (q == 2)
    {
        switch (f3)
        {
        case 0:
            p = put_i(p, end, "c.slli", rd, rd,
                      (((in >> 12) & 1) << 5) | ((in >> 2) & 0x1f));
            return;
        case 1:
            p = put_mem(p, end, "c.fldsp", freg[rd],
                        (((in >> 12) & 1) << 5) | (((in >> 5) & 3) << 3) |
                        (((in >> 2) & 7) << 6), 2);
            return;
        case 2:
            p = put_mem(p, end, "c.lwsp", xreg[rd],
                        (((in >> 12) & 1) << 5) | (((in >> 4) & 7) << 2) |
                        (((in >> 2) & 3) << 6), 2);
            return;
        case 3:
            p = put_mem(p, end, "c.ldsp", xreg[rd],
                        (((in >> 12) & 1) << 5) | (((in >> 5) & 3) << 3) |
                        (((in >> 2) & 7) << 6), 2);
            return;
        case 4:
            if (!((in >> 12) & 1))
            {
                if (rs2 == 0)
                {
                    p = putstr(p, end, rd == 1 ? "c.jr ra (ret)" : "c.jr ");
                    if (rd != 1)
                        p = putstr(p, end, xreg[rd]);
                }
                else
                {
                    p = putstr(p, end, "c.mv ");
                    p = putstr(p, end, xreg[rd]);
                    p = putstr(p, end, ",");
                    p = putstr(p, end, xreg[rs2]);
                }
            }
            else
            {
                if (rd == 0 && rs2 == 0)
                    p = putstr(p, end, "c.ebreak");
                else if (rs2 == 0)
                {
                    p = putstr(p, end, "c.jalr ");
                    p = putstr(p, end, xreg[rd]);
                }
                else
                {
                    p = putstr(p, end, "c.add ");
                    p = putstr(p, end, xreg[rd]);
                    p = putstr(p, end, ",");
                    p = putstr(p, end, xreg[rs2]);
                }
            }
            return;
        case 5:
            p = put_mem(p, end, "c.fsdsp", freg[rs2],
                        (((in >> 10) & 7) << 3) | (((in >> 7) & 7) << 6), 2);
            return;
        case 6:
            p = put_mem(p, end, "c.swsp", xreg[rs2],
                        (((in >> 9) & 0xf) << 2) | (((in >> 7) & 3) << 6), 2);
            return;
        case 7:
            p = put_mem(p, end, "c.sdsp", xreg[rs2],
                        (((in >> 10) & 7) << 3) | (((in >> 7) & 7) << 6), 2);
            return;
        }
    }

    p = putstr(p, end, ".insn ");
    p = puthex(p, end, in & 0xffff);
}

static void rvHex(struct RVDisCtx *ctx, ULONG in)
{
    static const char digits[] = "0123456789abcdef";
    int n = (ctx->insnlen == 4) ? 8 : 4;
    int i;

    for (i = 0; i < n; i++)
        ctx->hexbuf[i] = digits[(in >> ((n - 1 - i) * 4)) & 0xf];
    ctx->hexbuf[n] = 0;
}

AROS_LH3(APTR, InitDisassembleCtx,
        AROS_LHA(APTR, start, A0),
        AROS_LHA(APTR, end, A1),
        AROS_LHA(APTR, pc, A2),
        struct Library *, DebugBase, 9, Debug)
{
    AROS_LIBFUNC_INIT

    struct RVDisCtx *ctx = AllocVec(sizeof(struct RVDisCtx), MEMF_CLEAR);

    if (ctx)
    {
        /* Instructions are 2-byte aligned at minimum */
        ctx->DebugBase = DebugBase;
        ctx->start = (const UBYTE *)((IPTR)start & ~(IPTR)1);
        ctx->end = end;
        ctx->pc = pc;
        ctx->cur = ctx->start;
    }
    return ctx;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(IPTR, DisassembleCtx,
        AROS_LHA(APTR, handle, A0),
        struct Library *, DebugBase, 10, Debug)
{
    AROS_LIBFUNC_INIT

    struct RVDisCtx *ctx = handle;
    ULONG in;

    if (!ctx || ctx->cur >= ctx->end)
        return 0;

    ctx->insn = ctx->cur;
    in = ctx->cur[0] | (ctx->cur[1] << 8);
    if ((in & 3) == 3)
    {
        if (ctx->cur + 4 > ctx->end)
            return 0;
        in |= (ctx->cur[2] << 16) | (ctx->cur[3] << 24);
        ctx->insnlen = 4;
        rvHex(ctx, in);
        rvDecode32(ctx, in);
    }
    else
    {
        ctx->insnlen = 2;
        rvHex(ctx, in);
        rvDecode16(ctx, in);
    }

    ctx->cur += ctx->insnlen;
    return ctx->insnlen;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(IPTR, GetCtxInstructionA,
        AROS_LHA(APTR, handle, A0),
        AROS_LHA(struct TagItem *, tags, A1),
        struct Library *, DebugBase, 11, Debug)
{
    AROS_LIBFUNC_INIT

    struct RVDisCtx *ctx = handle;
    struct TagItem *tag, *tstate = tags;
    IPTR retval = 0;

    if (!ctx || !ctx->insn)
        return 0;

    while ((tag = LibNextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case DCIT_Instruction_Offset:
            tag->ti_Data = (IPTR)(ctx->insn - ctx->pc);
            retval++;
            break;

        case DCIT_Instruction_HexStr:
            tag->ti_Data = (IPTR)ctx->hexbuf;
            retval++;
            break;

        case DCIT_Instruction_Asm:
            tag->ti_Data = (IPTR)ctx->asmbuf;
            retval++;
            break;
        }
    }
    return retval;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeDisassembleCtx,
        AROS_LHA(APTR, handle, A0),
        struct Library *, DebugBase, 12, Debug)
{
    AROS_LIBFUNC_INIT

    FreeVec(handle);

    AROS_LIBFUNC_EXIT
}
