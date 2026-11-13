/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Display an alert in supervisor mode, Windows hosted version.
*/

#include "kernel_base.h"

void krnDisplayAlert(const char *text, struct KernelBase *KernelBase)
{
    /* This displays an alert via MessageBox() */
    KernelIFace.core_alert(text);
}
