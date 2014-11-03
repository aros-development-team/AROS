/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i sd/mmc control module
    Lang: english
*/

#ifndef HARDWARE_SUN4I_MMC_H
#define HARDWARE_SUN4I_MMC_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_MMC_BASE			0x01c0f000

struct MMC {
}__attribute__((__packed__));

#endif
