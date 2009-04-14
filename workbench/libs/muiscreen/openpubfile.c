/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#define DEBUG 1
#include <aros/debug.h>

#include "fileformat.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH2(APTR, MUIS_OpenPubFile,

/*  SYNOPSIS */
	AROS_LHA(char*, name,  A0),
	AROS_LHA(ULONG, mode, D0),

/*  LOCATION */
	struct Library *, MUIScreenBase, 0x2a, MUIScreen)

/*  FUNCTION

    INPUTS

    RESULT
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct IFFHandle *iff;

    D(bug("MUIS_OpenPubFile(%s, %d)\n", name, mode));

    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR) Open(name, mode)))
	{
	    InitIFFasDOS(iff);
	    if (!OpenIFF(iff, (mode == MODE_OLDFILE ? IFFF_READ : IFFF_WRITE)))
	    {
		if (mode == MODE_NEWFILE)
		{
		    if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
		    {
			if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
			{
			    struct FilePrefHeader head;
    
			    head.ph_Version  = 0; // FIXME: shouold be PHV_CURRENT, but see <prefs/prefhdr.h> 
			    head.ph_Type     = 0;
			    head.ph_Flags[0] =
			    head.ph_Flags[1] =
			    head.ph_Flags[2] =
			    head.ph_Flags[3] = 0;
    
			    if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
			    {
				PopChunk(iff);
				return (APTR) iff;
			    }
			}
		    }
		}
		else
		    return (APTR) iff;
	    }
	}
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
