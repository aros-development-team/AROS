#ifndef HIDD_VIRTUALPAD_H
#define HIDD_VIRTUALPAD_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Public include for virtualpad.hidd, the simulated game controller
          driver used to test controller.hidd without hardware.
*/

#include <exec/types.h>
#include <hidd/controller.h>

#define CLID_Hidd_VirtualPad "hidd.input.controller.virtual"

/* Output requests received by a simulated device, retrieved with GetOutputLog() */
enum {
    vVirtualPad_Out_Rumble = 1,     /* a = low, b = high, c = left trigger, d = right trigger */
    vVirtualPad_Out_LED,            /* a = red, b = green, c = blue */
    vVirtualPad_Out_PlayerIndex,    /* a = index */
    vVirtualPad_Out_Effect,         /* a = size, b = first byte */
    vVirtualPad_Out_Sensors,        /* a = enabled */
    vVirtualPad_Out_UploadEffect,   /* a = type, b = motor, c = returned id */
    vVirtualPad_Out_PlayEffect,     /* a = id, b = loops */
    vVirtualPad_Out_StopEffect,     /* a = id */
    vVirtualPad_Out_RemoveEffect    /* a = id */
};

#define VPAD_LOG_MAX_QUERY 32

struct VirtualPad_Output
{
    UWORD   type;
    UWORD   reserved;
    LONG    a, b, c, d;
};

/* Profile directory (read only, exported for tools listing the profiles) */
struct VirtualPad_ProfileInfo
{
    CONST_STRPTR name;
    CONST_STRPTR description;
};

#include <interface/Hidd_VirtualPad.h>

#endif /* HIDD_VIRTUALPAD_H */
