/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header file for lowlevel library
    Lang: english
*/
#ifndef __LOWLEVEL_INTERN_H__
#define __LOWLEVEL_INTERN_H__

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif

#include <aros/debug.h>

/*
    This is the LowLevelBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the workbench.library functions to get information.
*/

extern struct ExecBase * SysBase;

struct LowLevelBase
{
    struct Library   	LibNode;
    BPTR	     	ll_SegList;
    struct ExecBase  *	ll_SysBase;
};


/*
 * Defintion of internal structures.
 */

#endif /* __LOWLEVEL_INTERN_H__  */

