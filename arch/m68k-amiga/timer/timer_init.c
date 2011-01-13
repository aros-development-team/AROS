/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

/*

	implementation notes:

	- CIAA-A: keyboard
	- CIAA-B: normal timer jobs (microhz/e-clock)
	- CIAB-A: E-clock counter
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

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/timer.h>
#include <proto/cia.h>
#include <proto/battclock.h>

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include "timer_intern.h"

AROS_UFP4(APTR, ciab_ciainta,
    AROS_UFPA(ULONG, dummy, A0),
   	AROS_UFPA(void *, data, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_UFP4(APTR, ciaa_ciaintb,
    AROS_UFPA(ULONG, dummy, A0),
    AROS_UFPA(void *, data, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_UFP4(APTR, cia_vbint,
    AROS_UFPA(ULONG, dummy, A0),
    AROS_UFPA(void *, data, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, mySysBase, A6)); 

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
    volatile struct CIA *ciab = (struct CIA*)0xbfd000;
    struct Interrupt *inter;
    struct BattClockBase *BattClockBase;

    /* Setup the timer.device data */
    LIBBASE->tb_eclock_rate = 709379; // FIXME: PAL/NTSC check
    LIBBASE->tb_cia_micros = LIBBASE->tb_eclock_rate;
    LIBBASE->tb_vblank_rate = 50;
    LIBBASE->tb_vblank_micros = 1000000 / LIBBASE->tb_vblank_rate;

    BattClockBase = OpenResource("battclock.resource");
    if (BattClockBase) {
	ULONG t = ReadBattClock();
	if (t) {
	    LIBBASE->tb_CurrentTime.tv_secs = t;
	}
    }

    /* Initialise the lists */
    NEWLIST(&LIBBASE->tb_Lists[UNIT_VBLANK]);
    NEWLIST(&LIBBASE->tb_Lists[UNIT_MICROHZ]);
 
    inter = &LIBBASE->vbint;
    inter->is_Code = (APTR)cia_vbint;
    inter->is_Data         = LIBBASE;
    inter->is_Node.ln_Name = "timer.device VBlank";
    inter->is_Node.ln_Pri  = 20;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_VERTB, inter);

    if (!(LIBBASE->ciaares = OpenResource("ciaa.resource")))
	Alert(AT_DeadEnd | AG_OpenRes | AO_CIARsrc);
    if (!(LIBBASE->ciabres = OpenResource("ciab.resource")))
	Alert(AT_DeadEnd | AG_OpenRes | AO_CIARsrc);
	
    inter = &LIBBASE->ciaintb;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "timer.device CIAA-B";
    inter->is_Code = (APTR)ciaa_ciaintb;
    inter->is_Data = LIBBASE;
	
    Disable();
    if (AddICRVector(LIBBASE->ciaares, 1, inter)) // CIAA-B timer
	Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
    ciaa->ciacrb = 0x08; // one-shot
    SetICR(LIBBASE->ciaares, 0x02);
    Enable(); 

    inter = &LIBBASE->ciainta;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "timer.device CIAB-A";
    inter->is_Code = (APTR)ciab_ciainta;
    inter->is_Data = LIBBASE;
	
    Disable();
    if (AddICRVector(LIBBASE->ciabres, 0, inter)) // CIAB-A timer
	Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
    ciab->ciacra &= ~0x3f;
    // start CIA-A in continuous mode
    ciab->ciatalo = (UBYTE)(ECLOCK_BASE >> 0);
    ciab->ciatblo = (UBYTE)(ECLOCK_BASE >> 8);
    ciab->ciacra |= 0x10;
    ciab->ciacra |= 0x01;
    LIBBASE->tb_eclock_last = ECLOCK_BASE;
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
    RemIntServer(&LIBBASE->vbint, INTB_VERTB);
    RemICRVector(LIBBASE->ciaares, 1, &LIBBASE->ciaintb);
    RemICRVector(LIBBASE->ciabres, 0, &LIBBASE->ciainta);
    Enable();
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
