/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Due to lack of a Prefs program, a simple ASCII file named ENV:datatypes/picture.prefs
    gets parsed using ReadArgs (multiple lines allowed) with these options:

    MAXPENS /N/K: maximum number of pens to alloc (colormapped dest only)
    DITHERQ /N/K: Dither quality for display (colormapped dest only), 0 (worst) to 4 (best),
                  1 to 4 have same speed and are slower than 0
    FREESRC /S:   Change the default for FreeSourceBitMap to TRUE, to save memory;
                  might bring compatibility problems
    USEBM /S:     Use BitMap instead of Chunky Buffer for planar destinations
                  (ECS, AGA) or for debugging
    USECM /S:     Forces the destination to be colormapped, for debugging
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "debug.h"
#include "pictureclass.h"
#include "prefs.h"

/**************************************************************************************************/

#define FILENAME "ENV:datatypes/picture.prefs"
#define PREFSTEMPLATE "MAXPENS/N/K,DITHERQ/N/K,FREESRC/S,USEBM/S,USECM/S"
enum
{
	ARG_MAXPENS,
	ARG_DITHERQ,
	ARG_FREESRC,
	ARG_USEBM,
	ARG_USECM,
	ARG_MAX
};

/**************************************************************************************************/

BOOL ReadPrefs(struct Picture_Data *pd)
{
	struct RDArgs *rdargs;
	struct RDArgs *args;
	BPTR fh;

	if((fh = Open(FILENAME, MODE_OLDFILE)))
	{
		UBYTE buf[256];
		ULONG para[ARG_MAX];
		LONG i;

		D(bug("picture.datatype/ReadPrefs: File %s opened\n",FILENAME));
		while(FGets(fh,buf,sizeof(buf)))
		{
			D(bug("picture.datatype/ReadPrefs: Line: %s",buf));
			i=strlen(buf);
			if( i>1 && (buf[i-1]=='\n' && (rdargs=(struct RDArgs *)AllocDosObject(DOS_RDARGS,NULL))) )
			{
				rdargs->RDA_Source.CS_Buffer = buf;
				rdargs->RDA_Source.CS_Length = strlen(buf);

				for(i=0 ; i<ARG_MAX ; i++)
					para[i]=0;

				if((args = ReadArgs(PREFSTEMPLATE,(LONG *) para,rdargs)))
				{
					if(para[ARG_MAXPENS])
						pd->MaxDitherPens = *((LONG *) para[ARG_MAXPENS]);
					if(para[ARG_DITHERQ])
						pd->DitherQuality = *((LONG *) para[ARG_DITHERQ]);
					if(para[ARG_FREESRC])
						pd->FreeSource = para[ARG_FREESRC];
					if(para[ARG_USEBM])
						pd->UseBM = para[ARG_USEBM];
					if(para[ARG_USECM])
						pd->UseCM = para[ARG_USECM];
				}
				FreeDosObject(DOS_RDARGS , rdargs);
			}
		}
		Close(fh);
		D(bug("picture.datatype/ReadPrefs: MaxPens %d DitherQ %d FreeSrc %d UseBM %d UseCM %d\n",
			(int)pd->MaxDitherPens, (int)pd->DitherQuality,
			(int)pd->FreeSource, (int)pd->UseBM, (int)pd->UseCM));
	}

	return((BOOL) (fh == NULL));
}
