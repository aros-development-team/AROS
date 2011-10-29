/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: card_init.c $

    Desc: 
    Lang: English
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include "card_intern.h"

AROS_UFP4 (APTR, card_level2,
    AROS_UFPA(ULONG, dummy, A0),
    AROS_UFPA(void *, data, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, mySysBase, A6));

AROS_UFP4 (APTR, card_level6,
    AROS_UFPA(ULONG, dummy, A0),
    AROS_UFPA(void *, data, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, mySysBase, A6));

void CardTask(struct Task *parent, struct CardResource *CardResource);

static BOOL addpcmciaram(struct CardResource *CardResource, struct CardHandle *ch, struct DeviceTData *dtd)
{
    ULONG size = dtd->dtd_DTsize;
    APTR addr =(APTR)GAYLE_RAM;
    
    if (size < 262144)
    	return FALSE;
    size -= 512;
    addr += 512;
    AddMemList(size, MEMF_FAST | MEMF_PUBLIC, -5, addr, CardResource->crb_LibNode.lib_Node.ln_Name);
    return TRUE;
}

static BOOL checkcard(struct CardResource *CardResource)
{
    struct CardHandle *ch;
    BOOL sysram = FALSE;
    
    ch = AllocVec(sizeof(struct CardHandle), MEMF_CLEAR);
    if (ch) {
	ch->cah_CardFlags = CARDF_IFAVAILABLE;
	if (!OwnCard(ch)) {
	    struct Resident *res;
	    struct DeviceTData dtd;
	    UBYTE device[6];

	    BeginCardAccess(ch);

	    if (!CopyTuple(ch, NULL, 0, 0)) { /* debug log all tuples, check if tuple chain is valid */
	    	CARDDEBUG(bug("Invalid tuple chain detected\n"));
	    } else {
	    	res = IfAmigaXIP(ch);
	    	if (res) {
	    	    CARDDEBUG(bug("CISTPL_AMIGAXIP found\n"));
	    	} else if (CopyTuple(ch, device, CISTPL_DEVICE, sizeof(device) - 2)) {
	    	    if (DeviceTuple(device, &dtd)) {
	    	    	if (dtd.dtd_DTtype == DTYPE_SRAM || dtd.dtd_DTtype == DTYPE_DRAM) {
			    CARDDEBUG(bug("RAM card found, size %d bytes, speed %d\n", dtd.dtd_DTsize, dtd.dtd_DTspeed));
			    if (!CopyTuple(ch, device, CISTPL_FORMAT, 0) && !CopyTuple(ch, device, CISTPL_GEOMETRY, 0) && dtd.dtd_DTspeed <= 250) {
			    	if (addpcmciaram(CardResource, ch, &dtd)) {
			    	    CardAccessSpeed(ch, dtd.dtd_DTspeed);
				    CARDDEBUG(bug("Mapped as System RAM.\n"));
			    	    sysram = TRUE;
			    	}
			    } else {
			    	CARDDEBUG(bug("Not usable as System RAM.\n"));
			    }
			}
		    }
		}
	    }	    	

	    EndCardAccess(ch);

	    ReleaseCard(ch, CARDF_REMOVEHANDLE);
	}
	FreeVec(ch);
    }
    return sysram;
}

static int Cardres_Init(struct CardResource *CardResource)
{
    UBYTE gayle;
    struct CardMemoryMap *cmm;
    struct Interrupt *intr;
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    Disable();
    gayle = ReadGayle();
    Enable();
    
    /* No Gayle = No PCMCIA slot */
    if (!gayle)
    	return FALSE;
    
    /* Address space conflict? */
    if (TypeOfMem((UBYTE*)GAYLE_RAM + 0x1000) != 0)
    	return FALSE;
    
    NEWLIST(&CardResource->handles);
    cmm = &CardResource->cmm;
    cmm->cmm_CommonMemory = (UBYTE*)GAYLE_RAM;
    cmm->cmm_AttributeMemory = (UBYTE*)GAYLE_ATTRIBUTE;
    cmm->cmm_IOMemory = (UBYTE*)GAYLE_IO;
    cmm->cmm_CommonMemSize = GAYLE_RAMSIZE;
    cmm->cmm_AttributeMemSize = GAYLE_ATTRIBYTESIZE;
    cmm->cmm_IOMemSize = GAYLE_IOSIZE;

    CARDDEBUG(bug("PCMCIA slot enabled\n"));
    
    pcmcia_reset(CardResource);
    pcmcia_enable();
    
    CardResource->removed = TRUE;
    if (gio->status & GAYLE_CS_CCDET) {
    	/* Card inserted */
    	CARDDEBUG(bug("Inserted PCMCIA card detected\n"));
    	pcmcia_cardreset(CardResource);
    	CardResource->removed = FALSE;
    	if (checkcard(CardResource)) {
    	    CardResource->resetberr = GAYLE_IRQ_RESET;
    	    pcmcia_clear_requests(CardResource);
    	    /* Installed as Fast RAM. Do not initialize resource */
    	    return 0;
    	}
    }

    CardResource->task = NewCreateTask(
	TASKTAG_PC, CardTask,
	TASKTAG_NAME, CardResource->crb_LibNode.lib_Node.ln_Name,
	TASKTAG_PRI, 15,
	TASKTAG_ARG1, FindTask(0),
	TASKTAG_ARG2, CardResource,
	TAG_DONE);
    if (!CardResource->task)
    	return FALSE;
    Wait(SIGBREAKF_CTRL_F);

    Disable();

    intr = &CardResource->level2;
    intr->is_Node.ln_Pri = 127;
    intr->is_Node.ln_Type = NT_INTERRUPT;
    intr->is_Node.ln_Name = CardResource->crb_LibNode.lib_Node.ln_Name;
    intr->is_Code = (APTR)card_level2;
    intr->is_Data = CardResource;
    AddIntServer(INTB_PORTS, intr);

    intr = &CardResource->level6;
    intr->is_Node.ln_Pri = -127;
    intr->is_Node.ln_Type = NT_INTERRUPT;
    intr->is_Node.ln_Name = CardResource->crb_LibNode.lib_Node.ln_Name;
    intr->is_Code = (APTR)card_level6;
    intr->is_Data = CardResource;
    AddIntServer(INTB_EXTER, intr);

    pcmcia_clear_requests(CardResource);
    pcmcia_enable_interrupts();

    Enable();

    return TRUE;
}

ADD2INITLIB(Cardres_Init, 0)
