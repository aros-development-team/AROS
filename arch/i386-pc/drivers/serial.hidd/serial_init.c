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
    struct class_static_data *csd = &LIBBASE->hdg_csd;

    EnterFunc(bug("SerialHIDD_Init()\n"));

    // TODO: Probe for the Serial ports and register units.
    __IHidd = OOP_ObtainAttrBase(IID_Hidd);
    __IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);

    ReturnInt("SerialHIDD_Init", int, TRUE);
}

ADD2INITLIB(PCSer_Init, 0)

static int PCSer_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;

    EnterFunc(bug("PCSer_Expunge()\n"));

    OOP_ReleaseAttrBase(IID_Hidd_SerialUnit);
    OOP_ReleaseAttrBase(IID_Hidd);
        
    ReturnInt("PCSer_Expunge", int, TRUE);
}

ADD2EXPUNGELIB(PCSer_Expunge, 0)
