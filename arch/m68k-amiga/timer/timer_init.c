/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

/*

    implementation notes:

    - CIAA-A: normal timer jobs (microhz/e-clock)
    - CIAA-B: E-clock counter (can move to CIAB-A or CIAB-B)
    - vblank interrupt used for vblank timer unit

    Unit conversions and misuse of tv_sec/tv_usec fields probably looks strange..
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/kernel.h>
#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/alerts.h>
#include <exec/initializers.h>
#include <devices/timer.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>
#include <graphics/gfxbase.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/timer.h>
#include <proto/cia.h>
#include <proto/battclock.h>

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include <timer_intern.h>
#include <timer_platform.h>

AROS_INTP(ciab_eclock);
AROS_INTP(ciaint_timer);
AROS_INTP(cia_vbint);

/****************************************************************************************/

static void start_eclock(struct TimerBase *tb, UWORD cnt)
{
    *tb->tb_eclock_cr = 0x00;
    SetICR(tb->tb_eclock_res, 1 << tb->tb_eclock_intbit);
    // start timer in continuous mode
    *tb->tb_eclock_lo = cnt;
    *tb->tb_eclock_hi = cnt >> 8;
    *tb->tb_eclock_cr |= 0x10;
    *tb->tb_eclock_cr |= 0x01;
}
static void set_eclock(struct TimerBase *tb, WORD intbit)
{
    WORD cia = intbit >> 1;
    intbit &= 1;
    tb->tb_eclock_cia = (struct CIA*)(cia ? 0xbfd000 : 0xbfe001);
    tb->tb_eclock_cr = (UBYTE*)tb->tb_eclock_cia + (intbit ? 0xf00 : 0xe00);
    tb->tb_eclock_lo = (UBYTE*)tb->tb_eclock_cia + (intbit ? 0x600 : 0x400);
    tb->tb_eclock_hi = tb->tb_eclock_lo + 0x100;
    tb->tb_eclock_intbit = intbit;
    tb->tb_eclock_res = tb->tb_cia[cia];
}

