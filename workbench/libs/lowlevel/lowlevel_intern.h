/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header file for lowlevel library
    Lang: english
*/
#ifndef __LOWLEVEL_INTERN_H__
#define __LOWLEVEL_INTERN_H__

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>

#include <proto/exec.h>
#include <libcore/base.h>

#include <aros/debug.h>

/*
    This is the LowLevelBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the workbench.library functions to get information.
*/

struct LowLevelBase
{
    struct LibHeader ll_LibHeader;

    struct SignalSemaphore ll_Lock;
    struct Interrupt       ll_VBlank;
};


/*
 * Defintion of internal structures.
 */

#endif /* __LOWLEVEL_INTERN_H__  */

