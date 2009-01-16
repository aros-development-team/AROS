/*
    Copyright � 2009-2010, The AROS Development Team. All rights reserved.
    $Id: intelG33_hardware.c 29403 2008-09-05 19:13:26Z j.koivisto $

    Desc: intelG33_hardware.c
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <inttypes.h>

#include "intelG33_intern.h"
#include "intelG33_regs.h"

void GMBUS_Init(struct staticdata *sd) {
}

UWORD GMBUS_GetStatus(struct staticdata *sd) {

    return G33_RD_REGW(MMADR, GMBUS2);

}

