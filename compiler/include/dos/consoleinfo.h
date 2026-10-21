#ifndef DOS_CONSOLEINFO_H
#define DOS_CONSOLEINFO_H
/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <exec/types.h>
#include <exec/io.h>

/* AROS extension. Arg1: file handle argument, Arg2: APTR to output,
 * Arg3: output capacity in bytes, Arg4: CONSOLE_GEOMETRY_VERSION.
 * Success returns DOSTRUE and a copied snapshot. Unsupported handlers return
 * ERROR_ACTION_NOT_KNOWN. No handler-owned pointers escape in the reply.
 * This is a query only: it does not subscribe to resize notifications. */
#define ACTION_CONSOLE_GETGEOMETRY 0x434f4e47L /* CONG */
#define CONSOLE_GEOMETRY_VERSION 1

/* Shared by console.device and ConsoleNG; +4 is ConsoleNG CD_ASKTITLE.
 * IOStdReq: io_Data points to output, io_Length is capacity; io_Actual is
 * sizeof(struct ConsoleGeometry) on success. Older devices return NOCMD. */
#define CD_GETGEOMETRY (CMD_NONSTD + 5)
struct ConsoleGeometry
{
    ULONG cg_Size;
    ULONG cg_Version;
    ULONG cg_Columns;
    ULONG cg_Rows;
    ULONG cg_Width;  /* pixels occupied by complete character cells */
    ULONG cg_Height;
};
#endif
