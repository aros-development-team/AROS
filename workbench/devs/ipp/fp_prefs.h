/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef FP_PREFS_H
#define FP_PREFS_H

#include <exec/types.h>
#include <utility/tagitem.h>

/* Points printer.device unit 0 (ENV:SYS/printer.prefs, and ENVARC: with save) at
   printspool.device unit ippunit, which prints to ipp.device unit ippunit, keeping
   the rest of the preferences, or the Printer editor's defaults when there are none.
   *driver gets the driver they name. */
BOOL Prefs_UseIPP(LONG ippunit, BOOL save, char *driver, LONG drivermax);

/* PRD_SETDESTINATION on device's unit, opened for its settings only; 0 or the device's
   error, with IoErr() set to the DOS error behind it, if any. */
LONG Dest_Set(const char *device, LONG unit, struct TagItem *tags);

#endif
