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
    ULONG ret;

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

    printf("AROS release = %ld.%ld (V%ld)\n",
	ArosInquire(AI_ArosReleaseMajor),
	ArosInquire(AI_ArosReleaseMinor),
	ArosInquire(AI_ArosVersion));

    if( (ret = ArosInquire(AI_KickstartBase)))
    {
	printf("Kickstart base address = $%lx\n", ret);

	printf("Kickstart size = $%lx (%ld kB)\n",
	    ArosInquire(AI_KickstartSize),
	    ArosInquire(AI_KickstartSize)/1024);

	printf("Kickstart version = %ld.%ld\n",
	    ArosInquire(AI_KickstartVersion),
	    ArosInquire(AI_KickstartRevision));
    }
    else
    {
	printf("This machine has no Kickstart ROM.\n");
    }

    CloseLibrary(ArosBase);

    return rc;
}
