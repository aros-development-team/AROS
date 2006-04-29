#ifndef NVDISK_INTERN_H
#define NVDISK_INTERN_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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
    
    BPTR              nvd_location;
};

/* Locate library bases */

#define GPB(x) ((struct NVDBase *)x)

#endif  /* NVDISK_INTERN_H */

