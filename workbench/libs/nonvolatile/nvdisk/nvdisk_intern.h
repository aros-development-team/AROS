#ifndef NVDISK_INTERN_H
#define NVDISK_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/


#include  <exec/libraries.h>
#include  <exec/execbase.h>
#include  <dos/dos.h>

#include <libcore/base.h>

// Private structure of nvdisk.library library base
struct NVDBase
{
    struct LibHeader  nvd_LibHeader;
    
    struct Library   *nvd_DOSBase;

    BPTR              nvd_location;
};


/* Locate library bases */

#ifdef SysBase
#undef SysBase
#endif
#ifdef DOSBase
#undef DOSBase
#endif

#define GPB(x) ((struct NVDBase *)x)

#define SysBase    ((struct LibHeader *)nvdBase)->lh_SysBase
#define DOSBase    GPB(nvdBase)->nvd_DOSBase

#endif  /* NVDISK_INTERN_H */

