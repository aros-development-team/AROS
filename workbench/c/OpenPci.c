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
    CloseLibrary(OpenLibrary("SYS:Hidds/pci.hidd",0));
    
    /* Initialize PCI linux driver. Requires uid=0 to operate */
    CloseLibrary(OpenLibrary("SYS:Hidds/pcilinux.hidd",0));
    CloseLibrary(OpenLibrary("SYS:Hidds/pcipc.hidd",0));
   return 0;
}
