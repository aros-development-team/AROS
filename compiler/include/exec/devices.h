#ifndef EXEC_DEVICES_H
#define EXEC_DEVICES_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Device handling
    Lang: english
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif

struct Device
{
    struct Library dd_Library;
};

struct Unit
{
    struct MsgPort unit_MsgPort;
    UBYTE          unit_flags;
    UBYTE          unit_pad;
    UWORD          unit_OpenCnt;
};

#define UNITF_ACTIVE (1<<0)
#define UNITF_INTASK (1<<1)

#endif /* EXEC_DEVICES_H */
