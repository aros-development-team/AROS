#ifndef NONVOLATILE_INTERN_H
#define NONVOLATILE_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/


#include  <exec/libraries.h>
#include  <libraries/nonvolatile.h>
#include  <dos/dos.h>

// Private structure of nonvolatile.library library base
struct NVBase
{
    struct Library    nv_Lib;
    struct ExecBase  *nv_SysBase;
    APTR              nv_SegList;

    // Library talking to the hardware
    struct Library   *nv_ImplementationLib;
};

#define GM_SYSBASE_FIELD(lh) (((struct LibHeader *)lh)->lh_SysBase)
#define GM_SEGLIST_FIELD(lh) (((struct LibHeader *)lh)->lh_SegList)

/* Locate library bases */

#define nvdBase    ((struct NVBase *)nvBase)->nv_ImplementationLib

#define GPB(x)     ((struct NVBase *)x)

#endif  /* NONVOLATILE_INTERN_H */

