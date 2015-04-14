/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BCM2708_BOOT_H
#define _BCM2708_BOOT_H

#include <asm/arm/mmu.h>

/* define the boot strap memory layout */

#define __bcm2708_vectsize              0x20
#define __bcm2708_bootstacksize         16

struct bcm2708bootmem
{
    UBYTE       bm_vectors[__bcm2708_vectsize];
    UBYTE       bm_padding[(0x1000 - __bcm2708_vectsize - __bcm2708_bootstacksize)];
    UBYTE       bm_bootstack[__bcm2708_bootstacksize];
    /* 0x1000 */
    UBYTE       bm_mboxmsg[0x1000];
    UBYTE       bm_padding2[0x3000];
    /* 0x4000 */
    pde_t       bm_pde[4096];
}  __attribute__((packed));

#define BOOTMEMADDR(offset) (&(((struct bcm2708bootmem *)0x0)->offset))

#endif	/* _BCM2708_BOOT_H */
