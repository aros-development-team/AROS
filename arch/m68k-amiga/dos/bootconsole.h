/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef DOS_BOOTCONSOLE_H
#define DOS_BOOTCONSOLE_H

#include <exec/ports.h>
#include <exec/io.h>

/* The boot frame outlives synchronous commands and their console packets.
 * Use its reserved stack instead of fragmenting scarce Chip RAM with a
 * separate persistent allocation. The port must remain the first member. */
struct BootConsole
{
    struct MsgPort port;
    struct MsgPort *saved_console;
    struct IOStdReq read_request;
    BOOL raw;
    BOOL quiet;
};

#endif
