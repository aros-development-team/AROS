/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Use of aros.library/ArosInquireTagList
    Lang:
*/

#include <aros/arosbase.h>
#include <aros/inquire.h>

#include <proto/exec.h>
#include <proto/aros.h>

#include <stdio.h>

static const char version[]= "$VER: inquire 41.1 (29.3.1997)\n\r";

struct Library *ArosBase;

int main(int argc, char **argv)
{
    int rc;
    IPTR relMajor, relMinor, version;
    IPTR kickbase, kicksize, kickver, kickrev;

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

    ArosInquire
    (
	AI_ArosReleaseMajor, &relMajor,
	AI_ArosReleaseMinor, &relMinor,
	AI_ArosVersion,      &version,
	AI_KickstartBase,    &kickbase,
	AI_KickstartSize,    &kicksize,
	AI_KickstartVersion, &kickver,
	AI_KickstartRevision, &kickrev,
	TAG_END
    );

    printf("AROS release = %ld.%ld (V%ld)\n", relMajor, relMinor, version);

    if (kickbase)
    {
	printf("Kickstart base address = $%lx\n", kickbase);

	printf("Kickstart size = $%lx (%ld kB)\n", kicksize, kicksize/1024);

	printf("Kickstart version = %ld.%ld\n", kickver, kickrev);
    }
    else
    {
	printf("This machine has no Kickstart ROM.\n");
    }

    CloseLibrary(ArosBase);

    return rc;
}
