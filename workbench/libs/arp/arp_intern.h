/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef ARP_INTERN_H
#define ARP_INTERN_H

#include <exec/types.h>
#include <libraries/arp.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <exec/ports.h>

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

void DoQSort(ULONG baseptr,
             ULONG l,
             ULONG r,
             ULONG byte_size,
             LONG  (* user_function)());

void Exchange(void * Lptr1,
              void * Lptr2,
              ULONG byte_size);

LONG user_function(void * Lptr1,
                   void * Lptr2);

void StripIntuiMessages(struct MsgPort * mp,
                        struct Window * window);

int strlen(const char *);

#endif /* ARP_INTERN_H  */



