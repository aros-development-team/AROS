/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)
    m68kemu.library — Moira CPU subclass
*/
#include <aros/debug.h>
#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"
#include "m68kemu_thunks.h"
#include "Moira/Moira.h"

using namespace moira;

#define SENTINEL_ADDR 0x00DEAD00u
#define RTE_ADDR      0x000100u   /* address of our RTE instruction */
#define SV_TRAMP_ADDR 0x000108u   /* supervisor trampoline: skip frame + JMP (A5) */

class AROSMoira : public Moira
{
public:
    struct M68KEmuContext *ctx;

    explicit AROSMoira(struct M68KEmuContext *c) : ctx(c) {}

    /* ── Custom chip register emulation ──────────────────────────── */

    /* Advance beam position by 'ticks' color clocks (PAL: 313 lines × 227 clocks) */
    void beamAdvance(int ticks)
    {
        ctx->beam_h += ticks;
        while (ctx->beam_h >= PAL_CLOCKS_PER_LINE)
        {
            ctx->beam_h -= PAL_CLOCKS_PER_LINE;
            ctx->beam_v++;
            if (ctx->beam_v >= PAL_LINES_PER_FRAME)
            {
                ctx->beam_v = 0;
                ctx->beam_lof ^= 1;
            }
        }
    }

    /* Read custom chip register (word-aligned addr relative to DFF000) */
    u16 customRead16(u16 reg) const
    {
        const_cast<AROSMoira*>(this)->beamAdvance(2);
        switch (reg)
        {
            case 0x004: /* VPOSR */
                return ((u16)ctx->beam_lof << 15) |
                       ((ctx->beam_v >> 8) & 1);  /* PAL Agnus ID = 0x00 */
            case 0x006: /* VHPOSR */
                return ((u16)(ctx->beam_v & 0xFF) << 8) | ctx->beam_h;
            case 0x010: /* ADKCONR */
                return 0;
            case 0x016: /* POTGOR/POTINP */
                return POTGOR_DEFAULT; /* no buttons pressed */
            case 0x01C: /* INTREQR */
                return 0;
            case 0x01E: /* INTENA */
                return 0;
            case 0x002: /* DMACONR */
                return 0;
            default:
                return 0;
        }
    }

    void customWrite16(u16 reg, u16 val) const
    {
        switch (reg)
        {
            case 0x02A: /* VPOSW */
                const_cast<AROSMoira*>(this)->ctx->beam_lof = (val >> 15) & 1;
                const_cast<AROSMoira*>(this)->ctx->beam_v =
                    (ctx->beam_v & 0xFF) | ((val & 1) << 8);
                break;
            case 0x02C: /* VHPOSW */
                const_cast<AROSMoira*>(this)->ctx->beam_v =
                    (ctx->beam_v & 0x100) | (val >> 8);
                const_cast<AROSMoira*>(this)->ctx->beam_h = val & 0xFF;
                break;
            default:
                break; /* ignore writes to unimplemented registers */
        }
    }

    /*
     * Supervisor() redirect: push an exception frame for the caller's
     * return, then arrange for Moira's LINEA exception to land at the
     * user function via a trampoline that preserves supervisor mode.
     *
     * After willExecute returns, Moira will:
     *   1. Capture old SR (already done before willExecute)
     *   2. Enter supervisor mode
     *   3. Push exception frame (old_SR, PC-2) onto SSP
     *   4. Jump to LINEA vector
     *
     * We redirect the LINEA vector to SV_TRAMP_ADDR which skips
     * Moira's frame and JMPs to (A5) = userFunc, staying in
     * supervisor mode. The user function's RTE pops our manually
     * pushed frame, returning to the original caller.
     */
    void dispatchSupervisor(u32 ret_addr)
    {
        ULONG userFunc = ctx->sv_redirect;
        ctx->sv_redirect = 0;

        /* Push exception frame for the user function's RTE:
           68020 format: vector_offset(word) + PC(long) + SR(word) = 8 bytes
           68000 format: PC(long) + SR(word) = 6 bytes */
        /* The frame's SR is the caller's own: the program runs in supervisor
           mode here from reset, and an RTE into user mode would hand it the
           never-initialised user stack pointer. */
        u32 sp = getA(7);
        sp -= 2; m68k_write16(ctx, sp, 0x0000);  /* vector offset (68020) */
        sp -= 4; m68k_write32(ctx, sp, ret_addr); /* return PC */
        sp -= 2; m68k_write16(ctx, sp, getSR() & 0xFFFF); /* SR: the caller's */
        setA(7, sp);

        /* Redirect LINEA vector to supervisor trampoline */
        m68k_write32(ctx, M68K_VEC_LINEA, SV_TRAMP_ADDR);

        /* Set PC so Moira's exception frame points to userFunc.
           68020 pushes reg.pc0, 68000 pushes reg.pc-2. */
        setPC0(userFunc);
        setPC(userFunc + 2);

        if (ctx->trace_calls) bug("[m68kemu] Supervisor: dispatching to 0x%lx, ret=0x%lx\n",
            (unsigned long)userFunc, (unsigned long)ret_addr);
    }

