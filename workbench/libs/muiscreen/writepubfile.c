/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <libraries/iffparse.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>

#include "fileformat.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH2(BOOL, MUIS_WritePubFile,

/*  SYNOPSIS */
	AROS_LHA(APTR, pf,  A0),
	AROS_LHA(struct MUI_PubScreenDesc *, desc,  A0),

/*  LOCATION */
	struct Library *, MUIScreenBase, 0x3c, MUIScreen)

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

    D(bug("MUIS_WritePubFile(%p, %p)\n", pf, desc));

    struct IFFHandle *iff = (struct IFFHandle *) pf; 
    BOOL retval = FALSE;
    struct MUI_PubScreenDescArray desc_tmp;

    LONG_TO_ARRAY(desc->Version, desc_tmp.Version);
    CopyMem(desc->Name, desc_tmp.Name, sizeof(desc_tmp.Name));
    CopyMem(desc->Title, desc_tmp.Title, sizeof(desc_tmp.Title));
    CopyMem(desc->Font, desc_tmp.Font, sizeof(desc_tmp.Font));
    CopyMem(desc->Background, desc_tmp.Background, sizeof(desc_tmp.Background));
    LONG_TO_ARRAY(desc->DisplayID, desc_tmp.DisplayID);
    WORD_TO_ARRAY(desc->DisplayWidth, desc_tmp.DisplayWidth);
    WORD_TO_ARRAY(desc->DisplayHeight, desc_tmp.DisplayHeight);
    desc_tmp.DisplayDepth = desc->DisplayDepth;
    desc_tmp.OverscanType = desc->OverscanType;
    desc_tmp.AutoScroll = desc->AutoScroll;
    desc_tmp.NoDrag = desc->NoDrag;
    desc_tmp.Exclusive = desc->Exclusive;
    desc_tmp.Interleaved = desc->Interleaved;
    desc_tmp.SysDefault = desc->SysDefault;
    desc_tmp.Behind = desc->Behind;
    desc_tmp.AutoClose = desc->AutoClose;
    desc_tmp.CloseGadget = desc->CloseGadget;
    desc_tmp.DummyWasForeign = desc->DummyWasForeign;
    CopyMem(desc->SystemPens, desc_tmp.SystemPens, sizeof(desc_tmp.SystemPens));
    CopyMem(desc->Reserved, desc_tmp.Reserved, sizeof(desc_tmp.Reserved));
    COLS_TO_ARRAY(desc->Palette, desc_tmp.Palette);
    COLS_TO_ARRAY(desc->rsvd, desc_tmp.rsvd);
    CopyMem(desc->rsvd2, desc_tmp.rsvd2, sizeof(desc_tmp.rsvd2));
    LONG_TO_ARRAY(desc->Changed, desc_tmp.Changed);

    if (!PushChunk(iff, ID_PREF, ID_MPUB, sizeof(struct MUI_PubScreenDescArray)))
    {
	if (WriteChunkBytes(iff, &desc_tmp, sizeof(struct MUI_PubScreenDescArray)) == sizeof(struct MUI_PubScreenDescArray))
	{
	    retval = TRUE;
	}
	PopChunk(iff);
    }

    return retval;

    AROS_LIBFUNC_EXIT
}
