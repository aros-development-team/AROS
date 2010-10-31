/*
    copyright © 1995-2010, the aros development team. all rights reserved.
    $id$

    desc: m68k-amiga IRQ handling
    lang: english
 */

#ifndef AMIGA_IRQ_H
#define AMIGA_IRQ_H

#include <aros/kernel.h>
#include <exec/execbase.h>

void AmigaIRQInit(struct ExecBase *SysBase);

#endif /* AMIGA_IRQ_H */
