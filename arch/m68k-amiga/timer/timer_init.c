/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

/*

	implementation notes:

	- CIAA-A: normal timer jobs (microhz/e-clock)
	- CIAA-B: E-clock counter
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

AROS_UFIP(ciab_eclock);
AROS_UFIP(ciaint_timer);
AROS_UFIP(cia_vbint);

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct Interrupt *inter;
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

    /* CIA-A timer A = microhz */
    LIBBASE->tb_micro_cia = (struct CIA*)0xbfe001;
    LIBBASE->tb_micro_cr = (UBYTE*)LIBBASE->tb_micro_cia + 0xe00;
    LIBBASE->tb_micro_lo = (UBYTE*)LIBBASE->tb_micro_cia + 0x400;
    LIBBASE->tb_micro_hi = (UBYTE*)LIBBASE->tb_micro_cia + 0x500;
    LIBBASE->tb_micro_intbit = 0;
    if (!(LIBBASE->tb_micro_res = OpenResource("ciaa.resource")))
	Alert(AT_DeadEnd | AG_OpenRes | AO_CIARsrc);
    
    inter = &LIBBASE->tb_ciaint_timer;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "timer.device microhz";
    inter->is_Code = (APTR)ciaint_timer;
    inter->is_Data = LIBBASE;
	
    Disable();
    if (AddICRVector(LIBBASE->tb_micro_res, LIBBASE->tb_micro_intbit, inter))
	Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
    *LIBBASE->tb_micro_cr = 0x08; // one-shot
    SetICR(LIBBASE->tb_micro_res, 1 << LIBBASE->tb_micro_intbit);
    Enable(); 

    /* CIA-A timer B = E-Clock */
    LIBBASE->tb_eclock_cia = (struct CIA*)0xbfe001;
    LIBBASE->tb_eclock_cr = (UBYTE*)LIBBASE->tb_eclock_cia + 0xf00;
    LIBBASE->tb_eclock_lo = (UBYTE*)LIBBASE->tb_eclock_cia + 0x600;
    LIBBASE->tb_eclock_hi = (UBYTE*)LIBBASE->tb_eclock_cia + 0x700;
    LIBBASE->tb_eclock_intbit = 1;
    if (!(LIBBASE->tb_eclock_res = OpenResource("ciaa.resource")))
	Alert(AT_DeadEnd | AG_OpenRes | AO_CIARsrc);

    inter = &LIBBASE->tb_ciaint_eclock;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "timer.device eclock";
    inter->is_Code = (APTR)ciab_eclock;
    inter->is_Data = LIBBASE;
	
    Disable();
    if (AddICRVector(LIBBASE->tb_eclock_res, LIBBASE->tb_eclock_intbit, inter))
	Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
    *LIBBASE->tb_eclock_cr = 0x00;
    // start timer in continuous mode
    *LIBBASE->tb_eclock_lo = 0xff;
    *LIBBASE->tb_eclock_hi = 0xff;
    *LIBBASE->tb_eclock_cr |= 0x10;
    *LIBBASE->tb_eclock_cr |= 0x01;
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
