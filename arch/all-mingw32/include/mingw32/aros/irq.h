#ifndef _AROS_IRQ_H
#define _AROS_IRQ_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: host-side IRQ API for Windows-hosted AROS
    Lang: english
    
    Warning: this API is experimental and subject to change.
*/

/* These entry points are implemented with the AROS calling convention. */
#ifdef __x86_64__
#define AROS_IRQ_ABI __attribute__((sysv_abi))
#else
#define AROS_IRQ_ABI
#endif

int          __declspec(dllimport) AROS_IRQ_ABI KrnAllocSystemIRQ(void);
void         __declspec(dllimport) AROS_IRQ_ABI KrnFreeSystemIRQ(unsigned char irq);
void *       __declspec(dllimport) AROS_IRQ_ABI KrnGetSystemIRQObject(unsigned char irq);
unsigned int __declspec(dllimport) AROS_IRQ_ABI KrnCauseSystemIRQ(unsigned char irq);

#undef AROS_IRQ_ABI

#endif
