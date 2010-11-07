/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

/*

	implementation notes:

	- CIA-A is used as an E-clock counter
	  It is also shared with keyboard handshake timer,
	  keyboard handler must check if timer is already in use!
	- CIA-B is used for normal timer jobs (microhz/e-clock)
	- vblank interrupt used for vblank timer unit

	Unit conversions and misuse of tv_sec/tv_usec fields probably looks strange..
*/

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

#include <aros/symbolsets.h>
#define DEBUG 1
#include <aros/debug.h>
#include <proto/arossupport.h>

#include LC_LIBDEFS_FILE

#include "timer_intern.h"

AROS_UFP4(APTR, cia_ciainta,
    AROS_UFPA(ULONG, dummy, A0),
   	AROS_UFPA(void *, data, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_UFP4(APTR, cia_ciaintb,
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
	struct Interrupt *inter;

    /* Setup the timer.device data */
	LIBBASE->tb_eclock_rate = 709379; // FIXME: PAL/NTSC check
	LIBBASE->tb_cia_micros = LIBBASE->tb_eclock_rate;
	LIBBASE->tb_vblank_rate = 50;
	LIBBASE->tb_vblank_micros = 1000000 / LIBBASE->tb_vblank_rate;

    /* Initialise the lists */
    NEWLIST(&LIBBASE->tb_Lists[UNIT_VBLANK]);
    NEWLIST(&LIBBASE->tb_Lists[UNIT_MICROHZ]);
    NEWLIST(&LIBBASE->tb_Lists[UNIT_ECLOCK]);
 
	inter = &LIBBASE->vbint;
   	inter->is_Code = (APTR)cia_vbint;
    inter->is_Data         = LIBBASE;
    inter->is_Node.ln_Name = "timer.device VBlank";
    inter->is_Node.ln_Pri  = 20;
    inter->is_Node.ln_Type = NT_INTERRUPT;
	AddIntServer(INTB_VERTB, inter);

	if (!(LIBBASE->ciares = OpenResource("ciaa.resource")))
		Alert(AT_DeadEnd | AG_OpenRes | AO_CIARsrc);
	
	inter = &LIBBASE->ciaintb;
	inter->is_Node.ln_Pri = 0;
	inter->is_Node.ln_Type = NT_INTERRUPT;
	inter->is_Node.ln_Name = "timer.device CIA-B";
	inter->is_Code = (APTR)cia_ciaintb;
	inter->is_Data = LIBBASE;
	
	Disable();
	if (AddICRVector(LIBBASE->ciares, 1, inter)) // CIA-B timer
		Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
	ciaa->ciacrb = 0x08; // one-shot
	SetICR(LIBBASE->ciares, 0x02);
	Enable(); 

	inter = &LIBBASE->ciainta;
	inter->is_Node.ln_Pri = 0;
	inter->is_Node.ln_Type = NT_INTERRUPT;
	inter->is_Node.ln_Name = "timer.device CIA-A";
	inter->is_Code = (APTR)cia_ciainta;
	inter->is_Data = LIBBASE;
	
	Disable();
	if (AddICRVector(LIBBASE->ciares, 0, inter)) // CIA-A timer
		Alert(AT_DeadEnd | AG_NoMemory | AO_CIARsrc);
	ciaa->ciacra &= ~0x3f;
	// start CIA-A in continuous mode
	ciaa->ciatalo = (UBYTE)(ECLOCK_BASE >> 0);
	ciaa->ciatblo = (UBYTE)(ECLOCK_BASE >> 8);
	ciaa->ciacra |= 0x10;
	ciaa->ciacra |= 0x01;
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
    RemICRVector(LIBBASE->ciares, 1, &LIBBASE->ciaintb);
    RemICRVector(LIBBASE->ciares, 0, &LIBBASE->ciainta);
	Enable();
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
