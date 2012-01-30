/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef _PREFS_H_
#define _PREFS_H_

#include <prefs/printertxt.h>
#include <prefs/printergfx.h>
#include <dos/dos.h>

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(int printer_unit);

/*********************************************************************************************/

extern struct PrinterPrefs {
    struct PrinterTxtPrefs pp_Txt;
    struct PrinterUnitPrefs pp_Unit;
    struct PrinterDeviceUnitPrefs pp_DeviceUnit;
    struct PrinterGfxPrefs pp_Gfx;
} printerprefs;

/*********************************************************************************************/

#endif /* _PREFS_H_ */
