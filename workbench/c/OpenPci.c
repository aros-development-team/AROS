/*
    Copyright (C) 2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Opens and lists all PCI devices available through PCI classes
    Lang: English
*/
#include <proto/exec.h>

int main(void)
{
    /* Initialize PCI classes */
    CloseLibrary(OpenLibrary("pci.library",0));
    
    /* Initialize PCI linux driver. Requires uid=0 to operate */
    CloseLibrary(OpenLibrary("pcilinux.library",0));
    return 0;
}
