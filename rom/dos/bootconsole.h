/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef DOS_BOOTCONSOLE_H
#define DOS_BOOTCONSOLE_H

#include <exec/types.h>

/* Target implementations can extend this caller-owned boot-lifetime state. */
struct BootConsole
{
    BOOL quiet;
};

#endif
