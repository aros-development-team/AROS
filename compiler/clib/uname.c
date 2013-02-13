/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <exec/types.h>
#include <aros/debug.h>
#include <utility/tagitem.h>
#include <aros/inquire.h>
#include <proto/aros.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <sys/utsname.h>

	int uname(

/*  SYNOPSIS */
    struct utsname *name)
        
/*  FUNCTION
	Store information about the operating system in the structure pointed 
	by name.
	
    INPUTS
	name - Pointer to utsname structure defined in <sys/utsname.h>.

    RESULT
	If the information was stored successfully, zero is returned. Otherwise
	function returns -1 and sets errno appropriately.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	
    INTERNALS
    	This function is using ArosInquire() function to get information about
    	the operating system.

******************************************************************************/
{
    IPTR version;
    IPTR release_minor;
    IPTR release_major;
    char *str_builddate;
    char *architecture;
    char *cpu;

    memset(name, 0, sizeof(struct utsname));

    ArosInquire(AI_ArosBuildDate, &str_builddate,
                AI_ArosVersion, &version,
                AI_ArosReleaseMajor, &release_minor,
                AI_ArosReleaseMinor, &release_major,
                AI_ArosArchitecture, &architecture,
                TAG_DONE);

    strncpy(name->sysname, "AROS", sizeof(name->sysname) - 1);
    snprintf(name->release, sizeof(name->release) - 1, "%d.%d", (int) release_major, (int) release_minor);
    snprintf(name->version, sizeof(name->version) - 1, "%d %s", (int) version, str_builddate);
    cpu = rindex(architecture, '-') + 1;
    strncpy(name->machine, (cpu?cpu:architecture), sizeof(name->machine) - 1);

    /* If TCP is running it will set the ENV:HOSTNAME var with our hostname */
    if (GetVar("HOSTNAME", name->nodename, sizeof(name->nodename) - 1, GVF_GLOBAL_ONLY) == -1)
    {
        strncpy(name->nodename, "localhost.localdomain", sizeof(name->nodename) - 1);
    }
    return 0;
}
