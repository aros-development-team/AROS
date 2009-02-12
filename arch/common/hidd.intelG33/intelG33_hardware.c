/*
    Copyright � 2009-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: intelG33_hardware.c
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "intelG33_intern.h"
#include "intelG33_regs.h"

void init_GMBus(struct staticdata *sd) {
}

UWORD status_GMBus(struct staticdata *sd) {

    return G33_RD_REGW(MMADR, GMBUS2);

}

BOOL read_DDC2(struct staticdata *sd) {

    G33_WR_REGW(MMADR, GMBUS0, GMBUSRate400KHz);

    return TRUE;

}
