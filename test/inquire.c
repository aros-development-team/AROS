/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Use of aros.library/ArosInquire()
    Lang: english
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
	AI_ArosVersion, 	&vers,
	AI_ArosReleaseMajor,	&relMajor,
	AI_ArosReleaseMinor,	&relMinor,
	AI_KickstartBase,	&kickbase,
	AI_KickstartSize,	&kicksize,
	AI_KickstartVersion,	&kickver,
	AI_KickstartRevision,	&kickrev,
	TAG_DONE);

    printf ("AROS release = %ld.%ld\n", relMajor, relMinor);
    printf ("AROS module major version = V%ld\n", vers);

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
