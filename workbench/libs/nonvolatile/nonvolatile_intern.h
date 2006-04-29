#ifndef NONVOLATILE_INTERN_H
#define NONVOLATILE_INTERN_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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
};

/* Locate library bases */

#define GPB(x)     ((struct NVBase *)x)

#endif  /* NONVOLATILE_INTERN_H */

