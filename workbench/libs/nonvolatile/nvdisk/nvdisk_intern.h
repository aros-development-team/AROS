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

// Private structure of nvdisk.library library base
struct NVDBase
{
    struct Library    nvd_Lib;
    struct ExecBase  *nvd_SysBase;
    BPTR              nvd_SegList;
    
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

#define SysBase    GPB(nvdBase)->nvd_SysBase
#define DOSBase    GPB(nvdBase)->nvd_DOSBase

#endif  /* NVDISK_INTERN_H */

