 /*
    Copyright © 1995-2023, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

#include <aros/debug.h>

static int PCSer_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd; /* SerialHidd static data */
    EnterFunc(bug("SerialHIDD_Init()\n"));

    // TODO: Probe for the Serial ports and register units.

    ReturnInt("SerialHIDD_Init", int, TRUE);
}

ADD2INITLIB(PCSer_Init, 0)
