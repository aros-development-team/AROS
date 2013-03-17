#ifndef _SDCARDBCM2835_INTERN_H
#define _SDCARDBCM2835_INTERN_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <utility/utility.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <devices/timer.h>

#include <asm/bcm2835.h>

#include "sdcard_base.h"

#define TIMEOUT			        30

#define BCM2835SDUNIT_MAX               1
#define BCM2835SDCLOCK_MIN              400000

#define VCMB_PROPCHAN                   8

void FNAME_SDCBUS(BCMLEDCtrl)(int lvl);

UBYTE FNAME_SDCBUS(BCMMMIOReadByte)(ULONG, struct sdcard_Bus *);
UWORD FNAME_SDCBUS(BCMMMIOReadWord)(ULONG, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(BCMMMIOReadLong)(ULONG, struct sdcard_Bus *);

void FNAME_SDCBUS(BCMMMIOWriteByte)(ULONG, UBYTE, struct sdcard_Bus *);
void FNAME_SDCBUS(BCMMMIOWriteWord)(ULONG, UWORD, struct sdcard_Bus *);
void FNAME_SDCBUS(BCMMMIOWriteLong)(ULONG, ULONG, struct sdcard_Bus *);

#endif // _SDCARDBCM2835_INTERN_H
