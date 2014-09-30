/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include "kernel_base.h"

AROS_LH0I(int, KrnIsSuper,
	  struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return *KernelIFace.SuperState;

    AROS_LIBFUNC_EXIT
}
