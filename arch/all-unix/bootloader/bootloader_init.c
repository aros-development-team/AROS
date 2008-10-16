/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/bootloader.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include "bootloader_intern.h"
#include LC_LIBDEFS_FILE

#include <string.h>

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    BootLoaderBase->Flags = 0;
    
    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
