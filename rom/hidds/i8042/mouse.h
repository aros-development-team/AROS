#ifndef _MOUSE_H
#define _MOUSE_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the mouse native HIDD.
    Lang: English.
*/

#define __OOP_NOMETHODBASES__

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>
#include <hidd/mouse.h>

#include "libbase.h"

/* defines for buttonstate */

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MIDDLE_BUTTON   4

#define INTELLIMOUSE_SUPPORT        1

#define PS2_PROTOCOL_STANDARD       0
#define PS2_PROTOCOL_INTELLIMOUSE   1

/***** Mouse HIDD *******************/

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;

    UWORD buttonstate;

    struct Interrupt            irq;
    UBYTE                       mouse_data[5];
    UBYTE                       mouse_collected_bytes;
    UBYTE                       mouse_protocol;
    UBYTE                       mouse_packetsize;
    UBYTE                       expected_mouse_acks;
    UBYTE                       packetsize;
            
    struct pHidd_Mouse_Event    event;
};

/****************************************************************************************/

#define KBD_OUTCMD_SET_RES              0xE8
#define KBD_OUTCMD_SET_SCALE11          0xE6
#define KBD_OUTCMD_SET_SCALE21          0xE7
#define KBD_OUTCMD_STATUS_REQUEST       0xE9
#define KBD_OUTCMD_SET_STREAM_MODE      0xEA
#define KBD_OUTCMD_READ_DATA            0xEB
#define KBD_OUTCMD_SET_REMOTE_MODE      0xF0
#define KBD_OUTCMD_GET_ID               0xF2
#define KBD_OUTCMD_SET_RATE             0xF3
#define KBD_OUTCMD_SET_STREAM           0xEA
#define KBD_OUTCMD_ENABLE               0xF4
#define KBD_OUTCMD_DISABLE              0xF5
#define KBD_OUTCMD_RESET                0xFF

/****************************************************************************************/

#endif /* _MOUSE_H */

