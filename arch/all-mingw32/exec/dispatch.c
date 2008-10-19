/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: dispatch.c 29725 2008-10-11 23:48:13Z neil $

    Desc: Dispatch() entry :)
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>

/*
 * This function is not there anymore because:
 * 1) It is moved to kernel.resource.
 * 2) There's no reason to keep this exec.library call because it was a private function.
 *    No user software could call it.
 */

AROS_LH0(void, Dispatch,
         struct ExecBase *, SysBase, 10, Exec)
{
    AROS_LIBFUNC_INIT
    
    AROS_LIBFUNC_EXIT
}
