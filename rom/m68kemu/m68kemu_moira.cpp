/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
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

    AROSMoira(struct M68KEmuContext *c) : ctx(c) {}

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
        u32 sp = getA(7);
        sp -= 2; m68k_write16(ctx, sp, 0x0000);  /* vector offset (68020) */
        sp -= 4; m68k_write32(ctx, sp, ret_addr); /* return PC */
        sp -= 2; m68k_write16(ctx, sp, 0x0000);  /* SR: user mode */
        setA(7, sp);

        /* Redirect LINEA vector to supervisor trampoline */
        m68k_write32(ctx, M68K_VEC_LINEA, SV_TRAMP_ADDR);

        /* Set PC so Moira's exception frame points to userFunc.
           68020 pushes reg.pc0, 68000 pushes reg.pc-2. */
        setPC0(userFunc);
        setPC(userFunc + 2);

        bug("[m68kemu] Supervisor: dispatching to 0x%lx, ret=0x%lx\n",
            (unsigned long)userFunc, (unsigned long)ret_addr);
    }

    u8 read8(u32 addr) const override
    {
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
        if ((addr & CUSTOM_MASK) == CUSTOM_BASE) return;
        if (addr >= ctx->mem_size) return;
        ctx->mem[addr] = val;
    }

    void write16(u32 addr, u16 val) const override
    {
        if ((addr & CUSTOM_MASK) == CUSTOM_BASE)
        {
            customWrite16(addr & 0xFFE, val);
            return;
        }
        if (addr + 1 >= ctx->mem_size) return;
        ctx->mem[addr]     = (u8)(val >> 8);
        ctx->mem[addr + 1] = (u8)(val);
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
        if (exc != M68kException::LINEA) return;

        /* Restore LINEA vector to normal RTE return (may have been
           redirected by a previous Supervisor() call) */
        m68k_write32(ctx, M68K_VEC_LINEA, RTE_ADDR);

        u32 pc = getPC0();

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
                        bug("[m68kemu] CALL %s LVO -%lu\n", lib->name, lvo_offset);
                        IPTR ret = lib->thunks[j].thunk(ctx, (void *)this);

                        if (ctx->sv_redirect)
                        {
                            dispatchSupervisor(ret_addr);
                            return;
                        }

                        setD(0, (u32)ret);

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
                        bug("[m68kemu] GEN %s LVO -%lu\n", lib->name, lvo_offset);
                        IPTR ret = lib->gen_thunks[j].thunk(ctx, (void *)this);

                        if (ctx->sv_redirect)
                        {
                            dispatchSupervisor(ret_addr);
                            return;
                        }

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
                    bug("[m68kemu] UNIMPL %s LVO -%lu\n", lib->name, lvo_offset);
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

ULONG M68KEmu_GetD(void *cpu, int n) { return ((AROSMoira *)cpu)->getD(n); }
ULONG M68KEmu_GetA(void *cpu, int n) { return ((AROSMoira *)cpu)->getA(n); }
void  M68KEmu_SetD(void *cpu, int n, ULONG val) { ((AROSMoira *)cpu)->setD(n, (u32)val); }
void  M68KEmu_SetA(void *cpu, int n, ULONG val) { ((AROSMoira *)cpu)->setA(n, (u32)val); }
UWORD M68KEmu_GetSR(void *cpu) { return ((AROSMoira *)cpu)->getSR(); }
void  M68KEmu_SetSR(void *cpu, UWORD val) { ((AROSMoira *)cpu)->setSR((u16)val); }
void  M68KEmu_SetPC(void *cpu, ULONG val) { ((AROSMoira *)cpu)->setPC((u32)val); }

LONG M68KEmu_Execute(struct M68KEmuContext *ctx)
{
    AROSMoira cpu(ctx);
    cpu.setModel(Model::M68040);

    m68k_write32(ctx, 0, ctx->stack_top);
    m68k_write32(ctx, 4, ctx->entry_point);
    cpu.reset();

    /* Restore AbsExecBase at address 4 */
    m68k_write32(ctx, 4, ctx->m68k_sysbase);

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

    ctx->running = TRUE;
    ctx->exit_code = 0;

    while (ctx->running)
        cpu.execute();

    ctx->exit_code = (LONG)cpu.getD(0);
    return ctx->exit_code;
}

} /* extern "C" */
