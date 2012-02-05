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

struct PrinterPrefs {
    struct PrinterTxtPrefs pp_Txt;
    struct PrinterUnitPrefs pp_Unit;
    struct PrinterDeviceUnitPrefs pp_DeviceUnit;
    struct PrinterGfxPrefs pp_Gfx;
};

/*********************************************************************************************/

BOOL Printer_LoadPrefs(struct PrinterBase *PrinterBase, struct PrinterPrefs *prefs);

/*********************************************************************************************/

#endif /* _PREFS_H_ */