    void ciaTrace(const char *what, u32 addr, u32 val) const
    {
        if (ctx->trace_calls && ctx->cia_trace < 24)
        {
            const_cast<AROSMoira*>(this)->ctx->cia_trace++;
            bug("[m68kemu] CIA %s %08x val %02x at PC %08x\n", what, (unsigned)addr, (unsigned)val, (unsigned)getPC0());
        }
    }

    /* which CIA and register a byte access hits: A on odd $BFEx01, B on even $BFDx00 */
    bool ciaDecode(u32 addr, int &chip, int &reg) const
    {
        if ((addr & 0xFFF001) == 0xBFE001) { chip = 0; reg = (addr >> 8) & 15; return true; }
        if ((addr & 0xFFF001) == 0xBFD000) { chip = 1; reg = (addr >> 8) & 15; return true; }
        return false;
    }

    u8 ciaRead(int chip, int reg) const
    {
        ULONG freq = 709379;
        ULONG e = m68k_eclock_now(ctx, &freq);
        auto &c = ctx->cia[chip];
        switch (reg)
        {
            case 0:  return c.pra ? c.pra : 0xFF;            /* PRA: no buttons, no disk change */
            case 1:  return c.prb;
            case 2:  return c.ddra;
            case 3:  return c.ddrb;
            case 4: case 5:                                   /* timer A, counting down at E rate */
            {
                UWORD latch = c.ta_latch ? c.ta_latch : 0xFFFF;
                UWORD v = (UWORD)(latch - (e % (latch + 1u)));
                return reg == 4 ? (u8)v : (u8)(v >> 8);
            }
            case 6: case 7:                                   /* timer B */
            {
                UWORD latch = c.tb_latch ? c.tb_latch : 0xFFFF;
                UWORD v = (UWORD)(latch - (e % (latch + 1u)));
                return reg == 6 ? (u8)v : (u8)(v >> 8);
            }
            case 8: case 9: case 10:                          /* TOD: A at 50 Hz, B at horizontal rate */
            {
                ULONG rate = chip ? 15625 : 50;
                ULONG tod = (ULONG)(((unsigned long long)e * rate) / freq) + c.tod_base;
                tod &= 0xFFFFFF;
                return (u8)(tod >> ((reg - 8) * 8));
            }
            case 12: return 0;                                /* SDR */
            case 13: return 0;                                /* ICR: nothing pending */
            case 14: return c.cra;
            case 15: return c.crb;
            default: return 0;
        }
    }

    void ciaWrite(int chip, int reg, u8 val) const
    {
        auto &c = const_cast<AROSMoira*>(this)->ctx->cia[chip];
        switch (reg)
        {
            case 0:  c.pra = val; break;
            case 1:  c.prb = val; break;
            case 2:  c.ddra = val; break;
            case 3:  c.ddrb = val; break;
            case 4:  c.ta_latch = (UWORD)((c.ta_latch & 0xFF00) | val); break;
            case 5:  c.ta_latch = (UWORD)((c.ta_latch & 0x00FF) | (val << 8)); break;
            case 6:  c.tb_latch = (UWORD)((c.tb_latch & 0xFF00) | val); break;
            case 7:  c.tb_latch = (UWORD)((c.tb_latch & 0x00FF) | (val << 8)); break;
            case 14: c.cra = val; break;
            case 15: c.crb = val; break;
            default: break;                                   /* TOD writes, SDR, ICR mask: ignored */
        }
    }

