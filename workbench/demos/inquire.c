/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Use of aros.library/ArosInquire()
    Desc: Use of aros.library/ArosInquireTagList
    Lang:
*/

#include <aros/arosbase.h>
#include <aros/inquire.h>
#include <dos/dos.h>

#include <proto/exec.h>
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
	fprintf(stderr, "Couldn't open "AROSLIBNAME"\n");
	return RETURN_FAIL;
    }

    if(AROSLIBREVISION < ArosBase->lib_Revision)
    {
	CloseLibrary(ArosBase);
	fprintf(stderr,
	    AROSLIBNAME" is too old! Need at least version %ld.%ld\n",
		(ULONG)AROSLIBVERSION, (ULONG)AROSLIBREVISION);
	return RETURN_FAIL;
    }

    ArosInquire(
	AI_ArosVersion, 	(IPTR)&vers,
	AI_ArosReleaseMajor,	(IPTR)&relMajor,
	AI_ArosReleaseMinor,	(IPTR)&relMinor,
	AI_KickstartBase,	(IPTR)&kickbase,
	AI_KickstartSize,	(IPTR)&kicksize,
	AI_KickstartVersion,	(IPTR)&kickver,
	AI_KickstartRevision,	(IPTR)&kickrev,
	TAG_DONE);

    printf("AROS release = %ld.%ld\n", relMajor, relMinor);
    printf("AROS module major version = V%ld\n", vers);

    if (kickbase)
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
