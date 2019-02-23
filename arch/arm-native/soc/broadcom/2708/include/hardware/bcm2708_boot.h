/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BCM2708_BOOT_H
#define _BCM2708_BOOT_H

#include <asm/arm/mmu.h>

/* define the boot strap memory layout */

#define __bcm2708_vectsize              0x40
#define __bcm2708_bootstacksize         (768 << 2)
#define __bcm2708_boottagssize          (64 << 3)

struct bcm2708bootmem
{
    UBYTE       bm_vectors[__bcm2708_vectsize];
    UBYTE       bm_padding1[(0x1000 - __bcm2708_vectsize - __bcm2708_bootstacksize - __bcm2708_boottagssize - 16)];
    UBYTE       bm_boottags[__bcm2708_boottagssize];
    UBYTE       bm_bootstack[__bcm2708_bootstacksize];
    UBYTE       bm_padding2[16];
    /* 0x1000 */
    UBYTE       bm_mboxmsg[0x1000];
    UBYTE       bm_mctrampoline[0x2000];
    /* 0x4000 */
    pde_t       bm_pde[4096];
}  __attribute__((packed));

#define BOOTMEMADDR(offset) (&(((struct bcm2708bootmem *)0x0)->offset))

#endif	/* _BCM2708_BOOT_H */
