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

#include <libcore/base.h>

// Private structure of nonvolatile.library library base
struct NVBase
{
    struct LibHeader  nv_LibHeader;

    // Library talking to the hardware
    struct Library   *nv_ImplementationLib;
};


/* Locate library bases */

#define nvdBase    ((struct NVBase *)nvBase)->nv_ImplementationLib

#define GPB(x)     ((struct NVBase *)x)

#endif  /* NONVOLATILE_INTERN_H */

