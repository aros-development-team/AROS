#ifndef NONVOLATILE_INTERN_H
#define NONVOLATILE_INTERN_H

/*
    (C) 2000 AROS - The Amiga Research OS
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
    struct Library    nv_LibNode;
    BPTR              nv_SegList;

    struct ExecBase  *nv_SysBase;

    // Library talking to the hardware
    struct Library   *nv_ImplementationLib;
};


/* Locate library bases */

#ifdef SysBase
#undef SysBase
#endif

#define SysBase    ((struct NVBase *)nvBase)->nv_SysBase
#define nvdBase    ((struct NVBase *)nvBase)->nv_ImplementationLib

#define GPB(x)     ((struct NVBase *)x)

#define expunge() \
AROS_LC0(BPTR, expunge, struct NVBase *, nvBase, 3, Nonvolatile)

#endif  /* NONVOLATILE_INTERN_H */

