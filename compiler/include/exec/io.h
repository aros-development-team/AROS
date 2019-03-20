#ifndef EXEC_IO_H
#define EXEC_IO_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Device communication
    Lang: english
*/

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif

#define DEV_BEGINIO (-30)
#define DEV_ABORTIO (-36)

struct IORequest
{
    struct Message  io_Message;
    struct Device * io_Device;
    struct Unit   * io_Unit;
    UWORD           io_Command;
    UBYTE           io_Flags;
    BYTE            io_Error;
};

struct IOStdReq
{
    struct Message  io_Message;
    struct Device * io_Device;
    struct Unit   * io_Unit;
    UWORD           io_Command;
    UBYTE           io_Flags;
    BYTE            io_Error;
/* fields that are different from IORequest */
    ULONG           io_Actual;
    ULONG           io_Length;
    APTR            io_Data;
    ULONG           io_Offset;
};

#define CMD_INVALID 0
#define CMD_RESET   1
#define CMD_READ    2
#define CMD_WRITE   3
#define CMD_UPDATE  4
#define CMD_CLEAR   5
#define CMD_STOP    6
#define CMD_START   7
#define CMD_FLUSH   8
#define CMD_NONSTD  9

#define IOB_QUICK     0
#define IOF_QUICK (1<<0)

#endif /* EXEC_IO_H */
