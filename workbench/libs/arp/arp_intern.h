/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef ARP_INTERN_H
#define ARP_INTERN_H

#include <libraries/arp.h>

extern struct ArpBase ArpBase;

#ifndef NO_LIB_DEFINES
#define SysBase		(struct Library *)ArpBase->ExecBase
#define DOSBase		(struct Library *)ArpBase->DosBase
#define GfxBase		(struct Library *)ArpBase->GfxBase
#define IntuitionBase	(struct Library *)ArpBase->IntuiBase
#define AslBase		(struct Library *)ArpBase->AslBase
#define UtilityBase	(struct Library *)ArpBase->UtilityBase
#endif

void intern_AddTrackedResource(struct ArpBase * ArpBase,
                               WORD ID,
                               APTR Stuff);

int strlen(const char *);

#endif /* ARP_INTERN_H  */



