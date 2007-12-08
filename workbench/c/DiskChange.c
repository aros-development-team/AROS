/*
    Copyright © 2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DiskChange CLI command
    Lang: English
*/

#include <proto/dos.h>

#include <dos/dos.h>
#include <exec/types.h>

const TEXT __version__[] = "\0$VER: DiskChange 41.1 (8.12.2007)";
int __nocommandline;

int main(void)
{
    struct RDArgs *ra;
    STRPTR dev;
    int rc = RETURN_FAIL;

    ra = ReadArgs("DEVICE/A", &dev, NULL);
    if (ra) {
	if (Inhibit(dev, DOSTRUE) && Inhibit(dev, DOSFALSE))
	    rc = RETURN_OK;
	FreeArgs(ra);
    }
    if (rc != RETURN_OK);
	PrintFault(IoErr(), "DiskChange");
    return rc;
}
