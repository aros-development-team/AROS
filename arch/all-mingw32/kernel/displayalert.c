/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in supervisor mode, Windows hosted version.
    Lang: english
*/

#include <aros/libcall.h>

#include <inttypes.h>

#include "kernel_base.h"

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    /* This displays an alert via MessageBox() */
    KernelIFace.core_alert(text);

    AROS_LIBFUNC_EXIT
}
