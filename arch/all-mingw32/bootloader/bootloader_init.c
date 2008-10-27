/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: bootloader_init.c 29778 2008-10-16 19:40:21Z neil $

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
    NEWLIST(&(BootLoaderBase->Args));

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