    u8 read8(u32 addr) const override
    {
        int chip, reg;
        if (ciaDecode(addr, chip, reg))
        {
            u8 v = ciaRead(chip, reg);
            ciaTrace("read", addr, v);
            return v;
        }
        if ((addr & CUSTOM_MASK) == CUSTOM_BASE)
        {
            u16 w = customRead16(addr & 0xFFE);
            return (addr & 1) ? (u8)w : (u8)(w >> 8);
        }
        if (addr >= ctx->mem_size) return 0;
        return ctx->mem[addr];
    }

    u16 read16(u32 addr) const override
    {
        if (addr >= SENTINEL_ADDR && addr < SENTINEL_ADDR + 2)
        {
            const_cast<AROSMoira*>(this)->ctx->running = FALSE;
            return M68K_OP_STOP;
        }
        if ((addr & CUSTOM_MASK) == CUSTOM_BASE)
            return customRead16(addr & 0xFFE);
        if (addr + 1 >= ctx->mem_size) return 0;
        return ((u16)ctx->mem[addr] << 8) | ctx->mem[addr + 1];
    }

    void write8(u32 addr, u8 val) const override
    {
        int chip, reg;
        if (ciaDecode(addr, chip, reg))
        {
            ciaTrace("write", addr, val);
            ciaWrite(chip, reg, val);
            return;
        }
        if ((addr & CUSTOM_MASK) == CUSTOM_BASE) return;
        if (addr >= ctx->mem_size) return;
        if (ctx->low_guard && (addr < 8 || (addr >= 0x100 && addr < 0x180)))
        {
            if (ctx->low_trap_count++ < 8 && ctx->trace_calls)
                bug("[m68kemu] CPU wrote into the vector table at %08x from PC %08x (dropped)\n",
                    (unsigned)addr, (unsigned)getPC0());
            return;
        }
        ctx->mem[addr] = val;
        if (ctx->plane_base) m68k_planar_touch(ctx, addr, 1);
    }

    void write16(u32 addr, u16 val) const override
    {
        if ((addr & CUSTOM_MASK) == CUSTOM_BASE)
        {
            customWrite16(addr & 0xFFE, val);
            return;
        }
        if (addr + 1 >= ctx->mem_size) return;
        if (ctx->low_guard && (addr < 8 || (addr >= 0x100 && addr < 0x180)))
        {
            if (ctx->low_trap_count++ < 8 && ctx->trace_calls)
                bug("[m68kemu] CPU wrote into the vector table at %08x from PC %08x (dropped)\n",
                    (unsigned)addr, (unsigned)getPC0());
            return;
        }
        ctx->mem[addr]     = (u8)(val >> 8);
        ctx->mem[addr + 1] = (u8)(val);
        if (ctx->plane_base) m68k_planar_touch(ctx, addr, 2);
    }

    u16 read16OnReset(u32 addr) const override
    {
        if (addr + 1 >= ctx->mem_size) return 0;
        return ((u16)ctx->mem[addr] << 8) | ctx->mem[addr + 1];
    }

    u16 read16Dasm(u32 addr) const override
    {
        if (addr + 1 >= ctx->mem_size) return 0;
        return ((u16)ctx->mem[addr] << 8) | ctx->mem[addr + 1];
    }

