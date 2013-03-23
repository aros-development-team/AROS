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

#define FNAME_BCMSDC(x)                 BCM2835SD__Device__ ## x
#define FNAME_BCMSDCBUS(x)              BCM2835SD__SDBus__ ## x

#define TIMEOUT			        30

#define BCM2835SDUNIT_MAX               1
#define BCM2835SDCLOCK_MIN              400000

#define VCMB_PROPCHAN                   8

#define VCPOWER_SDHCI		        0
#define VCPOWER_STATE_ON	        (1 << 0)
#define VCPOWER_STATE_WAIT	        (1 << 1)
#define VCCLOCK_SDHCI                   1

void FNAME_BCMSDCBUS(BCMLEDCtrl)(int lvl);

UBYTE FNAME_BCMSDCBUS(BCMMMIOReadByte)(ULONG, struct sdcard_Bus *);
UWORD FNAME_BCMSDCBUS(BCMMMIOReadWord)(ULONG, struct sdcard_Bus *);
ULONG FNAME_BCMSDCBUS(BCMMMIOReadLong)(ULONG, struct sdcard_Bus *);

void FNAME_BCMSDCBUS(BCMMMIOWriteByte)(ULONG, UBYTE, struct sdcard_Bus *);
void FNAME_BCMSDCBUS(BCMMMIOWriteWord)(ULONG, UWORD, struct sdcard_Bus *);
void FNAME_BCMSDCBUS(BCMMMIOWriteLong)(ULONG, ULONG, struct sdcard_Bus *);

#endif // _SDCARDBCM2835_INTERN_H