static void TimerHook(struct Resource *cia, struct TimerBase *tb, WORD iCRBit)
{
    WORD cnt;
    BOOL intactive;
    UBYTE hi, lo;

    if (tb->tb_eclock_res != cia || tb->tb_eclock_intbit != iCRBit)
        return;
    /* We are in Disabled state already
     * Someone wants to allocate our eclock timer
     * Check if we have any available timers left..
     */
    D(bug("someone wants our timer %x %d\n", cia, iCRBit));
    for (cnt = 1; cnt < 4; cnt++) {
        if (!AddICRVector(tb->tb_cia[cnt >> 1], cnt & 1, &tb->tb_ciaint_eclock))
            break;
    }
    if (cnt >= 4) {
        D(bug("no free timers\n"));
        return;
    }
    /* We have one! */
    D(bug("e-clock timer moved to %d\n", cnt));
    for (;;) {
        hi = *tb->tb_eclock_hi;
        lo = *tb->tb_eclock_lo;
        if (hi == *tb->tb_eclock_hi)
            break;
    }
    intactive = (SetICR(tb->tb_eclock_res, 0) & (1 << tb->tb_eclock_intbit)) != 0;
    if (intactive) {
        // re-read, there may have been wrap-around
        for (;;) {
            hi = *tb->tb_eclock_hi;
            lo = *tb->tb_eclock_lo;
            if (hi == *tb->tb_eclock_hi)
                break;
        }
    }
    *tb->tb_eclock_cr = 0x00;
    RemICRVector(tb->tb_eclock_res, tb->tb_eclock_intbit, &tb->tb_ciaint_eclock);
    set_eclock (tb, cnt);
    start_eclock (tb, (hi << 8) | lo);
    if (intactive)
        SetICR(tb->tb_eclock_res, 0x80 | (1 << tb->tb_eclock_intbit));
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct Interrupt *inter;
    struct Interrupt fakeinter;
    struct BattClockBase *BattClockBase;
    struct GfxBase *GfxBase;

    GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);

    InitCustom(GfxBase);

    LIBBASE->tb_eclock_rate = (GfxBase->DisplayFlags & REALLY_PAL) ? 709379 : 715909;
    LIBBASE->tb_vblank_rate = (GfxBase->DisplayFlags & PAL) ? 50 : 60;
    LIBBASE->tb_vblank_micros = 1000000 / LIBBASE->tb_vblank_rate;
    SysBase->ex_EClockFrequency = LIBBASE->tb_eclock_rate;
    LIBBASE->tb_eclock_micro_mult = (GfxBase->DisplayFlags & REALLY_PAL) ? 92385 : 91542;
    LIBBASE->tb_micro_eclock_mult = (GfxBase->DisplayFlags & REALLY_PAL) ? 23245 : 23459;

    CloseLibrary((struct Library*)GfxBase);

    BattClockBase = OpenResource("battclock.resource");
    if (BattClockBase)
        LIBBASE->tb_CurrentTime.tv_secs = ReadBattClock();

    /* Initialise the lists */
    NEWLIST(&LIBBASE->tb_Lists[UNIT_VBLANK]);
    NEWLIST(&LIBBASE->tb_Lists[UNIT_MICROHZ]);
 
    inter = &LIBBASE->tb_vbint;
    inter->is_Code = (APTR)cia_vbint;
    inter->is_Data         = LIBBASE;
    inter->is_Node.ln_Name = "timer.device VBlank";
    inter->is_Node.ln_Pri  = 20;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_VERTB, inter);

    LIBBASE->tb_cia[0] = OpenResource("ciaa.resource");
    LIBBASE->tb_cia[1] = OpenResource("ciab.resource");
    if (!LIBBASE->tb_cia[0] || !LIBBASE->tb_cia[1])
        Alert(AT_DeadEnd | AG_OpenRes | AO_CIARsrc);
    LIBBASE->tb_eclock_res = LIBBASE->tb_micro_res = LIBBASE->tb_cia[0];

    /* CIA-A timer A = microhz */
    LIBBASE->tb_micro_cia = (struct CIA*)0xbfe001;
    LIBBASE->tb_micro_cr = (UBYTE*)LIBBASE->tb_micro_cia + 0xe00;
    LIBBASE->tb_micro_lo = (UBYTE*)LIBBASE->tb_micro_cia + 0x400;
    LIBBASE->tb_micro_hi = (UBYTE*)LIBBASE->tb_micro_cia + 0x500;
    LIBBASE->tb_micro_intbit = 0;

    inter = &LIBBASE->tb_ciaint_timer;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "timer.device microhz";
    inter->is_Code = (APTR)ciaint_timer;
    inter->is_Data = LIBBASE;

    /* CIA-A timer B = E-Clock */
    set_eclock (LIBBASE, 1);

    inter = &LIBBASE->tb_ciaint_eclock;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "timer.device eclock";
    inter->is_Code = (APTR)ciab_eclock;
    inter->is_Data = LIBBASE;

    Disable();

    if (AddICRVector(LIBBASE->tb_micro_res, LIBBASE->tb_micro_intbit, &LIBBASE->tb_ciaint_timer))
        Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
    *LIBBASE->tb_micro_cr = 0x08; // one-shot
    SetICR(LIBBASE->tb_micro_res, 1 << LIBBASE->tb_micro_intbit);

    if (AddICRVector(LIBBASE->tb_eclock_res, LIBBASE->tb_eclock_intbit, &LIBBASE->tb_ciaint_eclock))
        Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
    start_eclock (LIBBASE, 0xffff);

    /* Emulate AOS 2.0+ feature which moves 2.0+ only E-clock timer out of
     * the way if some other (Usually 1.x) program assumes CIAA-B is always
     * free and wants to allocate it. This is m68k AROS hack, there is no
     * official API.
     */
    fakeinter.is_Code = (APTR)TimerHook;
    fakeinter.is_Data = LIBBASE;
    AddICRVector(LIBBASE->tb_cia[0], -1, &fakeinter);
    AddICRVector(LIBBASE->tb_cia[1], -1, &fakeinter);

    Enable(); 

    D(bug("timer.device init\n"));

    return TRUE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct timerequest *tr,
    ULONG unitNum,
    ULONG flags
)
{
    switch(unitNum)
    {
    case UNIT_VBLANK:
    case UNIT_WAITUNTIL:
    case UNIT_MICROHZ:
    case UNIT_ECLOCK:
    case UNIT_WAITECLOCK:
        tr->tr_node.io_Error = 0;
        tr->tr_node.io_Unit = (struct Unit *)unitNum;
        tr->tr_node.io_Device = (struct Device *)LIBBASE;
    break;

    default:
        tr->tr_node.io_Error = IOERR_OPENFAIL;
    }

    return TRUE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    Disable();
    RemIntServer(INTB_VERTB, &LIBBASE->tb_vbint);
    RemICRVector(LIBBASE->tb_micro_res, LIBBASE->tb_micro_intbit, &LIBBASE->tb_ciaint_timer);
    RemICRVector(LIBBASE->tb_eclock_res, LIBBASE->tb_eclock_intbit, &LIBBASE->tb_ciaint_eclock);
    Enable();
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