    /*
     * willExecute is called BEFORE Moira processes the LINEA exception.
     * After we return, Moira will:
     *   1. Push SR and (reg.pc - 2) onto the m68k stack
     *   2. Jump to the LINEA exception vector (vector 10, address 0x28)
     *
     * The m68k code got here via JSR -offset(A6), which already pushed
     * the return address onto the stack. So the stack currently has:
     *   [SP] = return address from JSR
     *
     * Strategy:
     *   - Pop the JSR return address from the stack
     *   - Run the thunk, set D0
     *   - Set reg.pc0 = return_addr so Moira pushes return_addr
     *     in the 68020 exception frame (68000 uses reg.pc-2 instead,
     *     so we also set reg.pc = return_addr + 2 for compatibility)
     *   - LINEA vector points to an RTE instruction at RTE_ADDR
     *   - RTE pops the exception frame -> resumes at return_addr
     */
    void willExecute(M68kException exc, u16 vector) override
    {
        if (exc != M68kException::LINEA)
        {
            ULONG vec = m68k_read32(ctx, (ULONG)vector * 4);

            ctx->last_exc = (ULONG)exc;
            ctx->last_exc_pc = getPC0();

            /* A MOVEC on a control register the core does not implement
               (the 68040 TT/MMU registers SysInfo reads) is skipped: the
               destination reads 0, as an MMU that is switched off would. */
            if (exc == M68kException::ILLEGAL &&
                (vec == 0 || vec == RTE_ADDR || vec == M68KEMU_SKIP4_STUB || vec == M68KEMU_FAULT_STUB))
            {
                u16 op = m68k_read16(ctx, getPC0());
                if (op == 0x4E7A || op == 0x4E7B)
                {
                    u16 ext = m68k_read16(ctx, getPC0() + 2);
                    if (op == 0x4E7A)
                    {
                        if (ext & 0x8000) setA((ext >> 12) & 7, 0); else setD((ext >> 12) & 7, 0);
                    }
                    /* The frame about to be written takes its PC from pc0:
                       point it past the 4-byte MOVEC and return through the
                       plain RTE, as a thunk return does. */
                    {
                        u32 next = getPC0() + 4;
                        setPC0(next);
                        setPC(next + 2);
                    }
                    m68k_write32(ctx, (ULONG)vector * 4, RTE_ADDR);
                }
                else
                    m68k_write32(ctx, (ULONG)vector * 4, M68KEMU_FAULT_STUB);
            }
            else if (vec == 0)
                m68k_write32(ctx, (ULONG)vector * 4, M68KEMU_FAULT_STUB);

            if (ctx->trace_calls && ctx->trace_exc < 40)
            {
                ctx->trace_exc++;
                bug("[m68kemu] TRACE exception %d vector %u at PC %08x SP %08x\n",
                    (int)exc, (unsigned)vector, (unsigned)getPC0(), (unsigned)getA(7));
                if (ctx->trace_exc == 1)
                {
                    ULONG i;
                    bug("[m68kemu] TRACE last instructions (pc: opcode words):\n");
                    for (i = 0; i < 512; i++)
                    {
                        ULONG pc = ctx->pc_ring[(ctx->pc_ring_pos + i) & 511];
                        bug("[m68kemu]   %08x: %04x %04x %04x\n", (unsigned)pc,
                            (unsigned)m68k_read16(ctx, pc), (unsigned)m68k_read16(ctx, pc + 2),
                            (unsigned)m68k_read16(ctx, pc + 4));
                    }
                    bug("[m68kemu]   stack: %08x %08x %08x %08x %08x %08x\n",
                        (unsigned)m68k_read32(ctx, getA(7)), (unsigned)m68k_read32(ctx, getA(7) + 4),
                        (unsigned)m68k_read32(ctx, getA(7) + 8), (unsigned)m68k_read32(ctx, getA(7) + 12),
                        (unsigned)m68k_read32(ctx, getA(7) + 16), (unsigned)m68k_read32(ctx, getA(7) + 20));
                    bug("[m68kemu]   SR %04x vectors 2-15:", (unsigned)getSR());
                    for (i = 2; i < 16; i++)
                        bug(" %08x", (unsigned)m68k_read32(ctx, i * 4));
                    bug("\n");
                    {
                        ULONG h = m68k_read32(ctx, vector * 4);
                        bug("[m68kemu]   handler %08x:", (unsigned)h);
                        for (i = 0; i < 16; i++)
                            bug(" %04x", (unsigned)m68k_read16(ctx, h + i * 2));
                        bug("\n");
                    }
                }
            }
            return;
        }

        /* Restore LINEA vector to normal RTE return (may have been
           redirected by a previous Supervisor() call) */
        m68k_write32(ctx, M68K_VEC_LINEA, RTE_ADDR);

        u32 pc = getPC0();

        if (pc == M68KEMU_FAULT_STUB)
        {
            ULONG i;
            m68k_log(ctx, "%s: unhandled exception %lu at PC %08lx, program stopped%s",
                ctx->program_name[0] ? ctx->program_name : "68k program",
                (unsigned long)ctx->last_exc, (unsigned long)ctx->last_exc_pc,
                ctx->trace_calls ? "" : " (SetEnv M68KEMU_TRACE 1 for registers and the last instructions)");
            if (ctx->trace_calls)
            {
            bug("[m68kemu]   D0 %08x D1 %08x D2 %08x A0 %08x A1 %08x A2 %08x A5 %08x A6 %08x SP %08x\n",
                (unsigned)getD(0), (unsigned)getD(1), (unsigned)getD(2), (unsigned)getA(0), (unsigned)getA(1),
                (unsigned)getA(2), (unsigned)getA(5), (unsigned)getA(6), (unsigned)getA(7));
            bug("[m68kemu]   last instructions:\n");
            for (i = 464; i < 512; i++)
            {
                ULONG p = ctx->pc_ring[(ctx->pc_ring_pos + i) & 511];
                bug("[m68kemu]   %08x: %04x %04x %04x\n", (unsigned)p,
                    (unsigned)m68k_read16(ctx, p), (unsigned)m68k_read16(ctx, p + 2), (unsigned)m68k_read16(ctx, p + 4));
            }
            }
            ctx->running = FALSE;
            return;
        }

        /* Find which library jump table this PC falls in */
        for (UWORD i = 0; i < ctx->num_libs; i++)
        {
            struct M68KFakeLibBase *lib = &ctx->libs[i];

            if (pc >= lib->jt_start && pc < lib->m68k_addr)
            {
                ULONG lvo_offset = lib->m68k_addr - pc;

                /* Search thunk table */
                for (ULONG j = 0; j < lib->num_thunks; j++)
                {
                    if (lib->thunks[j].lvo == lvo_offset && lib->thunks[j].thunk)
                    {
                        /* Pop JSR return address */
                        u32 sp = getA(7);
                        u32 ret_addr = m68k_read32(ctx, sp);
                        setA(7, sp + 4);

                        /* Run the thunk */
                        if (ctx->trace_calls) bug("[m68kemu] CALL %s LVO -%lu d0 %08x d1 %08x d2 %08x a0 %08x a1 %08x from %08x %08x %08x\n", lib->name, lvo_offset, (unsigned)getD(0),
                            (unsigned)getD(1), (unsigned)getD(2), (unsigned)getA(0), (unsigned)getA(1),
                            (unsigned)ctx->pc_ring[(ctx->pc_ring_pos - 2) & 511], (unsigned)ctx->pc_ring[(ctx->pc_ring_pos - 3) & 511],
                            (unsigned)ctx->pc_ring[(ctx->pc_ring_pos - 4) & 511]);
                        IPTR ret = lib->thunks[j].thunk(ctx, static_cast<void *>(this));
                        if (ctx->trace_calls) bug("[m68kemu]   -> %08lx\n", (unsigned long)ret);

                        if (ctx->sv_redirect)
                        {
                            dispatchSupervisor(ret_addr);
                            return;
                        }

                        if ((ret >> 32) != 0 && (ret >> 32) != 0xFFFFFFFFUL)
                            ret = m68k_bptr_out(ctx, (BPTR)ret);
                        setD(0, (u32)ret);
                        if (ctx->ccr_valid)
                        {
                            setCCR((getCCR() & 0x10) | ctx->ccr_val);
                            ctx->ccr_valid = FALSE;
                        }
                        if (ctx->tail_pc)
                        {
                            /* run 68k code (a library's init) as if the caller had
                               called it: its RTS returns to the caller with D0 */
                            u32 tsp = getA(7) - 4;
                            m68k_write32(ctx, tsp, ret_addr);
                            setA(7, tsp);
                            setD(0, ctx->tail_d0);
                            setA(0, ctx->tail_a0);
                            setA(6, ctx->tail_a6);
                            ret_addr = ctx->tail_pc;
                            ctx->tail_pc = 0;
                        }

                        /*
                         * Set PC so Moira pushes ret_addr in the exception frame.
                         * 68020: pushes reg.pc0 directly.
                         * 68000: pushes reg.pc - 2.
                         */
                        setPC0(ret_addr);
                        setPC(ret_addr + 2);
                        return;
                    }
                }

                /* Try generated thunks as fallback */
                for (ULONG j = 0; j < lib->num_gen_thunks; j++)
                {
                    if (lib->gen_thunks[j].lvo == lvo_offset && lib->gen_thunks[j].thunk)
                    {
                        u32 sp = getA(7);
                        u32 ret_addr = m68k_read32(ctx, sp);
                        setA(7, sp + 4);
                        if (ctx->trace_calls) bug("[m68kemu] GEN %s LVO -%lu d0 %08x d1 %08x d2 %08x a0 %08x a1 %08x from %08x %08x %08x\n", lib->name, lvo_offset, (unsigned)getD(0),
                            (unsigned)getD(1), (unsigned)getD(2), (unsigned)getA(0), (unsigned)getA(1),
                            (unsigned)ctx->pc_ring[(ctx->pc_ring_pos - 2) & 511], (unsigned)ctx->pc_ring[(ctx->pc_ring_pos - 3) & 511],
                            (unsigned)ctx->pc_ring[(ctx->pc_ring_pos - 4) & 511]);
                        IPTR ret = lib->gen_thunks[j].thunk(ctx, static_cast<void *>(this));
                        if (ctx->trace_calls) bug("[m68kemu]   -> %08lx\n", (unsigned long)ret);

                        if (ctx->sv_redirect)
                        {
                            dispatchSupervisor(ret_addr);
                            return;
                        }

                        /* a native pointer (an opaque object: VisualInfo, CxObj,
                           BOOPSI object...) does not fit in D0: hand out a handle */
                        if ((ret >> 32) != 0 && (ret >> 32) != 0xFFFFFFFFUL)
                            ret = m68k_bptr_out(ctx, (BPTR)ret);
                        setD(0, (u32)ret);
                        setPC0(ret_addr);
                        setPC(ret_addr + 2);
                        return;
                    }
                }

                /* Unimplemented LVO — pop return addr, return 0 */
                {
                    u32 sp = getA(7);
                    u32 ret_addr = m68k_read32(ctx, sp);
                    setA(7, sp + 4);
                    setD(0, 0);
                    setPC0(ret_addr);
                    setPC(ret_addr + 2);
                    m68k_log(ctx, "%s: %s function -%lu is not supported yet",
                             ctx->program_name[0] ? ctx->program_name : "68k program", lib->name, lvo_offset);
                }
                return;
            }
        }

        /* Not in any library — unknown A-line. Let Moira handle it normally.
         * Point LINEA vector to RTE so it just returns. */
    }

