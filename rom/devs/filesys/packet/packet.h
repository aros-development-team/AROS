/*
 * packet.handler - Proxy filesystem for DOS packet handlers
 *
 * Copyright © 2007-2009 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef _PACKET_HANDLER_H
#define _PACKET_HANDLER_H 1

#define DEBUG 0
#include <aros/debug.h>

#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/errors.h>

#include <dos/dos.h>
#include <dos/filesystem.h>
#include <dos/bptr.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include LC_LIBDEFS_FILE

struct PacketBase {
    struct Device               device;

    struct MinList              mounts;
};

struct ph_handle {
    struct FileHandle           fh;

    void                        *actual;
    BOOL                        is_lock;

    struct ph_mount             *mount;
};

struct ph_mount {
    struct MinNode              node;

    char                        handler_name[MAXFILENAMELENGTH];
    char                        mount_point[MAXFILENAMELENGTH];

    struct Process              *process;
    BPTR                        seglist;
    BOOL                        is_loaded;

    struct Interrupt            reply_int;
    struct MsgPort              reply_port;

    struct ph_handle            root_handle;
};

struct ph_packet {
    struct Message              msg;
    struct DosPacket            dp;
    APTR                        pool;
};

#endif
