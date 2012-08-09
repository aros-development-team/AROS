/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: lowlevel.c $

    Desc: 
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <hardware/custom.h>

#include "card_intern.h"

#define INTDEBUG(x) ;

/* Card Change Detect interrupt */

AROS_UFH4(ULONG, card_level6,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    struct CardResource *CardResource = (struct CardResource*)data;
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE intreq, intena;

    intreq = gio->intreq;
    if (!(intreq & GAYLE_IRQ_CCDET) )
	return 0; /* not ours */
    intena = gio->intena;
    gio->intreq = ~GAYLE_IRQ_CCDET;
    if (!(intena & GAYLE_IRQ_CCDET))
    	return 0; /* not ours either */
    if (CardResource->disabled) {
    	pcmcia_reset(CardResource);
    	return 0; /* huh? shouldn't happen */
    }

    CardResource->disabled = TRUE;
    pcmcia_reset(CardResource);

    Signal(CardResource->task, CardResource->signalmask);

    INTDEBUG(bug("PCMCIA Card Change Detect interrupt\n"));

    return 1;

    AROS_USERFUNC_EXIT
}

/* All other PCMCIA related interrupts */

#define INTMASK (GAYLE_IRQ_BVD1 | GAYLE_IRQ_BVD2 | GAYLE_IRQ_WR | GAYLE_IRQ_BSY)
#define NOINTMASK (GAYLE_IRQ_IDE | GAYLE_IRQ_CCDET)

AROS_UFH4(ULONG, card_level2,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    struct CardResource *CardResource = (struct CardResource*)data;
    struct CardHandle *cah = CardResource->ownedcard;
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE intreq, intena, status;
    BOOL poststatus = FALSE;

    intreq = gio->intreq & INTMASK;
    if (!intreq)
    	return 0; /* not ours */
    status = ((intreq ^ INTMASK) & INTMASK) | NOINTMASK | CardResource->resetberr;
    intena = gio->intena;

    INTDEBUG(bug("%02x %02x\n", intena, intreq));

    if (!(intena & intreq)) {
    	gio->intreq = status;
    	return 0; /* not ours either */
    }

    if (CardResource->disabled) {
    	gio->intreq = status;
    	pcmcia_reset(CardResource);
    	return 0; /* huh? shouldn't happen */
    }
    intreq &= intena;

    status = intreq;
    if (cah && !CardResource->removed && cah->cah_CardStatus) {
    	if (cah->cah_CardFlags & CARDF_POSTSTATUS)
    	    poststatus = TRUE;
	INTDEBUG(bug("cah_CardStatus(%d,%x,%x)\n",
	   intreq,
	   cah->cah_CardStatus->is_Data,
	   cah->cah_CardStatus->is_Code));
	status = AROS_UFC4(UBYTE, cah->cah_CardStatus->is_Code,
	    AROS_UFCA(UBYTE, intreq, D0),
	    AROS_UFCA(APTR, cah->cah_CardStatus->is_Data, A1),
	    AROS_UFCA(APTR, cah->cah_CardStatus->is_Code, A5),
	    AROS_UFCA(struct ExecBase *, mySysBase, A6));
	INTDEBUG(bug("returned=%d\n", status));
    }
    if (status) {
	status = (status ^ INTMASK) & INTMASK;
	gio->intreq = status | NOINTMASK | CardResource->resetberr;
    }
    if (poststatus) {
    	INTDEBUG(bug("poststatus\n"));
	AROS_UFC4NR(void, cah->cah_CardStatus->is_Code,
	    AROS_UFCA(UBYTE, 0, D0),
	    AROS_UFCA(APTR, cah->cah_CardStatus->is_Data, A1),
	    AROS_UFCA(APTR, cah->cah_CardStatus->is_Code, A5),
	    AROS_UFCA(struct ExecBase *, mySysBase, A6));
	INTDEBUG(bug("returned\n"));
    }

    INTDEBUG(bug("exit\n"));

    return 1;

    AROS_USERFUNC_EXIT
}

void pcmcia_reset(struct CardResource *CardResource)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    /* Reset PCMCIA configuration, disable interrupts */
    gio->config = 0;
    gio->status = 0;
    /* Disable all PCMCIA interrupts */
    gio->intena &= ~(GAYLE_INT_CCDET | GAYLE_INT_BVD1 | GAYLE_INT_BVD2 | GAYLE_INT_WR | GAYLE_INT_BSY);
    pcmcia_clear_requests(CardResource);
}

void pcmcia_clear_requests(struct CardResource *CardResource)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    /* Clear all interrupt requests except IDE interrupt */
    gio->intreq = GAYLE_IRQ_IDE | CardResource->resetberr;
}

void pcmcia_enable_interrupts(void)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    /* Enable all interrupts except BVD2 */
    gio->intena |= GAYLE_INT_CCDET | GAYLE_INT_BVD1 | GAYLE_INT_WR | GAYLE_INT_BSY;
}

BOOL pcmcia_havecard(void)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    return gio->status & GAYLE_CS_CCDET;
}

void pcmcia_disable(void)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    gio->status = GAYLE_CS_DIS;
}

void pcmcia_enable(void)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    gio->status = 0;
}

/* ugly busy wait */
static void waitframe(UBYTE frames)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UWORD vpos = 0;

    while (frames != 0) {
        while (vpos < 128)
            vpos = custom->vhposr >> 8;
        while (vpos >= 128)
            vpos = custom->vhposr >> 8;
        frames--;
    }
}

void pcmcia_cardreset(struct CardResource *CardResource)
{
    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE x, rb, intena;

    Disable();
    rb = CardResource->resetberr;
    CardResource->resetberr = GAYLE_IRQ_CARD_RESET_MASK;
    intena = gio->intena;
    gio->intena = 0;
    gio->intreq = GAYLE_IRQ_IDE | GAYLE_IRQ_CARD_RESET_MASK;
    Enable();

    waitframe(10);

    Disable();
    CardResource->resetberr = rb;
    x = GAYLE_IRQ_IDE | CardResource->resetberr;
    gio->intreq = x;
    Enable();

    waitframe(10);

    gio->intena = intena;

    CARDDEBUG(bug("PCMCIA card reset %02x\n", x));
}
