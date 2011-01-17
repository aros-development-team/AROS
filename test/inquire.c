/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Use of aros.library/ArosInquire()
*/

#include <aros/inquire.h>
#include <aros/arosbase.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/aros.h>

#include <stdio.h>

static const char version[]= "$VER: inquire 41.1 (29.3.1997)\n\r";

struct Library *ArosBase;

int main(int argc, char **argv)
{
    int rc;
    ULONG relMajor, relMinor, vers;
    ULONG kickbase, kicksize;
    UWORD kickver, kickrev;
    STRPTR variant;

    rc = 0;

    if(!(ArosBase = OpenLibrary(AROSLIBNAME, AROSLIBVERSION)))
    {
	printf ("Couldn't open "AROSLIBNAME"\n");
	return RETURN_FAIL;
    }

    if(AROSLIBREVISION < ArosBase->lib_Revision)
    {
	CloseLibrary(ArosBase);
	printf (AROSLIBNAME" is too old! Need at least version %d.%d\n",
		AROSLIBVERSION, AROSLIBREVISION);
	return RETURN_FAIL;
    }

    ArosInquire (
	AI_ArosVersion, 	(IPTR) &vers,
	AI_ArosReleaseMajor,	(IPTR) &relMajor,
	AI_ArosReleaseMinor,	(IPTR) &relMinor,
	AI_KickstartBase,	(IPTR) &kickbase,
	AI_KickstartSize,	(IPTR) &kicksize,
	AI_KickstartVersion,	(IPTR) &kickver,
	AI_KickstartRevision,	(IPTR) &kickrev,
	AI_ArosVariant,         (IPTR) &variant,
	TAG_DONE);

    printf ("AROS release = %ld.%ld\n", relMajor, relMinor);
    printf ("AROS module major version = V%ld\n", vers);
    printf ("AROS Variant = %s\n", variant);

    if (kicksize)
    {
	printf("Kickstart base address = $%lx\n", kickbase);

	printf("Kickstart size = $%lx (%ld kB)\n", kicksize, kicksize/1024);

	printf("Kickstart version = %d.%d\n", kickver, kickrev);
    }
    else
    {
	printf("This machine has no Kickstart ROM.\n");
    }

    CloseLibrary(ArosBase);

    return 0;
}