    void cpuDidHalt() override
    {
        ctx->running = FALSE;
    }
};

extern "C" {

ULONG M68KEmu_GetD(void *cpu, int n) { return static_cast<AROSMoira *>(cpu)->getD(n); }
ULONG M68KEmu_GetA(void *cpu, int n) { return static_cast<AROSMoira *>(cpu)->getA(n); }
void  M68KEmu_SetD(void *cpu, int n, ULONG val) { static_cast<AROSMoira *>(cpu)->setD(n, (u32)val); }
void  M68KEmu_SetA(void *cpu, int n, ULONG val) { static_cast<AROSMoira *>(cpu)->setA(n, (u32)val); }
UWORD M68KEmu_GetSR(void *cpu) { return static_cast<AROSMoira *>(cpu)->getSR(); }
void  M68KEmu_SetSR(void *cpu, UWORD val) { static_cast<AROSMoira *>(cpu)->setSR((u16)val); }
void  M68KEmu_SetPC(void *cpu, ULONG val) { static_cast<AROSMoira *>(cpu)->setPC((u32)val); }

LONG M68KEmu_Execute(struct M68KEmuContext *ctx)
{
    if (ctx->trace_calls || ctx->trace_after_msg)
        m68k_watchdog_start(ctx);
    AROSMoira cpu(ctx);
    cpu.setModel(Model::M68LC040);          /* Moira has no FPU: F-line instructions trap as on an LC040 */

    m68k_write32(ctx, 0, ctx->stack_top);
    m68k_write32(ctx, 4, ctx->entry_point);
    cpu.reset();

    /* Restore AbsExecBase at address 4 */
    m68k_write32(ctx, 4, ctx->m68k_sysbase);

    /* Reset leaves the interrupt mask at 7; AmigaOS code runs with it at 0,
       and the vertical blank server needs level 3 to get through */
    cpu.setSR(cpu.getSR() & ~0x0700);

    /* A user stack, should anything drop the CPU to user mode: reset leaves
       the user stack pointer at 0 */
    {
        ULONG usp = M68KEmu_HeapAlloc(ctx, 16384, 0);
        if (usp)
            cpu.setUSP(usp + 16384);
    }

    /* Write RTE at RTE_ADDR for LINEA exception return */
    m68k_write16(ctx, RTE_ADDR, M68K_OP_RTE);

    /* Supervisor trampoline at SV_TRAMP_ADDR:
       ADDQ.L #8,SP   (508F)  — skip Moira's 68020 exception frame
       JMP    (A5)     (4ED5)  — jump to user function in supervisor mode */
    m68k_write16(ctx, SV_TRAMP_ADDR,     M68K_OP_ADDQ8_SP);
    m68k_write16(ctx, SV_TRAMP_ADDR + 2, M68K_OP_JMP_A5);

    /* Set LINEA exception vector to RTE_ADDR */
    m68k_write32(ctx, M68K_VEC_LINEA, RTE_ADDR);

    /* Push sentinel return address */
    u32 sp = cpu.getA(7);
    sp -= 4;
    m68k_write32(ctx, sp, SENTINEL_ADDR);
    cpu.setA(7, sp);

    /* AROS_PROCH entry: A0=argptr, D0=argsize, A6=SysBase */
    cpu.setA(0, ctx->m68k_argptr);
    cpu.setD(0, ctx->m68k_argsize);
    cpu.setA(6, ctx->m68k_sysbase);

    /* Vertical blank stub at M68KEMU_VBL_STUB, reached through autovector 3:
         movem.l d0-d1/a0-a1/a5-a6,-(sp)
         lea     $dff000,a0
         movea.l M68KEMU_VBL_DATA,a1
         movea.l M68KEMU_VBL_CODE,a5
         movea.l 4.w,a6
         jsr     (a5)
         movem.l (sp)+,d0-d1/a0-a1/a5-a6
         rte                                                               */
    {
        static const u16 stub[] = { 0x48E7, 0xC0C6, 0x41F9, 0x00DF, 0xF000, 0x2279, 0x0000, M68KEMU_VBL_DATA,
                                    0x2A79, 0x0000, M68KEMU_VBL_CODE, 0x2C78, 0x0004, 0x4E95, 0x4CDF, 0x6303, 0x4E73 };
        for (unsigned i = 0; i < sizeof(stub) / sizeof(stub[0]); i++)
            m68k_write16(ctx, M68KEMU_VBL_STUB + i * 2, stub[i]);
        m68k_write32(ctx, 0x6C, M68KEMU_VBL_STUB);          /* level 3 autovector */

        /* addq.l #4,2(sp); rte */
        m68k_write16(ctx, M68KEMU_SKIP4_STUB,     0x586F);
        m68k_write16(ctx, M68KEMU_SKIP4_STUB + 2, 0x0002);
        m68k_write16(ctx, M68KEMU_SKIP4_STUB + 4, 0x4E73);
        /* A-line the dispatcher recognises as "unhandled exception" */
        m68k_write16(ctx, M68KEMU_FAULT_STUB, 0xA0FF);
        for (unsigned v = 2; v < 48; v++)
            if (v != 10 && v != 27)                          /* LINEA, level 3 */
                m68k_write32(ctx, v * 4, M68KEMU_FAULT_STUB);
        ctx->irq_stack_top = M68KEmu_HeapAlloc(ctx, 8192, 0);
        if (ctx->irq_stack_top)
            ctx->irq_stack_top += 8192;
    }

    ctx->running = TRUE;
    ctx->exit_code = 0;
    ctx->low_guard = TRUE;

    if (ctx->trace_calls) bug("[m68kemu] TRACE core " __DATE__ " " __TIME__ " start entry %08x sp %08x argptr %08x argsize %u sysbase %08x\n",
        (unsigned)ctx->entry_point, (unsigned)sp, (unsigned)ctx->m68k_argptr,
        (unsigned)ctx->m68k_argsize, (unsigned)ctx->m68k_sysbase);
    {
        unsigned long icount = 0;
        while (ctx->running)
        {
            ULONG pc = cpu.getPC0();
            ctx->pc_ring[ctx->pc_ring_pos++ & 511] = pc;
            if (ctx->vbl_code)
            {
                if (ctx->vbl_irq)
                {
                    if (pc == M68KEMU_VBL_STUB)
                    {
                        cpu.setIPL(0);
                        ctx->vbl_irq = FALSE;
                        if ((ctx->vbl_count <= 6 || ctx->vbl_count == 50) && ctx->trace_calls)
                            bug("[m68kemu] VBL %lu stub entered, SR %04x SP %08x\n", (unsigned long)ctx->vbl_count,
                                (unsigned)cpu.getSR(), (unsigned)cpu.getA(7));
                    }
                }
                else if ((icount & 0x3FF) == 0 && m68k_vbl_due(ctx))
                {
                    if (!(cpu.getSR() & 0x2000) && ctx->irq_stack_top)
                        cpu.setISP(ctx->irq_stack_top);
                    ctx->vbl_irq = TRUE;
                    if ((ctx->vbl_count++ < 6 || ctx->vbl_count == 50) && ctx->trace_calls)
                        bug("[m68kemu] VBL %lu raised at PC %08x SR %04x\n", (unsigned long)ctx->vbl_count,
                            (unsigned)pc, (unsigned)cpu.getSR());
                    cpu.setIPL(3);
                }
            }
            if (m68k_read32(ctx, 4) != ctx->m68k_sysbase && ctx->low_trap_count < 8)
            {
                ULONG i;
                ctx->low_trap_count = 8;
                if (ctx->trace_calls)
                {
                bug("[m68kemu] TRACE AbsExecBase changed to %08x before PC %08x; last instructions:\n",
                    (unsigned)m68k_read32(ctx, 4), (unsigned)pc);
                for (i = 480; i < 512; i++)
                {
                    ULONG p = ctx->pc_ring[(ctx->pc_ring_pos + i) & 511];
                    bug("[m68kemu]   %08x: %04x %04x %04x\n", (unsigned)p,
                        (unsigned)m68k_read16(ctx, p), (unsigned)m68k_read16(ctx, p + 2),
                        (unsigned)m68k_read16(ctx, p + 4));
                }
                bug("[m68kemu]   mem0: %08x %08x %08x %08x\n", (unsigned)m68k_read32(ctx, 0),
                    (unsigned)m68k_read32(ctx, 4), (unsigned)m68k_read32(ctx, 8), (unsigned)m68k_read32(ctx, 12));
                }
                m68k_write32(ctx, 4, ctx->m68k_sysbase);
            }
            if (pc >= ctx->mem_size && !ctx->pc_exit_dumped)
            {
                ULONG i;
                ctx->pc_exit_dumped = TRUE;
                m68k_log(ctx, "%s: jumped to %08x outside its memory, program stopped%s",
                    ctx->program_name[0] ? ctx->program_name : "68k program", (unsigned)pc,
                    ctx->trace_calls ? "" : " (SetEnv M68KEMU_TRACE 1 for registers and the last instructions)");
                for (i = 448; i < 512 && ctx->trace_calls; i++)
                {
                    ULONG p = ctx->pc_ring[(ctx->pc_ring_pos + i) & 511];
                    bug("[m68kemu]   %08x: %04x %04x %04x\n", (unsigned)p,
                        (unsigned)m68k_read16(ctx, p), (unsigned)m68k_read16(ctx, p + 2),
                        (unsigned)m68k_read16(ctx, p + 4));
                }
                if (ctx->trace_calls) bug("[m68kemu]   D0 %08x D1 %08x A0 %08x A1 %08x A5 %08x A6 %08x SP %08x\n",
                    (unsigned)cpu.getD(0), (unsigned)cpu.getD(1), (unsigned)cpu.getA(0),
                    (unsigned)cpu.getA(1), (unsigned)cpu.getA(5), (unsigned)cpu.getA(6), (unsigned)cpu.getA(7));
                ctx->running = FALSE;
                break;
            }
            ctx->icount_lo = (ULONG)icount;
            cpu.execute();
            if ((++icount & 0xFFFFF) == 0)
            {
                static ULONG lastpc = 0xFFFFFFFF;
                if (ctx->trace_calls) bug("[m68kemu] TRACE %lu instructions, PC %08x SP %08x D0 %08x A6 %08x SR %04x\n",
                    icount, (unsigned)cpu.getPC0(), (unsigned)cpu.getA(7),
                    (unsigned)cpu.getD(0), (unsigned)cpu.getA(6), (unsigned)cpu.getSR());
                lastpc = cpu.getPC0();
                (void)lastpc;
            }
        }
        if (ctx->trace_calls) bug("[m68kemu] program ended after %lu instructions, D0 %08x, %lu VBLs\n", icount, (unsigned)cpu.getD(0),
            (unsigned long)ctx->vbl_count);
    }

    ctx->exit_code = (LONG)cpu.getD(0);
    m68k_watchdog_stop(ctx);
    return ctx->exit_code;
}

} /* extern "C" */
