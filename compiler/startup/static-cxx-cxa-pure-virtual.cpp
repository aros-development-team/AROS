/*
    Copyright (C) 2021-2022, The AROS Development Team. All rights reserved.
    $Id:$
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <stdlib.h>

/*
 * If a pure virtual function is called during object construction/destruction, __cxa_pure_virtual
 * will be called to report the error/halt execution.
 */

extern "C" void __cxa_pure_virtual()
{
   bug("*** %s : Pure virtual function called!\n", __func__);
    D(
       bug("*** %s :       This usually occurs if a virtual method is\n", __func__);
       bug("*** %s :       called from a constructor/destructor.\n", __func__);
    )
#if (0)
    abort();
#else
     while (1); 
#endif
}
