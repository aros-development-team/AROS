#ifndef DEVICES_CLIPBOARD_H
#define DEVICES_CLIPBOARD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Clipboard definitions
    Lang: english
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct ClipboardUnitPartial
{
    struct Node cu_Node;
    ULONG       cu_UnitNum;
};

#define PRIMARY_CLIP 0

#define CBD_POST           (CMD_NONSTD + 0)
#define CBD_CURRENTREADID  (CMD_NONSTD + 1)
#define CBD_CURRENTWRITEID (CMD_NONSTD + 2)
#define CBD_CHANGEHOOK     (CMD_NONSTD + 3)

#define CBR_OBSOLETEID 1

struct IOClipReq
{
    struct Message                io_Message;
    struct Device               * io_Device;
    struct ClipboardUnitPartial * io_Unit;

    UWORD  io_Command;
    UBYTE  io_Flags;
    BYTE   io_Error;
    ULONG  io_Actual;
    ULONG  io_Length;
    STRPTR io_Data;
    ULONG  io_Offset;
    LONG   io_ClipID;
};

struct SatisfyMsg
{
    struct Message sm_Msg;
    UWORD          sm_Unit;
    LONG           sm_ClipID;
};

struct ClipHookMsg
{
    ULONG chm_Type;
    LONG  chm_ChangeCmd;
    LONG  chm_ClipID;
};

#endif /* DEVICES_CLIPBOARD_H */
